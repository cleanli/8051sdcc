#ifndef _MUSIC_H
#define _MUSIC_H

enum MUSIC_STATUS {
    MUSIC_IDLE,
    MUSIC_PLAYING,
    MUSIC_PAUSE,
    MUSIC_END,
};

#define SCORE_END 127
#define HALF_PERIOD 126
#define DOUBLE_PERIOD 125
#define FLAGSTART 124
#define GOSTART 123
#define DIVERT 122
#define HP HALF_PERIOD
#define DP DOUBLE_PERIOD

#define NO_DIVERT 0xffff
struct music_play_info{
    __code char*pu;
    uint pu_index;
    uint8 music_status;
    uint divert_index;
    uint restart_index;
};

struct music_note_play_info{
    int8 music_note;
    uint period_ms_ct;
    ulong note_start_timerct;
};

extern __pdata struct music_note_play_info music_note_task_play_info;
extern __pdata struct music_play_info music_task_play_info;
extern __code const uint musical_scale_regv[];

//music score
extern __code const signed char fu[];
extern __code const signed char shaolshi[];
extern __code const signed char xianglian[];
extern __code const signed char notice_music[];
extern __code const signed char warning[];
extern __code const signed char xiyouji1[];
extern __code const signed char pwroff_music[];
extern __code const signed char count_down_music[];
extern __code const signed char YouJianChuiYan[];

uint8 get_note_index(signed char value);
#endif
