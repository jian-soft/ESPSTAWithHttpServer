#include <string.h>
#include <stdio.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "cJSON.h"
#include "my_gpio.h"
#include "sound.h"
#include "drv8833_pwm.h"
#include "led_strip.h"
#include "handle_message.h"

static const char *TAG = "handle";

const char* event_name[] = {
    "M_RUN",
    "M_STOP",
    "M_RUN_D",
    "M_CMDS",
    "S_PLAY",
    "S_FREQ",
    "S_NOTES",
    "L_CMDS",

    "error"
};

//----
char run_cmds_is_run;
static cJSON *run_cmds_root;
static cJSON *r_v1, *r_v2, *r_v3, *r_v4;
//----

static void run_cmds_task(void *pvParameters)
{
    if (!run_cmds_is_run) {
        return;
    }
    printf("run_cmds_create_task in, isLoop:%d\n", r_v1->valueint);
    if (r_v1->valueint) {
        play_notes_run(r_v2->valuestring);
        motor_run_cmds(r_v3->valuestring);
        led_run_cmds(r_v4->valuestring);
    } else {
        //udp_send_msg("run,0", 5);
        //TBD
        run_cmds_is_run = 0;
    }

    vTaskDelete(NULL);
}

static void run_cmds_create_task(void)
{
    xTaskCreate(run_cmds_task, "run_cmds_task", 4096, NULL, 5, NULL);
}


void run_cmds_one_time_cb()
{
    if (!run_cmds_is_run) {
        return;
    }
    run_cmds_create_task();
}

void run_cmds(cJSON *root)
{
    if (NULL != run_cmds_root) {
        cJSON_Delete(run_cmds_root);
    }
    run_cmds_root = root;

    r_v1 = cJSON_GetObjectItem(root, "V1");
    r_v2 = cJSON_GetObjectItem(root, "V2");
    r_v3 = cJSON_GetObjectItem(root, "V3");
    r_v4 = cJSON_GetObjectItem(root, "V4");
    if (r_v1 == NULL || r_v2 == NULL || r_v3 == NULL || r_v4 == NULL) {
        return;
    }

    run_cmds_is_run = 1;
    play_notes_run(r_v2->valuestring);
    motor_run_cmds(r_v3->valuestring);
    led_run_cmds(r_v4->valuestring);
}

int handle_message(char *json_str_in)
{
    int ret = -1;
    cJSON *root, *type;

    root = cJSON_Parse(json_str_in);
    if (NULL == root) {
        ESP_LOGW(TAG, "parse json_str_in fail");
        goto out;
    }

    type = cJSON_GetObjectItem(root, "T");
    if (!cJSON_IsNumber(type)) {
        goto out;
    }

    switch (type->valueint) {
        case M_RUN: {
            cJSON *d1 = cJSON_GetObjectItem(root, "m1d");
            cJSON *d2 = cJSON_GetObjectItem(root, "m2d");
            cJSON *s1 = cJSON_GetObjectItem(root, "m1s");
            cJSON *s2 = cJSON_GetObjectItem(root, "m2s");

            if (NULL == d1 || NULL == d2 || NULL == s1 || NULL == s2) {
                goto out;
            }

            gpio_enable_drv8833();
            drv8833_motorA_run(s1->valueint, d1->valueint);
            drv8833_motorB_run(s2->valueint, d2->valueint);

            //扩充一个键值属性
            cJSON *key = cJSON_GetObjectItem(root, "k");
            if (NULL == key) {
                goto out;
            }
            static int last_key;
            if (last_key == key->valueint) {
                goto out;
            }
            last_key = key->valueint;

            if (1 == key->valueint) {
                sound_play_mp3_run(0);  //play didi
            } else if (2 == key->valueint) {
                sound_play_mp3_run(1);  //play gun
            } else if (3 == key->valueint) {
                sound_play_mp3_run(0xF);  //play next music
            } else if (4 == key->valueint) {
                //do nothing
            }
        }
        break;
        case M_STOP: {
            car_stop();
        }
        break;
        case S_PLAY: {
            cJSON *file = cJSON_GetObjectItem(root, "fileid");
            if (NULL == file)
                goto out;

            sound_play_mp3_run(file->valueint);
        }
        break;
        case C_CMDS: {
            if (!run_cmds_is_run) {
                run_cmds(root);
                root = NULL;
            }
        }
        break;
        case C_STOP: {
            run_cmds_is_run = 0;
            motor_run_cmds_stop();
            led_run_cmds_stop();
            play_notes_stop();
            printf("dddd, send run 0 msg\n");
            //udp_send_msg("run,0", 5);
            //TBD
        }
        break;
    }
out:
    if (NULL != root) {
        cJSON_Delete(root);
    }

    return ret;
}


