#ifndef _PEPPER_EFL_OUTPUT_H_
#define _PEPPER_EFL_OUTPUT_H_

#include <pepper-output-backend.h>

struct pepper_efl_output
{
   Pepper_Efl_Comp *comp;

   Evas_Object *win;
   Eina_List *update_list;

   pepper_output_t *base;
   pepper_list_t link;

   pepper_format_t format;
   int32_t subpixel;
   int w, h;
   int bpp;
   int stride;

   struct wl_event_source  *frame_done_timer;

   pepper_plane_t *primary_plane;
};

pepper_efl_output_t  *pepper_efl_output_create(Pepper_Efl_Comp *comp, Evas_Object *win);
void                  pepper_efl_output_destroy(pepper_efl_output_t *output);

#endif
