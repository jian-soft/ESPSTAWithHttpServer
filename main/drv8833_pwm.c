#include <string.h>
#include <stdio.h>
#include <esp_log.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "cJSON.h"
#include "utils.h"
#include "my_gpio.h"
#include "io_assignment.h"

static const char *TAG = "pwm";

typedef struct {
    /* Task related */
    volatile bool                    task_run;             /*!< Component running status */
    EventGroupHandle_t               state_event;          /*!< Task's state event group */
    void                            *args;
} motor_cmds_handle_t;
static motor_cmds_handle_t g_motor_cmds_handler;

void pwm_init(void)
{
    //第一步 设置定时器
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_LOW_SPEED_MODE,
        .timer_num        = LEDC_TIMER_0,
        .duty_resolution  = LEDC_TIMER_10_BIT,
        .freq_hz          = 100,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    //第二步 设置每个通道对应的GPIO
#define PWM_CH_NUM 4
    ledc_channel_config_t ledc_channel[PWM_CH_NUM] = {
        {
            .channel    = LEDC_CHANNEL_0,
            .duty       = 0,
            .gpio_num   = DRV8833_AIN1_IO,
            .speed_mode = LEDC_LOW_SPEED_MODE,
            .hpoint     = 0,
            .timer_sel  = LEDC_TIMER_0,
            .intr_type  = LEDC_INTR_DISABLE,
        },
        {
            .channel    = LEDC_CHANNEL_1,
            .duty       = 0,
            .gpio_num   = DRV8833_AIN2_IO,
            .speed_mode = LEDC_LOW_SPEED_MODE,
            .hpoint     = 0,
            .timer_sel  = LEDC_TIMER_0,
            .intr_type  = LEDC_INTR_DISABLE,
        },
        {
            .channel    = LEDC_CHANNEL_2,
            .duty       = 0,
            .gpio_num   = DRV8833_BIN1_IO,
            .speed_mode = LEDC_LOW_SPEED_MODE,
            .hpoint     = 0,
            .timer_sel  = LEDC_TIMER_0,
            .intr_type  = LEDC_INTR_DISABLE,
        },
        {
            .channel    = LEDC_CHANNEL_3,
            .duty       = 0,
            .gpio_num   = DRV8833_BIN2_IO,
            .speed_mode = LEDC_LOW_SPEED_MODE,
            .hpoint     = 0,
            .timer_sel  = LEDC_TIMER_0,
            .intr_type  = LEDC_INTR_DISABLE,
        },
    };
    int ch;
    for (ch = 0; ch < PWM_CH_NUM; ch++) {
        ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel[ch]));
    }

    g_motor_cmds_handler.state_event = xEventGroupCreate();
}


/* @speed: 0~100 @direction: >=0-foward -1-back */
void drv8833_motorA_run(int speed, int direction)
{
    //if (speed < 25) speed = 25;
    int duty = speed*1024/100;
    if (direction >= 0) {
        ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty));
        ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, 0));
    } else {
        ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0));
        ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, duty));

    }

    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1));
}


void drv8833_motorA_stop(void)
{
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 1024));
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, 1024));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1));
}

void drv8833_motorB_run(int speed, int direction)
{
    int duty = speed*1024/100;
    if (direction >= 0) {
        ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2, duty));
        ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_3, 0));
    } else {
        ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2, 0));
        ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_3, duty));

    }

    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_3));
}

void drv8833_motorB_stop(void)
{
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2, 1024));
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_3, 1024));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_3));
}

void car_forward(int speed1, int speed2)
{
    gpio_enable_drv8833();
    //run_distance(40);
    drv8833_motorA_run(speed1, 1);
    drv8833_motorB_run(speed2, 1);
}

void car_back(int speed1, int speed2)
{
    gpio_enable_drv8833();
    //run_distance(20);
    drv8833_motorA_run(speed1, -1);
    drv8833_motorB_run(speed2, -1);
}


void car_stop(void)
{
    drv8833_motorA_stop();
    drv8833_motorB_stop();
    gpio_disable_drv8833();
}


esp_err_t motor_run_one_cmd(char *cmd)
{
    if (0 == strcmp(cmd, "F")) {
        car_forward(100, 100);
    } else if (0 == strcmp(cmd, "B")) {
        car_back(100, 100);
    } else if (0 == strcmp(cmd, "L")) {
        gpio_enable_drv8833();
        drv8833_motorA_run(75, -1);
        drv8833_motorB_run(75, 1);
    } else if (0 == strcmp(cmd, "R")) {
        gpio_enable_drv8833();
        drv8833_motorA_run(75, 1);
        drv8833_motorB_run(75, -1);
    } else if (0 == strcmp(cmd, "FL")) {
        car_forward(50, 100);
    } else if (0 == strcmp(cmd, "FR")) {
        car_forward(100, 50);
    } else if (0 == strcmp(cmd, "BL")) {
        car_back(50, 100);
    } else if (0 == strcmp(cmd, "BR")) {
        car_back(100, 50);
    } else {
        ESP_LOGW(TAG, "unknow motor cmd:%s, run forward", cmd);
        car_forward(100, 100);
    }
    return ESP_OK;
}

/*-------------------------------------------------------------*/

static void motor_run_cmds_task(void *args)
{
    motor_cmds_handle_t *handle = (motor_cmds_handle_t *)args;
    char *cmds = handle->args;
    char code[16];
    int beatCnt;
    int next_cmd_pos = 0;
    float delaytime;

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
            car_stop();
        } else {
            motor_run_one_cmd(code);
        }

        vTaskDelay(pdMS_TO_TICKS(delaytime));
    }

    car_stop();
    handle->task_run = false;
    xEventGroupSetBits(handle->state_event, BIT0);
    vTaskDelete(NULL);
}
esp_err_t motor_run_cmds_wait_for_stop(void)
{
    EventBits_t uxBits = xEventGroupWaitBits(g_motor_cmds_handler.state_event, BIT0, true, true, 5000 / portTICK_RATE_MS);
    esp_err_t ret = ESP_ERR_TIMEOUT;
    if (uxBits & BIT0) {
        ret = ESP_OK;
    } else {
        ESP_LOGE(TAG, "wait for stop timeout.");
    }
    return ret;

}
int motor_run_cmds_stop(void)
{
    motor_cmds_handle_t *handle = &g_motor_cmds_handler;
    if (false == handle->task_run) {
        return ESP_OK;
    }

    xEventGroupClearBits(handle->state_event, BIT0);
    handle->task_run = false;
    esp_err_t ret = motor_run_cmds_wait_for_stop();

    return ret;

}
/* @root: {type: M_CMDS, cmds:[]}, should call cJSON_Delete(root) to free memory */
void motor_run_cmds(char *str)
{
    motor_cmds_handle_t *handle = &g_motor_cmds_handler;
    if (true == handle->task_run) {
        motor_run_cmds_stop();
    }

    handle->task_run = true;
    handle->args = str;
    xTaskCreate(motor_run_cmds_task, "led_run_cmds_task", 4096, handle, 5, NULL);
}



