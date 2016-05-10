#include "private.h"
#include <linux/input.h>

#define SMART_NAME   "pepper_efl_object"

#define OBJ_DATA_GET                      \
   pepper_efl_object_t *po;               \
   po = evas_object_smart_data_get(obj);  \
   if (!po)                               \
      return

static Evas_Smart *_pepper_efl_smart = NULL;

static pepper_efl_object_t *_mouse_in_po = NULL;
static Eina_Bool _need_send_motion = EINA_TRUE;
static Eina_Bool _need_send_released = EINA_FALSE;

static void
_pepper_efl_object_buffer_release(Evas_Object *obj)
{
   OBJ_DATA_GET;

   DBG("[OBJECT] Release buffer: obj %p, buffer %p", obj, po->buffer);

   if (po->buffer)
     {
        pepper_buffer_unreference(po->buffer);
        po->buffer = NULL;

        PE_FREE_FUNC(po->buffer_destroy_listener, pepper_event_listener_remove);
     }
}

static void
_pepper_efl_object_del(pepper_efl_object_t *po)
{
   _pepper_efl_object_buffer_release(po->smart_obj);

   PE_FREE_FUNC(po->surface_destroy_listener, pepper_event_listener_remove);

   evas_object_del(po->clip);
   evas_object_del(po->img);

   free(po);
}

static void
_pepper_efl_smart_add(Evas_Object *obj)
{
   EVAS_SMART_DATA_ALLOC(obj, pepper_efl_object_t);

   priv->smart_obj = obj;
}

static void
_pepper_efl_smart_del(Evas_Object *obj)
{
   OBJ_DATA_GET;

   DBG("[OBJECT] Del: obj %p", obj);

   _pepper_efl_object_del(po);
}

static void
_pepper_efl_smart_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y)
{
   OBJ_DATA_GET;

   DBG("[OBJECT] Move: obj %p, x %d, y %d", obj, x, y);

   po->x = x;
   po->y = y;

   evas_object_move(po->img, x, y);
}

static void
_pepper_efl_object_cb_shell_configure_done(void *data, int w, int h)
{
   Evas_Object *img = data;

   DBG("[OBJECT] Callback configure done: w %d, h %d", w, h);

   evas_object_image_size_set(img, w, h);
   evas_object_resize(img, w, h);
}

static void
_pepper_efl_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h)
{
   pepper_efl_shell_surface_t *shsurf = NULL;

   OBJ_DATA_GET;

   DBG("[OBJECT] Resize: obj: %p, w %d, h %d", obj, w, h);

   if (!po->img)
     return;

   if (po->surface)
     {
        shsurf =
           pepper_object_get_user_data((pepper_object_t *)po->surface,
                                       pepper_surface_get_role(po->surface));
     }

   if (!shsurf)
     {
        evas_object_image_size_set(po->img, w, h);
        evas_object_resize(po->img, w, h);
     }
   else
     {
        pepper_efl_shell_configure(shsurf, w, h,
                                   _pepper_efl_object_cb_shell_configure_done,
                                   po->img);
     }
}

static void
_pepper_efl_smart_show(Evas_Object *obj)
{
   OBJ_DATA_GET;

   DBG("[OBJECT] Show: obj %p", obj);

   evas_object_show(po->img);
   evas_object_show(po->clip);
}

static void
_pepper_efl_smart_hide(Evas_Object *obj)
{
   OBJ_DATA_GET;

   DBG("[OBJECT] Hide: obj %p", obj);

   evas_object_hide(po->img);
   evas_object_hide(po->clip);
}

static void
_pepper_efl_smart_color_set(Evas_Object *obj, int a, int r, int g, int b)
{
   OBJ_DATA_GET;

   DBG("[OBJECT] Color Set: obj %p", obj);

   evas_object_color_set(po->clip, a, r, g, b);
}

static void
_pepper_efl_smart_clip_set(Evas_Object *obj, Evas_Object *clip)
{
   OBJ_DATA_GET;

   DBG("[OBJECT] Clip Set: obj %p", obj);

   evas_object_clip_set(po->clip, clip);
}

static void
_pepper_efl_smart_clip_unset(Evas_Object *obj)
{
   OBJ_DATA_GET;

   DBG("[OBJECT] Clip Unset: obj %p", obj);

   evas_object_clip_unset(po->clip);
}

static void
_pepper_efl_smart_init(void)
{
   if (_pepper_efl_smart)
     return;

   {
      static const Evas_Smart_Class sc =
      {
         SMART_NAME,
         EVAS_SMART_CLASS_VERSION,
         _pepper_efl_smart_add,
         _pepper_efl_smart_del,
         _pepper_efl_smart_move,
         _pepper_efl_smart_resize,
         _pepper_efl_smart_show,
         _pepper_efl_smart_hide,
         _pepper_efl_smart_color_set, // color_set
         _pepper_efl_smart_clip_set, // clip_set
         _pepper_efl_smart_clip_unset, // clip_unset
         NULL,
         NULL,
         NULL,

         NULL,
         NULL,
         NULL,
         NULL
      };
      _pepper_efl_smart = evas_smart_class_new(&sc);
   }
}

static void
_touch_down(pepper_efl_object_t *po, unsigned int timestamp, int device, int x, int y)
{
   pepper_efl_shell_surface_t *shsurf;
   int rel_x, rel_y;

   rel_x = x - po->x;
   rel_y = y - po->y;

   DBG("Touch (%d) Down: obj %p x %d y %d", device, po->smart_obj, rel_x, rel_y);

   if (!po->surface)
     return;

   shsurf = pepper_object_get_user_data((pepper_object_t *)po->surface,
                                        pepper_surface_get_role(po->surface));
   if (!shsurf)
     return;

   pepper_touch_add_point(po->input.touch, device, rel_x, rel_y);
   pepper_touch_send_down(po->input.touch, shsurf->view, timestamp, device, rel_x, rel_y);
}

static void
_touch_up(pepper_efl_object_t *po, unsigned int timestamp, int device)
{
   pepper_efl_shell_surface_t *shsurf;

   DBG("Touch (%d) Up: obj %p", device, po->smart_obj);

   if (!po->surface)
     return;

   shsurf = pepper_object_get_user_data((pepper_object_t *)po->surface,
                                        pepper_surface_get_role(po->surface));
   if (!shsurf)
     return;

   pepper_touch_send_up(po->input.touch, shsurf->view, timestamp, device);
   pepper_touch_remove_point(po->input.touch, device);
}

static void
_touch_move(pepper_efl_object_t *po, unsigned int timestamp, int device, int x, int y)
{
   pepper_efl_shell_surface_t *shsurf;
   int rel_x, rel_y;

   rel_x = x - po->x;
   rel_y = y - po->y;

   DBG("Touch (%d) Move: obj %p x %d y %d", device, po->smart_obj, rel_x, rel_y);
   if (!po->surface)
     return;

   shsurf = pepper_object_get_user_data((pepper_object_t *)po->surface,
                                        pepper_surface_get_role(po->surface));
   if (!shsurf)
     return;

   if ((!_need_send_motion) && (!_need_send_released))
     return;

   pepper_touch_send_motion(po->input.touch, shsurf->view, timestamp, device, rel_x, rel_y);
}

static void
_pepper_efl_object_evas_cb_mouse_in(void *data, Evas *evas EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event EINA_UNUSED)
{
   pepper_efl_object_t *po = data;

   if (EINA_UNLIKELY(!po))
     return;

   DBG("Mouse In");

   fprintf(stderr, "[touch] in\n");

   _mouse_in_po = po;
}

static void
_pepper_efl_object_evas_cb_mouse_out(void *data EINA_UNUSED, Evas *evas EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event EINA_UNUSED)
{
   pepper_efl_object_t *po = data;

   if (EINA_UNLIKELY(!po))
     return;

   DBG("Mouse Out");

   fprintf(stderr, "[touch] out\n");

   if (_mouse_in_po == po)
     _mouse_in_po = NULL;
}

static void
_pepper_efl_object_evas_cb_mouse_move(void *data, Evas *evas EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event)
{
   pepper_efl_object_t *po = data;
   Evas_Event_Mouse_Move *ev = event;

   fprintf(stderr, "[touch] move id:%d %dx%d\n", 0, ev->cur.canvas.x, ev->cur.canvas.y);

   if ((!_need_send_motion) && (!_need_send_released))
     return;

   _touch_move(po, ev->timestamp, 0, ev->cur.canvas.x, ev->cur.canvas.y);
}

static void
_pepper_efl_object_evas_cb_mouse_down(void *data, Evas *evas EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event)
{
   pepper_efl_object_t *po = data;
   Evas_Event_Mouse_Down *ev = event;

   _need_send_released = EINA_TRUE;

   fprintf(stderr, "[touch] down id:%d %dx%d\n", 0, ev->canvas.x, ev->canvas.y);
   _touch_down(po, ev->timestamp, 0, ev->canvas.x, ev->canvas.y);
}

static void
_pepper_efl_object_evas_cb_mouse_up(void *data, Evas *evas EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event)
{
   pepper_efl_object_t *po = data;
   Evas_Event_Mouse_Up *ev = event;

   fprintf(stderr, "[touch] up id:%d %dx%d\n", 0, ev->canvas.x, ev->canvas.y);

   if (!_need_send_released)
     _need_send_motion = EINA_TRUE;

   _need_send_released = EINA_FALSE;

   _touch_up(po, ev->timestamp, 0);
}

static void
_pepper_efl_object_evas_cb_multi_down(void *data, Evas *evas EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event)
{
   pepper_efl_object_t *po = data;
   Evas_Event_Multi_Down *ev = event;

   fprintf(stderr, "[touch] down id:%d %dx%d\n", ev->device, ev->canvas.x, ev->canvas.y);
   _touch_down(po, ev->timestamp, ev->device, ev->canvas.x, ev->canvas.y);
}

static void
_pepper_efl_object_evas_cb_multi_up(void *data EINA_UNUSED, Evas *evas EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event EINA_UNUSED)
{
   pepper_efl_object_t *po = data;
   Evas_Event_Multi_Up *ev = event;

   fprintf(stderr, "[touch] down id:%d %dx%d\n", ev->device, ev->canvas.x, ev->canvas.y);
   _touch_up(po, ev->timestamp, ev->device);
}

static void
_pepper_efl_object_evas_cb_multi_move(void *data EINA_UNUSED, Evas *evas EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event EINA_UNUSED)
{
   pepper_efl_object_t *po = data;
   Evas_Event_Multi_Move *ev = event;

   fprintf(stderr, "[touch] move id:%d %dx%d\n", ev->device, ev->cur.canvas.x, ev->cur.canvas.y);
   _touch_move(po, ev->timestamp, ev->device, ev->cur.canvas.x, ev->cur.canvas.y);
}

static void
_pepper_efl_object_evas_cb_focus_in(void *data, Evas *evas EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event EINA_UNUSED)
{
   pepper_efl_object_t *po = data;
   pepper_efl_shell_surface_t *shsurf;
   pepper_view_t *focused_view;

   if (!po->surface)
     return;

   shsurf = pepper_object_get_user_data((pepper_object_t *)po->surface,
                                        pepper_surface_get_role(po->surface));
   if ((!shsurf) || (!shsurf->view))
     return;

   DBG("[OBJECT] Focus In: obj %p, shsurf %p", obj, shsurf);

   focused_view = pepper_keyboard_get_focus(po->input.kbd);

   if (shsurf->view == focused_view)
     {
        // already focused view.
        return;
     }

   if (focused_view)
     {
        // send leave message for pre-focused view.
        pepper_keyboard_send_leave(po->input.kbd, focused_view);
     }

   // replace focused view.
   pepper_keyboard_set_focus(po->input.kbd, shsurf->view);
   // send enter message for newly focused view.
   pepper_keyboard_send_enter(po->input.kbd, shsurf->view);
}

static void
_pepper_efl_object_evas_cb_focus_out(void *data EINA_UNUSED, Evas *evas EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event EINA_UNUSED)
{
   pepper_efl_object_t *po = data;
   pepper_efl_shell_surface_t *shsurf;
   pepper_view_t *view;

   if (!po->surface)
     return;

   shsurf = pepper_object_get_user_data((pepper_object_t *)po->surface,
                                        pepper_surface_get_role(po->surface));
   if (!shsurf)
     return;

   DBG("[OBJECT] Focus Out: obj %p, shsurf %p", obj, shsurf);

   view = pepper_keyboard_get_focus(po->input.kbd);
   if ((!view) || (shsurf->view != view))
     {
        // I expect that already sends 'leave' message to this view,
        // when focus view is changed in _cb_focus_in().
        // so, nothing to do here.
        return;
     }

   // send leave message for pre-focused view.
   pepper_keyboard_send_leave(po->input.kbd, view);
   // unset focus view.
   pepper_keyboard_set_focus(po->input.kbd, NULL);
}

static void
_pepper_efl_object_cb_buffer_destroy(pepper_event_listener_t *listener EINA_UNUSED, pepper_object_t *object EINA_UNUSED, uint32_t id EINA_UNUSED, void *info EINA_UNUSED, void *data)
{
   pepper_efl_object_t *po = data;

   DBG("[OBJECT] Buffer destroy: obj %p", po->smart_obj);

   if (po->shm_buffer)
     {
        evas_object_image_data_set(po->img, NULL);
        po->shm_buffer = NULL;
     }
   else
     {
        evas_object_image_native_surface_set(po->img, NULL);
     }

   po->buffer = NULL;
}

static void
_pepper_efl_object_cb_surface_destroy(pepper_event_listener_t *listener EINA_UNUSED, pepper_object_t *object EINA_UNUSED, uint32_t id EINA_UNUSED, void *info EINA_UNUSED, void *data)
{
   pepper_efl_object_t *po = data;

   if (po->shm_buffer)
     {
        evas_object_image_data_set(po->img, NULL);
        po->shm_buffer = NULL;
     }
   else
     {
        evas_object_image_native_surface_set(po->img, NULL);
     }

   if (po->buffer)
     {
        pepper_buffer_unreference(po->buffer);
        PE_FREE_FUNC(po->buffer_destroy_listener, pepper_event_listener_remove);
        po->buffer = NULL;
     }

   po->surface = NULL;
   PE_FREE_FUNC(po->surface_destroy_listener, pepper_event_listener_remove);

   evas_object_smart_callback_call(po->parent, PEPPER_EFL_OBJ_DEL, (void *)po->smart_obj);
}

static void
_pepper_efl_object_setup(pepper_efl_object_t *po)
{
   po->img = evas_object_image_filled_add(po->evas);
   evas_object_image_colorspace_set(po->img, EVAS_COLORSPACE_ARGB8888);
   evas_object_image_alpha_set(po->img, EINA_TRUE);
   evas_object_resize(po->img, po->w, po->h);
   evas_object_smart_member_add(po->img, po->smart_obj);

   po->clip = evas_object_rectangle_add(po->evas);
   evas_object_smart_member_add(po->clip, po->smart_obj);
   evas_object_resize(po->clip, 999999, 999999);
   evas_object_clip_set(po->img, po->clip);
}

Evas_Object *
pepper_efl_object_get(pepper_efl_output_t *output, pepper_surface_t *surface)
{
   Evas *evas;
   Evas_Object *o;
   pepper_efl_object_t *po;

   po = pepper_object_get_user_data((pepper_object_t *)surface, output);
   if (!po)
     {
        _pepper_efl_smart_init();

        evas = evas_object_evas_get(output->win);
        o = evas_object_smart_add(evas, _pepper_efl_smart);

        po = evas_object_smart_data_get(o);
        if (!po)
          return NULL;

        po->input.ptr = output->comp->input->pointer;
        po->input.kbd = output->comp->input->keyboard;
        po->input.touch = output->comp->input->touch;
        po->evas = evas;
        po->parent = output->win;
        po->surface = surface;
        po->surface_destroy_listener =
           pepper_object_add_event_listener((pepper_object_t *)surface,
                                            PEPPER_EVENT_OBJECT_DESTROY, 0,
                                            _pepper_efl_object_cb_surface_destroy, po);

#define EVENT_ADD(type, func)                                                    \
        evas_object_event_callback_priority_add(o,                                    \
                                                EVAS_CALLBACK_##type,                 \
                                                EVAS_CALLBACK_PRIORITY_AFTER,         \
                                                _pepper_efl_object_evas_cb_##func,    \
                                                po);
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

        pepper_object_set_user_data((pepper_object_t *)surface, output, po, NULL);
     }

   return po->smart_obj;
}

Eina_Bool
pepper_efl_object_buffer_attach(Evas_Object *obj, int *w, int *h)
{
   pepper_efl_object_t *po;
   pepper_buffer_t *buffer;
   struct wl_shm_buffer *shm_buffer = NULL;
   struct wl_resource *buf_res;
   tbm_surface_h tbm_surface = NULL;
   int bw, bh;

   po = evas_object_smart_data_get(obj);
   if (!po)
     return EINA_FALSE;

   buffer = pepper_surface_get_buffer(po->surface);
   if (!buffer)
     {
        ERR("[OBJECT] Failed to get pepper_buffer_t");
        return EINA_FALSE;
     }

   DBG("[OBJECT] Attach buffer: obj %p, buffer %p", obj, buffer);

   if (po->buffer)
     {
        pepper_buffer_unreference(po->buffer);
        pepper_event_listener_remove(po->buffer_destroy_listener);
     }

   pepper_buffer_reference(buffer);

   po->buffer = buffer;
   po->buffer_destroy_listener =
      pepper_object_add_event_listener((pepper_object_t *)buffer,
                                       PEPPER_EVENT_OBJECT_DESTROY, 0,
                                       _pepper_efl_object_cb_buffer_destroy, po);

   buf_res = pepper_buffer_get_resource(buffer);

   if ((shm_buffer = wl_shm_buffer_get(buf_res)))
      {
        po->shm_buffer = shm_buffer;

        bw = wl_shm_buffer_get_width(shm_buffer);
        bh = wl_shm_buffer_get_height(shm_buffer);

        if ((po->w != bw) || (po->h != bh))
          {
             po->w = bw;
             po->h = bh;
             evas_object_resize(obj, bw, bh);
          }
     }
   else if ((tbm_surface = wayland_tbm_server_get_surface(NULL, buf_res)))
     {
       bw = tbm_surface_get_width(tbm_surface);
       bh = tbm_surface_get_height(tbm_surface);

        if ((po->w != bw) || (po->h != bh))
          {
             po->w = bw;
             po->h = bh;
             evas_object_resize(obj, bw, bh);
          }
     }
   else
     {
        ERR("[OBJECT] Failed to get buffer");
        return EINA_FALSE;
     }

   // first attach
   if (!po->img)
     {
        _pepper_efl_object_setup(po);
        evas_object_smart_callback_call(po->parent, PEPPER_EFL_OBJ_ADD, (void *)obj);
     }

   if (w) *w = bw;
   if (h) *h = bh;

   return EINA_TRUE;
}

void
pepper_efl_object_render(Evas_Object *obj)
{
   OBJ_DATA_GET;

   DBG("[OBJECT] Render: obj %p", obj);

   // FIXME: just mark dirty here, and set the data in pixels_get callback.
   if (po->shm_buffer)
     {
        evas_object_image_size_set(po->img, po->w, po->h);
        evas_object_image_data_set(po->img, wl_shm_buffer_get_data(po->shm_buffer));
        evas_object_image_data_update_add(po->img, 0, 0, po->w, po->h);
     }
   else if (po->buffer)
     {
        Evas_Native_Surface ns;

        ns.version = EVAS_NATIVE_SURFACE_VERSION;
        ns.type = EVAS_NATIVE_SURFACE_WL;
        ns.data.wl.legacy_buffer = pepper_buffer_get_resource(po->buffer);

        evas_object_image_size_set(po->img, po->w, po->h);
        evas_object_image_native_surface_set(po->img, &ns);
        evas_object_image_data_update_add(po->img, 0, 0, po->w, po->h);
     }
}

pid_t
pepper_efl_object_pid_get(Evas_Object *obj)
{
   OBJ_DATA_GET 0;

   pid_t pid = 0;
   struct wl_client *client;

   if (po->surface)
     {
        client = wl_resource_get_client(pepper_surface_get_resource(po->surface));
        if (client)
          wl_client_get_credentials(client, &pid, NULL, NULL);
     }
   DBG("[OBJECT] Pid Get : %d", pid);

   return pid;
}

const char *
pepper_efl_object_title_get(Evas_Object *obj)
{
   OBJ_DATA_GET NULL;

   pepper_efl_shell_surface_t *shsurf = NULL;
   pepper_surface_t *surface = NULL;
   const char *title = NULL;

   surface = po->surface;

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
   OBJ_DATA_GET NULL;

   pepper_efl_shell_surface_t *shsurf = NULL;
   pepper_surface_t *surface = NULL;
   const char *app_id = NULL;

   surface = po->surface;

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
   OBJ_DATA_GET EINA_FALSE;

   pepper_efl_shell_surface_t *shsurf;

   DBG("Touch Cancel: obj %p", po->smart_obj);

   if (po != _mouse_in_po)
     return EINA_FALSE;

   if (!po->surface)
     return EINA_FALSE;

   shsurf = pepper_object_get_user_data((pepper_object_t *)po->surface,
                                        pepper_surface_get_role(po->surface));
   if (!shsurf)
     return EINA_FALSE;

   if ((_mouse_in_po) && (_need_send_released))
     {
        pepper_touch_send_cancel(po->input.touch, shsurf->view);
        pepper_touch_remove_point(po->input.touch, 0);
        _need_send_released = EINA_FALSE;
        _need_send_motion = EINA_FALSE;
     }

   return EINA_TRUE;
}
