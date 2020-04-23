#ifndef _UI_H
#define _UI_H

typedef void (*func_p)(void*);
typedef struct ui_info_ {
    func_p ui_init;
    func_p ui_process_event;
    func_p ui_quit;
    int timeout;
    uint8 time_disp_mode;
    uint8 time_position_of_dispmem;
    uint8 power_position_of_dispmem;
    int8 ui_event_transfer[EVENT_MAX];
    __code signed char*timeout_music;
} ui_info;

extern __code const ui_info* current_ui;
extern __code const ui_info all_ui[];
#endif
