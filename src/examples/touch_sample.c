#include<Ecore.h>
#include<Ecore_Evas.h>
#include<Elementary.h>

#define WIDTH 200
#define HEIGHT 200

int now = 1;

static Evas_Object *
win_add(int w, int h)
{
   Evas_Object *win, *bg;


   win = elm_win_add(NULL, "widget", ELM_WIN_BASIC);
   elm_win_autodel_set(win, EINA_TRUE);

   bg = elm_bg_add(win);
   evas_object_size_hint_weight_set(bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_color_set(bg, 0, 0, 0, 0);
   elm_win_resize_object_add(win, bg);
   evas_object_show(bg);

   evas_object_resize(win, w, h);
   elm_win_borderless_set(win, EINA_TRUE);

   evas_object_show(win);

   return win;
}

static void
bt_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   int num = now%5;
   Evas_Object *rect = data;

   if (num==0)
     evas_object_color_set(rect, 125, 0, 0, 255);
   else if (num==1)
     evas_object_color_set(rect, 0, 125, 0, 255);
   else if (num==2)
     evas_object_color_set(rect, 0, 0, 125, 255);
   else if (num==3)
     evas_object_color_set(rect, 0, 0, 0, 255);
   else
     evas_object_color_set(rect, 125, 125, 125, 255);

   now++;
}

int main(int argc EINA_UNUSED, char *argv[] EINA_UNUSED)
{
   Evas_Object *win;
   Evas_Object *button, *rect;
   Evas_Object *bx;
 
   elm_init(argc, argv);

   elm_policy_set(ELM_POLICY_QUIT, ELM_POLICY_QUIT_LAST_WINDOW_CLOSED);
   win = win_add(WIDTH, HEIGHT);

   bx = elm_box_add(win);
   evas_object_size_hint_weight_set(bx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, bx);
   evas_object_show(bx);
   

   button = elm_button_add(win);
   evas_object_size_hint_weight_set(button, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(button, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_object_text_set(button, "CHANGE");
   elm_box_pack_end(bx, button);
   evas_object_show(button);

   
   rect = evas_object_rectangle_add(win);
   evas_object_color_set(rect, 125, 0, 0, 255);
   evas_object_size_hint_weight_set(rect, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(rect, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(bx, rect);
   evas_object_show(rect);


   evas_object_event_callback_add(button, EVAS_CALLBACK_MOUSE_DOWN, bt_cb, rect);

   elm_run();
   elm_shutdown();

   return 0;
}
