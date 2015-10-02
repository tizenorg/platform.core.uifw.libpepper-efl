#ifndef _PEPPER_EFL_H_
#define _PEPPER_EFL_H_

/**
* @addtogroup PEPPER_EFL_MODULE
* @{
*/

/**
 * @brief Event name for smart callback.
 * @details This is event name for smart callback to listen add/del event of the clients
 * @since_tizen 3.0
 */
#define PEPPER_EFL_OBJ_ADD "add.object"
#define PEPPER_EFL_OBJ_DEL "del.object"


/**
 * @brief Creates the embedding compositor.
 * @since_tizen 3.0
 * @param[in] win Evas object of main window
 * @param[in] name Socket name
 * @return name of socket on success, @c NULL otherwise
 * @NOTE Passing NULL parameter as a socket name is also allowed.
 *       The return name will be automatically determined and returned.
 * @see pepper_efl_compositor_destroy()
 */
const char  *pepper_efl_compositor_create(Evas_Object *win, const char *name);

/**
 * @brief Destroys the embedding compositor of given socket name.
 * @since_tizen 3.0
 * @param[in] name Socket name
 * @return EINA_TRUE on success, @c EINA_FALSE otherwise
 * @see pepper_efl_compositor_create()
 */
Eina_Bool    pepper_efl_compositor_destroy(const char *name);

#endif
