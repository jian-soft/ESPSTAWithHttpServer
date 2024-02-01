#include <esp_log.h>
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "io_assignment.h"
#include "drv8833_pwm.h"


//static const char *TAG = "pwm";
#define GPIO_OUTPUT_PIN_SEL ((1ULL<<DRV8833_ENABLE_IO) | (1ULL<<MAX98357_ENABLE_IO))
#define GPIO_INPUT_PIN_SEL  ((1ULL<<M1_SPEED_CNT) | (1ULL<<M2_SPEED_CNT))

int g_m1_cnt;
int g_m2_cnt;
int g_target_distance;

static QueueHandle_t gpio_evt_queue = NULL;

static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    //xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);


}

static void gpio_task_example(void* arg)
{
    uint32_t gpio_num;
    for (;;) {
        if (xQueueReceive(gpio_evt_queue, &gpio_num, portMAX_DELAY)) {
            printf("dddd: gpio event: %ld\n", gpio_num);
            if (gpio_num == M1_SPEED_CNT) {
                g_m1_cnt++;
                if (g_m1_cnt >= g_target_distance) {
                    drv8833_motorA_stop();
                }
            }
            else if (gpio_num == M2_SPEED_CNT) {
                g_m2_cnt++;
                if (g_m2_cnt >= g_target_distance) {
                    drv8833_motorB_stop();
                }
            }
        }
    }
}


int get_and_clear_m1_cnt()
{
    int t = g_m1_cnt;
    g_m1_cnt = 0;
    return t;
}
int get_and_clear_m2_cnt()
{
    int t = g_m2_cnt;
    g_m2_cnt = 0;
    return t;
}
int get_m1_cnt()
{
    return g_m1_cnt;
}
int get_m2_cnt()
{
    return g_m2_cnt;
}


void run_distance(int distance)
{
    g_target_distance = distance;
    g_m1_cnt = 0;
    g_m2_cnt = 0;
    drv8833_motorA_run(50, 0);
    drv8833_motorB_run(50, 0);
}


void gpio_enable_drv8833(void)
{
    gpio_set_level(DRV8833_ENABLE_IO, 1);
}
void gpio_disable_drv8833(void)
{
    gpio_set_level(DRV8833_ENABLE_IO, 0);
}
void gpio_enable_max98357()
{
    gpio_set_level(MAX98357_ENABLE_IO, 1);
}
void gpio_disable_max98357()
{
    gpio_set_level(MAX98357_ENABLE_IO, 0);
}


void gpio_init(void)
{
    //设置OUTPUT
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    //bit mask of the pins that you want to set,e.g.GPIO18/19
    io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);
    gpio_disable_drv8833();
    gpio_enable_max98357();

    //设置INPUT，检测小车转速
    io_conf.intr_type = GPIO_INTR_POSEDGE;
    io_conf.mode = GPIO_MODE_INPUT;
    //bit mask of the pins that you want to set,e.g.GPIO18/19
    io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);

    //create a queue to handle gpio event from isr
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    //start gpio task
    xTaskCreate(gpio_task_example, "gpio_task_example", 2048, NULL, 10, NULL);

    //install gpio isr service
    gpio_install_isr_service(0);
    //hook isr handler for specific gpio pin
    gpio_isr_handler_add(M1_SPEED_CNT, gpio_isr_handler, (void*) M1_SPEED_CNT);
    //hook isr handler for specific gpio pin
    gpio_isr_handler_add(M2_SPEED_CNT, gpio_isr_handler, (void*) M2_SPEED_CNT);
}



