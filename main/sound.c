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
#include "freertos/event_groups.h"
#include "driver/i2s_std.h"
#include "driver/gpio.h"
#include "esp_check.h"
#include "io_assignment.h"
#include <math.h>
#include "utils.h"
#include "my_play_mp3.h"
#include "sound.h"

static const char *TAG = "i2s";
static const char err_reason[][30] = {"input param is invalid",
                                      "operation timeout"
                                     };

static data_listen_cb g_data_listen_cb;
static i2s_chan_handle_t                tx_chan;        // I2S tx channel handler


SemaphoreHandle_t g_i2s_mutex = NULL;

#define MY_SR 16000.0

// Linear interpolation
float lerp(float x, float x1, float x2, float y1, float y2) {
    return y1 + (x-x1) * (y2-y1) / (x2-x1) ;
}


/*
    @sample: 原始值
    @t: 相对于节拍开始时间计算出的偏移时间
    @duration: 节拍时长，单拍时长*拍数，单节拍时长要大于0.3s
    @return: adsr之后的值
*/
float adsr(float sample, float t, float duration)
{
    float attackTime = 0.01;
    float decayTime = 0.1;
    float sustainGain = 0.8;
    float releaseTime = 0.1;

    if (t < attackTime) {
        sample *= lerp(t, 0, attackTime, 0, 1);
    }
    else if (t < attackTime + decayTime) {
        sample *= lerp(t, attackTime, attackTime + decayTime, 1, sustainGain);
    }
    else if (t < duration - releaseTime) {
        sample *= sustainGain;
    }
    else {
        sample *= lerp(t, duration - releaseTime, duration, sustainGain, 0);
    }
    return sample;
}


#define BUFF_SIZE 800
int16_t g_i2s_data_buffer[BUFF_SIZE];

/**************play notes task begin*****************/
float g_sample_rate;
float g_freq;
float g_inv_freq;
int g_tick_idx;
float g_duration;

static void saw_reset(float freq, float beat_cnt)
{
    g_sample_rate = MY_SR;
    g_freq = freq;
    g_inv_freq = 1.0 / freq;
    g_tick_idx = 0;
    g_duration = beat_cnt * BEAT_DURATION;
}
/*
    @tick_out: the output value
    @return: 1-return normal, 0-duration end
*/
static inline int saw_tick(int16_t *tick_out)
{
    float t = (float)g_tick_idx/g_sample_rate;
    if (t >= g_duration) {
        return 0;
    }

    float mod = fmod(t + g_inv_freq/2.0, g_inv_freq);
    float out = 2.0 * g_freq * mod - 1.0;
    out = adsr(out, t, g_duration);
    *tick_out = (int16_t)(5000 * out);

    g_tick_idx++;
    return 1;
}

const float g_note_normal[] = {261.626, 293.665, 329.628, 349.228, 391.995, 440.0, 493.883};
const float g_note_high[] = {523.251, 587.33, 659.255, 698.456, 783.991, 880.0, 987.767};
const float g_note_low[] = {130.813, 146.832, 164.814, 174.614, 195.998, 220.0, 246.942};

static float note_to_freq(char *note)
{
    int idx;
    float const *notes_table;

    if (note[0] == '.') {
        idx = note[1] - '1';
        notes_table = g_note_low;
    } else {
        idx = note[0] - '1';
        if (note[1] == '.') {
            notes_table = g_note_high;
        } else {
            notes_table = g_note_normal;
        }
    }

    if (idx >= 0 && idx < 7) {
        return notes_table[idx];
    }

    return 0.0;
}


typedef struct {
    volatile bool                    task_run;
    EventGroupHandle_t               state_event;
    void                            *args;
} play_notes_handle_t;

static play_notes_handle_t g_play_notes_handler;

static void play_notes_task(void *args)
{
    play_notes_handle_t *handle = (play_notes_handle_t *)args;
    esp_err_t ret = ESP_OK;
    ret = xSemaphoreTake(g_i2s_mutex, 0);
    if (pdTRUE != ret) {
        vTaskDelete(NULL);
        return;
    }

    size_t bytes_write = 0, total_write = 0;
    int bf_size, tick_ret;
    char *notes = handle->args;
    char label[4];
    int beatCnt;
    int next_cmd_pos = 0;
    float freq;

    while(*notes) {
        if (!handle->task_run) {
            break;
        }
        next_cmd_pos = parse_cmd(notes, label, &beatCnt);
        if (next_cmd_pos < 0) {
            break;
        }
        notes += next_cmd_pos + 1;

        if (label[0] == 'N' && label[1] == 'O') {
            vTaskDelay(pdMS_TO_TICKS(beatCnt * BEAT_DURATION * 1000));
            continue;
        }
        freq = note_to_freq(label);
        if (0.0 == freq) {
            vTaskDelay(pdMS_TO_TICKS(beatCnt * BEAT_DURATION * 1000));
            continue;
        }

        tick_ret = 1;
        saw_reset(freq, beatCnt);
        ESP_ERROR_CHECK(i2s_channel_enable(tx_chan));
        while (tick_ret) {
            for (bf_size = 0; bf_size < BUFF_SIZE; bf_size++) {
                if (!handle->task_run) {
                    break;
                }
                tick_ret = saw_tick(g_i2s_data_buffer + bf_size);
                if (0 == tick_ret) {
                    break;
                }
            }
            if (0 == bf_size) {
                break;
            }
            if (!handle->task_run) {
                break;
            }
            ret = i2s_channel_write(tx_chan, g_i2s_data_buffer, bf_size*2, &bytes_write, portMAX_DELAY);
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "[music] i2s write failed, %s", err_reason[ret == ESP_ERR_TIMEOUT]);
                abort();
            }
            if (bytes_write > 0) {
               total_write += bytes_write;
            } else {
               ESP_LOGE(TAG, "[music] i2s music play failed.");
               abort();
            }
        }
        ESP_ERROR_CHECK(i2s_channel_disable(tx_chan));


    }


    ESP_LOGI(TAG, "[music] i2s music played, %d bytes are written.", total_write);

    xSemaphoreGive(g_i2s_mutex);

    handle->task_run = false;
    xEventGroupSetBits(handle->state_event, BIT0);
    vTaskDelete(NULL);
}
static esp_err_t play_notes_wait_for_stop(void)
{
    EventBits_t uxBits = xEventGroupWaitBits(g_play_notes_handler.state_event, BIT0, true, true, 5000 / portTICK_RATE_MS);
    esp_err_t ret = ESP_ERR_TIMEOUT;
    if (uxBits & BIT0) {
        ret = ESP_OK;
    } else {
        ESP_LOGE(TAG, "wait for stop timeout.");
    }
    return ret;
}
int play_notes_stop(void)
{
    play_notes_handle_t *handle = &g_play_notes_handler;
    if (false == handle->task_run) {
        return ESP_OK;
    }

    xEventGroupClearBits(handle->state_event, BIT0);
    handle->task_run = false;
    esp_err_t ret = play_notes_wait_for_stop();

    return ret;
}
void play_notes_run(char *str)
{
    play_notes_handle_t *handle = &g_play_notes_handler;
    if (true == handle->task_run) {
        play_notes_stop();
    }

    handle->task_run = true;
    handle->args = str;
    xTaskCreate(play_notes_task, "play_notes_task", 4096, &g_play_notes_handler, 5, NULL);
}
void play_notes_init()
{
    g_play_notes_handler.state_event = xEventGroupCreate();
}
/**************play notes task end********************/
/**************play mp3 task start********************/
typedef struct {
    /* Task related */
    volatile bool                    task_run;             /*!< Component running status */
    EventGroupHandle_t               state_event;          /*!< Task's state event group */
    int                              fileid;
} sound_play_mp3_handle_t;

static sound_play_mp3_handle_t g_playmp3_handle;

int is_sound_play_run()
{
    return g_playmp3_handle.task_run;
}

static void sound_play_mp3_task(void *args)
{
#define READ_LEN 1024
    char *raw_buffer = (char *)g_i2s_data_buffer;
    sound_play_mp3_handle_t *handle = (sound_play_mp3_handle_t *)args;

    esp_err_t ret = ESP_OK;
    ret = xSemaphoreTake(g_i2s_mutex, 0);
    if (pdTRUE != ret) {
        handle->task_run = false;
        goto out;
    }

    play_mp3_start_pipeline(handle->fileid);
    ESP_ERROR_CHECK(i2s_channel_enable(tx_chan));

    int read_len;
    size_t bytes_write = 0, total_write = 0;
    while(handle->task_run) {
        read_len = play_mp3_read_buffer(raw_buffer, READ_LEN);
        if (read_len <= 0) {
            break;
        }

        if (g_data_listen_cb) {
            g_data_listen_cb(raw_buffer, read_len);
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
    handle->task_run = false;
    ESP_LOGI(TAG, "[music] i2s music played, %d bytes are written.", total_write);

    ESP_ERROR_CHECK(i2s_channel_disable(tx_chan));
    xSemaphoreGive(g_i2s_mutex);

out:
    xEventGroupSetBits(handle->state_event, BIT0);
    vTaskDelete(NULL);
}

esp_err_t sound_play_mp3_wait_for_stop(sound_play_mp3_handle_t *handle)
{
    EventBits_t uxBits = xEventGroupWaitBits(handle->state_event, BIT0, true, true, 500 / portTICK_RATE_MS);
    esp_err_t ret = ESP_ERR_TIMEOUT;
    if (uxBits & BIT0) {
        ret = ESP_OK;
    } else {
        ESP_LOGE(TAG, "wait for play-mp3 stop timeout.");
    }
    return ret;
}

esp_err_t sound_play_mp3_stop(void)
{
    sound_play_mp3_handle_t *handle = &g_playmp3_handle;
    if (false == handle->task_run) {
        return ESP_OK;
    }

    xEventGroupClearBits(handle->state_event, BIT0);
    handle->task_run = false;
    esp_err_t ret = sound_play_mp3_wait_for_stop(handle);

    return ret;
}

void sound_play_mp3_run(int fileid)
{
    sound_play_mp3_handle_t *handle = &g_playmp3_handle;
    if (true == handle->task_run) {
        sound_play_mp3_stop();
    }

    handle->task_run = true;
    handle->fileid = fileid;
    xTaskCreate(sound_play_mp3_task, "play_mp3_task", 4096, handle, 5, NULL);
}

void sound_play_mp3_init()
{
    g_playmp3_handle.state_event = xEventGroupCreate();
}
/**************play mp3 task end********************/


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

    sound_play_mp3_init();
    play_notes_init();
}



void sound_register_data_listen_cb(data_listen_cb cb)
{
    g_data_listen_cb = cb;
}

