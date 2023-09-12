/*  WiFi softAP Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_spiffs.h"

#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "my_wifi.h"
#include "my_gpio.h"
#include "drv8833_pwm.h"
#include "sound.h"
#include "led_strip.h"
#include "my_adc.h"

static const char *TAG = "fs";


esp_err_t init_fs(void)
{
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/www",
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = false
    };
    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return ESP_FAIL;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(NULL, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    }
    return ESP_OK;
}


void app_main(void)
{


    wifi_init_softap();

    pwm_init();
    gpio_init();
    init_fs();

    sound_init();

    led_strip_init();
    led_on();
    adc_init();


    //led_chase();
//    int cnt = 0;

    //adc_test();
    //while(1) {
    //    vTaskDelay(1000 / portTICK_PERIOD_MS);
    //    ESP_LOGI(TAG, "m1s:%d, m2s:%d\n", get_m1_cnt(), get_m2_cnt());
    //}

}
