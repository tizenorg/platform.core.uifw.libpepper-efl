#include "log.h"

int _pe_log_domain = -1;
static int _init_count = 0;

int
pe_log_init(const char *domain)
{
   if (_init_count)
     goto end;

   if (!eina_init())
     {
        fprintf(stderr, "%s:%d - %s() error initializing Eina\n",
                __FILE__, __LINE__, __func__);
        return 0;
     }

   _pe_log_domain = eina_log_domain_register(domain, EINA_COLOR_LIGHTCYAN);

   if (_pe_log_domain < 0)
     {
        EINA_LOG_ERR("Unable to register '%s' log domain", domain);
        eina_shutdown();
        return 0;
     }

end:
   return ++_init_count;
}

void
pe_log_shutdown(void)
{
   if ((_init_count <= 0) ||
       (--_init_count > 0))
     return;

   eina_log_domain_unregister(_pe_log_domain);
   eina_shutdown();
}
