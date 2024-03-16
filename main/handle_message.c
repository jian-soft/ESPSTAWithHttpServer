#include <string.h>
#include <stdio.h>
#include "esp_log.h"
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

int handle_message(char *json_str_in, char *json_str_out)
{
    int ret = -1;
    cJSON *root = NULL;

    root = cJSON_Parse(json_str_in);
    if (NULL == root) {
        ESP_LOGW(TAG, "parse json_str_in fail");
        goto out;
    }

    cJSON *type = cJSON_GetObjectItem(root, "type");
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

            ESP_LOGI(TAG, "M_RUN: m1d:%d, m1s:%d, m2d:%d, m2s:%d",
                d1->valueint, s1->valueint, d2->valueint, s2->valueint);
            gpio_enable_drv8833();
            drv8833_motorA_run(s1->valueint, d1->valueint);
            drv8833_motorB_run(s2->valueint, d2->valueint);
        }
        break;
        case M_STOP: {
            car_stop();
        }
        break;
        case M_CMDS: {
            cJSON *cmds = cJSON_GetObjectItem(root, "cmds");
            if (!cJSON_IsString(cmds))
                goto out;

            motor_run_cmds(root);
            root = NULL;
        }
        break;
        case S_PLAY: {
            cJSON *file = cJSON_GetObjectItem(root, "fileid");
            if (NULL == file)
                goto out;

            sound_play_mp3(file->valueint);
        }
        break;
        case S_NOTES: {
            cJSON *notes = cJSON_GetObjectItem(root, "notes");
            if (!cJSON_IsString(notes))
                goto out;

            sound_play_notes(root);
            root = NULL;
        }
        break;
        case L_CMDS: {
            cJSON *cmds = cJSON_GetObjectItem(root, "cmds");
            if (!cJSON_IsArray(cmds))
                goto out;
            led_run_cmds(root);
            root = NULL;
        }
        break;
    }
out:
    if (NULL != root) {
        cJSON_Delete(root);
    }

    return ret;
}


