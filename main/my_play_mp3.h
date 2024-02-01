
#ifndef __MY_PLAY_MP3_H__
#define __MY_PLAY_MP3_H__

#ifdef __cplusplus
extern "C" {
#endif


int play_mp3_read_buffer(char *buff, int buff_size);
    void play_mp3_start_pipeline(int fileid);
    void play_mp3_init(void);


#ifdef __cplusplus
}
#endif

#endif
