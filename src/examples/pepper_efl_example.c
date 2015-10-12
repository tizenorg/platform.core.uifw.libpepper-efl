#include <Elementary.h>
#include "Pepper_Efl.h"

#define WIDTH 1440
#define HEIGHT 2560

typedef struct
{
   Evas_Object *win;
   Eina_List *client_list;
   Evas_Object *tb;
   Evas_Object *tb2;
   Evas_Object *swap_bt;
   Evas_Object *scroller;
   Evas_Object *border_obj, *border_obj2;
   int app_num;
   int now_moving;
   int block_flag;
   int mode;
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

/* TODO this variable is declared for internal test. it should be deleted. */
int now;

static int
diff(int a, int b)
{
   if (a>b) return a-b;
   else return b-a;
}

static int
min(int a, int b)
{
   if (a>b) return b;
   else return a;
}

static void
_mode_swap_cb(void *data, Evas_Object *obj, void *event_info)
{
   app_data *d = (app_data*) data;
   Evas_Object *bt = d->swap_bt;
   Evas_Object *sc = d->scroller;

   if (!d->mode)     /* make -> scroll */
     {
        elm_object_text_set(bt, "scroll");
        elm_scroller_movement_block_set(sc, ELM_SCROLLER_MOVEMENT_BLOCK_VERTICAL);
        d->mode = 1;
     }
   else              /* scroll -> make */
     {
        elm_object_text_set(bt, "make");
        elm_scroller_movement_block_set(sc, 6);
        d->mode = 0;
     }
}

/* TODO This reset callback should be fixed.

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
*/

static void
_close_cb(void *data, Evas_Object *obj, void *event_info)
{
   elm_exit();
}

static void
_add_object_cb (void *data, Evas_Object *obj, void *event_info)
{
   int page;
   int width_c, height_c;
   int start_row, start_col;
   Evas_Object *img = event_info;
   app_data *d = data;

   /* TODO This lines are prevent codes for double packing. Now, when a widget app is connected to sub-compositor,
      the sub-comp receives commit event twice. */
   if (d->block_flag)
     {
        d->block_flag = 0;
        return;
     }
   d->block_flag = 1;

   if (img)
     {
        elm_scroller_current_page_get(d->scroller, &page, NULL);

        width_c = 1 + diff(coor_info.down_col, coor_info.up_col);
        height_c = 1 + diff(coor_info.down_row, coor_info.up_row);

        start_row = min(coor_info.down_row, coor_info.up_row);
        start_col = min(coor_info.down_col, coor_info.up_col);

        evas_object_size_hint_align_set(img, EVAS_HINT_FILL, EVAS_HINT_FILL);

        if (!page)
          elm_table_pack(d->tb, img, start_col, start_row, width_c, height_c);
        else
          elm_table_pack(d->tb2, img, start_col, start_row, width_c, height_c);

        d->client_list = eina_list_append(d->client_list, img);

        evas_object_show(img);
     }
}

static void
_del_object_cb(void *d, Evas_Object *obj, void *event_info)
{
   Evas_Object *img = event_info;
   if (img)
     {
        printf("deleted\n");
     }
}

/* functions for internal on_hole test : color_change_cb, onhold_test_create */
static void
color_change_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   int num = now % 5;
   Evas_Object *rect = data;
   Evas_Event_Mouse_Up *ev = event_info;

   if (ev->event_flags)
     return;

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

static void
onhold_test_create(app_data *d)
{
   Evas_Object *rect;
   int page;
   int width_c, height_c, start_row, start_col;

   rect = evas_object_rectangle_add(d->win);
   evas_object_color_set(rect, 125, 0, 0, 255);

   elm_scroller_current_page_get(d->scroller, &page, NULL);
   width_c = 1 + diff(coor_info.down_col, coor_info.up_col);
   height_c = 1 + diff(coor_info.down_row, coor_info.up_row);

   start_row = min(coor_info.down_row, coor_info.up_row);
   start_col = min(coor_info.down_col, coor_info.up_col);

   evas_object_size_hint_align_set(rect, EVAS_HINT_FILL, EVAS_HINT_FILL);

   if (!page)
     elm_table_pack(d->tb, rect, start_col, start_row, width_c, height_c);
   else
     elm_table_pack(d->tb2, rect, start_col, start_row, width_c, height_c);

   d->client_list = eina_list_append(d->client_list, rect);
   evas_object_event_callback_add(rect, EVAS_CALLBACK_MOUSE_UP, color_change_cb, rect);
   evas_object_show(rect);
}

static void
client_launch(app_data *d)
{
   pid_t pid;
   char path[64];

   /* internal onhold test */
   if (d->app_num == 2)
     {
        onhold_test_create(d);
        return;
     }

   pid = fork();

   if (pid == 0)
     {
        if (!d->app_num)
          {
             sprintf(path, "%s/touch_sample", SAMPLE_PATH);
             execl(path, "touch_sample", NULL);
          }
        else if (d->app_num == 1)
          {
             sprintf(path, "%s/thread_sample", SAMPLE_PATH);
             execl(path, "thread_sample", NULL);
          }
        else if (d->app_num == 3)
          {
             sprintf(path, "%s/entry_sample", SAMPLE_PATH);
             execl(path, "entry_sample", NULL);
          }
     }
}

static void
ret_index(int x, int y, int *row, int *col)
{
   int conv_x = x;
   int conv_y = y  - HEIGHT * 0.2;

   *col = conv_x / (WIDTH / 4);
   *row = conv_y / ((HEIGHT * 0.8) / 4);
}

static void
_table_up_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   int row, col;
   app_data *d = data;

   if (d->mode) return;

   Evas_Event_Mouse_Up *ev = event_info;

   ret_index(ev->canvas.x, ev->canvas.y, &row, &col);

   coor_info.up_row = row;
   coor_info.up_col = col;

   d->now_moving = 0;
   evas_object_hide(d->border_obj);
   evas_object_hide(d->border_obj2);

   client_launch(data);
}

static void
_table_move_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   int row, col, page;
   int diff_x, diff_y;
   Evas_Object *tb;
   Evas_Event_Mouse_Move *ev = event_info;
   app_data *d = (app_data*)data;

   if (d->mode) return;

   if (!d->now_moving) return;

   elm_scroller_current_page_get(d->scroller, &page, NULL);

   if (!page)
     tb = d->tb;
   else
     tb = d->tb2;

   ret_index(ev->cur.canvas.x, ev->cur.canvas.y, &row, &col);
   coor_info.up_row = row;
   coor_info.up_col = col;
   diff_x = diff(coor_info.up_col, coor_info.down_col);
   diff_y = diff(coor_info.up_row, coor_info.down_row);

   evas_object_size_hint_align_set(d->border_obj, EVAS_HINT_FILL, EVAS_HINT_FILL);

   if (!page)
     elm_table_pack(tb, d->border_obj, min(coor_info.up_col, coor_info.down_col), min(coor_info.up_row, coor_info.down_row), 1 + diff_x, 1 + diff_y);
   else
     elm_table_pack(tb, d->border_obj2, min(coor_info.up_col, coor_info.down_col), min(coor_info.up_row, coor_info.down_row), 1 + diff_x, 1 + diff_y);
}

static void
_table_down_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   int x, y, page;
   Evas_Object *tb;
   app_data *d = data;

   if (d->mode) return;

   elm_scroller_current_page_get(d->scroller, &page, NULL);

   if (!page)
     tb = d->tb;
   else
     tb = d->tb2;

   elm_table_pack_get(obj, &x, &y, NULL, NULL);
   coor_info.down_row = y;
   coor_info.down_col = x;

   if (!page)
     {
        evas_object_show(d->border_obj);
        elm_table_pack(tb, d->border_obj, x, y, 1, 1);
     }
   else
     {
        evas_object_show(d->border_obj2);
        elm_table_pack(tb, d->border_obj2, x, y, 1, 1);
     }
   d->now_moving = 1;
}

static void
_button_cb(void *data, Evas_Object *obj, void *event_info)
{
   app_data *d = data;

   d->app_num = 0;
}
static void
_button2_cb(void *data, Evas_Object *obj, void *event_info)
{
   app_data *d = data;

   d->app_num = 1;
}
static void
_button3_cb(void *data, Evas_Object *obj, void *event_info)
{
   app_data *d = data;

   d->app_num = 2;
}

static void
_button4_cb(void *data, Evas_Object *obj, void *event_info)
{
   app_data *d = data;

   d->app_num = 3;
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
   Evas_Object *bx, *upper_bx, *upper_bx2, *scroller, *bt, *bt2, *bt3, *bt4, *sc_bx, *swap_bt, *close_bt;
   const char *comp_name;
   char border_path[64];
   int ret = EXIT_FAILURE;
   int i, j;

   setenv("XDG_RUNTIME_DIR", "/run", 1);

   d = calloc(1, sizeof(app_data));

   elm_policy_set(ELM_POLICY_QUIT, ELM_POLICY_QUIT_LAST_WINDOW_CLOSED);
   win = elm_win_create("Pepper EFL Test", WIDTH, HEIGHT);

   /* init */
   d->win = win;
   d->app_num = 0;
   d->now_moving = 0;
   d->block_flag = 0;
   d->mode = 0;

   /* box setting */
   bx = elm_box_add(win);
   evas_object_size_hint_weight_set(bx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, bx);
   evas_object_show(bx);

   /* upper box(contain buttons) setting */
   upper_bx = elm_box_add(win);
   evas_object_size_hint_weight_set(upper_bx, EVAS_HINT_EXPAND, 0.1);
   evas_object_size_hint_align_set(upper_bx, EVAS_HINT_FILL, EVAS_HINT_FILL);

   elm_box_horizontal_set(upper_bx, 1);
   elm_box_pack_end(bx, upper_bx);
   evas_object_show(upper_bx);

   upper_bx2 = elm_box_add(win);
   evas_object_size_hint_weight_set(upper_bx2, EVAS_HINT_EXPAND, 0.1);
   evas_object_size_hint_align_set(upper_bx2, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_horizontal_set(upper_bx2, 1);
   elm_box_pack_end(bx, upper_bx2);
   evas_object_show(upper_bx2);

   /* buttons */
   bt = elm_button_add(win);
   elm_object_text_set(bt, "TOUCH");
   evas_object_smart_callback_add(bt, "clicked", _button_cb, d);
   elm_box_pack_end(upper_bx, bt);
   evas_object_size_hint_weight_set(bt, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(bt, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(bt);

   bt2 = elm_button_add(win);
   elm_object_text_set(bt2, "THREAD");
   evas_object_smart_callback_add(bt2, "clicked", _button2_cb, d);
   elm_box_pack_end(upper_bx, bt2);
   evas_object_size_hint_weight_set(bt2, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(bt2, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(bt2);

   bt3 = elm_button_add(win);
   elm_object_text_set(bt3, "HOLD");
   evas_object_smart_callback_add(bt3, "clicked", _button3_cb, d);
   elm_box_pack_end(upper_bx, bt3);
   evas_object_size_hint_weight_set(bt3, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(bt3, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(bt3);

   bt4 = elm_button_add(win);
   elm_object_text_set(bt4, "ENTRY");
   evas_object_smart_callback_add(bt4, "clicked", _button4_cb, d);
   elm_box_pack_end(upper_bx, bt4);
   evas_object_size_hint_weight_set(bt4, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(bt4, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(bt4);


   /* TODO when the reset function fixed, reset button should inserted in the box. */
   /*
   reset_bt = elm_button_add(win);
   elm_object_text_set(reset_bt, "RESET");
   evas_object_smart_callback_add(reset_bt, "clicked", _reset_cb, d);
   elm_box_pack_end(upper_bx, reset_bt);
   evas_object_size_hint_weight_set(reset_bt, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(reset_bt, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(reset_bt);
   */

   swap_bt = elm_button_add(win);
   elm_object_text_set(swap_bt, "Make");
   evas_object_smart_callback_add(swap_bt, "clicked", _mode_swap_cb, d);
   elm_box_pack_end(upper_bx2, swap_bt);
   evas_object_size_hint_weight_set(swap_bt, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(swap_bt, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(swap_bt);

   d->swap_bt = swap_bt;

   close_bt = elm_button_add(win);
   elm_object_text_set(close_bt, "CLOSE");
   evas_object_smart_callback_add(close_bt, "clicked", _close_cb, NULL);
   elm_box_pack_end(upper_bx2, close_bt);
   evas_object_size_hint_weight_set(close_bt, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(close_bt, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(close_bt);

   /* scroller */
   scroller = elm_scroller_add(bx);
   evas_object_size_hint_weight_set(scroller, EVAS_HINT_EXPAND, 0.8);
   evas_object_size_hint_align_set(scroller, EVAS_HINT_FILL, EVAS_HINT_FILL);

   elm_box_pack_end(bx, scroller);
   evas_object_show(scroller);

   d->scroller = scroller;

   /* scroller setting */
   elm_scroller_policy_set(scroller, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_OFF);
   elm_scroller_bounce_set(scroller, EINA_FALSE, EINA_TRUE);
   elm_scroller_page_relative_set(scroller, 1, 1);
   elm_scroller_page_scroll_limit_set(scroller, 1, 1);
   elm_scroller_loop_set(scroller, EINA_TRUE, EINA_FALSE);
   elm_scroller_movement_block_set(scroller, 6);

   /* Box in the scroller */
   sc_bx = elm_box_add(scroller);
   elm_box_horizontal_set(sc_bx, EINA_TRUE);
   elm_box_align_set(sc_bx, 0.5, 0.5);
   elm_object_content_set(scroller, sc_bx);
   evas_object_show(sc_bx);

   /* tables */
   d->tb = elm_table_add(sc_bx);
   elm_table_homogeneous_set(d->tb, EINA_TRUE);
   evas_object_size_hint_min_set(d->tb, WIDTH, HEIGHT*0.8);
   evas_object_size_hint_max_set(d->tb, WIDTH, HEIGHT*0.8);

   d->tb2 = elm_table_add(sc_bx);
   elm_table_homogeneous_set(d->tb2, EINA_TRUE);
   evas_object_size_hint_min_set(d->tb2, WIDTH, HEIGHT*0.8);
   evas_object_size_hint_max_set(d->tb2, WIDTH, HEIGHT*0.8);

   for(i=0;i<4;i++)
     {
        for(j=0;j<4;j++)
          {
             Evas_Object *obj;
             obj = evas_object_rectangle_add(evas_object_evas_get(d->tb));
             elm_table_pack(d->tb, obj, i, j, 1, 1);
             evas_object_color_set(obj, 0, 0, 0, 125);
             evas_object_size_hint_align_set(obj, EVAS_HINT_FILL, EVAS_HINT_FILL);
             evas_object_event_callback_add(obj, EVAS_CALLBACK_MOUSE_DOWN, _table_down_cb, d);
             evas_object_event_callback_add(obj, EVAS_CALLBACK_MOUSE_UP, _table_up_cb, d);
             evas_object_event_callback_add(obj, EVAS_CALLBACK_MOUSE_MOVE, _table_move_cb, d);
             evas_object_show(obj);
          }
     }
   evas_object_show(d->tb);
   elm_box_pack_end(sc_bx, d->tb);

   for(i=0;i<4;i++)
     {
        for(j=0;j<4;j++)
          {
             Evas_Object *obj;
             obj = evas_object_rectangle_add(evas_object_evas_get(d->tb2));
             elm_table_pack(d->tb2, obj, i, j, 1, 1);
             evas_object_color_set(obj, 125, 125, 125, 125);
             evas_object_size_hint_align_set(obj, EVAS_HINT_FILL, EVAS_HINT_FILL);
             evas_object_event_callback_add(obj, EVAS_CALLBACK_MOUSE_DOWN, _table_down_cb, d);
             evas_object_event_callback_add(obj, EVAS_CALLBACK_MOUSE_UP, _table_up_cb, d);
             evas_object_event_callback_add(obj, EVAS_CALLBACK_MOUSE_MOVE, _table_move_cb, d);
             evas_object_show(obj);
          }
     }
   evas_object_show(d->tb2);
   elm_box_pack_end(sc_bx, d->tb2);

   /* border object (pre-drawing box) */
   sprintf(border_path, "%s/red.png", DATA_PATH);

   d->border_obj = evas_object_image_filled_add(evas_object_evas_get(d->win));
   evas_object_image_file_set(d->border_obj, border_path, NULL);
   evas_object_image_border_set(d->border_obj, 2, 2, 2, 2);
   evas_object_image_border_center_fill_set(d->border_obj, EVAS_BORDER_FILL_NONE);
   evas_object_size_hint_align_set(d->border_obj, EVAS_HINT_FILL, EVAS_HINT_FILL);

   d->border_obj2 = evas_object_image_filled_add(evas_object_evas_get(d->win));
   evas_object_image_file_set(d->border_obj2, border_path, NULL);
   evas_object_image_border_set(d->border_obj2, 2, 2, 2, 2);
   evas_object_image_border_center_fill_set(d->border_obj2, EVAS_BORDER_FILL_NONE);
   evas_object_size_hint_align_set(d->border_obj2, EVAS_HINT_FILL, EVAS_HINT_FILL);

   /* pepper_efl init */
   comp_name = pepper_efl_compositor_create(win, NULL);
   if (!comp_name)
     return ret;

   setenv("WAYLAND_DISPLAY", comp_name, 1);

   evas_object_smart_callback_add(win, PEPPER_EFL_OBJ_ADD, _add_object_cb, d);
   evas_object_smart_callback_add(win, PEPPER_EFL_OBJ_DEL, _del_object_cb, NULL);

   elm_run();

   pepper_efl_compositor_destroy(comp_name);
   return ret;
}
ELM_MAIN()
