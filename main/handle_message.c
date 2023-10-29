#include <string.h>
#include "esp_log.h"
#include "cJSON.h"
#include "my_gpio.h"
#include "sound.h"
#include "drv8833_pwm.h"


static const char *TAG = "handle";


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
    if (NULL == type) {
        goto out;
    }
    char *type_string = cJSON_GetStringValue(type);
    if (0 == strcmp(type_string, "M_RUN")) {
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
    else if (0 == strcmp(type_string, "M_STOP")) {
        car_stop();
    }
    else if (0 == strcmp(type_string, "M_RUN_D")) {
        cJSON *d1 = cJSON_GetObjectItem(root, "m1d");
        cJSON *d2 = cJSON_GetObjectItem(root, "m2d");
        if (NULL == d1 || NULL == d2) {
            goto out;
        }
        ESP_LOGI(TAG, "M_RUN: m1distance:%d, m2distance:%d TBD", d1->valueint, d2->valueint);
    }
    else if (0 == strcmp(type_string, "S_PLAY")) {
        cJSON *file = cJSON_GetObjectItem(root, "fileid");
        if (NULL == file)
            goto out;

        sound_play_mp3(file->valueint);
    }
    else if (0 == strcmp(type_string, "S_FREQ")) {
        cJSON *value = cJSON_GetObjectItem(root, "value");
        if (NULL == value)
            goto out;
        float freq = (float)value->valuedouble;
        ESP_LOGI(TAG, "S_FREQ: freq:%f", freq);
        sound_play_freq(freq);
    }

out:
    if (NULL != root) {
        cJSON_Delete(root);
    }

    return ret;
}


