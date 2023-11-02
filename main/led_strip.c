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

const static int LEDSTRIP_RENDERER_STOPPED_BIT = BIT0;

struct led_strip_renderer_handle {
    /* Task related */
    volatile bool                    task_run;             /*!< Component running status */
    EventGroupHandle_t               state_event;          /*!< Task's state event group */
    QueueHandle_t                    in_queue;             /*!< Frame data receive queue */
};

static led_strip_renderer_handle_t g_ledstrip_renderer_handler;
static uint8_t led_strip_pixels[EXAMPLE_LED_NUMBERS * 3];

#define RMT_LED_STRIP_RESOLUTION_HZ 10000000 // 10MHz resolution, 1 tick = 0.1us (led strip needs a high resolution)
#define EXAMPLE_CHASE_SPEED_MS      50

static const char *TAG = "LED";

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

void led_chase(void)
{
    uint32_t red = 0;
    uint32_t green = 0;
    uint32_t blue = 0;
    uint16_t hue = 0;
    uint16_t start_rgb = 0;
    led_renderer_data_t data;

    ESP_LOGI(TAG, "Start LED rainbow chase");
    while (1) {
        for (int i = 0; i < EXAMPLE_LED_NUMBERS; i++) {
            // Build RGB pixels
            hue = i * 360 / EXAMPLE_LED_NUMBERS + start_rgb;
            led_strip_hsv2rgb(hue, 100, 100, &red, &green, &blue);
            data.pixels[i * 3 + 0] = green;
            data.pixels[i * 3 + 1] = blue;
            data.pixels[i * 3 + 2] = red;

            led_strip_fill_data(&data);
            vTaskDelay(pdMS_TO_TICKS(EXAMPLE_CHASE_SPEED_MS));

            memset(&data, 0, sizeof(led_renderer_data_t));
            led_strip_fill_data(&data);
            vTaskDelay(pdMS_TO_TICKS(EXAMPLE_CHASE_SPEED_MS));
        }
        start_rgb += 60;
    }
}

void led_on()
{
    led_renderer_data_t data;
    for (int i = 0; i < EXAMPLE_LED_NUMBERS; i++) {
        data.pixels[i * 3 + 0] = 0x9;
        data.pixels[i * 3 + 1] = 0x9;
        data.pixels[i * 3 + 2] = 0x9;
    }
    ESP_LOGI(TAG, "Turn on all LED");
    led_strip_fill_data(&data);
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


