#include<Ecore.h>
#include<Ecore_Evas.h>
#include<Elementary.h>

#define WIDTH 200
#define HEIGHT 200


static void
thread_run_cb(void *data, Ecore_Thread *thread)
{
   Evas_Object *obj = data;
   int now = 0;
   
   while (1)
     {

        int *num = malloc(sizeof(int));
        *num = now%5;
        now++;
        //elm_object_text_set(data, "clicked");


        ecore_thread_feedback(thread, num);

        usleep(1000000);

        if(ecore_thread_check(thread)) break;
     }
}

static void
thread_feedback_cb(void *data, Ecore_Thread *thread, void *msg)
{
   int *num = msg;
   Evas_Object *obj = data;

   if (*num==0)
     evas_object_color_set(obj, 125, 0, 0, 255);
   else if (*num==1)
     evas_object_color_set(obj, 0, 125, 0, 255);
   else if (*num==2)
     evas_object_color_set(obj, 0, 0, 125, 255);
   else if (*num==3)
     evas_object_color_set(obj, 0, 0, 0, 255);
   else
     evas_object_color_set(obj, 125, 125, 125, 255);

   free(num);

}

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
   int num = random()%5;
   //elm_object_text_set(data, "clicked");

   if (num==0)
     evas_object_color_set(obj, 125, 0, 0, 255);
   else if (num==1)
     evas_object_color_set(obj, 0, 125, 0, 255);
   else if (num==2)
     evas_object_color_set(obj, 0, 0, 125, 255);
   else if (num==3)
     evas_object_color_set(obj, 0, 0, 0, 255);
   else
     evas_object_color_set(obj, 125, 125, 125, 255);
}

int main(int argc EINA_UNUSED, char *argv[] EINA_UNUSED)
{
   Evas_Object *win;
   Evas_Object *button;
   Evas_Object *bx;
 
   elm_init(argc, argv);

   elm_policy_set(ELM_POLICY_QUIT, ELM_POLICY_QUIT_LAST_WINDOW_CLOSED);
   //ecore_evas_init();
   //ee = win_add();
   win = win_add(WIDTH, HEIGHT);

   bx = elm_box_add(win);
   evas_object_size_hint_weight_set(bx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, bx);
   evas_object_show(bx);
   

   //button = elm_button_add(win);
   button = evas_object_rectangle_add(win);
   //evas_object_move (button, 10, 10);
   //evas_object_resize(button, 100, 100);
   evas_object_color_set(button, 125, 125, 125, 255);
   evas_object_size_hint_weight_set(button, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(button, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(bx, button);
   evas_object_show(button);


   //evas_object_smart_callback_add(button, "clicked", bt_cb, NULL);
   //evas_object_event_callback_add(button, EVAS_CALLBACK_MOUSE_DOWN, bt_cb, NULL);


   Ecore_Thread *thread;

   thread = ecore_thread_feedback_run(thread_run_cb, thread_feedback_cb, NULL, NULL, button, EINA_FALSE);

   elm_run();
   elm_shutdown();

   //ecore_main_loop_begin();
   //ecore_evas_free(ee);
   //ecore_evas_shutdown();

   return 0;
}
