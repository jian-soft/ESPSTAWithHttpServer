
#ifndef __MY_WIFI_H__
#define __MY_WIFI_H__

#ifdef __cplusplus
extern "C" {
#endif

void wifi_init_softap(void);
void wifi_init_sta(void);
void wifi_set_config_and_start(char *ssid, char *passwd);


#ifdef __cplusplus
}
#endif

#endif
