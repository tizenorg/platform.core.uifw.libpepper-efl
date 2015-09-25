#ifndef _PEPPER_EFL_SURFACE_H_
#define _PEPPER_EFL_SURFACE_H_

typedef struct pepper_efl_surface pepper_efl_surface_t;

struct pepper_efl_surface
{
   pepper_efl_output_t *output;

   Evas_Object *obj;

   struct wl_shm_buffer *shm_buffer;

   pepper_surface_t *surface;
   pepper_buffer_t *buffer;

   int buffer_width, buffer_height;

   pepper_event_listener_t *surface_destroy_listener;
};

pepper_efl_surface_t *pepper_efl_surface_get(pepper_efl_output_t *output, pepper_surface_t *surface);

#endif
