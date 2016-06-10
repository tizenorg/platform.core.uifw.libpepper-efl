#ifndef _PEPPER_EFL_PRIVATE_H_
#define _PEPPER_EFL_PRIVATE_H_

#include <stdio.h>

#include <Eina.h>
#include <Ecore.h>
#include <Evas.h>

#include <pepper.h>
#include <wayland-server.h>
#include <wayland-tbm-server.h>

typedef struct _Pepper_Efl_Comp Pepper_Efl_Comp;
typedef struct pepper_efl_output pepper_efl_output_t;

#include "Pepper_Efl.h"
#include "log.h"
#include "output.h"
#include "input.h"
#include "shell.h"

struct _Pepper_Efl_Comp
{
   Eina_Stringshare *name;
   Evas_Object *screen;
   Ecore_Fd_Handler *fd_hdlr;

   Eina_Hash *output_hash;

   pepper_efl_input_t *input;
   pepper_efl_shell_t *shell;

   struct
   {
      pepper_compositor_t *comp;
   } pepper;

   struct
   {
      struct wl_display *disp;
      struct wl_event_loop *loop;
   } wl;

   struct wayland_tbm_server *tbm_server;
};

#define DEBUG

#define PEPPER_EFL_SURFACE_KEY         "pepper_efl_surface"

#define PE_FREE_FUNC(_h, _fn) do { if (_h) { _fn((void*)_h); _h = NULL; } } while (0)
#define PE_LIST_HANDLER_APPEND(list, type, callback, data) \
   do \
   { \
      Ecore_Event_Handler *_eh; \
      _eh = ecore_event_handler_add(type, (Ecore_Event_Handler_Cb)callback, data); \
      list = eina_list_append(list, _eh); \
   } while (0)
#define PE_FREE_LIST(list, free) \
   do \
   { \
      void *_tmp_; \
      EINA_LIST_FREE(list, _tmp_) \
        { \
           free(_tmp_); \
        } \
   } while (0)

#define container_of(ptr, sample, member)                                     \
   (__typeof__(sample))((char *)(ptr) - offsetof(__typeof__(*sample), member))

#ifdef DEBUG
# define PE_CHECK(x)             NULL
# define PE_CHECK_RET(x, ret)    NULL
#else
# define PE_CHECK(x)             EINA_SAFETY_ON_NULL_RETURN(x)
# define PE_CHECK_RET(x, ret)    EINA_SAFETY_ON_NULL_RETURN_VAL(x, ret)
#endif

/* Internal 'pepper_efl_object' */
Evas_Object       *pepper_efl_object_add(pepper_efl_output_t *output, pepper_surface_t *surface);
pepper_surface_t  *pepper_efl_object_pepper_surface_get(Evas_Object *obj);
void               pepper_efl_object_render(Evas_Object *obj);
Eina_Bool          pepper_efl_object_buffer_attach(Evas_Object *obj, int *w, int *h);

/* Internal 'tizen_policy' */
Eina_Bool    tizen_policy_init(Pepper_Efl_Comp *comp);
void         tizen_policy_shutdown(void);

#endif
