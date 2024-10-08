
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
void play_notes_run(char *str);
int play_notes_stop(void);

void sound_play_mp3_run(int fileid);
void sound_register_data_listen_cb(data_listen_cb);

int is_sound_play_run();

#ifdef __cplusplus
}
#endif

#endif
