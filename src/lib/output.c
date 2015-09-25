// internal header
#include "private.h"

#include <pepper-output-backend.h>

static void
_pepper_efl_output_destroy(void *o)
{
   pepper_efl_output_t *output = o;
   pepper_efl_comp_t *comp = output->comp;

   DBG("callback output");

   comp->output_list = eina_list_remove(comp->output_list, output);
   free(output);
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
_pepper_efl_output_cb_render_post(void *data, Evas *e EINA_UNUSED, void *event_info EINA_UNUSED)
{
   pepper_efl_output_t *output = data;
   pepper_efl_surface_t *es;

   DBG("render post");

   // FIXME: I'm not sure why this do twice both in output_repaint and render_post,
   // but if we remove it, then the client commiting only once when launched
   // couldn't update image.
   EINA_LIST_FREE(output->update_list, es)
      pepper_efl_object_render(es->obj);

   pepper_output_finish_frame(output->base, NULL);
}

static void
_pepper_efl_output_repaint(void *o EINA_UNUSED, const pepper_list_t *plane_list EINA_UNUSED)
{
   pepper_efl_output_t *output = o;
   pepper_efl_surface_t *es;
   Eina_List *l;

   DBG("callback output");

   EINA_LIST_FOREACH(output->update_list, l, es)
      pepper_efl_object_render(es->obj);
}

static void
_pepper_efl_output_attach_surface(void *o, pepper_surface_t *surface, int *w, int *h)
{
   pepper_efl_output_t *output = o;
   pepper_efl_surface_t *es;
   pepper_buffer_t *buffer;
   int rw = 0, rh = 0;

   es = pepper_efl_surface_get(output, surface);
   if (!es)
     {
        ERR("failed to get pepper_efl_surface_t");
        goto end;
     }

   buffer = pepper_surface_get_buffer(surface);
   if (!buffer)
     {
        ERR("failed to get pepper_buffer_t");
        goto end;
     }

   if (!pepper_efl_object_buffer_attach(es->obj, buffer, &rw, &rh))
     {
        ERR("failed to attach shm to pepper_efl_object_t");
        goto end;
     }

   output->update_list = eina_list_append(output->update_list, es);

end:
   if (w) *w = rw;
   if (h) *h = rh;
}

static void
_pepper_efl_output_flush_surface(void *o, pepper_surface_t *surface)
{
   (void)o;
   (void)surface;
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
   _pepper_efl_output_repaint,
   _pepper_efl_output_attach_surface,
   _pepper_efl_output_flush_surface
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
        return EINA_FALSE;
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
                                               &output_interface, "efl", output);
   if (!output->base)
     {
        ERR("failed add output to compositor");
        free(output);
        return EINA_FALSE;
     }

   output->primary_plane = pepper_output_add_plane(output->base, NULL);

   evas_object_event_callback_add(win, EVAS_CALLBACK_RESIZE, _pepper_efl_output_cb_window_resize, output);
   evas_event_callback_add(evas_object_evas_get(output->win),
                           EVAS_CALLBACK_RENDER_POST,
                           _pepper_efl_output_cb_render_post, output);

   comp->output_list = eina_list_append(comp->output_list, output);

   return output;
}
