#include "private.h"
#include <linux/input.h>

typedef struct _Pepper_Efl_Smart_Data
{
   Evas_Object_Smart_Clipped_Data base;

   Evas *evas;
   Evas_Object *parent;
   Evas_Object *smart_obj;
   Evas_Object *co;

   pepper_surface_t *surface;
   pepper_buffer_t *buffer;

   struct wl_shm_buffer *shm_buffer;
   int x, y, w, h;

   struct
   {
      pepper_event_listener_t *buffer;
      pepper_event_listener_t *surface;
   } destroy_listener;

   struct
   {
      pepper_pointer_t *ptr;
      pepper_keyboard_t *kbd;
      pepper_touch_t *touch;
   } input;
} Pepper_Efl_Smart_Data;

#define _smart_obj_type "Pepper_Efl_Smart"

#define SMART_DATA_GET(o, ptr)                           \
   Pepper_Efl_Smart_Data *ptr = evas_object_smart_data_get(o)    \

#define SMART_DATA_GET_OR_RETURN(o, ptr)                 \
   SMART_DATA_GET(o, ptr);                               \
   if (!ptr)                                             \
     {                                                   \
        ERR("No smart data for object %p (%s)",          \
            o, evas_object_type_get(o));                 \
        abort();                                         \
        return;                                          \
     }

#define SMART_DATA_GET_OR_RETURN_VAL(o, ptr, val)        \
   SMART_DATA_GET(o, ptr);                               \
   if (!ptr)                                             \
     {                                                   \
        ERR("No smart data for object %p (%s)",          \
            o, evas_object_type_get(o));                 \
        abort();                                         \
        return val;                                      \
     }

static const Evas_Smart_Cb_Description _smart_callbacks[] =
{
     {NULL, NULL}
};

EVAS_SMART_SUBCLASS_NEW(_smart_obj_type, _pepper_efl,
                        Evas_Smart_Class, Evas_Smart_Class,
                        evas_object_smart_clipped_class_get, _smart_callbacks)

static Pepper_Efl_Smart_Data *_mouse_in_po = NULL;
static Eina_Bool _need_send_motion = EINA_TRUE;
static Eina_Bool _need_send_released = EINA_FALSE;

static void
_pepper_efl_smart_add(Evas_Object *obj)
{
   EVAS_SMART_DATA_ALLOC(obj, Pepper_Efl_Smart_Data);

   _pepper_efl_parent_sc->add(obj);
}

static void
_pepper_efl_smart_del(Evas_Object *obj)
{
   SMART_DATA_GET(obj, sd);

   if (sd->buffer)
     {
        pepper_buffer_unreference(sd->buffer);
        sd->buffer = NULL;

        PE_FREE_FUNC(sd->destroy_listener.buffer, pepper_event_listener_remove);
     }

   PE_FREE_FUNC(sd->destroy_listener.surface, pepper_event_listener_remove);

   _pepper_efl_parent_sc->del(obj);
}

static void
_pepper_efl_smart_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y)
{
   SMART_DATA_GET(obj, sd);

   _pepper_efl_parent_sc->move(obj, x, y);

   if ((sd->x == x) && (sd->y == y)) return;
   sd->x = x;
   sd->y = y;

   evas_object_smart_changed(obj);
}

static void
_pepper_efl_object_cb_shell_configure_done(void *data, int w, int h)
{
   Evas_Object *co = data;

   evas_object_image_size_set(co, w, h);
   evas_object_resize(co, w, h);
}

static void
_pepper_efl_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h)
{
   pepper_efl_shell_surface_t *shsurf = NULL;

   SMART_DATA_GET(obj, sd);

   if (!sd->co)
     return;

   if ((sd->w == w) && (sd->h == h))
     return;

   sd->w = w;
   sd->h = h;

   if (sd->surface)
     {
        shsurf =
           pepper_object_get_user_data((pepper_object_t *)sd->surface,
                                       pepper_surface_get_role(sd->surface));
     }

   if (!shsurf)
     {
        evas_object_image_size_set(sd->co, w, h);
        evas_object_resize(sd->co, w, h);
     }
   else
     {
        pepper_efl_shell_configure(shsurf, w, h,
                                   _pepper_efl_object_cb_shell_configure_done,
                                   sd->co);
     }
}

static void
_pepper_efl_smart_show(Evas_Object *obj)
{
   pepper_efl_shell_surface_t *shsurf;

   _pepper_efl_parent_sc->show(obj);

   SMART_DATA_GET(obj, sd);

   shsurf = pepper_object_get_user_data((pepper_object_t *)sd->surface,
                                        pepper_surface_get_role(sd->surface));
   if (!shsurf)
     return;

   if (!shsurf->mapped)
     {
        shsurf->mapped = EINA_TRUE;
        pepper_view_map(shsurf->view);
     }
}

static void
_pepper_efl_smart_hide(Evas_Object *obj)
{
   pepper_efl_shell_surface_t *shsurf;

   _pepper_efl_parent_sc->show(obj);

   SMART_DATA_GET(obj, sd);

   if (!sd->surface)
     return;

   shsurf = pepper_object_get_user_data((pepper_object_t *)sd->surface,
                                        pepper_surface_get_role(sd->surface));
   if (!shsurf)
     return;

   if (shsurf->mapped)
     {
        shsurf->mapped = EINA_FALSE;
        pepper_view_unmap(shsurf->view);
     }
}

static void
_touch_down(Pepper_Efl_Smart_Data *sd, unsigned int timestamp, int device, int x, int y)
{
   pepper_efl_shell_surface_t *shsurf;
   int rel_x, rel_y;

   rel_x = x - sd->x;
   rel_y = y - sd->y;

   DBG("Touch (%d) Down: obj %p x %d y %d", device, sd->smart_obj, rel_x, rel_y);

   if (!sd->surface)
     return;

   shsurf = pepper_object_get_user_data((pepper_object_t *)sd->surface,
                                        pepper_surface_get_role(sd->surface));
   if (!shsurf)
     return;

   pepper_touch_add_point(sd->input.touch, device, rel_x, rel_y);
   pepper_touch_send_down(sd->input.touch, shsurf->view, timestamp, device, rel_x, rel_y);
}

static void
_touch_up(Pepper_Efl_Smart_Data *sd, unsigned int timestamp, int device)
{
   pepper_efl_shell_surface_t *shsurf;

   DBG("Touch (%d) Up: obj %p", device, sd->smart_obj);

   if (!sd->surface)
     return;

   shsurf = pepper_object_get_user_data((pepper_object_t *)sd->surface,
                                        pepper_surface_get_role(sd->surface));
   if (!shsurf)
     return;

   pepper_touch_send_up(sd->input.touch, shsurf->view, timestamp, device);
   pepper_touch_remove_point(sd->input.touch, device);
}

static void
_touch_move(Pepper_Efl_Smart_Data *sd, unsigned int timestamp, int device, int x, int y)
{
   pepper_efl_shell_surface_t *shsurf;
   int rel_x, rel_y;

   rel_x = x - sd->x;
   rel_y = y - sd->y;

   DBG("Touch (%d) Move: obj %p x %d y %d", device, sd->smart_obj, rel_x, rel_y);
   if (!sd->surface)
     return;

   shsurf = pepper_object_get_user_data((pepper_object_t *)sd->surface,
                                        pepper_surface_get_role(sd->surface));
   if (!shsurf)
     return;

   if ((!_need_send_motion) && (!_need_send_released))
     return;

   pepper_touch_send_motion(sd->input.touch, shsurf->view, timestamp, device, rel_x, rel_y);
}

static void
_pepper_efl_object_evas_cb_mouse_in(void *data, Evas *evas EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event EINA_UNUSED)
{
   Pepper_Efl_Smart_Data *sd = data;

   if (EINA_UNLIKELY(!sd))
     return;

   DBG("Mouse In");

   _mouse_in_po = sd;
}

static void
_pepper_efl_object_evas_cb_mouse_out(void *data EINA_UNUSED, Evas *evas EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event EINA_UNUSED)
{
   Pepper_Efl_Smart_Data *sd = data;

   if (EINA_UNLIKELY(!sd))
     return;

   DBG("Mouse Out");

   if (_mouse_in_po == sd)
     _mouse_in_po = NULL;
}

static void
_pepper_efl_object_evas_cb_mouse_move(void *data, Evas *evas EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event)
{
   Pepper_Efl_Smart_Data *sd = data;
   Evas_Event_Mouse_Move *ev = event;

   if ((!_need_send_motion) && (!_need_send_released))
     return;

   _touch_move(sd, ev->timestamp, 0, ev->cur.canvas.x, ev->cur.canvas.y);
}

static void
_pepper_efl_object_evas_cb_mouse_down(void *data, Evas *evas EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event)
{
   Pepper_Efl_Smart_Data *sd = data;
   Evas_Event_Mouse_Down *ev = event;

   _need_send_released = EINA_TRUE;

   _touch_down(sd, ev->timestamp, 0, ev->canvas.x, ev->canvas.y);
}

static void
_pepper_efl_object_evas_cb_mouse_up(void *data, Evas *evas EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event)
{
   Pepper_Efl_Smart_Data *sd = data;
   Evas_Event_Mouse_Up *ev = event;

   if (!_need_send_released)
     _need_send_motion = EINA_TRUE;

   _need_send_released = EINA_FALSE;

   _touch_up(sd, ev->timestamp, 0);
}

static void
_pepper_efl_object_evas_cb_multi_down(void *data, Evas *evas EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event)
{
   Pepper_Efl_Smart_Data *sd = data;
   Evas_Event_Multi_Down *ev = event;

   _touch_down(sd, ev->timestamp, ev->device, ev->canvas.x, ev->canvas.y);
}

static void
_pepper_efl_object_evas_cb_multi_up(void *data EINA_UNUSED, Evas *evas EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event EINA_UNUSED)
{
   Pepper_Efl_Smart_Data *sd = data;
   Evas_Event_Multi_Up *ev = event;

   _touch_up(sd, ev->timestamp, ev->device);
}

static void
_pepper_efl_object_evas_cb_multi_move(void *data EINA_UNUSED, Evas *evas EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event EINA_UNUSED)
{
   Pepper_Efl_Smart_Data *sd = data;
   Evas_Event_Multi_Move *ev = event;

   _touch_move(sd, ev->timestamp, ev->device, ev->cur.canvas.x, ev->cur.canvas.y);
}

static void
_pepper_efl_object_evas_cb_focus_in(void *data, Evas *evas EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event EINA_UNUSED)
{
   Pepper_Efl_Smart_Data *sd = data;
   pepper_efl_shell_surface_t *shsurf;
   pepper_view_t *focused_view;

   if (!sd->surface)
     return;

   shsurf = pepper_object_get_user_data((pepper_object_t *)sd->surface,
                                        pepper_surface_get_role(sd->surface));
   if ((!shsurf) || (!shsurf->view))
     return;

   focused_view = pepper_keyboard_get_focus(sd->input.kbd);

   if (shsurf->view == focused_view)
     {
        // already focused view.
        return;
     }

   if (focused_view)
     {
        // send leave message for pre-focused view.
        pepper_keyboard_send_leave(sd->input.kbd, focused_view);
     }

   // replace focused view.
   pepper_keyboard_set_focus(sd->input.kbd, shsurf->view);
   // send enter message for newly focused view.
   pepper_keyboard_send_enter(sd->input.kbd, shsurf->view);
}

static void
_pepper_efl_object_evas_cb_focus_out(void *data EINA_UNUSED, Evas *evas EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event EINA_UNUSED)
{
   Pepper_Efl_Smart_Data *sd = data;
   pepper_efl_shell_surface_t *shsurf;
   pepper_view_t *view;

   if (!sd->surface)
     return;

   shsurf = pepper_object_get_user_data((pepper_object_t *)sd->surface,
                                        pepper_surface_get_role(sd->surface));
   if (!shsurf)
     return;

   view = pepper_keyboard_get_focus(sd->input.kbd);
   if ((!view) || (shsurf->view != view))
     {
        // I expect that already sends 'leave' message to this view,
        // when focus view is changed in _cb_focus_in().
        // so, nothing to do here.
        return;
     }

   // send leave message for pre-focused view.
   pepper_keyboard_send_leave(sd->input.kbd, view);
   // unset focus view.
   pepper_keyboard_set_focus(sd->input.kbd, NULL);
}

static void
_pepper_efl_object_cb_buffer_destroy(pepper_event_listener_t *listener EINA_UNUSED, pepper_object_t *object EINA_UNUSED, uint32_t id EINA_UNUSED, void *info EINA_UNUSED, void *data)
{
   Pepper_Efl_Smart_Data *sd = data;

   DBG("[OBJECT] Buffer destroy: obj %p", sd->smart_obj);

   if (sd->shm_buffer)
     {
        evas_object_image_data_set(sd->co, NULL);
        sd->shm_buffer = NULL;
     }
   else
     {
        evas_object_image_native_surface_set(sd->co, NULL);
     }

   sd->buffer = NULL;
}

static void
_pepper_efl_object_cb_surface_destroy(pepper_event_listener_t *listener EINA_UNUSED, pepper_object_t *object EINA_UNUSED, uint32_t id EINA_UNUSED, void *info EINA_UNUSED, void *data)
{
   Pepper_Efl_Smart_Data *sd = data;

   if (sd->shm_buffer)
     {
        evas_object_image_data_set(sd->co, NULL);
        sd->shm_buffer = NULL;
     }
   else
     {
        evas_object_image_native_surface_set(sd->co, NULL);
     }

   if (sd->buffer)
     {
        pepper_buffer_unreference(sd->buffer);
        PE_FREE_FUNC(sd->destroy_listener.buffer, pepper_event_listener_remove);
        sd->buffer = NULL;
     }

   sd->surface = NULL;
   PE_FREE_FUNC(sd->destroy_listener.surface, pepper_event_listener_remove);

   evas_object_smart_callback_call(sd->parent, PEPPER_EFL_OBJ_DEL, (void *)sd->smart_obj);
}

static void
_pepper_efl_object_setup(Pepper_Efl_Smart_Data *sd)
{
   sd->co = evas_object_image_filled_add(sd->evas);
   evas_object_image_colorspace_set(sd->co, EVAS_COLORSPACE_ARGB8888);
   evas_object_image_alpha_set(sd->co, EINA_TRUE);
   evas_object_smart_member_add(sd->co, sd->smart_obj);
   evas_object_show(sd->co);
}

static void
_pepper_efl_smart_set_user(Evas_Smart_Class *sc)
{
   sc->add = _pepper_efl_smart_add;
   sc->del = _pepper_efl_smart_del;
   sc->move = _pepper_efl_smart_move;
   sc->resize = _pepper_efl_smart_resize;
   sc->show = _pepper_efl_smart_show;
   sc->hide = _pepper_efl_smart_hide;
}

static Pepper_Efl_Smart_Data *
_pepper_efl_smart_object_new(Evas *evas)
{
   Evas_Object *o;

   o = evas_object_smart_add(evas, _pepper_efl_smart_class_new());
   SMART_DATA_GET_OR_RETURN_VAL(o, sd, NULL);

   sd->evas = evas;
   sd->smart_obj = o;
#define EVENT_ADD(type, func)                                                    \
   evas_object_event_callback_priority_add(o,                                    \
                                           EVAS_CALLBACK_##type,                 \
                                           EVAS_CALLBACK_PRIORITY_AFTER,         \
                                           _pepper_efl_object_evas_cb_##func,    \
                                           sd);
   EVENT_ADD(MOUSE_IN, mouse_in);
   EVENT_ADD(MOUSE_OUT, mouse_out);
   EVENT_ADD(MOUSE_MOVE, mouse_move);
   EVENT_ADD(MOUSE_DOWN, mouse_down);
   EVENT_ADD(MOUSE_UP, mouse_up);
   EVENT_ADD(MULTI_DOWN, multi_down);
   EVENT_ADD(MULTI_UP, multi_up);
   EVENT_ADD(MULTI_MOVE, multi_move);

   EVENT_ADD(FOCUS_IN, focus_in);
   EVENT_ADD(FOCUS_OUT, focus_out);
#undef EVENT_ADD

   return sd;
}

Evas_Object *
pepper_efl_object_get(pepper_efl_output_t *output, pepper_surface_t *surface)
{
   Pepper_Efl_Smart_Data *sd;

   sd = pepper_object_get_user_data((pepper_object_t *)surface, output);
   if (!sd)
     {
        sd = _pepper_efl_smart_object_new(evas_object_evas_get(output->win));
        sd->input.ptr = output->comp->input->pointer;
        sd->input.kbd = output->comp->input->keyboard;
        sd->input.touch = output->comp->input->touch;
        sd->parent = output->win;
        sd->surface = surface;
        sd->destroy_listener.surface =
           pepper_object_add_event_listener((pepper_object_t *)surface,
                                            PEPPER_EVENT_OBJECT_DESTROY, 0,
                                            _pepper_efl_object_cb_surface_destroy, sd);

        pepper_object_set_user_data((pepper_object_t *)surface, output, sd, NULL);
     }

   return sd->smart_obj;
}

Eina_Bool
pepper_efl_object_buffer_attach(Evas_Object *obj, int *w, int *h)
{
   pepper_buffer_t *buffer;
   struct wl_shm_buffer *shm_buffer = NULL;
   struct wl_resource *buf_res;
   tbm_surface_h tbm_surface = NULL;
   int bw, bh;

   SMART_DATA_GET_OR_RETURN_VAL(obj, sd, EINA_FALSE);

   buffer = pepper_surface_get_buffer(sd->surface);
   if (!buffer)
     {
        ERR("[OBJECT] Failed to get pepper_buffer_t");
        return EINA_FALSE;
     }

   DBG("[OBJECT] Attach buffer: obj %p, buffer %p", obj, buffer);

   if (sd->buffer)
     {
        pepper_buffer_unreference(sd->buffer);
        pepper_event_listener_remove(sd->destroy_listener.buffer);
     }

   pepper_buffer_reference(buffer);

   sd->buffer = buffer;
   sd->destroy_listener.buffer=
      pepper_object_add_event_listener((pepper_object_t *)buffer,
                                       PEPPER_EVENT_OBJECT_DESTROY, 0,
                                       _pepper_efl_object_cb_buffer_destroy, sd);

   buf_res = pepper_buffer_get_resource(buffer);

   if ((shm_buffer = wl_shm_buffer_get(buf_res)))
     {
        sd->shm_buffer = shm_buffer;

        bw = wl_shm_buffer_get_width(shm_buffer);
        bh = wl_shm_buffer_get_height(shm_buffer);
     }
   else if ((tbm_surface = wayland_tbm_server_get_surface(NULL, buf_res)))
     {
        bw = tbm_surface_get_width(tbm_surface);
        bh = tbm_surface_get_height(tbm_surface);
     }
   else
     {
        ERR("[OBJECT] Failed to get buffer");
        return EINA_FALSE;
     }

   // first attach
   if (!sd->co)
     {
        _pepper_efl_object_setup(sd);
        evas_object_smart_callback_call(sd->parent, PEPPER_EFL_OBJ_ADD, (void *)obj);

        fprintf(stderr, "@@@@@@@ OBJ_ADD %p\n", obj);
     }

   if ((sd->w != bw) || (sd->h != bh))
     evas_object_resize(obj, bw, bh);

   if (w) *w = bw;
   if (h) *h = bh;

   return EINA_TRUE;
}

void
pepper_efl_object_render(Evas_Object *obj)
{
   SMART_DATA_GET(obj, sd);

   // FIXME: just mark dirty here, and set the data in pixels_get callback.
   if (sd->shm_buffer)
     {
        evas_object_image_size_set(sd->co, sd->w, sd->h);
        evas_object_image_data_set(sd->co, wl_shm_buffer_get_data(sd->shm_buffer));
        evas_object_image_data_update_add(sd->co, 0, 0, sd->w, sd->h);
     }
   else if (sd->buffer)
     {
        Evas_Native_Surface ns;

        ns.version = EVAS_NATIVE_SURFACE_VERSION;
        ns.type = EVAS_NATIVE_SURFACE_WL;
        ns.data.wl.legacy_buffer = pepper_buffer_get_resource(sd->buffer);

        evas_object_image_size_set(sd->co, sd->w, sd->h);
        evas_object_image_native_surface_set(sd->co, &ns);
        evas_object_image_data_update_add(sd->co, 0, 0, sd->w, sd->h);
     }
}

pid_t
pepper_efl_object_pid_get(Evas_Object *obj)
{
   pid_t pid = 0;
   struct wl_client *client;

   SMART_DATA_GET_OR_RETURN_VAL(obj, sd, 0);

   if (sd->surface)
     {
        client = wl_resource_get_client(pepper_surface_get_resource(sd->surface));
        if (client)
          wl_client_get_credentials(client, &pid, NULL, NULL);
     }
   DBG("[OBJECT] Pid Get : %d", pid);

   return pid;
}

const char *
pepper_efl_object_title_get(Evas_Object *obj)
{
   pepper_efl_shell_surface_t *shsurf = NULL;
   pepper_surface_t *surface = NULL;
   const char *title = NULL;

   SMART_DATA_GET_OR_RETURN_VAL(obj, sd, NULL);

   surface = sd->surface;

   if (surface)
     {
        shsurf = pepper_object_get_user_data((pepper_object_t *)surface,
                                             pepper_surface_get_role(surface));

        if (shsurf)
          title = shsurf->title;
     }

   if (title)
     DBG("[OBJECT] Title Get : %s", title);

   return title;
}

const char *
pepper_efl_object_app_id_get(Evas_Object *obj)
{
   pepper_efl_shell_surface_t *shsurf = NULL;
   pepper_surface_t *surface = NULL;
   const char *app_id = NULL;

   SMART_DATA_GET_OR_RETURN_VAL(obj, sd, NULL);

   surface = sd->surface;

   if (surface)
     {
        shsurf = pepper_object_get_user_data((pepper_object_t *)surface,
                                             pepper_surface_get_role(surface));

        if (shsurf)
          app_id = shsurf->app_id;
     }

   if (app_id)
     DBG("[OBJECT] App_id Get : %s", app_id);

   return app_id;
}

Eina_Bool
pepper_efl_object_touch_cancel(Evas_Object *obj)
{
   pepper_efl_shell_surface_t *shsurf;

   SMART_DATA_GET_OR_RETURN_VAL(obj, sd, EINA_FALSE);

   DBG("Touch Cancel: obj %p", sd->smart_obj);

   if (sd != _mouse_in_po)
     return EINA_FALSE;

   if (!sd->surface)
     return EINA_FALSE;

   shsurf = pepper_object_get_user_data((pepper_object_t *)sd->surface,
                                        pepper_surface_get_role(sd->surface));
   if (!shsurf)
     return EINA_FALSE;

   if ((_mouse_in_po) && (_need_send_released))
     {
        pepper_touch_send_cancel(sd->input.touch, shsurf->view);
        pepper_touch_remove_point(sd->input.touch, 0);
        _need_send_released = EINA_FALSE;
        _need_send_motion = EINA_FALSE;
     }

   return EINA_TRUE;
}

pepper_surface_t *
pepper_efl_object_pepper_surface_get(Evas_Object *obj)
{
   SMART_DATA_GET_OR_RETURN_VAL(obj, sd, NULL);

   return sd->surface;
}
