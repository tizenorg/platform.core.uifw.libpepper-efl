#ifndef _PE_LOG_H_
#define _PE_LOG_H_

#include <Eina.h>

#ifdef __cplusplus
extern "C" {
#endif

extern int _pe_log_domain;

#define DBG(...)  EINA_LOG_DOM_DBG(_pe_log_domain, __VA_ARGS__)
#define INF(...)  EINA_LOG_DOM_INFO(_pe_log_domain, __VA_ARGS__)
#define WRN(...)  EINA_LOG_DOM_WARN(_pe_log_domain, __VA_ARGS__)
#define ERR(...)  EINA_LOG_DOM_ERR(_pe_log_domain, __VA_ARGS__)
#define CRI(...)  EINA_LOG_DOM_CRIT(_pe_log_domain, __VA_ARGS__)

int   pe_log_init(const char *domain);
void  pe_log_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif
