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
#include "sound.h"
#include "cnv_audio.h"
#include "led_strip.h"
#include "audio_convert.h"

static const char *TAG = "CNV";
const static int AUDIO_CONVERT_STOPPED_BIT = BIT0;

typedef struct {
    void *data;
    int len;  //采样点个数
} audio_sample_data_t;

typedef struct {
    /* Task related */
    volatile bool                    task_run;             /*!< Component running status */
    EventGroupHandle_t               state_event;          /*!< Task's state event group */
    QueueHandle_t                    in_queue;             /*!< Frame data receive queue */
} audio_convert_handle_t;

static audio_convert_handle_t g_cnv_handler;

cnv_audio_t g_cnv_audio;


void volume_to_lightness(uint8_t volume, led_renderer_data_t *pleddata)
{
    uint8_t light;
    light = (volume * 255)/100;
    for (int i = 0; i < EXAMPLE_LED_NUMBERS; i++) {
        pleddata->pixels[i * 3 + 0] = light;
        pleddata->pixels[i * 3 + 1] = light;
        pleddata->pixels[i * 3 + 2] = light;
    }
}

/* @source_data: 16bit value, @data_len: len of 16bit value */
void cnv_pattern_energy_mode(void *source_data, int data_len)
{
    static uint16_t level, level_pre, x_forward, x_reverse;
    cnv_audio_t *audio = &g_cnv_audio;
    uint8_t volume = 0;
    led_renderer_data_t leddata = {0};
    uint8_t red, green, blue;
    uint8_t coordx;

    if (!(x_forward | x_reverse)) {
        x_reverse = (EXAMPLE_LED_NUMBERS) >> 1; //2
        if ((EXAMPLE_LED_NUMBERS) % 2) {
            x_forward = x_reverse + 1;
        } else {
            x_forward = x_reverse; //2
        }
    }

    cnv_audio_process(audio, source_data, NULL, data_len, CNV_AUDIO_VOLUME);
    cnv_audio_get_volume(audio, &volume);
    level = volume * (EXAMPLE_LED_NUMBERS) / 100 / 2;  //0 1 2
    printf("dddd, level:%d\n", level);

    if (level > level_pre) {
        level_pre = level;
        for (int x = 0; x < level; x ++) {
            red = CNV_PATTERN_RED_BASE_VALUE - CNV_PATTERN_COLOR_SPAN * x;
            green = 0;
            blue = CNV_PATTERN_GREEN_BASE_VALUE + CNV_PATTERN_COLOR_SPAN * x;
            coordx = x_forward + x;
            leddata.pixels[coordx*3] = red;
            leddata.pixels[coordx*3 + 1] = green;
            leddata.pixels[coordx*3 + 2] = blue;

            coordx = x_forward - x;
            leddata.pixels[coordx*3] = red;
            leddata.pixels[coordx*3 + 1] = green;
            leddata.pixels[coordx*3 + 2] = blue;
        }
    } else {
        if (level_pre != 0) {
            for (int i = 0; i < 2; i ++) {
                coordx = x_forward + (level_pre);
                leddata.pixels[coordx*3] = 0;
                leddata.pixels[coordx*3 + 1] = 0;
                leddata.pixels[coordx*3 + 2] = 0;

                coordx = x_reverse - (level_pre);
                leddata.pixels[coordx*3] = 0;
                leddata.pixels[coordx*3 + 1] = 0;
                leddata.pixels[coordx*3 + 2] = 0;
            }
            level_pre--;
        }
    }

    led_strip_fill_data(&leddata);
}


void cnv_pattern_myenergy_mode(void *source_data, int data_len)
{
    static uint16_t level; //level_pre, x_forward, x_reverse;
    cnv_audio_t *audio = &g_cnv_audio;
    uint8_t volume = 0;
    led_renderer_data_t leddata = {0};
    uint8_t red, green, blue;

    cnv_audio_process(audio, source_data, NULL, data_len, CNV_AUDIO_VOLUME);
    cnv_audio_get_volume(audio, &volume);
    level = volume * (EXAMPLE_LED_NUMBERS) / 100;  //0 1 2 3 4

    for (int x = 0; x < level; x ++) {
        red = (CNV_PATTERN_RED_BASE_VALUE - CNV_PATTERN_COLOR_SPAN * x)>>3;
        green = (CNV_PATTERN_GREEN_BASE_VALUE + CNV_PATTERN_COLOR_SPAN * x)>>3;
        blue =  (250 - 40 * x)>>3;

        leddata.pixels[x*3] = red;
        leddata.pixels[x*3 + 1] = green;
        leddata.pixels[x*3 + 2] = blue;
    }

    led_strip_fill_data(&leddata);
}


static void audio_convert_task(void *args)
{
    audio_convert_handle_t *handle = (audio_convert_handle_t *)args;
    audio_sample_data_t sample;

    while (handle->task_run) {
        if (xQueueReceive(handle->in_queue, &sample, portMAX_DELAY) == pdTRUE) {
            cnv_pattern_myenergy_mode(sample.data, sample.len);
        }
    }
    xEventGroupSetBits(handle->state_event, AUDIO_CONVERT_STOPPED_BIT);
    vTaskDelete(NULL);
}

esp_err_t audio_convert_run()
{
    g_cnv_handler.task_run = true;
    xTaskCreate(audio_convert_task, "audio_task", 4096, &g_cnv_handler, 5, NULL);
    return ESP_OK;
}


esp_err_t audio_convert_wait_for_stop(audio_convert_handle_t *handle)
{
    EventBits_t uxBits = xEventGroupWaitBits(handle->state_event, AUDIO_CONVERT_STOPPED_BIT, false, true, 8000 / portTICK_RATE_MS);
    esp_err_t ret = ESP_ERR_TIMEOUT;
    if (uxBits & AUDIO_CONVERT_STOPPED_BIT) {
        ret = ESP_OK;
    }
    return ret;
}

esp_err_t audio_convert_stop()
{
    audio_convert_handle_t *handle = &g_cnv_handler;

    handle->task_run = false;
    esp_err_t ret = audio_convert_wait_for_stop(handle);

    return ret;
}

esp_err_t audio_convert_fill_data(audio_sample_data_t *data)
{
    audio_convert_handle_t *handle = &g_cnv_handler;

    if (xQueueSend(handle->in_queue, data, 1) != pdPASS) {
        ESP_LOGW(TAG, "Discard this message");
        return ESP_FAIL;
    }
    return ESP_OK;
}

/* 典型情况是传过来1024字节 */
void audio_convert_mp3_data_cb(void *data, int size)
{
    static int16_t buffer[256];
    char *in_data = data;
    int i;

    if (size > 1024) {
        size = 1024;
    }

    for (i = 0; i < size/4; i++) {
        uint8_t mid = in_data[i * 4 + 2];
        uint8_t msb = in_data[i * 4 + 3];
        int16_t raw = (((uint32_t)msb) << 8) + ((uint32_t)mid);
        buffer[i] = raw;
    }

    audio_sample_data_t sample_data;

    sample_data.data = buffer;
    sample_data.len = i;

    audio_convert_fill_data(&sample_data);
}

void audio_convert_init()
{
    cnv_audio_t *audio = &g_cnv_audio;
    audio->cur_rms = 0;
    audio->variable_rms_max = 64;
    audio->default_rms_max = 64;
    audio->default_rms_min = 12;
    audio->window_max_width_db = 24;
    audio->regress_threshold_vol = 30;
    audio->samplerate = 16000;
    audio->n_samples = 256;
#if 0
    audio->vol_calc_types = CNV_AUDIO_VOLUME_STATIC;
#else
    audio->vol_calc_types = CNV_AUDIO_VOLUME_DYNAMIC;
    audio->max_rms_fall_back_cycle = 3;
#endif
    cnv_audio_set_resolution_bits(audio, 12);


    audio_convert_handle_t *handle = &g_cnv_handler;
    handle->state_event = xEventGroupCreate();
    handle->in_queue = xQueueCreate(3, sizeof(audio_sample_data_t));
    if (NULL == handle->in_queue) {
        ESP_LOGE(TAG, "xQueueCreate return null");
    }

    sound_register_data_listen_cb(audio_convert_mp3_data_cb);

    audio_convert_run();
}

void audio_convert_deinit()
{
    audio_convert_handle_t *handle = &g_cnv_handler;

    if (handle->in_queue) {
        vQueueDelete(handle->in_queue);
    }

    if (handle->state_event) {
        vEventGroupDelete(handle->state_event);
    }
}

