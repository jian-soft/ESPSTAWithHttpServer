
#ifndef __SOUND_H__
#define __SOUND_H__

#ifdef __cplusplus
extern "C" {
#endif
#include "cJSON.h"

typedef struct {
    float beats;
    float freq;
} sound_notation_t;


typedef void (*data_listen_cb)(void *data, int size);

void sound_init(void);
void sound_play_freq(cJSON *root);
void sound_play_mp3(int fileid);
void sound_register_data_listen_cb(data_listen_cb);

#ifdef __cplusplus
}
#endif

#endif
