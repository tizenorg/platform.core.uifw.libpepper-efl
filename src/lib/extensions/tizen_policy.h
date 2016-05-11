#ifndef _PEPPER_EFL_EXTENSION_H_
#define _PEPPER_EFL_EXTENSION_H_

typedef enum _Pepper_Efl_Visibility_Type
{
   PEPPER_EFL_VISIBILITY_TYPE_UNOBSCURED = 0,
   PEPPER_EFL_VISIBILITY_TYPE_PARTIALLY_OBSCURED,
   PEPPER_EFL_VISIBILITY_TYPE_FULLY_OBSCURED
} Pepper_Efl_Visibility_Type;

/**
 * @brief Set visibility type to given client of Evas_Object.
 * @since_tizen 3.0
 * @param[in] obj Evas object which is returned from libpepper-efl
 * @param[in] type The type of visibility
 * @return EINA_TRUE on success, @c EINA_FALSE otherwise
 */
Eina_Bool   pepper_efl_object_visibility_set(Evas_Object *obj, Pepper_Efl_Visibility_Type type);

#endif
