#include "private.h"
#include <linux/input.h>

#define SMART_NAME   "pepper_efl_object"

#define OBJ_DATA_GET                      \
   pepper_efl_object_t *po;               \
   po = evas_object_smart_data_get(obj);  \
   if (!po)                               \
      return

static Evas_Smart *_pepper_efl_smart = NULL;

static void
_pepper_efl_object_buffer_release(Evas_Object *obj)
{
   OBJ_DATA_GET;

   DBG("[OBJECT] Release buffer: obj %p, buffer %p", obj, po->buffer);

   if (po->buffer)
     {
        pepper_buffer_unreference(po->buffer);
        po->buffer = NULL;

        pepper_event_listener_remove(po->buffer_destroy_listener);
     }
}

static void
_pepper_efl_smart_add(Evas_Object *obj)
{
   pepper_efl_object_t *po;

   DBG("[OBJECT] Add: obj %p", obj);

   po = calloc(1, sizeof(*po));
   if (!po)
     {
        ERR("Out of memory");
        return;
     }

   po->smart_obj = obj;
   evas_object_smart_data_set(obj, po);
}

static void
_pepper_efl_smart_del(Evas_Object *obj)
{
   OBJ_DATA_GET;

   DBG("[OBJECT] Del: obj %p", obj);

   _pepper_efl_object_buffer_release(obj);

   PE_FREE_FUNC(po->surface_destroy_listener, pepper_event_listener_remove);
   evas_object_del(po->img);

   if (po->es)
     po->es->obj = NULL;

   free(po);
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
}

static void
_pepper_efl_smart_hide(Evas_Object *obj)
{
   OBJ_DATA_GET;

   DBG("[OBJECT] Hide: obj %p", obj);

   evas_object_hide(po->img);
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
         NULL, // color_set
         NULL, // clip_set
         NULL, // clip_unset
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
_pepper_efl_object_evas_cb_mouse_in(void *data, Evas *evas EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event EINA_UNUSED)
{
   pepper_efl_object_t *po = data;
   pepper_efl_shell_surface_t *shsurf;

   if (!po->surface)
     return;

   shsurf = pepper_object_get_user_data((pepper_object_t *)po->surface,
                                        pepper_surface_get_role(po->surface));
   if (!shsurf)
     return;

   DBG("[SURFACE] Mouse In: shsurf %p", shsurf);

   pepper_pointer_set_focus(po->input.ptr, shsurf->view);
}

static void
_pepper_efl_object_evas_cb_mouse_out(void *data EINA_UNUSED, Evas *evas EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event EINA_UNUSED)
{
   pepper_efl_object_t *po = data;

   DBG("[SURFACE] Mouse Out");

   pepper_pointer_set_focus(po->input.ptr, NULL);
}

static void
_pepper_efl_object_evas_cb_mouse_move(void *data EINA_UNUSED, Evas *evas EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event EINA_UNUSED)
{
   pepper_efl_object_t *po = data;
   Evas_Event_Mouse_Move *ev = event;
   int x, y;

   DBG("[SURFACE] Mouse Move: x %d y %d", ev->cur.canvas.x, ev->cur.canvas.y);

   x = ev->cur.canvas.x - po->x;
   y = ev->cur.canvas.y - po->y;

   pepper_pointer_send_motion(po->input.ptr, ev->timestamp, x, y);
}

static void
_pepper_efl_object_evas_cb_mouse_down(void *data EINA_UNUSED, Evas *evas EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event EINA_UNUSED)
{
   pepper_efl_object_t *po = data;
   Evas_Event_Mouse_Down *ev = event;

   DBG("[SURFACE] Mouse Down");

   pepper_pointer_send_button(po->input.ptr, ev->timestamp, BTN_LEFT, PEPPER_BUTTON_STATE_PRESSED);
}

static void
_pepper_efl_object_evas_cb_mouse_up(void *data, Evas *evas EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event EINA_UNUSED)
{
   pepper_efl_object_t *po = data;
   Evas_Event_Mouse_Up *ev = event;

   DBG("[SURFACE] Mouse Up");

   pepper_pointer_send_button(po->input.ptr, ev->timestamp, BTN_LEFT, PEPPER_BUTTON_STATE_RELEASED);

   evas_object_raise(po->smart_obj);
}

static void
_pepper_efl_object_cb_buffer_destroy(pepper_event_listener_t *listener EINA_UNUSED, pepper_object_t *object EINA_UNUSED, uint32_t id EINA_UNUSED, void *info EINA_UNUSED, void *data)
{
   pepper_efl_object_t *po = data;
   void *shm_data;

   DBG("[OBJECT] Buffer destroy: obj %p", po->smart_obj);

   _pepper_efl_object_buffer_release(po->smart_obj);

   shm_data = wl_shm_buffer_get_data(po->shm_buffer);
   evas_object_image_data_copy_set(po->img, shm_data);

   shm_data = evas_object_image_data_get(po->img, EINA_TRUE);
   evas_object_image_data_set(po->img, shm_data);
   evas_object_image_data_update_add(po->img, 0, 0, po->w, po->h);

   po->buffer_destroyed = EINA_TRUE;
}

static void
_pepper_efl_object_cb_surface_destroy(pepper_event_listener_t *listener EINA_UNUSED, pepper_object_t *object EINA_UNUSED, uint32_t id EINA_UNUSED, void *info EINA_UNUSED, void *data)
{
   pepper_efl_object_t *po = data;

   po->es = NULL;
   po->surface = NULL;
   PE_FREE_FUNC(po->surface_destroy_listener, pepper_event_listener_remove);
}

Evas_Object *
pepper_efl_object_add(pepper_efl_surface_t *es, Evas_Object *parent, pepper_surface_t *surface)
{
   Evas *evas;
   Evas_Object *o;
   pepper_efl_object_t *po;

   _pepper_efl_smart_init();

   evas = evas_object_evas_get(parent);
   o = evas_object_smart_add(evas, _pepper_efl_smart);

   po = evas_object_smart_data_get(o);
   if (!po)
     return NULL;

   po->es = es;
   po->input.ptr = es->output->comp->seat->ptr.resource;
   po->evas = evas;
   po->parent = parent;
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
#undef EVENT_ADD

   return o;
}

Eina_Bool
pepper_efl_object_buffer_attach(Evas_Object *obj, int *w, int *h)
{
   pepper_efl_object_t *po;
   pepper_buffer_t *buffer;
   struct wl_shm_buffer *shm_buffer;
   struct wl_resource *buf_res;
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
   shm_buffer = wl_shm_buffer_get(buf_res);
   if (!shm_buffer)
     {
        ERR("[OBJECT] Failed to get shm_buffer");
        return EINA_FALSE;
     }

   po->shm_buffer = shm_buffer;

   bw = wl_shm_buffer_get_width(shm_buffer);
   bh = wl_shm_buffer_get_height(shm_buffer);

   if ((po->w != bw) || (po->h != bh))
     {
        po->w = bw;
        po->h = bh;
        evas_object_resize(obj, bw, bh);
     }

   // first attach
   if (!po->img)
     {
        po->img = evas_object_image_filled_add(po->evas);
        evas_object_image_colorspace_set(po->img, EVAS_COLORSPACE_ARGB8888);
        evas_object_image_alpha_set(po->img, EINA_TRUE);
        evas_object_resize(po->img, po->w, po->h);

        evas_object_smart_member_add(po->img, obj);
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

   if (po->buffer_destroyed)
     return;

   // FIXME: just mark dirty here, and set the data in pixels_get callback.
   evas_object_image_size_set(po->img, po->w, po->h);
   evas_object_image_data_set(po->img, wl_shm_buffer_get_data(po->shm_buffer));
   evas_object_image_data_update_add(po->img, 0, 0, po->w, po->h);
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

