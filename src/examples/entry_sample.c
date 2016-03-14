#include <Elementary.h>

EAPI int
elm_main(int argc, char *argv[])
{
   Evas_Object *win;
   Evas_Object *bx, *fr, *en;

   elm_policy_set(ELM_POLICY_QUIT, ELM_POLICY_QUIT_LAST_WINDOW_CLOSED);

   win = elm_win_util_standard_add("entry-sample", "Entry Sample");
   elm_win_borderless_set(win, EINA_TRUE);
   elm_win_autodel_set(win, EINA_TRUE);

   bx = elm_box_add(win);
   evas_object_size_hint_weight_set(bx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(bx, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_win_resize_object_add(win, bx);
   evas_object_show(bx);

   fr = elm_frame_add(win);
   elm_object_text_set(fr, "Key Input Test");
   evas_object_size_hint_weight_set(fr, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(fr, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(bx, fr);
   evas_object_show(fr);

   en = elm_entry_add(win);
   evas_object_size_hint_weight_set(en, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(en, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_object_text_set(en, "Entry");
   elm_object_content_set(fr, en);
   evas_object_show(en);

   evas_object_resize(win, 100, 50);
   evas_object_show(win);

   elm_run();

   return 0;
}
ELM_MAIN()
