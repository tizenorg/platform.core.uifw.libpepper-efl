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

void
pepper_efl_compositor_destroy(const char *name)
{
   pepper_efl_comp_t *comp;
   pepper_efl_output_t *output;
   Eina_List *l;

   const char *use_name, *default_name = "wayland-0";

   if (!name)
     use_name = default_name;

   comp = eina_hash_find(comp_hash, use_name);
   if (!comp)
     return;

   eina_hash_del(comp_hash, comp->name, NULL);
   if (eina_hash_population(comp_hash) == 0)
     PE_FREE_FUNC(comp_hash, eina_hash_free);

   EINA_LIST_FOREACH(comp->output_list, l, output)
      pepper_output_destroy(output->base);

   PE_FREE_FUNC(comp->name, eina_stringshare_del);
   PE_FREE_FUNC(comp->pepper.comp, pepper_compositor_destroy);
   PE_FREE_FUNC(comp->fd_hdlr, ecore_main_fd_handler_del);
}

Eina_Bool
pepper_efl_compositor_create(Evas_Object *win, const char *name)
{
   pepper_efl_comp_t *comp;
   pepper_efl_output_t *output;
   int loop_fd;
   const char *use_name;
   const char *xdg_runtime_dir = "/tmp";
   char *saved_xdg_runtime_dir = NULL;

   if (comp_hash)
     {
        comp = eina_hash_find(comp_hash, name);
        if (comp)
          goto create_output;
     }
   else
     {
        if (!pe_log_init("pepper-efl"))
          return EINA_FALSE;

        comp_hash = eina_hash_string_superfast_new(NULL);
     }

   DBG("create compositor");

   comp = calloc(1, sizeof(pepper_efl_comp_t));
   if (!comp)
     {
        ERR("oom, alloc comp");
        return EINA_FALSE;
     }

   if (!name)
     use_name = NULL;
   else
     use_name = name;

   saved_xdg_runtime_dir = getenv("XDG_RUNTIME_DIR");
   setenv("XDG_RUNTIME_DIR", xdg_runtime_dir, 1);

   comp->pepper.comp = pepper_compositor_create(use_name);
   if (!comp->pepper.comp)
     {
        ERR("failed to create pepper compositor");
        return EINA_FALSE;
     }

   setenv("XDG_RUNTIME_DIR", saved_xdg_runtime_dir, 1);

   if (!pepper_efl_shell_init(comp))
     {
        ERR("failed to init shell");
        pepper_compositor_destroy(comp->pepper.comp);
        free(comp);
        return EINA_FALSE;
     }

   if (!pepper_efl_input_init(comp))
     {
        ERR("failed to init input");
        pepper_compositor_destroy(comp->pepper.comp);
        pepper_efl_shell_shutdown();
     }

   comp->name = eina_stringshare_add(use_name);
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

   eina_hash_add(comp_hash, use_name, comp);

create_output:
   output = pepper_efl_output_create(comp, win);
   if (!output)
     {
        ERR("failed to create output");
        return EINA_FALSE;
     }

   return EINA_TRUE;
}
