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

static void audio_convert_task(void *args)
{
    audio_convert_handle_t *handle = (audio_convert_handle_t *)args;
    audio_sample_data_t sample;

    while (handle->task_run) {
        if (xQueueReceive(handle->in_queue, &sample, 10000 / portTICK_PERIOD_MS) == pdTRUE) {
            //convert data
            printf("dddd, convert task get data: %d\n", sample.len);
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

