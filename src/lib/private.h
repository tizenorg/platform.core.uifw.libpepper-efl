#include <stdio.h>

#include <Eina.h>
#include <Ecore.h>
#include <Evas.h>

#include <pepper.h>
#include <wayland-server.h>
#include <wayland-tbm-server.h>

typedef struct pepper_efl_comp pepper_efl_comp_t;
typedef struct pepper_efl_output pepper_efl_output_t;

#include "Pepper_Efl.h"
#include "log.h"
#include "output.h"
#include "input.h"
#include "shell.h"
#include "object.h"

struct pepper_efl_comp
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
