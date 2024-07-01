/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "driver/rmt_tx.h"
#include "led_strip_encoder.h"
#include "io_assignment.h"
#include "led_strip.h"
#include "utils.h"

const static int LEDSTRIP_RENDERER_STOPPED_BIT = BIT0;

typedef struct {
    /* Task related */
    volatile bool                    task_run;             /*!< Component running status */
    EventGroupHandle_t               state_event;          /*!< Task's state event group */
    QueueHandle_t                    in_queue;             /*!< Frame data receive queue */
} led_strip_renderer_handle_t;

typedef struct {
    /* Task related */
    volatile bool                    task_run;             /*!< Component running status */
    EventGroupHandle_t               state_event;          /*!< Task's state event group */
    void                            *args;
} led_cmds_handle_t;

static led_strip_renderer_handle_t g_ledstrip_renderer_handler;
static led_cmds_handle_t g_led_cmds_handler;


static uint8_t led_strip_pixels[EXAMPLE_LED_NUMBERS * 3];

#define RMT_LED_STRIP_RESOLUTION_HZ 10000000 // 10MHz resolution, 1 tick = 0.1us (led strip needs a high resolution)

static const char *TAG = "LED";

void led_one_color(uint8_t r, uint8_t g, uint8_t b)
{
    led_renderer_data_t data;
    for (int i = 0; i < EXAMPLE_LED_NUMBERS; i++) {
        data.pixels[i * 3 + 0] = g;
        data.pixels[i * 3 + 1] = r;
        data.pixels[i * 3 + 2] = b;
    }
    led_strip_fill_data(&data);
}


/**
 * @brief Simple helper function, converting HSV color space to RGB color space
 *
 * Wiki: https://en.wikipedia.org/wiki/HSL_and_HSV
 *
 */
void led_strip_hsv2rgb(uint32_t h, uint32_t s, uint32_t v, uint32_t *r, uint32_t *g, uint32_t *b)
{
    h %= 360; // h -> [0,360]
    uint32_t rgb_max = v * 2.55f;
    uint32_t rgb_min = rgb_max * (100 - s) / 100.0f;

    uint32_t i = h / 60;
    uint32_t diff = h % 60;

    // RGB adjustment amount by hue
    uint32_t rgb_adj = (rgb_max - rgb_min) * diff / 60;

    switch (i) {
    case 0:
        *r = rgb_max;
        *g = rgb_min + rgb_adj;
        *b = rgb_min;
        break;
    case 1:
        *r = rgb_max - rgb_adj;
        *g = rgb_max;
        *b = rgb_min;
        break;
    case 2:
        *r = rgb_min;
        *g = rgb_max;
        *b = rgb_min + rgb_adj;
        break;
    case 3:
        *r = rgb_min;
        *g = rgb_max - rgb_adj;
        *b = rgb_max;
        break;
    case 4:
        *r = rgb_min + rgb_adj;
        *g = rgb_min;
        *b = rgb_max;
        break;
    default:
        *r = rgb_max;
        *g = rgb_min;
        *b = rgb_max - rgb_adj;
        break;
    }
}

void led_chase(int beatCnt)
{
#define CHASE_SPEED_MS      75

    uint32_t red = 0;
    uint32_t green = 0;
    uint32_t blue = 0;
    uint16_t hue = 0;
    uint16_t start_rgb = 0;
    led_renderer_data_t data = {0};

    for (int j = 0; j < beatCnt; j++) {
        for (int i = 0; i < EXAMPLE_LED_NUMBERS; i++) {
            // Build RGB pixels
            hue = i * 360 / EXAMPLE_LED_NUMBERS + start_rgb;
            led_strip_hsv2rgb(hue, 100, 100, &red, &green, &blue);
            data.pixels[i * 3 + 0] = green;
            data.pixels[i * 3 + 1] = red;
            data.pixels[i * 3 + 1] = blue;
            led_strip_fill_data(&data);
            vTaskDelay(pdMS_TO_TICKS(CHASE_SPEED_MS));

            memset(&data, 0, sizeof(led_renderer_data_t));
            led_strip_fill_data(&data);
            vTaskDelay(pdMS_TO_TICKS(CHASE_SPEED_MS));
        }
        start_rgb += 50;
    }
}


void led_on()
{
    led_one_color(20, 20, 20);
}
void led_off()
{
    led_one_color(0, 0, 0);
}
/* @count: blink times, each time cost 100ms */
void led_blink(uint8_t r, uint8_t g, uint8_t b, int count)
{
    for (int i = 0; i < count; i++) {
        led_one_color(r, g, b);
        vTaskDelay(pdMS_TO_TICKS(50));
        led_one_color(0, 0, 0);
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}


static rmt_channel_handle_t led_chan = NULL;
static rmt_encoder_handle_t led_encoder = NULL;
void led_strip_init()
{
    ESP_LOGI(TAG, "Create RMT TX channel");
    rmt_tx_channel_config_t tx_chan_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT, // select source clock
        .gpio_num = RMT_LED_STRIP_GPIO_NUM,
        .mem_block_symbols = 64, // increase the block size can make the LED less flickering
        .resolution_hz = RMT_LED_STRIP_RESOLUTION_HZ,
        .trans_queue_depth = 4, // set the number of transactions that can be pending in the background
    };
    ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_chan_config, &led_chan));

    ESP_LOGI(TAG, "Install led strip encoder");
    led_strip_encoder_config_t encoder_config = {
        .resolution = RMT_LED_STRIP_RESOLUTION_HZ,
    };
    ESP_ERROR_CHECK(rmt_new_led_strip_encoder(&encoder_config, &led_encoder));

    ESP_LOGI(TAG, "Enable RMT TX channel");
    ESP_ERROR_CHECK(rmt_enable(led_chan));

    g_ledstrip_renderer_handler.in_queue = xQueueCreate(3, sizeof(led_renderer_data_t));
    if (NULL == g_ledstrip_renderer_handler.in_queue) {
        ESP_LOGE(TAG, "xQueueCreate return null");
    }
    g_ledstrip_renderer_handler.state_event = xEventGroupCreate();
    g_led_cmds_handler.state_event = xEventGroupCreate();

    led_strip_renderer_run();
    led_on();
}

static void led_strip_renderer_task(void *args)
{
    led_strip_renderer_handle_t *handle = (led_strip_renderer_handle_t *)args;
    led_renderer_data_t *in_data = (led_renderer_data_t *)led_strip_pixels;

    rmt_transmit_config_t tx_config = {
        .loop_count = 0, // no transfer loop
    };

    while (handle->task_run) {
        if (xQueueReceive(handle->in_queue, in_data, 10000 / portTICK_PERIOD_MS) == pdTRUE) {
            ESP_ERROR_CHECK(rmt_transmit(led_chan, led_encoder, in_data, sizeof(led_renderer_data_t), &tx_config));
        }
    }

    xEventGroupSetBits(handle->state_event, LEDSTRIP_RENDERER_STOPPED_BIT);
    vTaskDelete(NULL);
}

esp_err_t led_strip_renderer_run(void)
{
    g_ledstrip_renderer_handler.task_run = true;
    xTaskCreate(led_strip_renderer_task, "led_task", 4096, &g_ledstrip_renderer_handler, 5, NULL);
    return ESP_OK;
}

esp_err_t led_strip_renderer_wait_for_stop(led_strip_renderer_handle_t *handle)
{
    EventBits_t uxBits = xEventGroupWaitBits(handle->state_event, LEDSTRIP_RENDERER_STOPPED_BIT, false, true, 8000 / portTICK_RATE_MS);
    esp_err_t ret = ESP_ERR_TIMEOUT;
    if (uxBits & LEDSTRIP_RENDERER_STOPPED_BIT) {
        ret = ESP_OK;
    }
    return ret;
}

esp_err_t led_strip_renderer_stop(void)
{
    led_strip_renderer_handle_t *handle = &g_ledstrip_renderer_handler;

    handle->task_run = false;
    esp_err_t ret = led_strip_renderer_wait_for_stop(handle);

    return ret;
}

esp_err_t led_strip_fill_data(led_renderer_data_t *data)
{
    led_strip_renderer_handle_t *handle = &g_ledstrip_renderer_handler;

    if (xQueueSend(handle->in_queue, data, 1) != pdPASS) {
        ESP_LOGW(TAG, "Discard this message");
        return ESP_FAIL;
    }
    return ESP_OK;
}

/* @return: 0-delay outside, 1-do not delay outside */
int led_cmd2data(char *cmd, int beatCnt)
{
    int ret = 0;
    if (0 == strcmp(cmd, "R")) {
        led_one_color(0x33, 0, 0);
    } else if (0 == strcmp(cmd, "G")) {
        led_one_color(0, 0x33, 0);
    } else if (0 == strcmp(cmd, "B")) {
        led_one_color(0, 0, 0x33);
    } else if (0 == strcmp(cmd, "RB")) {
        led_blink(0x33, 0, 0, 6*beatCnt);
        ret = 1;
    } else if (0 == strcmp(cmd, "GB")) {
        led_blink(0, 0x33, 0, 6*beatCnt);
        ret = 1;
    } else if (0 == strcmp(cmd, "BB")) {
        led_blink(0, 0, 0x33, 6*beatCnt);
        ret = 1;
    } else if (0 == strcmp(cmd, "Chase")) {
        led_chase(beatCnt);
        ret = 1;
    } else if (0 == strcmp(cmd, "OFF")) {
        led_off();
    } else {
        ESP_LOGW(TAG, "unknow led cmd, display white");
        led_one_color(0x33, 0x33, 0x33);
    }
    return ret;
}


/* 创建线程说明
    创建一个线程需要4个函数：
    1) led_run_cmds，对外接口，创建线程。如果线程已在运行，会先结束线程
    2) led_run_cmds_stop, 对外接口，结束线程
    3) led_run_cmds_wait_for_stop, 内部接口，等待线程结束，被led_run_cmds_stop调用
    4) led_run_cmds_task, 内部接口，最终创建的线程执行体
    5) 可选 xxxx_init，将下面2)描述的初始化放到这个函数里

    1) 需要一个handle结构体(led_cmds_handle_t) + handle结构体对应的全局变量(g_led_cmds_handler)
    2) 全局变量里的EventGroupHandle_t成员需要调用xEventGroupCreate()初始化

typedef struct {
    volatile bool                    task_run;
    EventGroupHandle_t               state_event;
    void                            *args;
} xxxx_handle_t;

static xxxx_handle_t g_xxxx_handler;

static void xxxx_task(void *args)
{
    xxxx_handle_t *handle = (xxxx_handle_t *)args;

    while (handle->task_run) {

    }
    xEventGroupSetBits(handle->state_event, BIT0);
    vTaskDelete(NULL);
}
static esp_err_t xxxx_wait_for_stop(void)
{
    EventBits_t uxBits = xEventGroupWaitBits(g_xxxx_handler.state_event, BIT0, true, true, 5000 / portTICK_RATE_MS);
    esp_err_t ret = ESP_ERR_TIMEOUT;
    if (uxBits & BIT0) {
        ret = ESP_OK;
    } else {
        ESP_LOGE(TAG, "wait for stop timeout.");
    }
    return ret;
}
esp_err_t xxxx_stop(void)
{
    xxxx_handle_t *handle = &g_xxxx_handler;
    if (false == handle->task_run) {
        return ESP_OK;
    }

    xEventGroupClearBits(handle->state_event, BIT0);
    handle->task_run = false;
    esp_err_t ret = xxxx_wait_for_stop();

    return ret;
}
void xxxx_run(void)
{
    xxxx_handle_t *handle = &g_xxxx_handler;
    if (true == handle->task_run) {
        xxxx_stop();
    }

    handle.task_run = true;
    xTaskCreate(xxxx_task, "xxxx_task", 4096, &g_xxxx_handler, 5, NULL);
}
void xxxx_init()
{
    g_xxxx_handler.state_event = xEventGroupCreate();
}

*/
extern void run_cmds_one_time_cb();
static void led_run_cmds_task(void *args)
{
    led_cmds_handle_t *handle = (led_cmds_handle_t *)args;
    char *cmds = handle->args;
    char code[16];
    int beatCnt;
    int next_cmd_pos = 0;
    float delaytime;
    int ret;

    while(*cmds) {
        if (!handle->task_run) {
            break;
        }
        next_cmd_pos = parse_cmd(cmds, code, &beatCnt);
        if (next_cmd_pos < 0) {
            break;
        }
        cmds += next_cmd_pos + 1;

        delaytime = beatCnt * BEAT_DURATION * 1000;

        if (code[0] == 'N' && code[1] == 'O') {
            vTaskDelay(pdMS_TO_TICKS(delaytime));
        } else {
            ret = led_cmd2data(code, beatCnt);
            if (0 == ret) {
                vTaskDelay(pdMS_TO_TICKS(delaytime));
            }
        }
    }

    led_on();

    if (handle->task_run) {
        //end normal
        handle->task_run = false;
        run_cmds_one_time_cb();
    }

    xEventGroupSetBits(handle->state_event, BIT0);
    vTaskDelete(NULL);
}
static esp_err_t led_run_cmds_wait_for_stop(void)
{
    EventBits_t uxBits = xEventGroupWaitBits(g_led_cmds_handler.state_event, BIT0, true, true, 5000 / portTICK_RATE_MS);
    esp_err_t ret = ESP_ERR_TIMEOUT;
    if (uxBits & BIT0) {
        ret = ESP_OK;
    } else {
        ESP_LOGE(TAG, "wait for stop timeout.");
    }
    return ret;
}
int led_run_cmds_stop(void)
{
    led_cmds_handle_t *handle = &g_led_cmds_handler;
    if (false == handle->task_run) {
        return ESP_OK;
    }

    xEventGroupClearBits(handle->state_event, BIT0);
    handle->task_run = false;
    esp_err_t ret = led_run_cmds_wait_for_stop();

    return ret;
}
/* @root: {type: L_CMDS, cmds:[]}, should call cJSON_Delete(root) to free memory */
void led_run_cmds(char *str)
{
    led_cmds_handle_t *handle = &g_led_cmds_handler;
    if (true == handle->task_run) {
        led_run_cmds_stop();
    }

    handle->task_run = true;
    handle->args = str;
    xTaskCreate(led_run_cmds_task, "led_run_cmds_task", 4096, handle, 5, NULL);
}



