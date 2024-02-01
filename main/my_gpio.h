
#ifndef __MY_GPIO_H__
#define __MY_GPIO_H__

#ifdef __cplusplus
extern "C" {
#endif

void gpio_init(void);
void gpio_enable_drv8833(void);
void gpio_disable_drv8833(void);
void gpio_enable_max98357();
void gpio_disable_max98357();


int get_and_clear_m1_cnt();
int get_and_clear_m2_cnt();
int get_m1_cnt();
int get_m2_cnt();
void run_distance(int distance);


#ifdef __cplusplus
}
#endif

#endif /* __MY_GPIO_H__ */
