#ifndef _PEPPER_EFL_OBJECT_H_
#define _PEPPER_EFL_OBJECT_H_

typedef struct pepper_efl_object pepper_efl_object_t;

struct pepper_efl_object
{
   Evas *evas;
   Evas_Object *win;
   Evas_Object *smart_obj;
   Evas_Object *img;

   struct wl_shm_buffer *shm_buffer;

   pepper_efl_surface_t *es;

   pepper_buffer_t *buffer;
   pepper_event_listener_t *buffer_destroy_listener;

   int x, y, w, h;
};

Evas_Object *pepper_efl_object_add(pepper_efl_surface_t *es);
Eina_Bool    pepper_efl_object_buffer_attach(Evas_Object *obj, pepper_buffer_t *buffer, int *w, int *h);
void         pepper_efl_object_buffer_release(Evas_Object *obj);
void         pepper_efl_object_render(Evas_Object *obj);

#endif
