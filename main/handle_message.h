
#ifndef __HANDLE_MESSAGE_H__
#define __HANDLE_MESSAGE_H__

#ifdef __cplusplus
extern "C" {
#endif

/* 小车控制消息协议 */
/* 枚举必须以0开始，顺序递增，与全局变量 event_name 有严格对应关系 */
enum message_type{
    /* 电机持续运行
    {
        type: M_RUN,
        m1d: 1, // 1 or -1, 1-forward, -1-back, 电机1方向，必选
        m1s: 100, //0~100，电机1速度，必选
        m2d: -1, //电机2方向，必选
        m2s: 100  //电机2速度，必选
    } */
    M_RUN = 0,

    /* 电机停止：
    {
        type: M_STOP
    } */
    M_STOP,

    /* 电机运行一段距离：
    {
        type: M_RUN_D,
        m1d: 10,  //数值，电机1移动距离
        m2d: -10  //数值，电机2移动距离
    } */
    M_RUN_D,

    /* 电机执行一段指令：
    {
        type: M_CMDS,
        cmds: "L,1;R,4;FL,1;BL,4;"
    }
    code定义，字符串：
    "F" "B": 前进 后退
    "L" "R": 原地左转 原地右转
    "FL" "FR": 前进左转 前进右转
    "BL" "BR": 后退左 后退右
    */
    M_CMDS,

    /* 播放预制的声音
    {
        type: S_PLAY,
        file: "didi"  //要播放的文件，这个是硬编码，取值为固件支持的文件
    } */
    S_PLAY,

    /* 播放音高
    {
        type: S_FREQ,
        value: 440  //声音频率值, 0表示按键松开
    } */
    S_FREQ,

    /* 播放乐谱
    {
        type: S_NOTE,
        notes: [
            {freq: 222.2, beat: 1},
            {freq: 410.1, beat: 4},
        ]
    }
    {
        type: S_NOTE,
        notes: "1.,4;1,4;.1,4"
    }
    */
    S_NOTES,

    /* LED控制：L_CMDS
    执行一段灯光指令
    {
        type: "L_CMDS",
        cmds: [
            {code: "R", beat: 1},
            {code: "R", beat: 4},
        ]
    }
    code定义，字符串：
    "R" "G" "B": 按照颜色全亮
    "RB" "GB" "BB": 按照颜色闪烁
    "C": chase
    */
    L_CMDS,
};
typedef unsigned int enum_message_type;



int handle_message(char *json_in, char *json_out);



#ifdef __cplusplus
}
#endif

#endif
