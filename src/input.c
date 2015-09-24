#include "private.h"

#include <pepper-input-backend.h>

pepper_pointer_t *ptr = NULL;

Eina_Bool
pepper_efl_input_init(pepper_efl_comp_t *comp)
{
   pepper_seat_t *seat;
   pepper_input_device_t *ptr_dev;
   const char *seat_name = "seat0";

   seat = pepper_compositor_add_seat(comp->pepper.comp, seat_name);
   ptr_dev = pepper_input_device_create(comp->pepper.comp,
                                        WL_SEAT_CAPABILITY_POINTER,
                                        NULL, NULL);

   pepper_seat_add_input_device(seat, ptr_dev);

   ptr = pepper_seat_get_pointer(seat);

   DBG("POINTER %p", ptr);

   return EINA_TRUE;
}
