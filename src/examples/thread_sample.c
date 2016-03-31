#include<Ecore.h>
#include<Ecore_Evas.h>
#include<Elementary.h>

#define WIDTH 200
#define HEIGHT 200

static void
thread_run_cb(void *data, Ecore_Thread *thread)
{
   int now = 0;
   int *num;

   while (1)
     {
        num = (int*)malloc(sizeof(int));
        *num = now%5;
        now++;

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

int
main(int argc EINA_UNUSED, char *argv[] EINA_UNUSED)
{
   Evas_Object *win;
   Evas_Object *button;
   Evas_Object *bx;
   Ecore_Thread *thread;

   elm_init(argc, argv);

   elm_policy_set(ELM_POLICY_QUIT, ELM_POLICY_QUIT_LAST_WINDOW_CLOSED);
   win = win_add(WIDTH, HEIGHT);

   bx = elm_box_add(win);
   evas_object_size_hint_weight_set(bx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, bx);
   evas_object_show(bx);

   button = evas_object_rectangle_add(win);
   evas_object_color_set(button, 125, 125, 125, 255);
   evas_object_size_hint_weight_set(button, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(button, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(bx, button);
   evas_object_show(button);

   thread = ecore_thread_feedback_run(thread_run_cb, thread_feedback_cb, NULL, NULL, button, EINA_FALSE);
   if (!thread)
     {
        fprintf(stderr, "failed to launch a thread\n");
        return -1;
     }

   elm_run();
   elm_shutdown();

   return 0;
}
