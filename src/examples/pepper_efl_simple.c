#include <Elementary.h>
#include "Pepper_Efl.h"
#include <time.h>

#define WIDTH 1920
#define HEIGHT 1080

static unsigned int seedx = 1;
static unsigned int seedy = 43210;

typedef struct
{
   Evas_Object *win;
} app_data;

static void
_add_object_cb (void *data, Evas_Object *obj, void *event_info)
{
   Evas_Coord x, y, w,h;
   Evas_Object *img = event_info;

   evas_object_geometry_get(img, &x, &y, &w, &h);
   fprintf(stderr, "[ECOMP] _add_object_cb %p:%p(%dx%d+%d+%d)\n",obj, img, w, h, x, y);

   x = rand_r(&seedx)%(WIDTH-w);
   y = rand_r(&seedy)%(HEIGHT-h);
   evas_object_move(img, x, y);
   evas_object_show(img);
}

static void
_del_object_cb(void *d, Evas_Object *obj, void *event_info)
{
   Evas_Object *img = event_info;
   if (img)
     {
        fprintf(stderr,"[ECOMP] _del_object_cb %p:%p\n", obj, img);
     }
   evas_object_del(img);
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
   app_data *d;
   Evas_Object *win;
   const char *comp_name;
   int ret = EXIT_FAILURE;

   d = calloc(1, sizeof(app_data));

   elm_policy_set(ELM_POLICY_QUIT, ELM_POLICY_QUIT_LAST_WINDOW_CLOSED);
   win = elm_win_create("Pepper EFL Test", WIDTH, HEIGHT);
   evas_object_show(win);

   /* init */
   d->win = win;

   /* pepper_efl init */
   comp_name = pepper_efl_compositor_create(win, "pepper-efl");
   if (!comp_name)
     goto err;

   fprintf(stderr, "display name:%s\n", comp_name);

   evas_object_smart_callback_add(win, PEPPER_EFL_OBJ_ADD, _add_object_cb, d);
   evas_object_smart_callback_add(win, PEPPER_EFL_OBJ_DEL, _del_object_cb, NULL);

   elm_run();

   pepper_efl_compositor_destroy(comp_name);

   ret = EXIT_SUCCESS;

err:
   free(d);
   return ret;
}
ELM_MAIN()
