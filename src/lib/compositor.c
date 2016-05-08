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
#include <wayland-tbm-server.h>

// internal header
#include "private.h"

static Eina_Hash *_comp_hash = NULL;
static pthread_mutex_t _comp_hash_lock = PTHREAD_MUTEX_INITIALIZER;

static Eina_Bool
_comp_cb_fd_read(void *data, Ecore_Fd_Handler *hdl EINA_UNUSED)
{
   Pepper_Efl_Comp *comp = data;

   wl_event_loop_dispatch(comp->wl.loop, 0);

   return ECORE_CALLBACK_RENEW;
}

static void
_comp_cb_fd_prepare(void *data, Ecore_Fd_Handler *hdlr EINA_UNUSED)
{
   Pepper_Efl_Comp *comp = data;

   wl_display_flush_clients(comp->wl.disp);
}

static void
_comp_tz_cb_socket(void *data, struct tizen_embedded_compositor *tec, int fd)
{
   int *socket_fd = (int*)data;

   *socket_fd = fd;
   return;
}

static const struct tizen_embedded_compositor_listener _tz_embedded_comp_listener =
{
   _comp_tz_cb_socket,
};

static int
_comp_socket_fd_get_from_server(Pepper_Efl_Comp *comp)
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
             tizen_embedded_compositor_add_listener(tec, &_tz_embedded_comp_listener, &fd);
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

static void
_comp_output_del(Pepper_Efl_Comp *comp, Evas_Object *eo)
{
   pepper_efl_output_t *output;

   output = eina_hash_find(comp->output_hash, &eo);
   if (EINA_UNLIKELY(!output))
     return;

   pepper_efl_output_destroy(output);
   eina_hash_del(comp->output_hash, &eo, output);
}

static void
_comp_win_cb_del(void *data, Evas *e, Evas_Object *eo, void *event_info)
{
   Pepper_Efl_Comp *comp;

   comp = data;
   if (EINA_UNLIKELY(!comp))
     return;

   _comp_output_del(comp, eo);
}

static Eina_Bool
_comp_output_add(Pepper_Efl_Comp *comp, Evas_Object *eo)
{
   pepper_efl_output_t *output;

   output = eina_hash_find(comp->output_hash, &eo);
   if (output)
     {
        ERR("Already compositing given window.");
        return EINA_TRUE;
     }

   output = pepper_efl_output_create(comp, eo);
   if (!output)
     return EINA_FALSE;

   evas_object_event_callback_add(eo, EVAS_CALLBACK_DEL, _comp_win_cb_del, comp);
   eina_hash_add(comp->output_hash, &eo, output);

   return EINA_TRUE;
}

static void
_comp_output_all_del(Pepper_Efl_Comp *comp)
{
   pepper_efl_output_t *output;
   Eina_Iterator *itr;

   itr = eina_hash_iterator_data_new(comp->output_hash);
   EINA_ITERATOR_FOREACH(itr, output)
      pepper_efl_output_destroy(output);
   eina_iterator_free(itr);
   PE_FREE_FUNC(comp->output_hash, eina_hash_free);
}

static Pepper_Efl_Comp *
_comp_create(const char *name)
{
   Pepper_Efl_Comp *comp;
   int loop_fd;
   int socket_fd = -1;
   const char *sock_name;

   comp = calloc(1, sizeof(*comp));
   if (!comp)
     return NULL;

   socket_fd = _comp_socket_fd_get_from_server(comp);
   if (socket_fd == -1)
     {
        ERR("failed to get socket_fd from server");
        goto err_fd;
     }

   comp->pepper.comp = pepper_compositor_create_fd(name, socket_fd);
   if (!comp->pepper.comp)
     {
        ERR("failed to create pepper compositor");
        goto err_pepper;
     }

   sock_name = pepper_compositor_get_socket_name(comp->pepper.comp);
   comp->name = eina_stringshare_add(sock_name);
   comp->wl.disp = pepper_compositor_get_display(comp->pepper.comp);
   comp->wl.loop = wl_display_get_event_loop(comp->wl.disp);
   loop_fd = wl_event_loop_get_fd(comp->wl.loop);
   comp->fd_hdlr = ecore_main_fd_handler_add(loop_fd,
                                             (ECORE_FD_READ | ECORE_FD_ERROR),
                                             _comp_cb_fd_read, comp, NULL, NULL);
   ecore_main_fd_handler_prepare_callback_set(comp->fd_hdlr, _comp_cb_fd_prepare, comp);

   eina_hash_add(_comp_hash, name, comp);

   return comp;
err_pepper:
   /* Is it OK not to close fd? */
   /* close(socket_fd); */
err_fd:
   free(comp);

   return NULL;
}

static void
_comp_destroy(Pepper_Efl_Comp *comp)
{
   eina_stringshare_del(comp->name);
   pepper_compositor_destroy(comp->pepper.comp);
   ecore_main_fd_handler_del(comp->fd_hdlr);
   free(comp);
}

static Eina_Bool
_comp_efl_init(void)
{
   if (!eina_init())
     return EINA_FALSE;

   if (!ecore_init())
     goto err_ecore;

   if (!pepper_efl_log_init("pepper-efl"))
     goto err_log;

   _comp_hash = eina_hash_string_superfast_new(NULL);

   return EINA_TRUE;

err_log:
   ecore_shutdown();
err_ecore:
   eina_shutdown();

   return EINA_FALSE;
}

static void
_comp_efl_shutdown(void)
{
   PE_FREE_FUNC(_comp_hash, eina_hash_free);
   pepper_efl_log_shutdown();
   ecore_shutdown();
   eina_shutdown();
}

Eina_Bool
pepper_efl_compositor_destroy(const char *name)
{
   Pepper_Efl_Comp *comp;

   if (!name)
     return EINA_FALSE;

   pthread_mutex_lock(&_comp_hash_lock);

   comp = eina_hash_find(_comp_hash, name);
   if (!comp)
     {
        pthread_mutex_unlock(&_comp_hash_lock);
        return EINA_FALSE;
     }

   _comp_output_all_del(comp);

   pepper_efl_shell_shutdown();
   tizen_policy_shutdown();

   PE_FREE_FUNC(comp->input, pepper_efl_input_destroy);
   PE_FREE_FUNC(comp->name, eina_stringshare_del);
   PE_FREE_FUNC(comp->pepper.comp, pepper_compositor_destroy);
   PE_FREE_FUNC(comp->fd_hdlr, ecore_main_fd_handler_del);

   eina_hash_del(_comp_hash, name, NULL);
   if (eina_hash_population(_comp_hash) == 0)
     _comp_efl_shutdown();

   pthread_mutex_unlock(&_comp_hash_lock);

   return EINA_TRUE;
}

const char *
pepper_efl_compositor_create(Evas_Object *win, const char *name)
{
   Pepper_Efl_Comp *comp;
   Eina_Bool first_init = EINA_FALSE;
   Eina_Bool res;

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

        res = _comp_efl_init();
        if (!res)
          goto err_efl;
     }

   comp = _comp_create(name);
   if (!comp)
     {
        ERR("failed to create compositor");
        goto err_comp;
     }

   res = pepper_efl_shell_init(comp);
   if (!res)
     {
        ERR("failed to init shell");
        goto err_shell;
     }

   if (!tizen_policy_init(comp))
     {
        ERR("failed to init extension");
        goto err_extension;
     }

   comp->tbm_server = wayland_tbm_server_init(comp->wl.disp, NULL, -1, 0);
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

create_output:
   res = _comp_output_add(comp, win);
   if (!res)
     {
        ERR("failed to add output");

        if (first_init)
          goto err_output;

        pthread_mutex_unlock(&_comp_hash_lock);

        return NULL;
     }

   pthread_mutex_unlock(&_comp_hash_lock);

   return comp->name;

err_output:
   pepper_efl_input_destroy(comp->input);

err_input:
   wayland_tbm_server_deinit(comp->tbm_server);

err_tbm:
   tizen_policy_shutdown();

err_extension:
   pepper_efl_shell_shutdown();

err_shell:
   _comp_destroy(comp);

err_comp:
   if (first_init)
     _comp_efl_shutdown();

err_efl:
   pthread_mutex_unlock(&_comp_hash_lock);

   return NULL;
}
