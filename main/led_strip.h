
#ifndef __LED_STRIP_H__
#define __LED_STRIP_H__

#ifdef __cplusplus
extern "C" {
#endif
#include "esp_err.h"
#include "cJSON.h"

#define EXAMPLE_LED_NUMBERS         4

typedef struct {
    uint8_t pixels[EXAMPLE_LED_NUMBERS * 3];
} led_renderer_data_t;

void led_strip_init(void);
void led_on();

/* 使用说明
    默认led_strip_renderer_run已经被调用，已经起了led线程
    调用者只需要调用led_strip_fill_data 传入4个LED灯的数据即可
*/
esp_err_t led_strip_fill_data(led_renderer_data_t *data);
esp_err_t led_strip_renderer_run(void);
esp_err_t led_strip_renderer_stop(void);

void led_run_cmds(char *str);
int led_run_cmds_stop(void);


#ifdef __cplusplus
}
#endif

#endif
