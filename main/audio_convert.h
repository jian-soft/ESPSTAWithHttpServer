
#ifndef __AUDIO_CONVERT_H__
#define __AUDIO_CONVERT_H__

#ifdef __cplusplus
extern "C" {
#endif

#define   CNV_PATTERN_RED_BASE_VALUE    (90)
#define   CNV_PATTERN_COLOR_SPAN        (3)
#define   CNV_PATTERN_GREEN_BASE_VALUE  (10)




void audio_convert_init();
esp_err_t audio_convert_run();
esp_err_t audio_convert_stop();


#ifdef __cplusplus
}
#endif

#endif
