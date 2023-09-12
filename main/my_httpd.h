
#ifndef __MY_HTTPD_H__
#define __MY_HTTPD_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <esp_http_server.h>


httpd_handle_t start_webserver(void);
esp_err_t stop_webserver(httpd_handle_t server);



#ifdef __cplusplus
}
#endif

#endif /* __MY_HTTPD_H__ */
