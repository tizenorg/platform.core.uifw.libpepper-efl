#include "private.h"

static void
_pepper_efl_surface_cb_destroy(pepper_event_listener_t *listener EINA_UNUSED, pepper_object_t *object EINA_UNUSED, uint32_t id EINA_UNUSED, void *info EINA_UNUSED, void *data)
{
   pepper_efl_surface_t *es = data;

   evas_object_smart_callback_call(es->output->win, PEPPER_EFL_OBJ_DEL, (void *)es->obj);

   pepper_event_listener_remove(es->surface_destroy_listener);
   pepper_object_set_user_data((pepper_object_t *)es->surface,
                               (const void *)es->output, NULL, NULL);

   es->output->update_list = eina_list_remove(es->output->update_list, es);

   free(es);
}

Eina_Bool
pepper_efl_surface_update(pepper_efl_surface_t *es, int *w, int *h)
{
   if (!es->obj)
     return EINA_FALSE;

   if (!pepper_efl_object_buffer_attach(es->obj, w, h))
     return EINA_FALSE;

   return EINA_TRUE;
}

pepper_efl_surface_t *
pepper_efl_surface_get(pepper_efl_output_t *output, pepper_surface_t *surface)
{
   pepper_efl_surface_t *es;

   es = pepper_object_get_user_data((pepper_object_t *)surface, output);
   if (!es)
     {
        es = calloc(1, sizeof(pepper_efl_surface_t));
        if (!es)
          return NULL;

        es->output = output;
        es->obj = pepper_efl_object_add(es, output->win, surface);
        es->surface = surface;
        es->surface_destroy_listener =
           pepper_object_add_event_listener((pepper_object_t *)surface,
                                            PEPPER_EVENT_OBJECT_DESTROY, 0,
                                            _pepper_efl_surface_cb_destroy, es);

        pepper_object_set_user_data((pepper_object_t *)surface, output, es, NULL);
     }

   return es;
}
