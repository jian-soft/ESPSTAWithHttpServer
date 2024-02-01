
#ifndef __IO_ASSIGNMENT_H__
#define __IO_ASSIGNMENT_H__

#ifdef __cplusplus
extern "C" {
#endif
#include "driver/gpio.h"


//DRV8833
#define DRV8833_AIN1_IO     GPIO_NUM_4
#define DRV8833_AIN2_IO     GPIO_NUM_5
#define DRV8833_BIN1_IO     GPIO_NUM_6
#define DRV8833_BIN2_IO     GPIO_NUM_7

//MAX98397
#define STD_BCLK_IO        GPIO_NUM_9      // I2S bit clock io number
#define STD_WS_IO          GPIO_NUM_8      // I2S word select io number
#define STD_DOUT_IO        GPIO_NUM_10      // I2S data out io number

//LED strip
#define RMT_LED_STRIP_GPIO_NUM      GPIO_NUM_20 //U0RxD, ok

//battery voytage ADC
#define EXAMPLE_ADC1_CHAN0          ADC_CHANNEL_2  //GPIO2


//GPIO
#define DRV8833_ENABLE_IO   GPIO_NUM_3     //output
#define MAX98357_ENABLE_IO  GPIO_NUM_21    //output, U0TxD
#define ME6212_ENABLE_IO    GPIO_NUM_21     //output, U0TxD
#define M1_SPEED_CNT        GPIO_NUM_0      //input, isr
#define M2_SPEED_CNT        GPIO_NUM_1      //input, isr





#ifdef __cplusplus
}
#endif

#endif /* __IO_ASSIGNMENT_H__ */
