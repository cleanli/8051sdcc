#ifndef _UI_H
#define _UI_H
#include "task.h"

#define TIME_DISP_EN (1<<0)
#define TIME_DISP_SECOND (1<<1)
#define TIME_DISP_LEFT (1<<2)
#define TIME_OUT_INPUT (1<<3)
#define TIME_OUT_EN (1<<4)

#define UI_TRANSFER_DEFAULT (-2)

extern __pdata int8 cur_ui_index ;
extern __pdata int8 last_ui_index ;
extern __pdata float speed;

typedef void (*func_p)(void*);
typedef struct ui_info_ {
    const char* ui_name;
    func_p ui_init;
    func_p ui_process_event;
    func_p ui_quit;
    uint timeout;
    uint8 time_disp_mode;
    uint8 time_position_of_dispmem;
    uint8 power_position_of_dispmem;
    int8 ui_event_transfer[EVENT_MAX];
    __code const signed char*timeout_music;
} ui_info;

typedef struct change_level_info_{
    uint factor;
    uint min_adder;
    uint max_adder;
    uint8 cursor_jump;
    uint8 max_cursor_posi;
    uint8 min_cursor_posi;
    uint8 switch_cursor_type;
} change_level_info;

enum SWITCH_CURSOR_TYPE{
    SWITCH_CURSOR_BY_DOUBLE_KEY,
    SWITCH_CURSOR_BY_LONG_PRESS,
    SWITCH_CURSOR_BY_LEFT_KEY,
};

extern __code const ui_info* current_ui;
extern __code const ui_info all_ui[];
#endif
