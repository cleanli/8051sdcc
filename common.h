#ifndef _COMMON_H
#define _COMMON_H

#define CDB printf("line%d\r\n", __LINE__)
#define VERSION "0.1"
#define WHEEL_R 197 //mm
#define WHEEL_CIRCUMFERENCE (wheelr*2*3.14159f)
#define MS_COUNT 1279
#define TIMER0_COUNT_PER_SECOND 8303
#define COUNT10MS ((tcops+50)/100)
#define TC0PS_EEROM_ADDR 4
#define WHEEL_R_EEROM_ADDR 6

enum EVENT_TYPE{
    EVENT_KEYA1_UP,
    EVENT_KEYA2_UP,
    EVENT_KEYA3_UP,
    EVENT_KEYA4_UP,
    EVENT_UI_TIMEOUT,
    EVENT_MUSIC_PLAY_END,
    EVENT_MAX
};
struct s_lfs_data{
    char*buf;
    float fv;
    uint8 number_int;
    uint8 number_decimal;
    const char*follows;
};
void LCD_Init();
void lcd_update(unsigned char*);
void lcd_cursor(uint8 d);
uint8 get_key_status_raw();
uint8 key_down_in_time(uint8 timeout_in_20ms);
void music_led_flash();
void sound_en(bool en);
void update_led_lcj();
void update_freq(uint v);
void local_float_sprintf(struct s_lfs_data* lfsd);
void system_init();

extern volatile ulong timer_ct;
extern volatile ulong saved_int_timer_ct;
extern __pdata unsigned long saved_timer_ct ;
extern __pdata unsigned long saved_timer_ct_music ;
extern __pdata unsigned char disp_mem[33];
extern __pdata float mileage;
extern __pdata float last_speed;
extern __pdata uint tcops;
extern __pdata uint wheelr;
extern bool flag_10ms , flag_1s ;
extern bool target ;
extern bool disp_left_time;
extern uint target_hour , target_minute;
extern ulong target_seconds;

extern bool disp_mem_update ;
extern bool keyA1_down ;
extern bool keyA2_down ;
extern bool keyA3_down ;
extern bool keyA4_down ;
extern bool keyA1_up ;
extern bool keyA2_up ;
extern bool keyA3_up ;
extern bool keyA4_up ;
extern bool power_meas_trigged ;
extern bool g_flag_1s ;
extern bool g_flag_10ms ;
extern __pdata uint keyA1_down_ct;
extern __pdata uint keyA2_down_ct;
extern __pdata uint keyA3_down_ct;
extern __pdata uint keyA4_down_ct;
extern __pdata int cur_task_timeout_ct;
extern __pdata uint8 cur_task_event_flag;
extern __pdata int8 cur_ui_index ;
extern __pdata int8 last_ui_index ;
extern __pdata float power_voltage;
extern __pdata struct s_lfs_data float_sprintf;
extern __pdata uint8 ui_common_uint8 ;
extern __pdata uint ui_common_uint ;
extern __pdata int8 ui_common_int8 ;
extern __pdata int ui_common_int ;
extern __pdata ulong ui_common_ulong ;
extern __pdata int input_timeout; 
extern __pdata uint last_count_1s ;
extern __pdata uint8 last_count_10ms ;
extern __pdata uint count_1s;
extern __pdata uint8 count_10ms;
extern __pdata uint8 cursor_cmd ;
extern __pdata float speed;
extern __pdata float mileage;
extern __pdata ulong last_saved_int_timer_ct ;

struct task;
typedef void (*task_func)(struct task*);
typedef void (*func_p)(void*);
struct task {
    task_func t_func;
    //char flag_1s;
};
typedef struct ui_info_ {
    func_p ui_init;
    func_p ui_process_event;
    func_p ui_quit;
    int timeout;
    uint8 time_disp_mode;
    uint8 time_position_of_dispmem;
    uint8 power_position_of_dispmem;
    int8 ui_event_transfer[EVENT_MAX];
    __code char*timeout_music;
} ui_info;


extern __code const ui_info* current_ui;

#define TIME_DISP_EN (1<<0)
#define TIME_DISP_SECOND (1<<1)
#define TIME_DISP_LEFT (1<<2)

#define UI_TRANSFER_DEFAULT (-2)
#define TIMEOUT_DISABLE (-1)
#define TIMEOUT_INPUT (-2)

#endif
