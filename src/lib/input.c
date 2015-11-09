#include "private.h"

#include <Ecore_Input.h>

#include <xkbcommon/xkbcommon.h>
#include <pepper-input-backend.h>

static Eina_List *handlers = NULL;

static void
_pepper_efl_input_keymap_set(pepper_efl_input_t *input)
{
   struct xkb_keymap *keymap;
   struct xkb_context *context;
   struct xkb_rule_names names = { /* FIXME: hard coded */
      .rules = "evdev",
      .model = "pc105",
      .layout = "us",
      .options = NULL
   };

   DBG("Set Keymap");

   context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
   if (!context)
     {
        ERR("failed to create xkb_context");
        return;
     }

   keymap = xkb_map_new_from_names(context, &names, XKB_KEYMAP_COMPILE_NO_FLAGS);
   if (!keymap)
     {
        ERR("failed to get keymap");
        xkb_context_unref(context);
        return;
     }

   pepper_keyboard_set_keymap(input->keyboard, keymap);
   xkb_map_unref(keymap);

   input->xkb.context = context;
}

static Eina_Bool
_pepper_efl_input_cb_key_down(void *data, int ev_type EINA_UNUSED, Ecore_Event_Key *ev)
{
   pepper_efl_input_t *input = data;
   pepper_view_t  *view;
   uint32_t keycode;

   DBG("Key Press: keycode %d", ev->keycode);

   if (!view = pepper_keyboard_get_focus(input->keyboard))
     goto end;

   keycode = (ev->keycode - 8);
   pepper_keyboard_send_key(input->keyboard, view, ev->timestamp, keycode,
                            WL_KEYBOARD_KEY_STATE_PRESSED);
end:
   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
_pepper_efl_input_cb_key_up(void *data, int ev_type EINA_UNUSED, Ecore_Event_Key *ev)
{
   pepper_efl_input_t *input = data;
   pepper_view_t  *view;
   uint32_t keycode;

   DBG("Key Release: keycode %d", ev->keycode);

   if (!view = pepper_keyboard_get_focus(input->keyboard))
     goto end;

   keycode = (ev->keycode - 8);
   pepper_keyboard_send_key(input->keyboard, view, ev->timestamp, keycode,
                            WL_KEYBOARD_KEY_STATE_RELEASED);
end:
   return ECORE_CALLBACK_RENEW;
}

pepper_efl_input_t *
pepper_efl_input_create(pepper_efl_comp_t *comp)
{
   pepper_efl_input_t *input;
   const char *default_name = "seat0";

   DBG("Create Input Device");

   input = calloc(1, sizeof(*input));

   input->seat = pepper_compositor_add_seat(comp->pepper.comp, default_name);
   if (!input->seat)
     {
        ERR("failed to add seat");
        goto err_add_seat;
     }

   /* create and add devices. */
   input->device = pepper_input_device_create(comp->pepper.comp,
                                              (WL_SEAT_CAPABILITY_POINTER |
                                               WL_SEAT_CAPABILITY_KEYBOARD |
                                               WL_SEAT_CAPABILITY_TOUCH),
                                              NULL, NULL);
   if (!input->device)
     {
        ERR("failed to create input device");
        goto err_dev_create;
     }
   pepper_seat_add_input_device(input->seat, input->device);

   /* get device resource from seat */
   input->pointer = pepper_seat_get_pointer(input->seat);
   input->keyboard = pepper_seat_get_keyboard(input->seat);
   input->touch = pepper_seat_get_touch(input->seat);
   input->comp = comp;

   _pepper_efl_input_keymap_set(input);

   PE_LIST_HANDLER_APPEND(handlers, ECORE_EVENT_KEY_DOWN, _pepper_efl_input_cb_key_down, input);
   PE_LIST_HANDLER_APPEND(handlers, ECORE_EVENT_KEY_UP, _pepper_efl_input_cb_key_up, input);

   return input;

err_dev_create:
   pepper_seat_destroy(input->seat);

err_add_seat:
   free(input);

   return NULL;
}

void
pepper_efl_input_destroy(pepper_efl_input_t *input)
{
   Eina_List *l, *ll;
   Ecore_Event_Handler *hdl;

   EINA_LIST_FOREACH_SAFE(handlers, l, ll, hdl)
     {
        if (input != ecore_event_handler_data_get(hdl)) continue;

        ecore_event_handler_del(hdl);
        handlers = eina_list_remove(handlers, hdl);
     }

   xkb_context_unref(input->xkb.context);

   pepper_seat_remove_input_device(input->seat, input->device);
   pepper_input_device_destroy(input->device);

   pepper_seat_destroy(input->seat);
   free(input);
}
