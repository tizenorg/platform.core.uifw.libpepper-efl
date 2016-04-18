#ifndef _PEPPER_EFL_OBJECT_H_
#define _PEPPER_EFL_OBJECT_H_

typedef struct pepper_efl_object pepper_efl_object_t;

struct pepper_efl_object
{
   Evas *evas;
   Evas_Object *parent;
   Evas_Object *smart_obj;
   Evas_Object *img;

   pepper_efl_surface_t *es;

   pepper_surface_t *surface;
   pepper_buffer_t *buffer;
   pepper_event_listener_t *buffer_destroy_listener;
   pepper_event_listener_t *surface_destroy_listener;

   struct wl_shm_buffer *shm_buffer;
   tbm_surface_h tbm_surface;
   int x, y, w, h;

   struct
   {
      pepper_pointer_t *ptr;
      pepper_keyboard_t *kbd;
      pepper_touch_t *touch;
   } input;
};

Evas_Object *pepper_efl_object_add(pepper_efl_surface_t *es, Evas_Object *parent, pepper_surface_t *surface);
Eina_Bool    pepper_efl_object_buffer_attach(Evas_Object *obj, int *w, int *h);
void         pepper_efl_object_render(Evas_Object *obj);

#endif
