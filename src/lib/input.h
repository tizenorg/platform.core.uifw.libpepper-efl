#ifndef _PEPPER_EFL_INPUT_H_
#define _PEPPER_EFL_INPUT_H_

typedef struct pepper_efl_input pepper_efl_input_t;

struct pepper_efl_input
{
   pepper_efl_comp_t *comp;

   pepper_seat_t *seat;
   pepper_input_device_t *device;
   pepper_pointer_t *pointer;
   pepper_keyboard_t *keyboard;
   pepper_touch_t *touch;

   char *name;

   struct
   {
      struct xkb_context *context;
   } xkb;
};

pepper_efl_input_t   *pepper_efl_input_create(pepper_efl_comp_t *comp);
void                  pepper_efl_input_destroy(pepper_efl_input_t *seat);
#endif
