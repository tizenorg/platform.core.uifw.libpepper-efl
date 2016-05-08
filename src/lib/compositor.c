#ifndef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <pthread.h>

// EFL header
#include <Eina.h>
#include <Ecore.h>
#include <Ecore_Wayland.h>
// wayland compositor header
#include <pepper.h>
#include <wayland-server.h>
#include <tizen-extension-client-protocol.h>

// internal header
#include "private.h"

static Eina_Hash *_comp_hash = NULL;
static pthread_mutex_t _comp_hash_lock = PTHREAD_MUTEX_INITIALIZER;

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

static void
_pepper_efl_compositor_socket(void *data, struct tizen_embedded_compositor *tec, int fd)
{
   int *socket_fd = (int*)data;

   *socket_fd = fd;
   return;
}

static const struct tizen_embedded_compositor_listener tizen_embedded_compositor_listener =
{
   _pepper_efl_compositor_socket
};

static int
_pepper_efl_compositor_get_socket_fd_from_server(pepper_efl_comp_t *comp)
{
   Eina_Inlist *l, *tmp;
   Ecore_Wl_Global *global;
   struct tizen_embedded_compositor *tec=NULL;
   int fd = -1;

   l = ecore_wl_globals_get();
   if (!l)
      return -1;

   EINA_INLIST_FOREACH_SAFE(l, tmp, global)
     {
        if (!strcmp(global->interface, "tizen_embedded_compositor"))
          {
             tec = wl_registry_bind(ecore_wl_registry_get(),
                         global->id,
                         &tizen_embedded_compositor_interface,
                         1);
             tizen_embedded_compositor_add_listener(tec, &tizen_embedded_compositor_listener, &fd);
             break;
          }
     }

   if (!tec)
      return -1;

   tizen_embedded_compositor_get_socket(tec);
   ecore_wl_sync();

   tizen_embedded_compositor_destroy(tec);
   return fd;
}

Eina_Bool
pepper_efl_compositor_destroy(const char *name)
{
   pepper_efl_comp_t *comp;
   pepper_efl_output_t *output;
   Eina_List *l;

   if (!name)
     return EINA_FALSE;

   pthread_mutex_lock(&_comp_hash_lock);

   comp = eina_hash_find(_comp_hash, name);
   if (!comp)
     {
        pthread_mutex_unlock(&_comp_hash_lock);
        return EINA_FALSE;
     }

   EINA_LIST_FOREACH(comp->output_list, l, output)
      pepper_output_destroy(output->base);

   pepper_efl_shell_shutdown();

   PE_FREE_FUNC(comp->input, pepper_efl_input_destroy);
   PE_FREE_FUNC(comp->name, eina_stringshare_del);
   PE_FREE_FUNC(comp->pepper.comp, pepper_compositor_destroy);
   PE_FREE_FUNC(comp->fd_hdlr, ecore_main_fd_handler_del);

   eina_hash_del(_comp_hash, name, NULL);
   if (eina_hash_population(_comp_hash) == 0)
     {
        PE_FREE_FUNC(_comp_hash, eina_hash_free);
        ecore_shutdown();
        pepper_efl_log_shutdown();
     }

   pthread_mutex_unlock(&_comp_hash_lock);

   return EINA_TRUE;
}

const char *
pepper_efl_compositor_create(Evas_Object *win, const char *name)
{
   pepper_efl_comp_t *comp;
   pepper_efl_output_t *output;
   Eina_Bool first_init = EINA_FALSE;
   int loop_fd;
   int socket_fd = -1;
   const char *sock_name;

   pthread_mutex_lock(&_comp_hash_lock);

   if (_comp_hash)
     {
        comp = eina_hash_find(_comp_hash, name);

        if (comp)
          goto create_output;
     }
   else
     {
        first_init = EINA_TRUE;

        _comp_hash = eina_hash_string_superfast_new(NULL);

        if (!pepper_efl_log_init("pepper-efl"))
          {
             fprintf(stderr, "failed to init log system\n");
             goto err_log_init;
          }

        if (!ecore_init())
          {
             ERR("failed to init ecore");
             goto err_ecore_init;
          }
     }

   DBG("create compositor");
   comp = calloc(1, sizeof(pepper_efl_comp_t));
   if (!comp)
     {
        ERR("oom, alloc comp");
        goto err_alloc;
     }

   DBG("Get socket_fd from server");
   socket_fd = _pepper_efl_compositor_get_socket_fd_from_server(comp);
   if (socket_fd == -1)
     {
        ERR("failed to get socket_fd from server");
        goto err_comp;
     }

   comp->pepper.comp = pepper_compositor_create_fd(name, socket_fd);
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

   comp->tbm_server = wayland_tbm_server_init(pepper_compositor_get_display(comp->pepper.comp),
                                              NULL, -1, 0);
   if (!comp->tbm_server)
     {
        ERR("failed to create wayland_tbm_server");
        goto err_tbm;
     }

   comp->input = pepper_efl_input_create(comp);
   if (!comp->input)
     {
        ERR("failed to init input");
        goto err_input;
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

        pthread_mutex_unlock(&_comp_hash_lock);

        return NULL;
     }

   eina_hash_add(_comp_hash, comp->name, comp);

   pthread_mutex_unlock(&_comp_hash_lock);

   return comp->name;

err_output:
   eina_stringshare_del(comp->name);
   ecore_main_fd_handler_del(comp->fd_hdlr);

err_input:
   /* Shutdown the wayland tbm server */
   /* wayland_tbm_server_shutdown(comp->tbm_server); */

err_tbm:
   pepper_efl_shell_shutdown();

err_shell:
   pepper_compositor_destroy(comp->pepper.comp);

err_comp:
   free(comp);

err_alloc:
   ecore_shutdown();

err_ecore_init:
   pepper_efl_log_shutdown();

err_log_init:
   PE_FREE_FUNC(_comp_hash, eina_hash_free);

   pthread_mutex_unlock(&_comp_hash_lock);

   return NULL;
}
