#include "private.h"

#define SMART_NAME   "pepper_efl_object"

#define OBJ_DATA_GET                      \
   pepper_efl_object_t *po;               \
   po = evas_object_smart_data_get(obj);  \
   if (!po)                               \
      return

static Evas_Smart *_pepper_efl_smart = NULL;

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

   pepper_efl_object_buffer_release(obj);

   pepper_efl_surface_destroy(po->es);
   evas_object_del(po->img);
   free(po);
}

static void
_pepper_efl_smart_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y)
{
   OBJ_DATA_GET;

   DBG("[OBJECT] Move: obj %p, x %d, y %d", obj, x, y);

   evas_object_move(po->img, x, y);
}

static void
_cb_configure_done(void *data, int w, int h)
{
   Evas_Object *img = data;

   DBG("[OBJECT] Callback configure done: w %d, h %d", w, h);

   evas_object_image_size_set(img, w, h);
   evas_object_resize(img, w, h);
}

static void
_pepper_efl_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h)
{
   pepper_efl_shell_surface_t *shsurf;

   OBJ_DATA_GET;

   DBG("[OBJECT] Resize: obj: %p, w %d, h %d", obj, w, h);

   if (po->img)
     {
        shsurf = pepper_object_get_user_data((pepper_object_t *)po->es->surface,
                                             pepper_surface_get_role(po->es->surface));
        if (!shsurf)
          {
             DBG("failed to get shsurf");
             evas_object_image_size_set(po->img, w, h);
             evas_object_resize(po->img, w, h);
             return;
          }

        pepper_efl_shell_configure(shsurf, w, h, _cb_configure_done, po->img);
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

void
pepper_efl_object_buffer_release(Evas_Object *obj)
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
_pepper_efl_object_cb_buffer_destroy(pepper_event_listener_t *listener EINA_UNUSED, pepper_object_t *object EINA_UNUSED, uint32_t id EINA_UNUSED, void *info EINA_UNUSED, void *data)
{
   pepper_efl_object_t *po = data;
   void *shm_data;

   pepper_efl_object_buffer_release(po->smart_obj);

   shm_data = wl_shm_buffer_get_data(po->shm_buffer);
   evas_object_image_data_copy_set(po->img, shm_data);
   evas_object_image_data_update_add(po->img, 0, 0, po->w, po->h);

   po->buffer_destroyed = EINA_TRUE;
}

Evas_Object *
pepper_efl_object_add(pepper_efl_surface_t *es)
{
   Evas *evas;
   Evas_Object *o;
   pepper_efl_object_t *po;

   _pepper_efl_smart_init();

   evas = evas_object_evas_get(es->output->win);
   o = evas_object_smart_add(evas, _pepper_efl_smart);

   po = evas_object_smart_data_get(o);
   if (!po)
     return NULL;

   po->es = es;
   po->win = es->output->win;
   po->evas = evas;

   return o;
}

Eina_Bool
pepper_efl_object_buffer_attach(Evas_Object *obj, pepper_buffer_t *buffer, int *w, int *h)
{
   pepper_efl_object_t *po;
   struct wl_shm_buffer *shm_buffer;
   int bw, bh;

   po = evas_object_smart_data_get(obj);
   if (!po)
     return EINA_FALSE;

   DBG("[OBJECT] Attach buffer: obj %p, buffer %p", obj, buffer);

   shm_buffer = wl_shm_buffer_get(pepper_buffer_get_resource(buffer));
   if (!shm_buffer)
     return EINA_FALSE;

   po->shm_buffer = shm_buffer;

   bw = wl_shm_buffer_get_width(shm_buffer);
   bh = wl_shm_buffer_get_height(shm_buffer);

   if ((po->w != bw) || (po->h != bh))
     {
        po->w = bw;
        po->h = bh;
        evas_object_resize(obj, bw, bh);
     }

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

   // first attach
   if (!po->img)
     {
        po->img = evas_object_image_filled_add(po->evas);
        evas_object_image_colorspace_set(po->img, EVAS_COLORSPACE_ARGB8888);
        evas_object_image_alpha_set(po->img, EINA_TRUE);
        evas_object_image_size_set(po->img, po->w, po->h);

        evas_object_smart_member_add(po->img, obj);
        evas_object_smart_callback_call(po->win, PEPPER_EFL_OBJ_ADD, (void *)obj);
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

   evas_object_image_data_set(po->img, wl_shm_buffer_get_data(po->shm_buffer));
   evas_object_image_data_update_add(po->img, 0, 0, po->w, po->h);
}
