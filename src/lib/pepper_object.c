#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "private.h"

#include <Eo.h>
#include <Evas.h>
#include "pepper_object.eo.h"
#include <linux/input.h>

#ifdef MY_CLASS
#undef MY_CLASS
#endif

#define MY_CLASS        PEPPER_OBJECT_CLASS
#define MY_CLASS_NAME   "pepper_object"

typedef struct
{
   Evas *evas;

   Evas_Object *parent;
   Evas_Object *co;

   int x, y, w, h;

   struct
   {
      int w, h;
   } buffer;

   struct
   {
      struct wl_shm_buffer *shm;
   } wl;

   struct
   {
      struct
      {
         pepper_surface_t *obj;
         pepper_event_listener_t *destroy_listener;
      } surface;
      struct
      {
         pepper_buffer_t *obj;
         pepper_event_listener_t *destroy_listener;
      } buffer;
      struct
      {
         pepper_pointer_t *ptr;
         pepper_keyboard_t *kbd;
         pepper_touch_t *touch;
      } input;
   } pepper;
} Pepper_Object_Data;

static Evas_Object *_mouse_in_obj = NULL;
static Eina_Bool _need_send_motion = EINA_TRUE;
static Eina_Bool _need_send_released = EINA_FALSE;

#define PEPPER_OBJECT_DATA_GET(o, ptr)                      \
   Pepper_Object_Data *ptr = eo_data_scope_get(o, MY_CLASS)

#define PEPPER_OBJECT_DATA_GET_OR_RETURN(o, ptr)            \
   PEPPER_OBJECT_DATA_GET(o, ptr);                          \
   if (!ptr)                                                \
     {                                                      \
        ERR("no eo data for object %p (%s)",                \
            o, evas_object_type_get(o));                    \
        return;                                             \
     }

#define PEPPER_OBJECT_DATA_GET_OR_RETURN_VAL(o, ptr, val)   \
   PEPPER_OBJECT_DATA_GET(o, ptr);                          \
   if (!ptr)                                                \
     {                                                      \
        ERR("no eo data for object %p (%s)",                \
            o, evas_object_type_get(o));                    \
        return val;                                         \
     }

static void
_touch_down(Pepper_Object_Data *pd, unsigned int timestamp, int device, int x, int y)
{
   pepper_efl_shell_surface_t *shsurf;
   int rel_x, rel_y;

   rel_x = x - pd->x;
   rel_y = y - pd->y;

   DBG("Touch (%d) Down: obj %p x %d y %d", device, pd, rel_x, rel_y);

   if (!pd->pepper.surface.obj)
     return;

   shsurf = pepper_object_get_user_data((pepper_object_t *)pd->pepper.surface.obj,
                                        pepper_surface_get_role(pd->pepper.surface.obj));
   if (!shsurf)
     return;

   pepper_touch_add_point(pd->pepper.input.touch, device, rel_x, rel_y);
   pepper_touch_send_down(pd->pepper.input.touch, shsurf->view, timestamp, device, rel_x, rel_y);
}

static void
_touch_up(Pepper_Object_Data *pd, unsigned int timestamp, int device)
{
   pepper_efl_shell_surface_t *shsurf;

   DBG("Touch (%d) Up: obj %p", device, pd);

   if (!pd->pepper.surface.obj)
     return;

   shsurf = pepper_object_get_user_data((pepper_object_t *)pd->pepper.surface.obj,
                                        pepper_surface_get_role(pd->pepper.surface.obj));
   if (!shsurf)
     return;

   pepper_touch_send_up(pd->pepper.input.touch, shsurf->view, timestamp, device);
   pepper_touch_remove_point(pd->pepper.input.touch, device);
}

static void
_touch_move(Pepper_Object_Data *pd, unsigned int timestamp, int device, int x, int y)
{
   pepper_efl_shell_surface_t *shsurf;
   int rel_x, rel_y;

   rel_x = x - pd->x;
   rel_y = y - pd->y;

   DBG("Touch (%d) Move: obj %p x %d y %d", device, pd, rel_x, rel_y);
   if (!pd->pepper.surface.obj)
     return;

   shsurf = pepper_object_get_user_data((pepper_object_t *)pd->pepper.surface.obj,
                                        pepper_surface_get_role(pd->pepper.surface.obj));
   if (!shsurf)
     return;

   if ((!_need_send_motion) && (!_need_send_released))
     return;

   pepper_touch_send_motion(pd->pepper.input.touch, shsurf->view, timestamp, device, rel_x, rel_y);
}

static void
_obj_cb_mouse_in(void *data, Evas *evas EINA_UNUSED, Evas_Object *obj, void *event EINA_UNUSED)
{
   Pepper_Object_Data *pd = data;

   if (EINA_UNLIKELY(!pd))
     return;

   DBG("Mouse In");

   _mouse_in_obj = obj;
}

static void
_obj_cb_mouse_out(void *data EINA_UNUSED, Evas *evas EINA_UNUSED, Evas_Object *obj, void *event EINA_UNUSED)
{
   Pepper_Object_Data *pd = data;

   if (EINA_UNLIKELY(!pd))
     return;

   DBG("Mouse Out");

   if (_mouse_in_obj == obj)
     _mouse_in_obj = NULL;
}

static void
_obj_cb_mouse_move(void *data, Evas *evas EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event)
{
   Pepper_Object_Data *pd = data;
   Evas_Event_Mouse_Move *ev = event;

   if ((!_need_send_motion) && (!_need_send_released))
     return;

   _touch_move(pd, ev->timestamp, 0, ev->cur.canvas.x, ev->cur.canvas.y);
}

static void
_obj_cb_mouse_down(void *data, Evas *evas EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event)
{
   Pepper_Object_Data *pd = data;
   Evas_Event_Mouse_Down *ev = event;

   _need_send_released = EINA_TRUE;

   _touch_down(pd, ev->timestamp, 0, ev->canvas.x, ev->canvas.y);
}

static void
_obj_cb_mouse_up(void *data, Evas *evas EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event)
{
   Pepper_Object_Data *pd = data;
   Evas_Event_Mouse_Up *ev = event;

   if (!_need_send_released)
     _need_send_motion = EINA_TRUE;

   _need_send_released = EINA_FALSE;

   _touch_up(pd, ev->timestamp, 0);
}

static void
_obj_cb_multi_down(void *data, Evas *evas EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event)
{
   Pepper_Object_Data *pd = data;
   Evas_Event_Multi_Down *ev = event;

   _touch_down(pd, ev->timestamp, ev->device, ev->canvas.x, ev->canvas.y);
}

static void
_obj_cb_multi_up(void *data EINA_UNUSED, Evas *evas EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event EINA_UNUSED)
{
   Pepper_Object_Data *pd = data;
   Evas_Event_Multi_Up *ev = event;

   _touch_up(pd, ev->timestamp, ev->device);
}

static void
_obj_cb_multi_move(void *data EINA_UNUSED, Evas *evas EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event EINA_UNUSED)
{
   Pepper_Object_Data *pd = data;
   Evas_Event_Multi_Move *ev = event;

   _touch_move(pd, ev->timestamp, ev->device, ev->cur.canvas.x, ev->cur.canvas.y);
}

static void
_obj_cb_focus_in(void *data, Evas *evas EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event EINA_UNUSED)
{
   Pepper_Object_Data *pd;
   pepper_efl_shell_surface_t *shsurf;
   pepper_surface_t *psurf;
   pepper_view_t *focused_view;
   const char *key;

   pd = data;
   psurf = pd->pepper.surface.obj;
   if (!psurf)
     return;

   key = pepper_surface_get_role(psurf);
   shsurf = pepper_object_get_user_data((pepper_object_t *)psurf, key);
   if ((!shsurf) || (!shsurf->view))
     return;

   focused_view = pepper_keyboard_get_focus(pd->pepper.input.kbd);

   if (shsurf->view == focused_view)
     {
        // already focused view.
        return;
     }

   if (focused_view)
     {
        // send leave message for pre-focused view.
        pepper_keyboard_send_leave(pd->pepper.input.kbd, focused_view);
     }

   // replace focused view.
   pepper_keyboard_set_focus(pd->pepper.input.kbd, shsurf->view);
   // send enter message for newly focused view.
   pepper_keyboard_send_enter(pd->pepper.input.kbd, shsurf->view);
}

static void
_obj_cb_focus_out(void *data, Evas *evas EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event EINA_UNUSED)
{
   Pepper_Object_Data *pd;
   pepper_efl_shell_surface_t *shsurf;
   pepper_surface_t *psurf;
   pepper_view_t *view;
   const char *key;

   pd = data;
   psurf = pd->pepper.surface.obj;
   if (!psurf)
     return;

   key = pepper_surface_get_role(psurf);
   shsurf = pepper_object_get_user_data((pepper_object_t *)psurf, key);
   if (!shsurf)
     return;

   view = pepper_keyboard_get_focus(pd->pepper.input.kbd);
   if ((!view) || (shsurf->view != view))
     {
        // I expect that already sends 'leave' message to this view,
        // when focus view is changed in _cb_focus_in().
        // so, nothing to do here.
        return;
     }

   // send leave message for pre-focused view.
   pepper_keyboard_send_leave(pd->pepper.input.kbd, view);
   // unset focus view.
   pepper_keyboard_set_focus(pd->pepper.input.kbd, NULL);
}

static void
_obj_cb_buffer_destroy(pepper_event_listener_t *listener EINA_UNUSED, pepper_object_t *object EINA_UNUSED, uint32_t id EINA_UNUSED, void *info EINA_UNUSED, void *data)
{
   Pepper_Object_Data *pd;

   pd = data;

   if (pd->wl.shm)
     {
        evas_object_image_data_set(pd->co, NULL);
        pd->wl.shm = NULL;
     }
   else
     evas_object_image_native_surface_set(pd->co, NULL);

   if (pd->pepper.buffer.obj)
     {
        pepper_buffer_unreference(pd->pepper.buffer.obj);
        pd->pepper.buffer.obj = NULL;
     }

   pd->pepper.buffer.destroy_listener = NULL;
}

static void
_obj_cb_surface_destroy(pepper_event_listener_t *listener EINA_UNUSED, pepper_object_t *object EINA_UNUSED, uint32_t id EINA_UNUSED, void *info EINA_UNUSED, void *data)
{
   Pepper_Object_Data *pd;

   pd = data;
   pd->pepper.surface.obj = NULL;
   /* 'destroy_listener' is freed by caller */
   pd->pepper.surface.destroy_listener = NULL;
}

static void
_obj_setup(Evas_Object *obj)
{
   PEPPER_OBJECT_DATA_GET_OR_RETURN(obj, pd);

   pd->co = evas_object_image_filled_add(pd->evas);
   evas_object_image_colorspace_set(pd->co, EVAS_COLORSPACE_ARGB8888);
   evas_object_image_alpha_set(pd->co, EINA_TRUE);
   evas_object_smart_member_add(pd->co, obj);
   evas_object_show(pd->co);
}

Evas_Object *
pepper_efl_object_add(pepper_efl_output_t *output, pepper_surface_t *surface)
{
   Evas_Object *obj;

   obj = pepper_object_get_user_data((pepper_object_t *)surface, output);
   if (!obj)
     {
        obj = eo_add(MY_CLASS, evas_object_evas_get(output->win));
        PEPPER_OBJECT_DATA_GET_OR_RETURN_VAL(obj, pd, NULL);
        pd->parent = output->win;
        pd->pepper.surface.obj = surface;
        pd->pepper.surface.destroy_listener =
           pepper_object_add_event_listener((pepper_object_t *)surface,
                                            PEPPER_EVENT_OBJECT_DESTROY, 0,
                                            _obj_cb_surface_destroy, pd);

        pd->pepper.input.ptr = output->comp->input->pointer;
        pd->pepper.input.kbd = output->comp->input->keyboard;
        pd->pepper.input.touch = output->comp->input->touch;

        pepper_object_set_user_data((pepper_object_t *)surface, output, obj, NULL);
     }

   return obj;
}

pepper_surface_t *
pepper_efl_object_pepper_surface_get(Evas_Object *obj)
{
   PEPPER_OBJECT_DATA_GET_OR_RETURN_VAL(obj, pd, NULL);

   return pd->pepper.surface.obj;
}

Eina_Bool
pepper_efl_object_buffer_attach(Evas_Object *obj, int *w, int *h)
{
   pepper_buffer_t *buffer;
   struct wl_shm_buffer *shm_buffer;
   struct wl_resource *buf_res;
   tbm_surface_h tbm_surface;
   int bw = 0, bh = 0;

   PEPPER_OBJECT_DATA_GET_OR_RETURN_VAL(obj, pd, EINA_FALSE);

   buffer = pepper_surface_get_buffer(pd->pepper.surface.obj);

   if (pd->pepper.buffer.obj)
     {
        pepper_buffer_unreference(pd->pepper.buffer.obj);
        pepper_event_listener_remove(pd->pepper.buffer.destroy_listener);
     }

   if (buffer)
     {
        pepper_buffer_reference(buffer);
        pd->pepper.buffer.destroy_listener =
           pepper_object_add_event_listener((pepper_object_t *)buffer,
                                            PEPPER_EVENT_OBJECT_DESTROY, 0,
                                            _obj_cb_buffer_destroy, pd);

        buf_res = pepper_buffer_get_resource(buffer);
        if ((shm_buffer = wl_shm_buffer_get(buf_res)))
          {
             pd->wl.shm = shm_buffer;

             bw = wl_shm_buffer_get_width(shm_buffer);
             bh = wl_shm_buffer_get_height(shm_buffer);
          }
        else if ((tbm_surface = wayland_tbm_server_get_surface(NULL, buf_res)))
          {
             bw = tbm_surface_get_width(tbm_surface);
             bh = tbm_surface_get_height(tbm_surface);
          }
        else
          ERR("Unknown buffer type");

        if (!pd->co)
          {
             /* first attached */
             _obj_setup(obj);
             evas_object_smart_callback_call(pd->parent, PEPPER_EFL_OBJ_ADD, (void *)obj);
          }

        if ((pd->buffer.w != bw) || (pd->buffer.h != bh))
          {
             pd->buffer.w = bw;
             pd->buffer.h = bh;
             evas_object_resize(pd->co, bw, bh);
             evas_object_image_size_set(pd->co, bw, bh);
          }
     }
   else
     WRN("NULL buffer attached");

   pd->pepper.buffer.obj = buffer;

   if (w) *w = bw;
   if (h) *h = bh;

   return EINA_TRUE;
}

void
pepper_efl_object_render(Evas_Object *obj)
{
   PEPPER_OBJECT_DATA_GET_OR_RETURN(obj, pd);

   if (pd->wl.shm)
     {
        void *data;

        data = wl_shm_buffer_get_data(pd->wl.shm);
        evas_object_image_data_set(pd->co, data);
     }
   else if (pd->pepper.buffer.obj)
     {
        Evas_Native_Surface ns;

        ns.version = EVAS_NATIVE_SURFACE_VERSION;
        ns.type = EVAS_NATIVE_SURFACE_WL;
        ns.data.wl.legacy_buffer = pepper_buffer_get_resource(pd->pepper.buffer.obj);

        evas_object_image_native_surface_set(pd->co, &ns);
     }
   else
     {
        evas_object_hide(obj);
        return;
     }

   evas_object_image_data_update_add(pd->co, 0, 0, pd->buffer.w, pd->buffer.h);
}

EOLIAN static Eo_Base *
_pepper_object_eo_base_constructor(Eo *obj, Pepper_Object_Data *pd EINA_UNUSED)
{
   obj = eo_do_super_ret(obj, MY_CLASS, obj, eo_constructor());
   eo_do(obj, evas_obj_type_set(MY_CLASS_NAME));
   return obj;
}

EOLIAN static void
_pepper_object_evas_object_smart_add(Eo *obj, Pepper_Object_Data *pd)
{
   pd->evas = evas_object_evas_get(obj);

   eo_do_super(obj, MY_CLASS, evas_obj_smart_add());

#define EVENT_ADD(type, func)                                              \
   evas_object_event_callback_priority_add(obj,                            \
                                           EVAS_CALLBACK_##type,           \
                                           EVAS_CALLBACK_PRIORITY_AFTER,   \
                                           _obj_cb_##func, pd)
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
}

EOLIAN static void
_pepper_object_evas_object_smart_del(Eo *obj, Pepper_Object_Data *pd)
{
   if (pd->pepper.buffer.obj)
     {
        pepper_buffer_unreference(pd->pepper.buffer.obj);
        pd->pepper.buffer.obj = NULL;
     }
   PE_FREE_FUNC(pd->pepper.buffer.destroy_listener, pepper_event_listener_remove);
   PE_FREE_FUNC(pd->pepper.surface.destroy_listener, pepper_event_listener_remove);

   eo_do_super(obj, MY_CLASS, evas_obj_smart_del());
}

EOLIAN static void
_pepper_object_evas_object_smart_move(Eo *obj, Pepper_Object_Data *pd, Evas_Coord x, Evas_Coord y)
{
   if ((pd->x == x) && (pd->y == y))
     return;

   pd->x = x;
   pd->y = y;

   eo_do_super(obj, MY_CLASS, evas_obj_smart_move(x, y));
}

EOLIAN static void
_pepper_object_evas_object_smart_resize(Eo *obj, Pepper_Object_Data *pd, Evas_Coord w, Evas_Coord h)
{
   pepper_efl_shell_surface_t *shsurf = NULL;
   pepper_surface_t *psurf;
   const char *key;

   eo_do_super(obj, MY_CLASS, evas_obj_smart_resize(w, h));

   if (!pd->co)
     return;

   pd->w = w;
   pd->h = h;

   psurf = pd->pepper.surface.obj;
   if (psurf)
     {
        key = pepper_surface_get_role(psurf);
        shsurf = pepper_object_get_user_data((pepper_object_t *)psurf, key);
     }

   if (shsurf)
     pepper_efl_shell_configure(shsurf, w, h, NULL, pd->co);
}

EOLIAN static void
_pepper_object_evas_object_smart_show(Eo *obj, Pepper_Object_Data *pd)
{
   pepper_surface_t *psurf;
   pepper_efl_shell_surface_t *shsurf;
   const char *key;

   eo_do_super(obj, MY_CLASS, evas_obj_smart_show());

   psurf = pd->pepper.surface.obj;
   if (!psurf)
     return;

   key = pepper_surface_get_role(psurf);
   shsurf = pepper_object_get_user_data((pepper_object_t *)psurf, (const void *)key);
   if (!shsurf)
     return;

   if (!shsurf->mapped)
     {
        shsurf->mapped = EINA_TRUE;
        pepper_view_map(shsurf->view);
     }
}

EOLIAN static void
_pepper_object_evas_object_smart_hide(Eo *obj, Pepper_Object_Data *pd)
{
   pepper_surface_t *psurf;
   pepper_efl_shell_surface_t *shsurf;
   const char *key;

   eo_do_super(obj, MY_CLASS, evas_obj_smart_hide());

   psurf = pd->pepper.surface.obj;
   if (!psurf)
     return;

   key = pepper_surface_get_role(psurf);
   shsurf = pepper_object_get_user_data((pepper_object_t *)psurf, (const void *)key);
   if (!shsurf)
     return;

   if (shsurf->mapped)
     {
        shsurf->mapped = EINA_FALSE;
        pepper_view_unmap(shsurf->view);
     }
}

EOLIAN static pid_t
_pepper_object_pid_get(Eo *obj, Pepper_Object_Data *pd)
{
   pepper_surface_t *psurf;
   struct wl_client *client;
   pid_t pid = 0;

   psurf = pd->pepper.surface.obj;
   if (!psurf)
     return 0;

   client = wl_resource_get_client(pepper_surface_get_resource(psurf));
   if (!client)
     return 0;

   wl_client_get_credentials(client, &pid, NULL, NULL);

   return pid;
}

EOLIAN static const char *
_pepper_object_title_get(Eo *obj, Pepper_Object_Data *pd)
{
   pepper_efl_shell_surface_t *shsurf;
   pepper_surface_t *psurf;
   const char *key;

   psurf = pd->pepper.surface.obj;
   if (!psurf)
     return NULL;

   key = pepper_surface_get_role(psurf);
   shsurf = pepper_object_get_user_data((pepper_object_t *)psurf, key);
   if (!shsurf)
     return NULL;

   return shsurf->title;
}

EOLIAN static const char *
_pepper_object_app_id_get(Eo *obj, Pepper_Object_Data *pd)
{
   pepper_efl_shell_surface_t *shsurf;
   pepper_surface_t *psurf;
   const char *key;

   psurf = pd->pepper.surface.obj;
   if (!psurf)
     return NULL;

   key = pepper_surface_get_role(psurf);
   shsurf = pepper_object_get_user_data((pepper_object_t *)psurf, key);
   if (!shsurf)
     return NULL;

   return shsurf->app_id;
}

EOLIAN static Eina_Bool
_pepper_object_touch_cancel(Eo *obj, Pepper_Object_Data *pd)
{
   pepper_efl_shell_surface_t *shsurf;
   pepper_surface_t *psurf;
   const char *key;

   if (obj != _mouse_in_obj)
     return EINA_FALSE;

   psurf = pd->pepper.surface.obj;
   if (!psurf)
     return EINA_FALSE;

   key = pepper_surface_get_role(psurf);
   shsurf = pepper_object_get_user_data((pepper_object_t *)psurf, key);
   if (!shsurf)
     return EINA_FALSE;

   if ((!_mouse_in_obj) || (_need_send_released))
     return EINA_FALSE;

   pepper_touch_send_cancel(pd->pepper.input.touch, shsurf->view);
   pepper_touch_remove_point(pd->pepper.input.touch, 0);
   _need_send_released = EINA_FALSE;
   _need_send_motion = EINA_FALSE;

   return EINA_TRUE;
}

#include "pepper_object.eo.c"
