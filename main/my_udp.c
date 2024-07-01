#include <string.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "handle_message.h"
#include "my_adc.h"
#include "bt_gatts.h"

#define UDP_SERVER_PORT 9000

static const char *TAG = "udp";
static char g_has_controller;
static struct sockaddr_in g_controller_addr;
static int g_sock = -1;

void udp_send_msg(char *msg, int len)
{
    int err;
    if (g_sock < 0) {
        return;
    }

    err = sendto(g_sock, msg, len, 0, (struct sockaddr *)&g_controller_addr, sizeof(g_controller_addr));
    if (err < 0) {
        ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
    }

    return;
}

static void udp_hearbeat_task(void *pvParameters)
{
    char hb_buffer[32];
    int voltage = 0; //mv

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(3000));  //5s心跳周期
        voltage = adc_get_voltage();
        sprintf(hb_buffer, "hb,%dmV", voltage);

        udp_send_msg(hb_buffer, strlen(hb_buffer));
    }

    vTaskDelete(NULL);
}

static void udp_create_hearbeat_task(void)
{
    printf("udp_create_hearbeat_task in...\n");
    xTaskCreate(udp_hearbeat_task, "udp_hb", 4096, NULL, 5, NULL);
}


static void udp_server_task(void *pvParameters)
{
    int addr_family = (int)pvParameters;
    int ip_protocol = 0;
    struct sockaddr_in6 dest_addr;
    static char rx_buffer[1024];

    if (addr_family == AF_INET) {
        struct sockaddr_in *dest_addr_ip4 = (struct sockaddr_in *)&dest_addr;
        dest_addr_ip4->sin_addr.s_addr = htonl(INADDR_ANY);
        dest_addr_ip4->sin_family = AF_INET;
        dest_addr_ip4->sin_port = htons(UDP_SERVER_PORT);
        ip_protocol = IPPROTO_IP;
    }

    struct sockaddr_in source_addr = {0};
    socklen_t socklen = sizeof(source_addr);
    while (1) {
        //step1. socket()
        g_sock = socket(addr_family, SOCK_DGRAM, ip_protocol);
        if (g_sock < 0) {
            ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
            break;
        }
        ESP_LOGI(TAG, "Socket created");

        //step2. bind()
        int err = bind(g_sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        if (err < 0) {
            ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
        }
        ESP_LOGI(TAG, "Socket bound, port %d", UDP_SERVER_PORT);

        //step3. recvfrom
        while (1) {
            int len = recvfrom(g_sock, rx_buffer, sizeof(rx_buffer) - 1, 0, (struct sockaddr *)&source_addr, &socklen);
            if (len < 0) {
                ESP_LOGE(TAG, "recvfrom failed: errno %d", errno);
                break;
            } else {
                if (!g_has_controller) {
                    g_has_controller = 1;
                    g_controller_addr = source_addr;
                    udp_create_hearbeat_task();
                    bt_disable();
                } else {
                    if (g_controller_addr.sin_addr.s_addr != source_addr.sin_addr.s_addr) {
                        ESP_LOGE(TAG, "recvfrom: not the same controller addr");
                        continue;
                    }
                    if (g_controller_addr.sin_port != source_addr.sin_port) {
                        ESP_LOGE(TAG, "recvfrom: controller's port change to:%d", source_addr.sin_port);
                        g_controller_addr.sin_port = source_addr.sin_port;
                    }
                }

                rx_buffer[len] = 0; // Null-terminate whatever we received and treat like a string...
                ESP_LOGI(TAG, "UDP: Received %d bytes", len);
                ESP_LOGI(TAG, "RECV: %s", rx_buffer);

                handle_message(rx_buffer, NULL);
            }
        }
        if (g_sock != -1) {
            ESP_LOGE(TAG, "Shutting down socket and restarting...");
            shutdown(g_sock, 0);
            close(g_sock);
            g_sock = -1;
        }
    }

    vTaskDelete(NULL);
}


static char g_is_udp_server_created;
void udp_create_server_task(void)
{
    if (g_is_udp_server_created) {
        return;
    }

    printf("udp_create_server_task in...\n");
    xTaskCreate(udp_server_task, "udp_server", 4096, (void*)AF_INET, 5, NULL);
    g_is_udp_server_created = 1;
}




