
#ifndef __MY_ADC_H__
#define __MY_ADC_H__

#ifdef __cplusplus
extern "C" {
#endif

void adc_init(void);
void adc_test();
int adc_get_voltage();
void adc_deinit();


#ifdef __cplusplus
}
#endif

#endif /* __MY_ADC_H__ */
