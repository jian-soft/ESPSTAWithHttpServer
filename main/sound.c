/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include <stdint.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/i2s_std.h"
#include "driver/gpio.h"
#include "esp_check.h"
#include "io_assignment.h"
#include <math.h>
#include "my_play_mp3.h"

#define VOLUME_SCALE                1

static const char *TAG = "i2s";
static const char err_reason[][30] = {"input param is invalid",
                                      "operation timeout"
                                     };


static i2s_chan_handle_t                tx_chan;        // I2S tx channel handler
extern const uint8_t gun_pcm_start[] asm("_binary_gun_s8_16k_mono_pcm_start");
extern const uint8_t gun_pcm_end[]   asm("_binary_gun_s8_16k_mono_pcm_end");
extern const uint8_t didi_pcm_start[] asm("_binary_didi_s8_16k_mono_pcm_start");
extern const uint8_t didi_pcm_end[]   asm("_binary_didi_s8_16k_mono_pcm_end");

SemaphoreHandle_t g_i2s_mutex = NULL;

#define MY_SR 16000.0

float inv_sr;
float inv_freq;
float half_inv_freq;
static void square_init(float freq)
{
    inv_sr = 1.0/MY_SR;
    inv_freq = 1.0 / (float)freq;
    half_inv_freq = 0.5 / (float)freq;
}
static int16_t square_tick(int idx)
{
    float mod;
    mod = fmod((float)idx * inv_sr, inv_freq);

    if (mod >= half_inv_freq) {
        return 5000;
    } else {
        return -5000;
    }
}
#define BUFF_SIZE 512
int16_t buffer[BUFF_SIZE];
static void i2s_task_play_freq(void *args)
{
    float freq = *((float *)args);
    if (0 == freq) {
        vTaskDelete(NULL);
        return;
    }

    esp_err_t ret = ESP_OK;
    ret = xSemaphoreTake(g_i2s_mutex, 0);
    if (pdTRUE != ret) {
        vTaskDelete(NULL);
        return;
    }

    size_t bytes_write = 0, total_write = 0;
    ESP_LOGI(TAG, "i2s_task_play_freq: freq:%f", freq);
    square_init(freq);

    ESP_ERROR_CHECK(i2s_channel_enable(tx_chan));
    for (int j = 0; j < 16; j++) {
        for (int i = 0; i < BUFF_SIZE; i ++) {
            if (j==13 || j == 14 || j == 15) buffer[i] = 0;
            else buffer[i] = square_tick(i + j*BUFF_SIZE);

        }
        ret = i2s_channel_write(tx_chan, buffer, BUFF_SIZE*2, &bytes_write, portMAX_DELAY);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "[music] i2s write failed, %s", err_reason[ret == ESP_ERR_TIMEOUT]);
            abort();
        }
        if (bytes_write > 0) {
           //ESP_LOGI(TAG, "[music] i2s music played, %d bytes are written.", bytes_write);
           total_write += bytes_write;
        } else {
           ESP_LOGE(TAG, "[music] i2s music play falied.");
           abort();
        }
    }

    ESP_LOGI(TAG, "[music] i2s music played, %d bytes are written.", total_write);

    ESP_ERROR_CHECK(i2s_channel_disable(tx_chan));
    xSemaphoreGive(g_i2s_mutex);
    vTaskDelete(NULL);

}


static void i2s_task_play_mp3(void *args)
{
    static char raw_buffer[1024];
    int read_len;

    esp_err_t ret = ESP_OK;
    ret = xSemaphoreTake(g_i2s_mutex, 0);
    if (pdTRUE != ret) {
        vTaskDelete(NULL);
        return;
    }

    size_t bytes_write = 0, total_write = 0;

    ESP_ERROR_CHECK(i2s_channel_enable(tx_chan));

    while(1) {
        read_len = play_mp3_read_buffer(raw_buffer, sizeof(raw_buffer));
        if (read_len <= 0) {
            break;
        }

        ret = i2s_channel_write(tx_chan, raw_buffer, read_len, &bytes_write, portMAX_DELAY);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "[music] i2s write failed, %s", err_reason[ret == ESP_ERR_TIMEOUT]);
            abort();
        }
        if (bytes_write > 0) {
           //ESP_LOGI(TAG, "[music] i2s music played, %d bytes are written.", bytes_write);
           total_write += bytes_write;
        } else {
           ESP_LOGE(TAG, "[music] i2s music play falied.");
           abort();
        }
    }

    ESP_LOGI(TAG, "[music] i2s music played, %d bytes are written.", total_write);

    ESP_ERROR_CHECK(i2s_channel_disable(tx_chan));
    xSemaphoreGive(g_i2s_mutex);
    vTaskDelete(NULL);

}


static void i2s_write_task_16(void *args)
{
    esp_err_t ret = ESP_OK;

    ret = xSemaphoreTake(g_i2s_mutex, 0);
    if (pdTRUE != ret) {
        vTaskDelete(NULL);
        return;
    }

    size_t bytes_write = 0;

    const int8_t *music_start = (int8_t *)args;
    size_t size;
    if (args == (void *)didi_pcm_start) {
        size = didi_pcm_end - didi_pcm_start;
    } else {
        size = gun_pcm_end - gun_pcm_start;
    }

    ESP_LOGI(TAG, "the music size: %d", size);

    //size_t i;
    //int16_t le_data[4];

    ESP_ERROR_CHECK(i2s_channel_enable(tx_chan));
    ret = i2s_channel_write(tx_chan, music_start, size, &bytes_write, portMAX_DELAY);
    if (ret != ESP_OK) {
       ESP_LOGE(TAG, "[music] i2s write failed, %s", err_reason[ret == ESP_ERR_TIMEOUT]);
       abort();
    }
    if (bytes_write > 0) {
       ESP_LOGI(TAG, "[music] i2s music played, %d bytes are written.", bytes_write);
    } else {
       ESP_LOGE(TAG, "[music] i2s music play failed.");
       abort();
    }

/*
    for (i = 0; i < size; i += 4) {
        le_data[0] = (music_start[i] << 8)/VOLUME_SCALE;
        le_data[1] = (music_start[i + 1] << 8)/VOLUME_SCALE;
        le_data[2] = (music_start[i + 2] << 8)/VOLUME_SCALE;
        le_data[3] = (music_start[i + 3] << 8)/VOLUME_SCALE;

        ret = i2s_channel_write(tx_chan, le_data, 8, &bytes_write, portMAX_DELAY);
        if (ret != ESP_OK) {
           ESP_LOGE(TAG, "[music] i2s write failed, %s", err_reason[ret == ESP_ERR_TIMEOUT]);
           abort();
        }
        if (bytes_write > 0) {
           //ESP_LOGI(TAG, "[music] i2s music played, %d bytes are written.", bytes_write);
           total_write += bytes_write;
        } else {
           ESP_LOGE(TAG, "[music] i2s music play falied.");
           abort();
        }
    }

    ESP_LOGI(TAG, "[music] i2s music played, %d bytes are written.", total_write);
*/
    ESP_ERROR_CHECK(i2s_channel_disable(tx_chan));
    xSemaphoreGive(g_i2s_mutex);
    vTaskDelete(NULL);
}


static void i2s_example_init_std_simplex(void)
{
    /* Setp 1: Determine the I2S channel configuration and allocate two channels one by one
     * The default configuration can be generated by the helper macro,
     * it only requires the I2S controller id and I2S role
     * The tx and rx channels here are registered on different I2S controller,
     * only ESP32-C3, ESP32-S3 and ESP32-H2 allow to register two separate tx & rx channels on a same controller */
    i2s_chan_config_t tx_chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
    ESP_ERROR_CHECK(i2s_new_channel(&tx_chan_cfg, &tx_chan, NULL));

    /* Step 2: Setting the configurations of standard mode and initialize each channels one by one
     * The slot configuration and clock configuration can be generated by the macros
     * These two helper macros is defined in 'i2s_std.h' which can only be used in STD mode.
     * They can help to specify the slot and clock configurations for initialization or re-configuring */
    i2s_std_config_t tx_std_cfg = {
        .clk_cfg  = I2S_STD_CLK_DEFAULT_CONFIG(16000),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,    // some codecs may require mclk signal, this example doesn't need it
            .bclk = STD_BCLK_IO,
            .ws   = STD_WS_IO,
            .dout = STD_DOUT_IO,
            .din  = I2S_GPIO_UNUSED,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv   = false,
            },
        },
    };
    ESP_ERROR_CHECK(i2s_channel_init_std_mode(tx_chan, &tx_std_cfg));
}

void sound_init()
{
    i2s_example_init_std_simplex();

    g_i2s_mutex = xSemaphoreCreateMutex();
    if (NULL == g_i2s_mutex) {
        ESP_LOGE(TAG, "create g_i2s_mutex fail");
        abort();
    }
}

void sound_play_didi()
{
    xTaskCreate(i2s_write_task_16, "play_didi_task", 4096, (void *)didi_pcm_start, 5, NULL);
}

void sound_play_gun()
{
    xTaskCreate(i2s_write_task_16, "play_gun_task", 4096, (void *)gun_pcm_start, 5, NULL);
}

void sound_play_freq(float freq)
{
    static float t;
    t = freq;
    xTaskCreate(i2s_task_play_freq, "play_freq_task", 4096, (void *)(&t), 5, NULL);
}


void sound_play_mp3()
{
    xTaskCreate(i2s_task_play_mp3, "play_mp3_task", 4096, NULL, 5, NULL);
}

