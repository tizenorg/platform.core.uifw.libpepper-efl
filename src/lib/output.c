// internal header
#include "private.h"

#include <pepper-output-backend.h>

static void
_pepper_efl_output_destroy(void *o EINA_UNUSED)
{
   DBG("callback output");
}

static int32_t
_pepper_efl_output_get_subpixel_order(void *o)
{
   pepper_efl_output_t *output = o;
   DBG("callback output");

   return output->subpixel;
}

static const char *
_pepper_efl_output_get_maker_name(void *o EINA_UNUSED)
{
   DBG("callback output");
   return "PePPer EFL";
}

static const char *
_pepper_efl_output_get_model_name(void *o EINA_UNUSED)
{
   DBG("callback output");
   return "PePPer EFL";
}

static int
_pepper_efl_output_get_mode_count(void *o EINA_UNUSED)
{
   DBG("callback output");
   return 1;
}

static void
_pepper_efl_output_get_mode(void *o, int index, pepper_output_mode_t *mode)
{
   pepper_efl_output_t *output = o;

   if (index != 0)
     return;

   DBG("callback output");

   if (mode)
     {
        mode->flags = WL_OUTPUT_MODE_CURRENT | WL_OUTPUT_MODE_PREFERRED;
        mode->w = output->w;
        mode->h = output->h;
        mode->refresh = 60000;
     }
}

static pepper_bool_t
_pepper_efl_output_set_mode(void *o EINA_UNUSED, const pepper_output_mode_t *mode EINA_UNUSED)
{
   DBG("callback output");
   return PEPPER_FALSE;
}

static void
_pepper_efl_output_assign_planes(void *o, const pepper_list_t *view_list)
{
   pepper_efl_output_t *output = o;
   pepper_list_t  *l;

   DBG("callback output");

   pepper_list_for_each_list(l, view_list)
     {
        pepper_view_t *view = l->item;
        pepper_view_assign_plane(view, output->base, output->primary_plane);
     }
}

static void
_pepper_efl_output_start_repaint_loop(void *o)
{
   pepper_efl_output_t *output = o;
   struct timespec     ts;

   DBG("callback start repaint loop");

   pepper_compositor_get_time(output->comp->pepper.comp, &ts);
   pepper_output_finish_frame(output->base, &ts);
}

static void
_pepper_efl_output_cb_render_post(void *data, Evas *e EINA_UNUSED, void *event_info EINA_UNUSED)
{
   pepper_efl_output_t *output = data;
   struct timespec     ts;

   DBG("render post");

   pepper_compositor_get_time(output->comp->pepper.comp, &ts);
   pepper_output_finish_frame(output->base, &ts);
}

static void
_pepper_efl_output_attach_surface(void *o, pepper_surface_t *surface, int *w, int *h)
{
   pepper_efl_output_t *output;
   Evas_Object *co;
   int rw = 0, rh = 0;
   Eina_Bool res;

   output = o;
   co = pepper_efl_object_get(output, surface);
   if (!co)
     {
        ERR("failed to get Evas_Object of client");
        goto end;
     }

   res = pepper_efl_object_buffer_attach(co, &rw, &rh);
   if (!res)
     goto end;

   pepper_efl_object_render(co);

end:
   if (w) *w = rw;
   if (h) *h = rh;
}

static void
_pepper_efl_output_flush_surface_damage(void *o, pepper_surface_t *surface, pepper_bool_t *keep_buffer)
{
   /* TODO */
   (void)o;
   (void)surface;
   (void)keep_buffer;
}

static const struct pepper_output_backend output_interface =
{
   _pepper_efl_output_destroy,

   _pepper_efl_output_get_subpixel_order,
   _pepper_efl_output_get_maker_name,
   _pepper_efl_output_get_model_name,

   _pepper_efl_output_get_mode_count,
   _pepper_efl_output_get_mode,
   _pepper_efl_output_set_mode,

   _pepper_efl_output_assign_planes,
   _pepper_efl_output_start_repaint_loop,
   NULL,
   _pepper_efl_output_attach_surface,
   _pepper_efl_output_flush_surface_damage
};

static void
_pepper_efl_output_cb_window_resize(void *data, Evas *e EINA_UNUSED, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   pepper_efl_output_t *output = data;
   int w, h;

   evas_object_geometry_get(obj, NULL, NULL, &w, &h);

   output->w = w;
   output->h = h;

   pepper_output_update_mode(output->base);
}

pepper_efl_output_t *
pepper_efl_output_create(pepper_efl_comp_t *comp, Evas_Object *win)
{
   pepper_efl_output_t *output = NULL;
   int w, h;

   DBG("create pepper-efl output");

   EINA_SAFETY_ON_NULL_RETURN_VAL(comp, NULL);

   output = calloc(1, sizeof(pepper_efl_output_t));
   if (!output)
     {
        ERR("oom, alloc output");
        goto err;
     }

   evas_object_geometry_get(win, NULL, NULL, &w, &h);

   output->comp = comp;
   output->win = win;
   output->format = PEPPER_FORMAT_XRGB8888;
   output->subpixel = WL_OUTPUT_SUBPIXEL_UNKNOWN;
   output->w = w;
   output->h = h;
   output->bpp = 32;
   output->stride = output->w * (output->bpp / 8);
   output->base = pepper_compositor_add_output(comp->pepper.comp,
                                               &output_interface, "efl", output, WL_OUTPUT_TRANSFORM_NORMAL, 1);
   if (!output->base)
     {
        ERR("failed add output to compositor");
        goto err_output;
     }

   output->primary_plane = pepper_output_add_plane(output->base, NULL);
   if (!output->primary_plane)
     {
        ERR("failed to add primary_plane");
        goto err_plane;
     }

   evas_object_event_callback_add(win, EVAS_CALLBACK_RESIZE, _pepper_efl_output_cb_window_resize, output);
   evas_event_callback_add(evas_object_evas_get(output->win),
                           EVAS_CALLBACK_RENDER_POST,
                           _pepper_efl_output_cb_render_post, output);

   return output;

err_plane:
   pepper_output_destroy(output->base);

err_output:
   free(output);

err:
   return NULL;
}

void
pepper_efl_output_destroy(pepper_efl_output_t *output)
{
   evas_object_event_callback_del_full(output->win, EVAS_CALLBACK_RESIZE, _pepper_efl_output_cb_window_resize, output);
   evas_event_callback_del_full(evas_object_evas_get(output->win), EVAS_CALLBACK_RENDER_POST, _pepper_efl_output_cb_render_post, output);
   PE_FREE_FUNC(output->primary_plane, pepper_plane_destroy);
   pepper_output_destroy(output->base);
   free(output);
}
