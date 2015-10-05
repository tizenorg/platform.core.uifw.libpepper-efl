#ifndef _PEPPER_EFL_INPUT_H_
#define _PEPPER_EFL_INPUT_H_

typedef struct pepper_efl_seat pepper_efl_seat_t;

struct pepper_efl_seat
{
   pepper_efl_comp_t *comp;

   char *name;

   pepper_seat_t *seat;
   struct
   {
      pepper_input_device_t *device;
      pepper_pointer_t *resource;
   } ptr;
};

pepper_efl_seat_t *pepper_efl_input_create(pepper_efl_comp_t *comp);
void               pepper_efl_input_destroy(pepper_efl_seat_t *seat);
#endif
