#ifndef _TASK_H
#define _TASK_H
struct task;
typedef void (*task_func)(struct task*);
struct task {
    task_func t_func;
    //char flag_1s;
};
extern __code struct task all_tasks[];
extern __pdata uint cur_task_timeout_ct;
extern __pdata uint8 cur_task_event_flag;
extern __pdata uint8 cursor_cmd ;
extern __pdata uint8 count_10ms;
extern bool disp_mem_update ;
extern bool g_flag_1s ;
extern bool g_flag_10ms ;
void task_init();
void task_ui(struct task*v);
void task_key_status(struct task*v);
void task_timer(struct task*v);
void task_disp(struct task*v);
void task_music(struct task*v);
void task_power(struct task*v);


void pause_music();
void continue_music();
bool is_playing_music();
void play_music(__code const signed char* pu);
void time_hms(char*buf, uint t);
void local_float_sprintf(struct s_lfs_data* lfsd);
#endif
