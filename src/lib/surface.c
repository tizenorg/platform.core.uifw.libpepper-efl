#include "private.h"
#include <linux/input.h>

static void
_pepper_efl_surface_evas_cb_mouse_in(void *data, Evas *evas EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event EINA_UNUSED)
{
   pepper_efl_surface_t *es = data;
   pepper_efl_shell_surface_t *shsurf;

   shsurf = pepper_object_get_user_data((pepper_object_t *)es->surface,
                                        pepper_surface_get_role(es->surface));

   if (!shsurf)
     return;

   DBG("[SURFACE] Mouse In: shsurf %p", shsurf);

   pepper_pointer_set_focus(ptr, shsurf->view);
}

static void
_pepper_efl_surface_evas_cb_mouse_out(void *data, Evas *evas EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event EINA_UNUSED)
{
   DBG("[SURFACE] Mouse Out");

   pepper_pointer_set_focus(ptr, NULL);
}

static void
_pepper_efl_surface_evas_cb_mouse_move(void *data, Evas *evas EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event EINA_UNUSED)
{
   Evas_Event_Mouse_Move *ev = event;

   DBG("[SURFACE] Mouse Move: x %d y %d", ev->cur.canvas.x, ev->cur.canvas.y);

   pepper_pointer_send_motion(ptr, ev->timestamp, ev->cur.canvas.x, ev->cur.canvas.y);
}

static void
_pepper_efl_surface_evas_cb_mouse_down(void *data, Evas *evas EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event EINA_UNUSED)
{
   Evas_Event_Mouse_Down *ev = event;
   pepper_efl_surface_t *es = data;

   DBG("[SURFACE] Mouse Down");

   pepper_pointer_send_button(ptr, ev->timestamp, BTN_LEFT, PEPPER_BUTTON_STATE_PRESSED);

   evas_object_raise(es->obj);
}

static void
_pepper_efl_surface_evas_cb_mouse_up(void *data, Evas *evas EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event EINA_UNUSED)
{
   Evas_Event_Mouse_Up *ev = event;

   DBG("[SURFACE] Mouse Up");

   pepper_pointer_send_button(ptr, ev->timestamp, BTN_LEFT, PEPPER_BUTTON_STATE_RELEASED);
}

static void
_pepper_efl_surface_evas_cb_mouse_wheel(void *data, Evas *evas EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event EINA_UNUSED)
{
   DBG("[SURFACE] Mouse Wheel");
}

static void
_pepper_efl_surface_evas_cb_focus_in(void *data, Evas *evas EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event EINA_UNUSED)
{
   DBG("[SURFACE] Focus In");
}

static void
_pepper_efl_surface_evas_cb_focus_out(void *data, Evas *evas EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event EINA_UNUSED)
{
   DBG("[SURFACE] Focus Out");
}

static void
_pepper_efl_surface_cb_destroy(pepper_event_listener_t *listener EINA_UNUSED, pepper_object_t *object EINA_UNUSED, uint32_t id EINA_UNUSED, void *info EINA_UNUSED, void *data)
{
   pepper_efl_surface_t *es = data;

   evas_object_smart_callback_call(es->output->win, PEPPER_EFL_OBJ_DEL, (void *)es->obj);
   pepper_efl_object_buffer_release(es->obj);
   PE_FREE_FUNC(es->obj, evas_object_del);
   pepper_event_listener_remove(es->surface_destroy_listener);
   pepper_object_set_user_data((pepper_object_t *)es->surface,
                               (const void *)es->output, NULL, NULL);
   pepper_object_set_user_data((pepper_object_t *)es->surface,
                               PEPPER_EFL_SURFACE_KEY, NULL, NULL);

   es->output->update_list = eina_list_remove(es->output->update_list, es);

   free(es);
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
        es->obj = pepper_efl_object_add(es);
        es->surface = surface;
        es->surface_destroy_listener =
           pepper_object_add_event_listener((pepper_object_t *)surface,
                                            PEPPER_EVENT_OBJECT_DESTROY, 0,
                                            _pepper_efl_surface_cb_destroy, es);

        pepper_object_set_user_data((pepper_object_t *)surface, output, es, NULL);
        pepper_object_set_user_data((pepper_object_t *)surface, PEPPER_EFL_SURFACE_KEY, es, NULL);

#define EVENT_ADD(type, func)                                                    \
   evas_object_event_callback_priority_add(es->obj,                              \
                                           EVAS_CALLBACK_##type,                 \
                                           EVAS_CALLBACK_PRIORITY_AFTER,         \
                                           _pepper_efl_surface_evas_cb_##func,   \
                                           es);
        EVENT_ADD(MOUSE_IN, mouse_in);
        EVENT_ADD(MOUSE_OUT, mouse_out);
        EVENT_ADD(MOUSE_MOVE, mouse_move);
        EVENT_ADD(MOUSE_DOWN, mouse_down);
        EVENT_ADD(MOUSE_UP, mouse_up);
        EVENT_ADD(MOUSE_WHEEL, mouse_wheel);

        EVENT_ADD(FOCUS_IN, focus_in);
        EVENT_ADD(FOCUS_OUT, focus_out);
#undef EVENT_ADD
     }

   return es;
}
