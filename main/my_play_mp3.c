/* Play mp3 file by audio pipeline
   with possibility to start, stop, pause and resume playback
   as well as adjust volume

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "audio_element.h"
#include "audio_pipeline.h"
#include "audio_event_iface.h"
#include "audio_mem.h"
#include "audio_common.h"
#include "raw_stream.h"
#include "mp3_decoder.h"

static const char *TAG = "PLAY_MP3";
static audio_pipeline_handle_t g_pipeline;
static audio_element_handle_t g_mp3_decoder;
static audio_element_handle_t g_raw_writer;

static struct marker {
    int pos;
    const uint8_t *start;
    const uint8_t *end;
} file_marker;


extern uint8_t didi_start[] asm("_binary_didi_1c_16khz_mp3_start");
extern uint8_t didi_end[]   asm("_binary_didi_1c_16khz_mp3_end");
extern uint8_t gun_start[] asm("_binary_gun_1c_16khz_mp3_start");
extern uint8_t gun_end[]   asm("_binary_gun_1c_16khz_mp3_end");
extern uint8_t canon_start[] asm("_binary_canon_1c_16khz_21s_mp3_start");
extern uint8_t canon_end[]   asm("_binary_canon_1c_16khz_21s_mp3_end");
extern uint8_t croatian_start[] asm("_binary_Croatian_1c_16khz_12s_mp3_start");
extern uint8_t croatian_end[]   asm("_binary_Croatian_1c_16khz_12s_mp3_end");
extern uint8_t galway_start[] asm("_binary_Galway_1c_16khz_18s_mp3_start");
extern uint8_t galway_end[]   asm("_binary_Galway_1c_16khz_18s_mp3_end");
extern uint8_t vivacity_start[] asm("_binary_Vivacity_1c_16khz_20s_mp3_start");
extern uint8_t vivacity_end[]   asm("_binary_Vivacity_1c_16khz_20s_mp3_end");
extern uint8_t aiyaya_start[] asm("_binary_aiyaya_1c_16khz_23s_mp3_start");
extern uint8_t aiyaya_end[]   asm("_binary_aiyaya_1c_16khz_23s_mp3_end");



static void set_next_file_marker(int fileid)
{
    static uint8_t *mp3_start[] = {
            didi_start,
            gun_start,
            canon_start,
            croatian_start,
            galway_start,
            vivacity_start,
            aiyaya_start
        };
    static uint8_t *mp3_end[] = {
            didi_end,
            gun_end,
            canon_end,
            croatian_end,
            galway_end,
            vivacity_end,
            aiyaya_end
        };


    if (fileid < 0 || fileid >= 7) {
        fileid = 0;
    }

    file_marker.start = mp3_start[fileid];
    file_marker.end   = mp3_end[fileid];

    file_marker.pos = 0;
}

int mp3_music_read_cb(audio_element_handle_t el, char *buf, int len, TickType_t wait_time, void *ctx)
{
    int read_size = file_marker.end - file_marker.start - file_marker.pos;
    if (read_size == 0) {
        return AEL_IO_DONE;
    } else if (len < read_size) {
        read_size = len;
    }
    memcpy(buf, file_marker.start + file_marker.pos, read_size);
    file_marker.pos += read_size;
    return read_size;
}

void play_mp3_init(void)
{
    ESP_LOGI(TAG, "[ 2 ] Create audio pipeline, add all elements to pipeline, and subscribe pipeline event");
    audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    g_pipeline = audio_pipeline_init(&pipeline_cfg);
    mem_assert(g_pipeline);

    ESP_LOGI(TAG, "[2.1] Create mp3 decoder to decode mp3 file and set custom read callback");
    mp3_decoder_cfg_t mp3_cfg = DEFAULT_MP3_DECODER_CONFIG();
    g_mp3_decoder = mp3_decoder_init(&mp3_cfg);
    audio_element_set_read_cb(g_mp3_decoder, mp3_music_read_cb, NULL);

    ESP_LOGI(TAG, "[2.2] Create raw stream");
    raw_stream_cfg_t raw_cfg = RAW_STREAM_CFG_DEFAULT();
    raw_cfg.type = AUDIO_STREAM_READER;
    g_raw_writer = raw_stream_init(&raw_cfg);

    ESP_LOGI(TAG, "[2.3] Register all elements to audio pipeline");
    audio_pipeline_register(g_pipeline, g_mp3_decoder, "mp3");
    audio_pipeline_register(g_pipeline, g_raw_writer, "raw");

    ESP_LOGI(TAG, "[2.4] Link it together [mp3_music_read_cb]-->mp3_decoder-->i2s_stream-->[codec_chip]");
    const char *link_tag[2] = {"mp3", "raw"};
    audio_pipeline_link(g_pipeline, &link_tag[0], 2);
}

void play_mp3_deinit(void)
{
    ESP_LOGI(TAG, "[ 6 ] Stop audio_pipeline");
    audio_pipeline_stop(g_pipeline);
    audio_pipeline_wait_for_stop(g_pipeline);
    audio_pipeline_terminate(g_pipeline);
    audio_pipeline_unregister(g_pipeline, g_mp3_decoder);
    audio_pipeline_unregister(g_pipeline, g_raw_writer);

    /* Release all resources */
    audio_pipeline_deinit(g_pipeline);
    audio_element_deinit(g_mp3_decoder);
}


void play_mp3_start_pipeline(int fileid)
{
    audio_pipeline_stop(g_pipeline);
    audio_pipeline_wait_for_stop(g_pipeline);
    audio_pipeline_terminate(g_pipeline);
    audio_pipeline_reset_ringbuffer(g_pipeline);
    audio_pipeline_reset_elements(g_pipeline);
    audio_pipeline_change_state(g_pipeline, AEL_STATE_INIT);
    set_next_file_marker(fileid);
    audio_pipeline_run(g_pipeline);
}

#if 0
void play_mp3_init_and_run(void)
{
    audio_pipeline_handle_t pipeline;
    audio_element_handle_t mp3_decoder;

    int player_volume = 100;

    ESP_LOGI(TAG, "[ 2 ] Create audio pipeline, add all elements to pipeline, and subscribe pipeline event");
    audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    pipeline = audio_pipeline_init(&pipeline_cfg);
    mem_assert(pipeline);

    ESP_LOGI(TAG, "[2.1] Create mp3 decoder to decode mp3 file and set custom read callback");
    mp3_decoder_cfg_t mp3_cfg = DEFAULT_MP3_DECODER_CONFIG();
    mp3_decoder = mp3_decoder_init(&mp3_cfg);
    audio_element_set_read_cb(mp3_decoder, mp3_music_read_cb, NULL);

    ESP_LOGI(TAG, "[2.2] Create raw stream");
    raw_stream_cfg_t raw_cfg = RAW_STREAM_CFG_DEFAULT();
    raw_cfg.type = AUDIO_STREAM_READER;
    g_raw_writer = raw_stream_init(&raw_cfg);

    ESP_LOGI(TAG, "[2.3] Register all elements to audio pipeline");
    audio_pipeline_register(pipeline, mp3_decoder, "mp3");
    audio_pipeline_register(pipeline, g_raw_writer, "raw");

    ESP_LOGI(TAG, "[2.4] Link it together [mp3_music_read_cb]-->mp3_decoder-->i2s_stream-->[codec_chip]");
    const char *link_tag[2] = {"mp3", "raw"};
    audio_pipeline_link(pipeline, &link_tag[0], 2);

    ESP_LOGI(TAG, "[ 4 ] Set up  event listener");
    audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
    audio_event_iface_handle_t evt = audio_event_iface_init(&evt_cfg);

    ESP_LOGI(TAG, "[4.1] Listening event from all elements of pipeline");
    audio_pipeline_set_listener(pipeline, evt);

    ESP_LOGI(TAG, "[ 5.1 ] Start audio_pipeline");
    set_next_file_marker();
    audio_pipeline_run(pipeline);

    while (1) {
        audio_event_iface_msg_t msg;
        audio_element_handle_t event_el;
        esp_err_t ret = audio_event_iface_listen(evt, &msg, portMAX_DELAY);
        if (ret != ESP_OK) {
            continue;
        }

        if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT) {
            if (msg.source == (void *) mp3_decoder && msg.cmd == AEL_MSG_CMD_REPORT_MUSIC_INFO) {
                audio_element_info_t music_info = {0};
                audio_element_getinfo(mp3_decoder, &music_info);
                ESP_LOGI(TAG, "[ * ] Receive music info from mp3 decoder, sample_rates=%d, bits=%d, ch=%d",
                         music_info.sample_rates, music_info.bits, music_info.channels);

                sound_play_mp3();
                continue;
            } else if (AEL_MSG_CMD_REPORT_STATUS == msg.cmd) {
                event_el = (struct audio_element *)msg.source;
                ESP_LOGI(TAG, "%s REPORT_STATUS: %d", audio_element_get_tag(event_el), (int)msg.data);
            }
        }
    }

    ESP_LOGI(TAG, "[ 6 ] Stop audio_pipeline");
    audio_pipeline_stop(pipeline);
    audio_pipeline_wait_for_stop(pipeline);
    audio_pipeline_terminate(pipeline);
    audio_pipeline_unregister(pipeline, mp3_decoder);

    /* Terminate the pipeline before removing the listener */
    audio_pipeline_remove_listener(pipeline);

    /* Make sure audio_pipeline_remove_listener is called before destroying event_iface */
    audio_event_iface_destroy(evt);

    /* Release all resources */
    audio_pipeline_deinit(pipeline);
    audio_element_deinit(mp3_decoder);
}
#endif

int play_mp3_read_buffer(char *buff, int buff_size)
{
    return raw_stream_read(g_raw_writer, buff, buff_size);
}


