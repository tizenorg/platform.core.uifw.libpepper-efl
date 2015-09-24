#include <Elementary.h>
#include "Pepper_Efl.h"

#define WIDTH 720
#define HEIGHT 480

static int gap = 0;

static Eina_Bool
_cb_timer(void *data)
{
   Evas_Object *img = data;

   evas_object_resize(img, 720, 1280);

   return ECORE_CALLBACK_CANCEL;
}

static void
_add_object_cb (void *d, Evas_Object *obj, void *event_info)
{
   Evas_Object *img = event_info;
   int x, y, w, h;

   if (img)
     {
        if (gap>300) gap = 0;
        evas_object_geometry_get(img, &x, &y, &w, &h);
        evas_object_move (img, x + gap, y + gap);
        evas_object_resize (img, w + gap, h + gap);
        evas_object_show(img);
        gap+=60;
     }
   ecore_timer_add(1, _cb_timer, img);
}

static void
_del_object_cb (void *d, Evas_Object *obj, void *event_info)
{
   Evas_Object *img = event_info;
   if (img)
     {
        gap-=60;
     }
}


static Evas_Object *
elm_win_create(const char *name, int w, int h)
{
   Evas_Object *win, *bg;

   win = elm_win_add(NULL, name, ELM_WIN_BASIC);
   elm_win_autodel_set(win, EINA_TRUE);

   bg = elm_bg_add(win);
   evas_object_size_hint_weight_set(bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, bg);
   evas_object_show(bg);

   evas_object_resize(win, w, h);
   evas_object_show(win);

   return win;
}

EAPI_MAIN int
elm_main(int argc EINA_UNUSED, char *argv[] EINA_UNUSED)
{
   Evas_Object *win;
   int ret = EXIT_FAILURE;

   elm_policy_set(ELM_POLICY_QUIT, ELM_POLICY_QUIT_LAST_WINDOW_CLOSED);
   win = elm_win_create("Pepper EFL Test", WIDTH, HEIGHT);

   if (!pepper_efl_compositor_create(win, NULL))
     return ret;

   evas_object_smart_callback_add(win, PEPPER_EFL_OBJ_ADD, _add_object_cb, NULL);
   evas_object_smart_callback_add(win, PEPPER_EFL_OBJ_DEL, _del_object_cb, NULL);

   elm_run();

   pepper_efl_compositor_destroy(NULL);

   return ret;
}
ELM_MAIN()
