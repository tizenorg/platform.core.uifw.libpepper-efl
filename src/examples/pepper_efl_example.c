#include <Elementary.h>
#include "Pepper_Efl.h"

#define WIDTH 1440
#define HEIGHT 2560

typedef struct
{
   Eina_List *client_list;
   Evas_Object *tb;

} app_data;

struct
{
   int down_row;
   int down_col;
   int up_row;
   int up_col;
} coor_info = {
     .down_row = 0,
     .down_col = 0, 
     .up_row = 0, 
     .up_col = 0
};

Evas_Object *moving_obj;
int app_num;
int now_moving = 0;
int block_flag = 0;


static int diff(int a, int b)
{
   if (a>b) return a-b;
   else return b-a;
}

static int min(int a, int b)
{
   if (a>b) return b;
   else return a;
}

static void
_img_mouse_down_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
}

static void
_reset_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   app_data *d = data;
   Evas_Object *img;

   EINA_LIST_FREE(d->client_list, img)
     {
        evas_object_del(img);
     }
}

static void
_add_object_cb (void *d, Evas_Object *obj, void *event_info)
{
   if (block_flag)
     {
        block_flag = 0;
        return;
     }

   block_flag = 1;

   Evas_Object *img = event_info;
   int x, y, w, h;

   if (img)
     {
        int width_c, height_c;
        int start_row, start_col;
        app_data *data = d;
        Evas_Object *tb = data->tb;


        //evas_object_smart_callback_add(img, "clicked", _img_mouse_down_cb, NULL);

        width_c = 1 + diff(coor_info.down_col, coor_info.up_col);
        height_c = 1 + diff(coor_info.down_row, coor_info.up_row);

        start_row = min(coor_info.down_row, coor_info.up_row);
        start_col = min(coor_info.down_col, coor_info.up_col);

        evas_object_size_hint_align_set(img, EVAS_HINT_FILL, EVAS_HINT_FILL);


        elm_table_pack(tb, img, start_col, start_row, width_c, height_c);

        data->client_list = eina_list_append(data->client_list, img);
        
        evas_object_show(img);

     }
}

static void
_del_object_cb (void *d, Evas_Object *obj, void *event_info)
{
   Evas_Object *img = event_info;
   if (img)
     {
        printf("deleted\n");
     }
}

static void client_launch()
{

   pid_t pid;
   char path[64];
  
   pid = fork();


   if (pid == 0)
     {
        setenv("XDG_RUNTIME_DIR", "/tmp", 1);

        if (!app_num)
          {
             sprintf(path, "%s/touch_sample", SAMPLE_PATH);
             execl(path, "touch_sample", NULL);
          }
        else
          {
             sprintf(path, "%s/thread_sample", SAMPLE_PATH);
             execl(path, "thread_sample", NULL);
          }
     }
}

static void ret_index(int x, int y, int *row, int *col)
{
  int conv_x = x;
  int conv_y = y  - HEIGHT * 0.1;

  *col = conv_x / (WIDTH / 4);
  *row = conv_y / ((HEIGHT * 0.9) / 4);

}

static void _table_up_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   int row, col;

   Evas_Event_Mouse_Up *ev = event_info;
   Evas_Object *tb = data;

   ret_index(ev->canvas.x, ev->canvas.y, &row, &col);

   coor_info.up_row = row;
   coor_info.up_col = col;



   elm_table_unpack(tb, moving_obj);
   //evas_object_del(moving_obj);
   now_moving = 0;
   evas_object_hide(moving_obj);


   client_launch();

}


static void _table_move_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   //if (!moving_obj) return;
   if (!now_moving) return;

   int row, col;
   int diff_x, diff_y;
   Evas_Event_Mouse_Move *ev = event_info;
   Evas_Object *tb = data;

   ret_index(ev->cur.canvas.x, ev->cur.canvas.y, &row, &col);

   coor_info.up_row = row;
   coor_info.up_col = col;


   diff_x = diff(coor_info.up_col, coor_info.down_col);
   diff_y = diff(coor_info.up_row, coor_info.down_row);

   elm_table_unpack(tb, moving_obj);
   
   evas_object_size_hint_align_set(moving_obj, EVAS_HINT_FILL, EVAS_HINT_FILL);

   elm_table_pack(tb, moving_obj, min(coor_info.up_col, coor_info.down_col), min(coor_info.up_row, coor_info.down_row), 1 + diff_x, 1 + diff_y);

}

static void _table_down_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   int x, y;
   Evas_Object *m, *tb;
   char border_path[64];

   sprintf(border_path, "%s/red.png", DATA_PATH);

   tb = data;

   elm_table_pack_get(obj, &x, &y, NULL, NULL);

   coor_info.down_row = y;
   coor_info.down_col = x;

   if (!moving_obj)
     {
        moving_obj = evas_object_image_filled_add(evas_object_evas_get(tb));
        evas_object_image_file_set(moving_obj, border_path, NULL);
        evas_object_image_border_set(moving_obj, 2, 2, 2, 2);
        evas_object_image_border_center_fill_set(moving_obj, EVAS_BORDER_FILL_NONE);

        evas_object_size_hint_align_set(moving_obj, EVAS_HINT_FILL, EVAS_HINT_FILL);
     }
   now_moving = 1;
   evas_object_show(moving_obj);
   elm_table_pack(tb, moving_obj, x, y, 1, 1);

}

static void _button_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   app_num = 0;
}
static void _button2_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   app_num = 1;
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
   Evas_Object *tb;
   Evas_Object *bx, *upper_bx, *mid_bx, *bt, *bt2, *reset_bt;
   Evas_Object *mid_bx_bg;
   char *comp_name;
   int i, j;

   int ret = EXIT_FAILURE;
   setenv("XDG_RUNTIME_DIR", "/run", 1);

   d = calloc(1, sizeof(app_data));

   elm_policy_set(ELM_POLICY_QUIT, ELM_POLICY_QUIT_LAST_WINDOW_CLOSED);
   win = elm_win_create("Pepper EFL Test", WIDTH, HEIGHT);

   //container
   bx = elm_box_add(win);
   evas_object_size_hint_weight_set(bx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, bx);
   evas_object_show(bx);

   //box container
   upper_bx = elm_box_add(win);
   evas_object_size_hint_weight_set(upper_bx, EVAS_HINT_EXPAND, 0.1);
   evas_object_size_hint_align_set(upper_bx, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_horizontal_set(upper_bx, 1);
   elm_box_pack_end(bx, upper_bx);
   evas_object_show(upper_bx);


   //buttons
   bt = elm_button_add(win);
   elm_object_text_set(bt, "TOUCH");
   evas_object_smart_callback_add(bt, "clicked", _button_cb, NULL);
   elm_box_pack_end(upper_bx, bt);
   evas_object_size_hint_weight_set(bt, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(bt, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(bt);


   bt2 = elm_button_add(win);
   elm_object_text_set(bt2, "THREAD");
   evas_object_smart_callback_add(bt2, "clicked", _button2_cb, NULL);
   elm_box_pack_end(upper_bx, bt2);
   evas_object_size_hint_weight_set(bt2, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(bt2, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(bt2);

   reset_bt = elm_button_add(win);
   elm_object_text_set(reset_bt, "RESET");
   evas_object_smart_callback_add(reset_bt, "clicked", _reset_cb, d);
   elm_box_pack_end(upper_bx, reset_bt);
   evas_object_size_hint_weight_set(reset_bt, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(reset_bt, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(reset_bt);


   //under box
   mid_bx = elm_box_add(win);
   evas_object_size_hint_weight_set(mid_bx,EVAS_HINT_EXPAND, 0.9);
   evas_object_size_hint_align_set(mid_bx, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(bx, mid_bx);
   evas_object_show(mid_bx);

   //table
   d->tb = elm_table_add(win);
   elm_table_homogeneous_set(d->tb, EINA_TRUE);
   evas_object_size_hint_align_set(d->tb, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_size_hint_weight_set(d->tb, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

   for(i=0;i<4;i++)
     {
        for(j=0;j<4;j++)
          {
             Evas_Object *obj;
             obj = evas_object_rectangle_add(evas_object_evas_get(d->tb));
             elm_table_pack(d->tb, obj, i, j, 1, 1);
             evas_object_color_set(obj, 125, 125, 125, 125);
             evas_object_size_hint_align_set(obj, EVAS_HINT_FILL, EVAS_HINT_FILL);
             evas_object_event_callback_add(obj, EVAS_CALLBACK_MOUSE_DOWN, _table_down_cb, d->tb);
             evas_object_event_callback_add(obj, EVAS_CALLBACK_MOUSE_UP, _table_up_cb, d->tb);
             evas_object_event_callback_add(obj, EVAS_CALLBACK_MOUSE_MOVE, _table_move_cb, d->tb);


             evas_object_show(obj);
          }
     }
   evas_object_show(d->tb);
   elm_box_pack_end(mid_bx, d->tb);



   comp_name = pepper_efl_compositor_create(win, NULL);
   if (!comp_name)
     return ret;

   evas_object_smart_callback_add(win, PEPPER_EFL_OBJ_ADD, _add_object_cb, d);
   evas_object_smart_callback_add(win, PEPPER_EFL_OBJ_DEL, _del_object_cb, NULL);


   elm_run();

   pepper_efl_compositor_destroy(comp_name);

   return ret;
}
ELM_MAIN()
