
#ifndef __UTILS_H__
#define __UTILS_H__

#ifdef __cplusplus
extern "C" {
#endif

#define BEAT_DURATION (0.6)  //一拍的时长，单位秒 100/min~0.6s/beat


int parse_cmd(char *cmds, char *code, int *beat);


#ifdef __cplusplus
}
#endif

#endif
