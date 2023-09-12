#include <esp_log.h>
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "my_gpio.h"
#include "io_assignment.h"

//static const char *TAG = "pwm";

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


