#ifndef _TASK_H
#define _TASK_H

#define NO_KEY_DOWN_CT_MAX 100
enum EVENT_TYPE{
    EVENT_KEYA1_UP,
    EVENT_KEYA2_UP,
    EVENT_KEYA3_UP,
    EVENT_KEYA4_UP,
    EVENT_UI_TIMEOUT,
    EVENT_MUSIC_PLAY_END,
    EVENT_NOKEYCT_MAXREACHED,
    EVENT_MAX
};
struct task;
typedef void (*task_func)(struct task*);
struct task {
    task_func t_func;
    //char flag_1s;
};
struct delay_work_info {
    func_p function;
    uint ct_10ms;
    void * para;
};
extern __code struct task all_tasks[];
extern bool cur_task_timer_started;
extern __pdata uint cur_task_timeout_ct;
extern __pdata uint8 cur_task_event_flag;
extern __pdata uint8 cursor_cmd ;
extern __pdata uint8 count_10ms;
extern bool stop_feed_wtd;
extern bool disp_mem_update ;
extern bool g_flag_1s ;
extern bool g_flag_10ms ;

extern bool keyA1_down ;
extern bool keyA2_down ;
extern bool keyA3_down ;
extern bool keyA4_down ;
extern bool keyA1_up ;
extern bool keyA2_up ;
extern bool keyA3_up ;
extern bool keyA4_up ;
extern __pdata uint keyA1_down_ct;
extern __pdata uint keyA2_down_ct;
extern __pdata uint keyA3_down_ct;
extern __pdata uint keyA4_down_ct;
extern __pdata uint last_keyA1_down_ct;
extern __pdata uint last_keyA2_down_ct;
extern __pdata uint last_keyA3_down_ct;
extern __pdata uint last_keyA4_down_ct;
extern __pdata uint no_key_down_ct;

void task_ui(struct task*v);
void task_key_status(struct task*v);
void task_timer(struct task*v);
void task_disp(struct task*v);
void task_music(struct task*v);
void task_power(struct task*v);
void task_lcd_bklight(struct task*vp);
void task_misc(struct task*v);

void set_delayed_work(uint tct, func_p f, void*pa);
void set_music_note_period(uint p);
void reset_music_note();
bool play_music_note(int8 note, uint period);
void pause_music();
void continue_music();
bool is_playing_music();
void play_music(__code const signed char* pu, uint note_period);
void time_hms(char*buf, uint t);
void local_float_sprintf(struct s_lfs_data* lfsd);
#endif
