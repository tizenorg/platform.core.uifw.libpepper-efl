#ifndef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>

// EFL header
#include <Eina.h>
#include <Ecore.h>

// wayland compositor header
#include <pepper.h>
#include <wayland-server.h>

// internal header
#include "private.h"

__thread Eina_Hash *comp_hash = NULL;

static Eina_Bool
_pepper_efl_compositor_cb_fd_read(void *data, Ecore_Fd_Handler *hdl EINA_UNUSED)
{
   pepper_efl_comp_t *comp = data;

   wl_event_loop_dispatch(comp->wl.loop, 0);

   return ECORE_CALLBACK_RENEW;
}

static void
_pepper_efl_compositor_cb_fd_prepare(void *data, Ecore_Fd_Handler *hdlr EINA_UNUSED)
{
   pepper_efl_comp_t *comp = data;

   wl_display_flush_clients(comp->wl.disp);
}

Eina_Bool
pepper_efl_compositor_destroy(const char *name)
{
   pepper_efl_comp_t *comp;
   pepper_efl_output_t *output;
   Eina_List *l;

   if (!name)
     return EINA_FALSE;

   comp = eina_hash_find(comp_hash, name);
   if (!comp)
     return EINA_FALSE;

   eina_hash_del(comp_hash, name, NULL);
   if (eina_hash_population(comp_hash) == 0)
     PE_FREE_FUNC(comp_hash, eina_hash_free);

   EINA_LIST_FOREACH(comp->output_list, l, output)
      pepper_output_destroy(output->base);

   pepper_efl_shell_shutdown();

   PE_FREE_FUNC(comp->seat, pepper_efl_input_destroy);
   PE_FREE_FUNC(comp->name, eina_stringshare_del);
   PE_FREE_FUNC(comp->pepper.comp, pepper_compositor_destroy);
   PE_FREE_FUNC(comp->fd_hdlr, ecore_main_fd_handler_del);

   return EINA_TRUE;
}

const char *
pepper_efl_compositor_create(Evas_Object *win, const char *name)
{
   pepper_efl_comp_t *comp;
   pepper_efl_output_t *output;
   Eina_Bool first_init = EINA_FALSE;
   int loop_fd;
   const char *sock_name;

   if (comp_hash)
     {
        comp = eina_hash_find(comp_hash, name);
        if (comp)
          goto create_output;
     }
   else
     {
        comp_hash = eina_hash_string_superfast_new(NULL);
        first_init = EINA_TRUE;
     }

   if (!pepper_efl_log_init("pepper-efl"))
     goto err;

   DBG("create compositor");

   comp = calloc(1, sizeof(pepper_efl_comp_t));
   if (!comp)
     {
        ERR("oom, alloc comp");
        goto err_alloc;
     }

   comp->pepper.comp = pepper_compositor_create(name);
   if (!comp->pepper.comp)
     {
        ERR("failed to create pepper compositor");
        goto err_comp;
     }

   if (!pepper_efl_shell_init(comp))
     {
        ERR("failed to init shell");
        goto err_shell;
     }

   comp->seat = pepper_efl_input_create(comp);
   if (!comp->seat)
     {
        ERR("failed to init input");
        goto err_seat;
     }

   sock_name = pepper_compositor_get_socket_name(comp->pepper.comp);
   comp->name = eina_stringshare_add(sock_name);
   comp->wl.disp = pepper_compositor_get_display(comp->pepper.comp);

   comp->wl.loop = wl_display_get_event_loop(comp->wl.disp);
   loop_fd = wl_event_loop_get_fd(comp->wl.loop);
   comp->fd_hdlr =
      ecore_main_fd_handler_add(loop_fd, (ECORE_FD_READ | ECORE_FD_ERROR),
                                _pepper_efl_compositor_cb_fd_read, comp,
                                NULL, NULL);
   ecore_main_fd_handler_prepare_callback_set(comp->fd_hdlr,
                                              _pepper_efl_compositor_cb_fd_prepare,
                                              comp);

create_output:
   output = pepper_efl_output_create(comp, win);
   if (!output)
     {
        ERR("failed to create output");

        if (first_init)
          goto err_output;

        return NULL;
     }

   eina_hash_add(comp_hash, comp->name, comp);

   return comp->name;

err_output:
   eina_stringshare_del(comp->name);
   ecore_main_fd_handler_del(comp->fd_hdlr);

err_seat:
   pepper_efl_shell_shutdown();

err_shell:
   pepper_compositor_destroy(comp->pepper.comp);

err_comp:
   free(comp);

err_alloc:
   pepper_efl_log_shutdown();

err:
   return NULL;
}
