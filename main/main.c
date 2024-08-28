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
#include "my_play_mp3.h"
#include "audio_convert.h"
#include "bt_gatts.h"
#include "ucp.h"



static int g_sys_vol;
uint8_t get_sys_bat_level()
{
    int vol = g_sys_vol*2;
    uint8_t level = 0;

    if (vol > 3900) {
        level = 4;
    } else if (vol > 3800) {
        level = 3;
    } else if (vol > 3750) {
        level = 2;
    } else if (vol > 3680) {
        level = 1;
    } else {
        level = 0;
    }

    return level;
}

static void app_heart_beat_task(void *args)
{
    int print_count = 0;
    static char buf[100];

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(33));
        adc_get_voltage(&g_sys_vol);
        sprintf(buf, "bat:%d", g_sys_vol);
        ucp_send_heart_beat((uint8_t *)buf, strlen(buf));

        print_count++;
        if (0 == print_count % 30)
            ucp_print_stats();
    }

    vTaskDelete(NULL);
}


/*-----------------------------------------*/
// LED App, 以1s为周期轮询系统状态，然后闪灯
static void app_led_task(void *args)
{
    uint8_t bat_level;

    while (1) {
        if (is_sound_play_run()) {
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        if (is_motor_run()) {
            led_blink(5, 5, 5, 10);
            continue;
        }

        bat_level = get_sys_bat_level();
        if (0 == bat_level) {
            //低电量 闪红灯
            led_blink2(5, 0, 0);
            continue;
        }

        if (0 == ucp_get_conn_state()) {
            //未联网，闪黄灯
            led_blink2(5, 5, 0);
            continue;
        }

        //联网，绿灯常亮指示电量状态
        led_one_color2(0, 5, 0, bat_level);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void my_app_init()
{
    //创建hb task, 采集摇杆ADC 按键值 系统电压，然后组成心跳包
    xTaskCreate(app_heart_beat_task, "hb", 4096, NULL, 5, NULL);

    //创建闪灯task
    xTaskCreate(app_led_task, "appled", 2048, NULL, 5, NULL);
}

void app_main(void)
{
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    wifi_init_softap();
    //wifi_init_sta();

    //bt_gatts_init_and_run();

    pwm_init();
    gpio_init();

    sound_init();
    play_mp3_init();
    audio_convert_init();  //audio conver task run

    led_strip_init();  //led strip render task run
    adc_init();

    my_app_init();
}
