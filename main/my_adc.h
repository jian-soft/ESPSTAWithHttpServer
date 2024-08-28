
#ifndef __MY_ADC_H__
#define __MY_ADC_H__

#ifdef __cplusplus
extern "C" {
#endif

void adc_init(void);
void adc_deinit();

void adc_get_voltage(int *vol);



#ifdef __cplusplus
}
#endif

#endif /* __MY_ADC_H__ */
