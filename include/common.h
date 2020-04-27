#ifndef _COMMON_H
#define _COMMON_H

#define CDB printf("line%d\r\n", __LINE__)
#define VERSION "2.0"
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
void local_float_sprintf(struct s_lfs_data* lfsd);

extern volatile ulong timer_ct;
extern volatile ulong saved_int_timer_ct;
extern __pdata unsigned char disp_mem[33];
extern __pdata float mileage;
extern __pdata float last_speed;
extern __pdata uint tcops;
extern __pdata uint wheelr;
extern bool flag_10ms , flag_1s ;

extern bool disp_mem_update ;
extern bool g_flag_1s ;
extern bool g_flag_10ms ;
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

#define TIME_DISP_EN (1<<0)
#define TIME_DISP_SECOND (1<<1)
#define TIME_DISP_LEFT (1<<2)

#define UI_TRANSFER_DEFAULT (-2)
#define TIMEOUT_DISABLE (-1)
#define TIMEOUT_INPUT (-2)

#endif
