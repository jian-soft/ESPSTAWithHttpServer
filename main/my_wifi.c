#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "mdns.h"
#include "my_httpd.h"
#include "my_udp.h"
#include "bt_gatts.h"


static const char *TAG = "wifi";

#define MY_WIFI_SSID      "my_car"
#define MY_WIFI_PASS      "11111111"
#define MY_WIFI_CHANNEL   1
#define MY_STA_CONN       4

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                    int32_t event_id, void* event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" join, AID=%d",
                 MAC2STR(event->mac), event->aid);
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" leave, AID=%d",
                 MAC2STR(event->mac), event->aid);
    } else if (event_id == WIFI_EVENT_AP_START) {
        ESP_LOGI(TAG, "Soft AP started, now start httpd");
        start_webserver();
    }
}

void wifi_init_softap(void)
{
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = MY_WIFI_SSID,
            .ssid_len = strlen(MY_WIFI_SSID),
            .channel = MY_WIFI_CHANNEL,
            .password = MY_WIFI_PASS,
            .max_connection = MY_STA_CONN,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK,
            .pmf_cfg = {
                    .required = false,
            },
        },
    };
    if (strlen(MY_WIFI_PASS) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s channel:%d",
             MY_WIFI_SSID, MY_WIFI_PASS, MY_WIFI_CHANNEL);
}

static void initialise_mdns(void)
{
    mdns_init();
    mdns_hostname_set("smartcar");
    mdns_instance_name_set("smart car service");

    mdns_txt_item_t serviceTxtData[] = {
        {"company", "Jiansoft"}
    };

    ESP_ERROR_CHECK(mdns_service_add(NULL, "_car", "_udp", 9000, serviceTxtData,
                                     sizeof(serviceTxtData) / sizeof(serviceTxtData[0])));
}


static void station_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        esp_wifi_connect();
        ESP_LOGI(TAG, "WIFI_EVENT_STA_DISCONNECTED, retry to connect to the AP");
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED) {
        ESP_LOGI(TAG, "WIFI_EVENT_STA_CONNECTED");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        udp_create_server_task();
    }
}

void wifi_init_sta(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    initialise_mdns();
    //start_webserver();
    //udp_create_server_task();

    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &station_event_handler,
                                                        NULL,
                                                        NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &station_event_handler,
                                                        NULL,
                                                        NULL));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );

    wifi_config_t wifi_config = {0};
    ESP_ERROR_CHECK(esp_wifi_get_config(WIFI_IF_STA, &wifi_config));
    ESP_LOGI(TAG, "sta.config.ssid:%s, sta.config.password:%s", wifi_config.sta.ssid, wifi_config.sta.password);

    if (wifi_config.sta.ssid[0]) {
        car_set_wifi_config_status(1);
        ESP_ERROR_CHECK(esp_wifi_start() );
    }
}


void wifi_set_config_and_start(char *ssid, char *passwd)
{
    wifi_config_t wifi_config = {0};
    memcpy(wifi_config.sta.ssid, ssid, 32);
    memcpy(wifi_config.sta.password, passwd, 64);
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start() );
}

