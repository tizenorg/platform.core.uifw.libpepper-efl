#include "private.h"

#include <pepper-input-backend.h>

pepper_efl_seat_t *
pepper_efl_input_create(pepper_efl_comp_t *comp)
{
   pepper_efl_seat_t *seat;
   const char *default_name = "seat0";

   seat = calloc(1, sizeof(*seat));

   seat->seat = pepper_compositor_add_seat(comp->pepper.comp, default_name);
   if (!seat->seat)
     {
        ERR("failed to add seat");
        goto add_seat_err;
     }

   seat->ptr.device = pepper_input_device_create(comp->pepper.comp,
                                                 WL_SEAT_CAPABILITY_POINTER,
                                                 NULL, NULL);
   if (!seat->ptr.device)
     {
        ERR("failed to create pointer device");
        goto dev_create_err;
     }

   pepper_seat_add_input_device(seat->seat, seat->ptr.device);

   seat->ptr.resource = pepper_seat_get_pointer(seat->seat);
   seat->comp = comp;

   return seat;

dev_create_err:
   pepper_seat_destroy(seat->seat);
add_seat_err:
   free(seat);

   return NULL;
}

void
pepper_efl_input_destroy(pepper_efl_seat_t *seat)
{
   pepper_seat_remove_input_device(seat->seat, seat->ptr.device);
   pepper_seat_destroy(seat->seat);
   free(seat);
}
