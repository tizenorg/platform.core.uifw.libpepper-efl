#include <Elementary.h>
#include "Pepper_Efl.h"

#define WINDOW_WIDTH 1024
#define WINDOW_HEIGHT 768

#define CLIENT_WIDTH 100
#define CLIENT_HEIGHT 100

static Evas_Object *
elm_win_create(const char *name, int w, int h)
{
   Evas_Object *win, *bg;

   win = elm_win_add(NULL, name, ELM_WIN_BASIC);
   elm_win_autodel_set(win, EINA_TRUE);

   bg = elm_bg_add(win);
   elm_win_resize_object_add(win, bg);
   evas_object_show(bg);

   evas_object_resize(win, w, h);
   evas_object_show(win);

   return win;
}

static void add_client_cb(void *data, Evas_Object *obj, void *event_info)
{
   Evas_Object *client = (Evas_Object*)event_info;

   if (!client)
     {
        printf("Couldn't get data\n");
        abort();
     }

   evas_object_move(client, random()%(WINDOW_WIDTH-CLIENT_WIDTH), random()%(WINDOW_HEIGHT-CLIENT_HEIGHT));
   evas_object_resize(client, CLIENT_WIDTH, CLIENT_HEIGHT);
}

static void remove_client_cb(void *data, Evas_Object *obj, void *event_info)
{

}

int main(int argc, char **argv)
{
   Evas_Object *win;

   char name[] = "SubComp";

   elm_init(argc, argv);

   elm_policy_set(ELM_POLICY_QUIT, ELM_POLICY_QUIT_LAST_WINDOW_CLOSED);
   win = elm_win_create(name, WINDOW_WIDTH, WINDOW_HEIGHT);

   evas_object_smart_callback_add(win, "object.add", add_client_cb, NULL);
   evas_object_smart_callback_add(win, "object.del", remove_client_cb, NULL);

   pepper_efl_compositor_init(win, name);

   func(win);


   elm_run();
   elm_shutdown();

   return 0;
}
