
#ifndef __HANDLE_MESSAGE_H__
#define __HANDLE_MESSAGE_H__

#ifdef __cplusplus
extern "C" {
#endif

/***********消息格式***************/
/*
电机控制：M_
电机持续运行：
{
    type: "M_RUN",
    m1d: 1, // 1 or -1, 1-forward, -1-back, 电机1方向，必选
    m1s: 100, //0~100，电机1速度，必选
    m2d: -1, //电机2方向，必选
    m2s: 100  //电机2速度，必选
}*/
#define M_RUN   "M_RUN"

/*
电机停止：
{
    type: "M_STOP",
}
*/
#define M_STOP  "M_STOP"

/*
电机运行一段距离：
{
    type: "M_RUN_D",
    m1d: 10,  //数值，电机1移动距离
    m2d: -10  //数值，电机2移动距离
}
*/
#define M_RUN_D "M_RUN_D"

/*
声音控制：S_
播放预制的声音
{
    type: "S_PLAY",
    file: "didi"  //要播放的文件，这个是硬编码，取值为固件支持的文件
}
*/
#define S_PLAY  "S_PLAY"

/*
播放音高
{
    type: "S_FREQ",
    value: 440  //声音频率值, 0表示按键松开
}
*/
#define S_FREQ  "S_FREQ"

/*
播放乐谱
{
    type: "S_NOTES"
    notes: [
        {freq: 222.2, beat: 1},
        {freq: 410.1, beat: 4},
    ]
}
*/
#define S_NOTES "S_NOTES"


/*
LED控制：L_
播放预制的声音
{
    type: "L_PLAY",

}

*/





int handle_message(char *json_in, char *json_out);


#ifdef __cplusplus
}
#endif

#endif
