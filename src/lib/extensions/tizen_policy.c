#include "private.h"

#include <tizen-extension-server-protocol.h>

/* tizen policy data for pepper_surface_t */
typedef struct
{
   pepper_surface_t *psurf;
   pepper_event_listener_t *psurf_destroy_listener;
   struct wl_list resource_list;
   Pepper_Efl_Visibility_Type visibility;
   int references;
} tzpol_surface_t;

const static char       *tsurf_data_key = "tzpol-surf";
static struct wl_global *_tzpol_global = NULL;
static struct wl_list    _tzpol_res_list;

static void
_tzpol_surface_del(tzpol_surface_t *tsurf)
{
   free(tsurf);
}

static void
_tzpol_surface_unref(tzpol_surface_t *tsurf, struct wl_resource *unref)
{
   int res;

   res = wl_list_empty(&tsurf->resource_list);
   if (res)
     {
        CRI("Couldn't unreference, list is empty");
        return;
     }

   wl_list_remove(wl_resource_get_link(unref));
   wl_resource_set_user_data(unref, NULL);

   res = wl_list_empty(&tsurf->resource_list);
   if (res)
     {
        pepper_event_listener_remove(tsurf->psurf_destroy_listener);
        pepper_object_set_user_data((pepper_object_t *)tsurf->psurf,
                                    (const void *)tsurf_data_key, NULL, NULL);
        _tzpol_surface_del(tsurf);
     }
}

static void
_tzpol_surface_ref(tzpol_surface_t *tsurf, struct wl_resource *ref)
{
   wl_list_insert(&tsurf->resource_list, wl_resource_get_link(ref));
   wl_resource_set_user_data(ref, tsurf);
}

static void
_tzpol_surface_pepper_surf_cb_destroy(pepper_event_listener_t *listener, pepper_object_t *object, uint32_t id, void *info, void *data)
{
   tzpol_surface_t *tsurf;
   struct wl_resource *res, *tmp;

   tsurf = data;

   wl_resource_for_each_safe(res, tmp, &tsurf->resource_list)
        _tzpol_surface_unref(tsurf, res);
}

static tzpol_surface_t *
_tzpol_surface_get(pepper_surface_t *psurf)
{
   tzpol_surface_t *tsurf;

   tsurf = pepper_object_get_user_data((pepper_object_t *)psurf,
                                       (const void *)tsurf_data_key);
   if (!tsurf)
     {
        tsurf = calloc(1, sizeof(*tsurf));
        if (!tsurf)
          return NULL;

        tsurf->psurf = psurf;

        wl_list_init(&tsurf->resource_list);

        tsurf->psurf_destroy_listener =
           pepper_object_add_event_listener((pepper_object_t *)psurf,
                                            PEPPER_EVENT_OBJECT_DESTROY, 0,
                                            _tzpol_surface_pepper_surf_cb_destroy, tsurf);
        pepper_object_set_user_data((pepper_object_t *)psurf,
                                    (const void *)tsurf_data_key, tsurf, NULL);
     }

   tsurf->references++;

   return tsurf;
}

// --------------------------------------------------------
// visibility
// --------------------------------------------------------
static void
_tzvis_iface_cb_destroy(struct wl_client *client, struct wl_resource *res_tzvis)
{
   wl_resource_destroy(res_tzvis);
}

static const struct tizen_visibility_interface _tzvis_iface =
{
   _tzvis_iface_cb_destroy
};

static void
_tzvis_res_cb_destroy(struct wl_resource *res_tzvis)
{
   tzpol_surface_t *tsurf;

   tsurf = wl_resource_get_user_data(res_tzvis);
   if (!tsurf)
     return;

   _tzpol_surface_unref(tsurf, res_tzvis);
}

static void
_tzpol_iface_cb_vis_get(struct wl_client *client, struct wl_resource *res_tzpol, uint32_t id, struct wl_resource *surf)
{
   tzpol_surface_t *tsurf;
   pepper_surface_t *psurf;
   struct wl_resource *new_res;
   int ver;

   if (!surf)
     {
        wl_resource_post_error(res_tzpol, WL_DISPLAY_ERROR_INVALID_OBJECT,
                               "tizen_policy::visibility_get requires surface");
        return;
     }

   psurf = wl_resource_get_user_data(surf);
   if (!psurf)
     {
        ERR("failed to get pepper_surface from wl_resource");
        wl_resource_post_error(res_tzpol, WL_DISPLAY_ERROR_INVALID_OBJECT,
                               "tizen_policy::visibility_get invalid surface resource");
        return;
     }

   DBG("tizen_policy::visibility_get");
   tsurf = _tzpol_surface_get(psurf);
   if (!tsurf)
     {
        wl_resource_post_no_memory(res_tzpol);
        return;
     }

   ver = wl_resource_get_version(res_tzpol);
   new_res = wl_resource_create(client, &tizen_visibility_interface, ver, id);
   if (!new_res)
     {
        wl_resource_post_no_memory(res_tzpol);
        return;
     }

   wl_resource_set_implementation(new_res, &_tzvis_iface, tsurf, _tzvis_res_cb_destroy);
   _tzpol_surface_ref(tsurf, new_res);
}

// --------------------------------------------------------
// position
// --------------------------------------------------------
static void
_tzpos_iface_cb_destroy(struct wl_client *client EINA_UNUSED, struct wl_resource *res_tzpos)
{
   wl_resource_destroy(res_tzpos);
}

static void
_tzpos_iface_cb_set(struct wl_client *client, struct wl_resource *res_tzpos, int32_t x, int32_t y)
{
   (void)client;
   (void)res_tzpos;
   (void)x;
   (void)y;
}

static const struct tizen_position_interface _tzpos_iface =
{
   _tzpos_iface_cb_destroy,
   _tzpos_iface_cb_set,
};

static void
_tzpos_res_cb_destroy(struct wl_resource *res_tzpos)
{
   tzpol_surface_t *tsurf;

   tsurf = wl_resource_get_user_data(res_tzpos);
   if (!tsurf)
     return;

   _tzpol_surface_unref(tsurf, res_tzpos);
}

static void
_tzpol_iface_cb_pos_get(struct wl_client *client, struct wl_resource *res_tzpol, uint32_t id, struct wl_resource *surf)
{
   tzpol_surface_t *tsurf;
   pepper_surface_t *psurf;
   struct wl_resource *new_res;
   int ver;

   if (!surf)
     {
        wl_resource_post_error(res_tzpol, WL_DISPLAY_ERROR_INVALID_OBJECT,
                               "tizen_policy::visibility_get requires surface");
        return;
     }

   psurf = wl_resource_get_user_data(surf);
   if (!psurf)
     {
        ERR("failed to get pepper_surface from wl_resource");
        wl_resource_post_error(res_tzpol, WL_DISPLAY_ERROR_INVALID_OBJECT,
                               "tizen_policy::visibility_get invalid surface resource");
        return;
     }

   DBG("tizen_policy::visibility_get");
   tsurf = _tzpol_surface_get(psurf);
   if (!tsurf)
     {
        wl_resource_post_no_memory(res_tzpol);
        return;
     }

   ver = wl_resource_get_version(res_tzpol);
   new_res = wl_resource_create(client, &tizen_position_interface, ver, id);
   if (!new_res)
     {
        wl_resource_post_no_memory(res_tzpol);
        return;
     }

   wl_resource_set_implementation(new_res, &_tzpos_iface, tsurf, _tzpos_res_cb_destroy);
   _tzpol_surface_ref(tsurf, new_res);
}

// --------------------------------------------------------
// stack: activate, raise, lower
// --------------------------------------------------------
static void
_tzpol_iface_cb_activate(struct wl_client *client, struct wl_resource *res_tzpol, struct wl_resource *surf)
{
   (void)client;
   (void)res_tzpol;
   (void)surf;
}

static void
_tzpol_iface_cb_activate_below_by_res_id(struct wl_client *client, struct wl_resource *res_tzpol,  uint32_t res_id, uint32_t below_res_id)
{
   (void)client;
   (void)res_tzpol;
   (void)res_id;
   (void)below_res_id;
}

static void
_tzpol_iface_cb_raise(struct wl_client *client, struct wl_resource *res_tzpol, struct wl_resource *surf)
{
   (void)client;
   (void)res_tzpol;
   (void)surf;
}

static void
_tzpol_iface_cb_lower(struct wl_client *client, struct wl_resource *res_tzpol, struct wl_resource *surf)
{
   (void)client;
   (void)res_tzpol;
   (void)surf;
}

static void
_tzpol_iface_cb_lower_by_res_id(struct wl_client *client, struct wl_resource *res_tzpol,  uint32_t res_id)
{
   (void)client;
   (void)res_tzpol;
   (void)res_id;
}

// --------------------------------------------------------
// focus
// --------------------------------------------------------
static void
_tzpol_iface_cb_focus_skip_set(struct wl_client *client, struct wl_resource *res_tzpol, struct wl_resource *surf)
{
   (void)client;
   (void)res_tzpol;
   (void)surf;
}

static void
_tzpol_iface_cb_focus_skip_unset(struct wl_client *client, struct wl_resource *res_tzpol, struct wl_resource *surf)
{
   (void)client;
   (void)res_tzpol;
   (void)surf;
}

// --------------------------------------------------------
// role
// --------------------------------------------------------
static void
_tzpol_iface_cb_role_set(struct wl_client *client, struct wl_resource *res_tzpol, struct wl_resource *surf, const char *role)
{
   (void)client;
   (void)res_tzpol;
   (void)surf;
   (void)role;
}

static void
_tzpol_iface_cb_type_set(struct wl_client *client, struct wl_resource *res_tzpol, struct wl_resource *surf, uint32_t type)
{
   (void)client;
   (void)res_tzpol;
   (void)surf;
   (void)type;
}

// --------------------------------------------------------
// conformant
// --------------------------------------------------------
static void
_tzpol_iface_cb_conformant_set(struct wl_client *client, struct wl_resource *res_tzpol, struct wl_resource *surf)
{
   (void)client;
   (void)res_tzpol;
   (void)surf;
}

static void
_tzpol_iface_cb_conformant_unset(struct wl_client *client, struct wl_resource *res_tzpol, struct wl_resource *surf)
{
   (void)client;
   (void)res_tzpol;
   (void)surf;
}

static void
_tzpol_iface_cb_conformant_get(struct wl_client *client, struct wl_resource *res_tzpol, struct wl_resource *surf)
{
   (void)client;
   (void)res_tzpol;
   (void)surf;
}

// --------------------------------------------------------
// notification level
// --------------------------------------------------------
static void
_tzpol_iface_cb_notilv_set(struct wl_client *client, struct wl_resource *res_tzpol, struct wl_resource *surf, int32_t lv)
{
   (void)client;
   (void)res_tzpol;
   (void)surf;
   (void)lv;
}

// --------------------------------------------------------
// transient for
// --------------------------------------------------------
static void
_tzpol_iface_cb_transient_for_set(struct wl_client *client, struct wl_resource *res_tzpol, uint32_t child_id, uint32_t parent_id)
{
   (void)client;
   (void)res_tzpol;
   (void)child_id;
   (void)parent_id;
}

static void
_tzpol_iface_cb_transient_for_unset(struct wl_client *client, struct wl_resource *res_tzpol, uint32_t child_id)
{
   (void)client;
   (void)res_tzpol;
   (void)child_id;
}

// --------------------------------------------------------
// window screen mode
// --------------------------------------------------------
static void
_tzpol_iface_cb_win_scrmode_set(struct wl_client *client, struct wl_resource *res_tzpol, struct wl_resource *surf, uint32_t mode)
{
   (void)client;
   (void)res_tzpol;
   (void)surf;
   (void)mode;
}

// --------------------------------------------------------
// subsurface
// --------------------------------------------------------
static void
_tzpol_iface_cb_subsurf_place_below_parent(struct wl_client *client, struct wl_resource *res_tzpol, struct wl_resource *subsurf)
{
   (void)client;
   (void)res_tzpol;
   (void)subsurf;
}

static void
_tzpol_iface_cb_subsurf_stand_alone_set(struct wl_client *client, struct wl_resource *res_tzpol, struct wl_resource *subsurf)
{
   (void)client;
   (void)res_tzpol;
   (void)subsurf;
}

static void
_tzpol_iface_cb_subsurface_get(struct wl_client *client, struct wl_resource *res_tzpol, uint32_t id, struct wl_resource *surface, uint32_t parent_id)
{
   (void)client;
   (void)res_tzpol;
   (void)id;
   (void)surface;
   (void)parent_id;
}

static void
_tzpol_iface_cb_opaque_state_set(struct wl_client *client, struct wl_resource *res_tzpol, struct wl_resource *surface, int32_t state)
{
   (void)client;
   (void)res_tzpol;
   (void)surface;
   (void)state;
}

// --------------------------------------------------------
// iconify
// --------------------------------------------------------
static void
_tzpol_iface_cb_iconify(struct wl_client *client, struct wl_resource *res_tzpol, struct wl_resource *surf)
{
   (void)client;
   (void)res_tzpol;
   (void)surf;
}

static void
_tzpol_iface_cb_uniconify(struct wl_client *client, struct wl_resource *res_tzpol, struct wl_resource *surf)
{
   (void)client;
   (void)res_tzpol;
   (void)surf;
}

static void
_tzpol_iface_cb_aux_hint_add(struct wl_client *client, struct wl_resource *res_tzpol, struct wl_resource *surf, int32_t id, const char *name, const char *value)
{
   (void)client;
   (void)res_tzpol;
   (void)surf;
   (void)id;
   (void)name;
   (void)value;
}

static void
_tzpol_iface_cb_aux_hint_change(struct wl_client *client, struct wl_resource *res_tzpol, struct wl_resource *surf, int32_t id, const char *value)
{
   (void)client;
   (void)res_tzpol;
   (void)surf;
   (void)id;
   (void)value;
}

static void
_tzpol_iface_cb_aux_hint_del(struct wl_client *client, struct wl_resource *res_tzpol, struct wl_resource *surf, int32_t id)
{
   (void)client;
   (void)res_tzpol;
   (void)surf;
   (void)id;
}

static void
_tzpol_iface_cb_supported_aux_hints_get(struct wl_client *client, struct wl_resource *res_tzpol, struct wl_resource *surf)
{
   (void)client;
   (void)res_tzpol;
   (void)surf;
}

static void
_tzpol_iface_cb_background_state_set(struct wl_client *client, struct wl_resource *res_tzpol, uint32_t pid)
{
   (void)client;
   (void)res_tzpol;
   (void)pid;
}

static void
_tzpol_iface_cb_background_state_unset(struct wl_client *client, struct wl_resource *res_tzpol, uint32_t pid)
{
   (void)client;
   (void)res_tzpol;
   (void)pid;
}

static void
_tzpol_iface_cb_floating_mode_set(struct wl_client *client, struct wl_resource *res_tzpol, struct wl_resource *surf)
{
   (void)client;
   (void)res_tzpol;
   (void)surf;
}

static void
_tzpol_iface_cb_floating_mode_unset(struct wl_client *client, struct wl_resource *res_tzpol, struct wl_resource *surf)
{
   (void)client;
   (void)res_tzpol;
   (void)surf;
}

static void
_tzpol_iface_cb_stack_mode_set(struct wl_client *client, struct wl_resource *res_tzpol, struct wl_resource *surf, uint32_t mode)
{
   (void)client;
   (void)res_tzpol;
   (void)surf;
   (void)mode;
}

static const struct tizen_policy_interface _tzpol_iface =
{
   _tzpol_iface_cb_vis_get,
   _tzpol_iface_cb_pos_get,
   _tzpol_iface_cb_activate,
   _tzpol_iface_cb_activate_below_by_res_id,
   _tzpol_iface_cb_raise,
   _tzpol_iface_cb_lower,
   _tzpol_iface_cb_lower_by_res_id,
   _tzpol_iface_cb_focus_skip_set,
   _tzpol_iface_cb_focus_skip_unset,
   _tzpol_iface_cb_role_set,
   _tzpol_iface_cb_type_set,
   _tzpol_iface_cb_conformant_set,
   _tzpol_iface_cb_conformant_unset,
   _tzpol_iface_cb_conformant_get,
   _tzpol_iface_cb_notilv_set,
   _tzpol_iface_cb_transient_for_set,
   _tzpol_iface_cb_transient_for_unset,
   _tzpol_iface_cb_win_scrmode_set,
   _tzpol_iface_cb_subsurf_place_below_parent,
   _tzpol_iface_cb_subsurf_stand_alone_set,
   _tzpol_iface_cb_subsurface_get,
   _tzpol_iface_cb_opaque_state_set,
   _tzpol_iface_cb_iconify,
   _tzpol_iface_cb_uniconify,
   _tzpol_iface_cb_aux_hint_add,
   _tzpol_iface_cb_aux_hint_change,
   _tzpol_iface_cb_aux_hint_del,
   _tzpol_iface_cb_supported_aux_hints_get,
   _tzpol_iface_cb_background_state_set,
   _tzpol_iface_cb_background_state_unset,
   _tzpol_iface_cb_floating_mode_set,
   _tzpol_iface_cb_floating_mode_unset,
   _tzpol_iface_cb_stack_mode_set,
};

static void
_tzpol_cb_unbind(struct wl_resource *resource)
{
   wl_list_remove(wl_resource_get_link(resource));
}

static void
_tzpol_cb_bind(struct wl_client *client, void *data, uint32_t ver, uint32_t id)
{
   struct wl_resource *resource;

   DBG("tizen_policy::bind");

   resource = wl_resource_create(client, &tizen_policy_interface, ver, id);
   if (!resource)
     {
        ERR("failed to create resource for tizen_policy");
        wl_client_post_no_memory(client);
        return;
     }

   wl_resource_set_implementation(resource, &_tzpol_iface, NULL, _tzpol_cb_unbind);

   wl_list_insert(&_tzpol_res_list, wl_resource_get_link(resource));
}

Eina_Bool
tizen_policy_init(pepper_efl_comp_t *comp)
{
   struct wl_display *wl_disp;

   DBG("tizen_policy::init");

   if (_tzpol_global)
     goto end;

   wl_list_init(&_tzpol_res_list);

   wl_disp = pepper_compositor_get_display(comp->pepper.comp);
   _tzpol_global = wl_global_create(wl_disp, &tizen_policy_interface, 1, NULL, _tzpol_cb_bind);
   if (!_tzpol_global)
     {
        ERR("failed to create global for tizen policy");
        return EINA_FALSE;
     }
end:
   return EINA_TRUE;
}

void
tizen_policy_shutdown(void)
{
   struct wl_resource *res, *tmp;

   DBG("tizen_policy::shutdown");

   wl_resource_for_each_safe(res, tmp, &_tzpol_res_list)
     {
        wl_list_remove(wl_resource_get_link(res));
        wl_resource_destroy(res);
     }

   wl_global_destroy(_tzpol_global);
   _tzpol_global = NULL;
}

Eina_Bool
pepper_efl_object_visibility_set(Evas_Object *obj, Pepper_Efl_Visibility_Type type)
{
   tzpol_surface_t *tsurf;
   pepper_surface_t *psurf;
   struct wl_resource *resource;
   int res;

   EINA_SAFETY_ON_NULL_RETURN_VAL(obj, EINA_FALSE);

   psurf = pepper_efl_object_pepper_surface_get(obj);
   if (!psurf)
     return EINA_FALSE;

   tsurf = pepper_object_get_user_data((pepper_object_t *)psurf, (const void *)tsurf_data_key);
   if (!tsurf)
     {
        WRN("there is no data for 'tzpol_surface_t' in 'pepper_surface_t'");
        return EINA_FALSE;
     }

   if (tsurf->visibility == type)
     return EINA_TRUE;

   tsurf->visibility = type;
   wl_resource_for_each(resource, &tsurf->resource_list)
     {
        res = wl_resource_instance_of(resource, &tizen_visibility_interface, &_tzvis_iface);
        if (!res)
          continue;
        tizen_visibility_send_notify(resource, type);
     }

   return EINA_TRUE;
}
