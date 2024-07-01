
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
        T: M_RUN,
        m1d: 1, // 1 or -1, 1-forward, -1-back, 电机1方向，必选
        m1s: 100, //0~100，电机1速度，必选
        m2d: -1, //电机2方向，必选
        m2s: 100  //电机2速度，必选
    } */
    M_RUN = 0,

    /* 电机停止：
    {
        T: M_STOP
    } */
    M_STOP = 1,

    /* 电机运行一段距离：
    {
        T: M_RUN_D,
        m1d: 10,  //数值，电机1移动距离
        m2d: -10  //数值，电机2移动距离
    } */
    M_RUN_D = 2,

    /* 电机执行一段指令：
    {
        T: M_CMDS,
        V: "L,1;R,4;FL,1;BL,4;NO,4;"
    }
    code定义，字符串：
    "F" "B": 前进 后退
    "L" "R": 原地左转 原地右转
    "FL" "FR": 前进左转 前进右转
    "BL" "BR": 后退左 后退右
    "NO":无操作
    */
    M_CMDS = 3,

    /* 播放预制的声音
    {
        T: S_PLAY,
        V: "didi"  //要播放的文件，这个是硬编码，取值为固件支持的文件
    } */
    S_PLAY = 4,

    /* 播放音高
    {
        T: S_FREQ,
        v: 440  //声音频率值, 0表示按键松开
    } */
    S_FREQ = 5,

    /* 播放乐谱
    {
        T: S_NOTE,
        V: "1.,4;1,4;.1,4;NO,4;"
    }
    */
    S_NOTES = 6,

    /* LED控制：L_CMDS
    执行一段灯光指令
    {
        T: L_CMDS,
        V: "R,1;RB,4;GB,1;C,4;NO,4;"
    }
    code定义，字符串：
    "R" "G" "B": 按照颜色全亮
    "RB" "GB" "BB": 按照颜色闪烁
    "C": chase
    */
    L_CMDS = 7,

    /* 音符 电机 光效 3合一命令
        {T: C_CMDS, V1: <0|1>, V2:<note cmds>, V3:<motor cmds>, V4:<light cmds>}
    */
    C_CMDS = 8,

    /* 终止执行
        {T: C_STOP}
    */
    C_STOP = 9,
};
typedef unsigned int enum_message_type;



int handle_message(char *json_in, char *json_out);



#ifdef __cplusplus
}
#endif

#endif
