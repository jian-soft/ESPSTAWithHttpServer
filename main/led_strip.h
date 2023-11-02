
#ifndef __LED_STRIP_H__
#define __LED_STRIP_H__

#ifdef __cplusplus
extern "C" {
#endif
#define EXAMPLE_LED_NUMBERS         4

typedef struct led_strip_renderer_handle led_strip_renderer_handle_t;

typedef struct {
    uint8_t pixels[EXAMPLE_LED_NUMBERS * 3];
} led_renderer_data_t;


void led_strip_init(void);
void led_chase(void);
void led_on();

esp_err_t led_strip_fill_data(led_renderer_data_t *data);
esp_err_t led_strip_renderer_run(void);
esp_err_t led_strip_renderer_stop(void);



#ifdef __cplusplus
}
#endif

#endif
