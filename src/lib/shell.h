#ifndef _PEPPER_EFL_SHELL_H_
#define _PEPPER_EFL_SHELL_H_

typedef struct pepper_efl_shell pepper_efl_shell_t;
typedef struct pepper_efl_shell_client pepper_efl_shell_client_t;
typedef struct pepper_efl_shell_surface pepper_efl_shell_surface_t;

struct pepper_efl_shell_client {
   Pepper_Efl_Comp *comp;

   Eina_List *shsurf_list;

   struct wl_resource *resource;
   struct wl_listener destroy_listener;
};

struct pepper_efl_shell_surface
{
   Pepper_Efl_Comp *comp;
   pepper_efl_shell_client_t *shell_client;

   pepper_surface_t *surface;
   pepper_view_t *view;
   /* Listeners */
   pepper_event_listener_t *surface_destroy_listener;
   pepper_event_listener_t *surface_commit_listener;

   struct wl_resource *resource;

   void (*send_configure)(pepper_efl_shell_surface_t *shsurf, int32_t width, int32_t height);
   void (*shell_surface_map)(pepper_efl_shell_surface_t *shsurf);

   struct
   {
      void (*func)(void *data, int w, int h);
      void *data;
      int width, height;
   } configure_done;

   Eina_Bool mapped;
   Eina_Bool ack_configure;

   const char *title;
   const char *app_id;
};

Eina_Bool   pepper_efl_shell_init(Pepper_Efl_Comp *comp);
void        pepper_efl_shell_shutdown(void);
void        pepper_efl_shell_configure(pepper_efl_shell_surface_t *shsurf, int width, int height, void (*configure_done)(void *data, int w, int h), void *data);

#endif
