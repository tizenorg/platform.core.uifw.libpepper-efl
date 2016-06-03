#ifndef _PEPPER_EFL_OBJECT_H_
#define _PEPPER_EFL_OBJECT_H_

Evas_Object       *pepper_efl_object_get(pepper_efl_output_t *output, pepper_surface_t *surface);
Eina_Bool          pepper_efl_object_buffer_attach(Evas_Object *obj, int *w, int *h);
void               pepper_efl_object_render(Evas_Object *obj);
pepper_surface_t  *pepper_efl_object_pepper_surface_get(Evas_Object *obj);

#endif
