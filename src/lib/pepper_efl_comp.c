#define EFL_BETA_API_SUPPORT
#include <Eo.h>
#include "pepper_efl_compositor.eo.h"

#define MY_CLASS PEPPER_EFL_COMP_CLASS

typedef struct
{

} Pepper_Efl_Compositor_Data;

Evas_Object *
pepper_efl_compositor_add(Evas_Object *parent, const char *name, Elm_Win_Type type)
{
   obj = eo_add(MY_CLASS, parent,
                elm_obj_win_name_set(name),
                elm_obj_win_type_set(type));

   return obj;
}

EOLIAN static Eo_Base *
_pepper_efl_comp_eo_base_constructor(Eo *obj, Pepper_Efl_Compositor_Data *pd)
{
   return eo_do_super_ret(obj, MY_CLASS, obj, eo_constructor());
}

EOLIAN static void
_pepper_efl_comp_eo_base_destructor(Eo *obj, Pepper_Efl_Compositor_Data *pd)
{
   eo_do_super(obj, MY_CLASS, eo_destructor());
}

#include "pepper_efl_compositor.eo.c"
