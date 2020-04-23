#ifndef _MUSIC_H
#define _MUSIC_H

enum MUSIC_STATUS {
    MUSIC_IDLE,
    MUSIC_PLAYING,
    MUSIC_PAUSE,
    MUSIC_END,
};

#define SCORE_END 127
struct music_play_info{
    __code char*pu;
    uint pu_index;
    ulong last_note_start_timerct;
    uint8 music_status;
};

extern __pdata struct music_play_info music_task_play_info;
extern __code uint musical_scale_regv[];

//music score
extern __code char fu[];
extern __code char shaolshi[];
extern __code char xianglian[];
extern __code char notice_music[];
extern __code char testmu[];

uint8 get_note_index(signed char value);
#endif
