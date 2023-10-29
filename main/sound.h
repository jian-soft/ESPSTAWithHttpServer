
#ifndef __SOUND_H__
#define __SOUND_H__

#ifdef __cplusplus
extern "C" {
#endif

void sound_init(void);
void sound_play_freq(float freq);

void sound_play_mp3(int fileid);


#ifdef __cplusplus
}
#endif

#endif
