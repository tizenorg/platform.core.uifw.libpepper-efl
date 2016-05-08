#include "private.h"

#include <xdg-shell-server-protocol.h>

static void
pepper_efl_shell_surface_destroy(pepper_efl_shell_surface_t *shsurf)
{
   pepper_efl_shell_client_t *sc;

   sc = shsurf->shell_client;
   if (sc)
     sc->shsurf_list = eina_list_remove(sc->shsurf_list, shsurf);

   if (shsurf->surface_destroy_listener)
     {
        pepper_event_listener_remove(shsurf->surface_destroy_listener);
        shsurf->surface_destroy_listener = NULL;
     }

   if (shsurf->title)
     eina_stringshare_del(shsurf->title);

   if (shsurf->app_id)
     eina_stringshare_del(shsurf->app_id);

   if (shsurf->surface)
     pepper_object_set_user_data((pepper_object_t *)shsurf->surface,
                                 pepper_surface_get_role((pepper_surface_t *)shsurf->surface),
                                 NULL, NULL);

   free(shsurf);
}

static void
handle_resource_destroy(struct wl_resource *resource)
{
   pepper_efl_shell_surface_t *shsurf;

   shsurf = wl_resource_get_user_data(resource);
   PE_CHECK(shsurf);

   wl_resource_set_user_data(resource, NULL);
   pepper_efl_shell_surface_destroy(shsurf);
}

static void
handle_surface_destroy(pepper_event_listener_t *listener, pepper_object_t *surface, uint32_t id, void *info, void *data)
{
   pepper_efl_shell_surface_t *shsurf;

   DBG("Destroy Shell Surface");

   shsurf = data;
   PE_CHECK(shsurf);

   /* 'surface_destroy_listener' and 'user_data' of surface will be freed
    * by caller. should be set null to avoid double free. */
   shsurf->surface_destroy_listener = NULL;

   shsurf->surface = NULL;
}

static void
handle_surface_commit(pepper_event_listener_t *listener, pepper_object_t *surface, uint32_t id, void *info, void *data)
{
   pepper_efl_shell_surface_t *shsurf;

   shsurf = data;
   PE_CHECK(shsurf);

   if ((shsurf->mapped) &&
       (!pepper_view_is_mapped(shsurf->view)))
     pepper_view_map(shsurf->view);
}

static void
shsurf_xdg_surface_map(pepper_efl_shell_surface_t *shsurf)
{
   DBG("Map XDG surface");

   pepper_view_map(shsurf->view);

   shsurf->mapped = EINA_TRUE;
}

static void
shsurf_xdg_surface_send_configure(pepper_efl_shell_surface_t *shsurf, int32_t width, int32_t height)
{
   struct wl_display *display;
   struct wl_array states;
   uint32_t *state;
   uint32_t serial;

   wl_array_init(&states);

   state = wl_array_add(&states, sizeof(uint32_t));
   *state = XDG_SURFACE_STATE_ACTIVATED; // this is arbitary.

   display = pepper_compositor_get_display(shsurf->comp->pepper.comp);
   serial = wl_display_next_serial(display);

   xdg_surface_send_configure(shsurf->resource, width, height, &states, serial);

   wl_array_release(&states);

   shsurf->ack_configure = EINA_FALSE;
}

static void
xdg_surface_cb_destroy(struct wl_client *client, struct wl_resource *resource)
{
   DBG("client %p", client);

   wl_resource_destroy(resource);
}

static void
xdg_surface_cb_set_parent(struct wl_client *client, struct wl_resource *resource, struct wl_resource *parent_resource)
{
   DBG("client %p", client);
}

static void
xdg_surface_cb_set_title(struct wl_client *client, struct wl_resource *resource, const char *title)
{
   pepper_efl_shell_surface_t *shsurf = wl_resource_get_user_data(resource);
   DBG("client %p", client);

   if (shsurf->title)
     eina_stringshare_del(shsurf->title);
   shsurf->title = eina_stringshare_add(title);
}

static void
xdg_surface_cb_set_app_id(struct wl_client *client, struct wl_resource *resource, const char *app_id)
{
   pepper_efl_shell_surface_t *shsurf = wl_resource_get_user_data(resource);
   DBG("client %p", client);

   if (shsurf->app_id)
     eina_stringshare_del(shsurf->app_id);
   shsurf->app_id = eina_stringshare_add(app_id);
}

static void
xdg_surface_cb_show_window_menu(struct wl_client *client, struct wl_resource *surface_resource, struct wl_resource *seat_resource, uint32_t serial, int32_t x, int32_t y)
{
   DBG("client %p", client);
}

static void
xdg_surface_cb_move(struct wl_client *client, struct wl_resource *resource, struct wl_resource *seat_resource, uint32_t serial)
{
   DBG("client %p", client);
}

static void
xdg_surface_cb_resize(struct wl_client *client, struct wl_resource *resource, struct wl_resource *seat_resource, uint32_t serial, uint32_t edges)
{
   DBG("client %p", client);
}

static void
xdg_surface_cb_ack_configure(struct wl_client *client, struct wl_resource *resource, uint32_t serial)
{
   pepper_efl_shell_surface_t *shsurf = wl_resource_get_user_data(resource);

   DBG("client %p", client);

   shsurf->ack_configure = EINA_TRUE;
   if (shsurf->configure_done.func)
     {
        shsurf->configure_done.func(shsurf->configure_done.data,
                                    shsurf->configure_done.width,
                                    shsurf->configure_done.height);
        shsurf->configure_done.func = NULL;
        shsurf->configure_done.data = NULL;
     }
}

static void
xdg_surface_cb_set_window_geometry(struct wl_client *client, struct wl_resource *resource, int32_t x, int32_t y, int32_t width, int32_t height)
{
   DBG("-");
}

static void
xdg_surface_cb_set_maximized(struct wl_client *client, struct wl_resource *resource)
{
   DBG("-");
}

static void
xdg_surface_cb_unset_maximized(struct wl_client *client, struct wl_resource *resource)
{
   DBG("-");
}

static void
xdg_surface_cb_set_fullscreen(struct wl_client *client, struct wl_resource *resource, struct wl_resource *output_resource)
{
   DBG("-");
}

static void
xdg_surface_cb_unset_fullscreen(struct wl_client *client, struct wl_resource *resource)
{
   DBG("-");
}

static void
xdg_surface_cb_set_minimized(struct wl_client *client, struct wl_resource *resource)
{
   DBG("-");
}

static const struct xdg_surface_interface xdg_surface_implementation =
{
   xdg_surface_cb_destroy,
   xdg_surface_cb_set_parent,
   xdg_surface_cb_set_title,
   xdg_surface_cb_set_app_id,
   xdg_surface_cb_show_window_menu,
   xdg_surface_cb_move,
   xdg_surface_cb_resize,
   xdg_surface_cb_ack_configure,
   xdg_surface_cb_set_window_geometry,
   xdg_surface_cb_set_maximized,
   xdg_surface_cb_unset_maximized,
   xdg_surface_cb_set_fullscreen,
   xdg_surface_cb_unset_fullscreen,
   xdg_surface_cb_set_minimized,
};

static void xdg_shell_cb_destroy(struct wl_client *client,
                                 struct wl_resource *resource)
{
   DBG("-");
}

static void
xdg_shell_cb_use_unstable_version(struct wl_client *client, struct wl_resource *resource, int32_t version)
{
   DBG("-");
}

static void
xdg_shell_cb_surface_get(struct wl_client *client, struct wl_resource *resource, uint32_t id, struct wl_resource *surface_resource)
{
   pepper_surface_t *surface = wl_resource_get_user_data(surface_resource);
   pepper_efl_shell_client_t *sc = wl_resource_get_user_data(resource);
   pepper_efl_shell_surface_t *shsurf;

   DBG("Get XDG surface");

   shsurf = calloc(1, sizeof(*shsurf));
   if (!shsurf)
     {
        ERR("out of memory");
        goto err;
     }

   shsurf->resource = wl_resource_create(client, &xdg_surface_interface, 1, id);
   if (!shsurf->resource)
     {
        ERR("failed to create resource for xdg_surface");
        goto err;
     }

   shsurf->comp = sc->comp;
   shsurf->shell_client = sc;
   shsurf->surface = surface;
   shsurf->view = pepper_compositor_add_view(sc->comp->pepper.comp);
   if (!shsurf->view)
     {
        ERR("failed to add pepper_view");
        goto err;
     }

   if (!pepper_view_set_surface(shsurf->view, surface))
     {
        ERR("failed to set surface to view");
        goto err;
     }

   wl_resource_set_implementation(shsurf->resource, &xdg_surface_implementation,
                                  shsurf, handle_resource_destroy);

   shsurf->surface_destroy_listener =
      pepper_object_add_event_listener((pepper_object_t *)surface, PEPPER_EVENT_OBJECT_DESTROY, 0,
                                       handle_surface_destroy, shsurf);
   shsurf->surface_commit_listener =
      pepper_object_add_event_listener((pepper_object_t *)surface, PEPPER_EVENT_SURFACE_COMMIT, 0,
                                       handle_surface_commit, shsurf);

   shsurf->send_configure = shsurf_xdg_surface_send_configure;
   shsurf->shell_surface_map = shsurf_xdg_surface_map;

   shsurf->mapped = EINA_FALSE;

   sc->shsurf_list = eina_list_append(sc->shsurf_list, shsurf);

   pepper_surface_set_role(surface, "xdg_surface");

   pepper_object_set_user_data((pepper_object_t *)surface,
                               pepper_surface_get_role(surface), shsurf, NULL);

   return;
err:
   if (shsurf)
     free(shsurf);

   wl_client_post_no_memory(client);
}

static void
xdg_shell_cb_popup_get(struct wl_client *client, struct wl_resource *resource, uint32_t id, struct wl_resource *surface_resource, struct wl_resource *parent_resource, struct wl_resource *seat_resource, uint32_t serial, int32_t x, int32_t y)
{
   DBG("-");
}

static void
xdg_shell_cb_pong(struct wl_client *client, struct wl_resource *resource, uint32_t serial)
{
   DBG("-");
}

static const struct xdg_shell_interface xdg_implementation = {
     xdg_shell_cb_destroy,
     xdg_shell_cb_use_unstable_version,
     xdg_shell_cb_surface_get,
     xdg_shell_cb_popup_get,
     xdg_shell_cb_pong
};

static void
shell_client_destroy_handle(struct wl_resource *resource)
{
   pepper_efl_shell_client_t *sc;
   pepper_efl_shell_surface_t *shsurf;

   sc = wl_resource_get_user_data(resource);
   DBG("Destroy shell client");

   EINA_LIST_FREE(sc->shsurf_list, shsurf)
      shsurf->shell_client = NULL;

   free(sc);
}

static void
xdg_shell_bind(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
   Pepper_Efl_Comp *comp = data;
   pepper_efl_shell_client_t *sc;

   DBG("Bind Shell - client %p version %d", client, version);

   sc = calloc(1, sizeof(pepper_efl_shell_client_t));
   if (!sc)
     {
        wl_client_post_no_memory(client);
        return;
     }

   sc->resource = wl_resource_create(client, &xdg_shell_interface, version, id);
   if (!sc->resource)
     {
        wl_client_post_no_memory(client);
        free(sc);
        return;
     }

   wl_resource_set_implementation(sc->resource, &xdg_implementation, sc, shell_client_destroy_handle);

   sc->comp  = comp;
}

Eina_Bool
pepper_efl_shell_init(Pepper_Efl_Comp *comp)
{
   struct wl_display *wl_disp;

   DBG("Init Shell");

   wl_disp = pepper_compositor_get_display(comp->pepper.comp);
   if (!wl_global_create(wl_disp, &xdg_shell_interface, 1, comp, xdg_shell_bind))
     {
        ERR("failed to create global for xdg_shell");
        return EINA_FALSE;
     }

   return EINA_TRUE;
}

void
pepper_efl_shell_shutdown(void)
{
   DBG("Shutdown Shell");
}

void
pepper_efl_shell_configure(pepper_efl_shell_surface_t *shsurf, int width, int height, void (*configure_done)(void *data, int w, int h), void *data)
{
   DBG("shell - configure send (%d x %d)", width, height);

   shsurf->send_configure(shsurf, width, height);

   shsurf->configure_done.width = width;
   shsurf->configure_done.height = height;
   shsurf->configure_done.func = configure_done;
   shsurf->configure_done.data = data;
}
