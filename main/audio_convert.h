
#ifndef __AUDIO_CONVERT_H__
#define __AUDIO_CONVERT_H__

#ifdef __cplusplus
extern "C" {
#endif



void audio_convert_init();
esp_err_t audio_convert_run();
esp_err_t audio_convert_stop();


#ifdef __cplusplus
}
#endif

#endif
