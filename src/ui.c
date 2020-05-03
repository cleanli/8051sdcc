#include <stdio.h>
#include <string.h>
#include <limits.h>
#include "type.h"
#include "common.h"
#include "music.h"
#include "stc12_drv.h"
#include "ui.h"
#include "task.h"

__pdata float mileage;
__pdata float last_speed;
bool disp_mem_update = false;
bool g_flag_1s = false;
bool g_flag_10ms = false;
__pdata int8 cur_ui_index = 0;
__pdata int8 last_ui_index = 1;
__pdata struct s_lfs_data float_sprintf;
__pdata uint input_timeout = 60;

__pdata ui_info* __pdata current_ui=NULL;
__pdata float speed = 0.0f;
__pdata float mileage = 0.0f;
__pdata ulong last_saved_int_timer_ct = 0;
__pdata uint ui_tcops;
__pdata uint ui_wheelr;
__pdata change_level_info cli;

__pdata uint8 ui_common_uint8 = 0;
__pdata uint ui_common_uint = 0;
__pdata int8 ui_common_int8 = 0;
__pdata int ui_common_int = 0;
__pdata ulong ui_common_ulong = 0;
__pdata ui_info working_ui_info;
uint* __pdata ui_common_uint_p = NULL;
bool ui_common_bit = false;
bool led_flash_per_second = false;
//common

void ui_start()
{
    memcpy(&working_ui_info, &all_ui[0], sizeof(ui_info));
    current_ui = &working_ui_info;
    current_ui->ui_init(current_ui);
}

void common_ui_init(void*vp)
{
    ui_info* uif =(ui_info*)vp;
    if(uif->time_disp_mode & TIMER_TRIGGER_START){
        cur_task_timer_started = false;
    }
    else{
        cur_task_timer_started = true;
    }
    if(uif->time_disp_mode & TIME_OUT_INPUT){
        cur_task_timeout_ct = input_timeout;
    }
    else{
        cur_task_timeout_ct = uif->timeout;
    }
    led_flash_per_second = true;
    printf("cur task timect---init %x\r\n", cur_task_timeout_ct);
    cur_task_event_flag = 0;
    memset(disp_mem, 0, 33);
    disp_mem_update = true;
    no_key_down_ct = 0;
    set_led1(false);
    set_led2(false);
}

void ui_transfer(uint8 ui_id)
{
    if(current_ui->ui_quit){
        current_ui->ui_quit(current_ui);
    }
    last_ui_index = cur_ui_index;
    cur_ui_index = ui_id;
    memcpy(&working_ui_info, &all_ui[cur_ui_index], sizeof(ui_info));
    current_ui = &working_ui_info;
    if(current_ui->ui_init){
        current_ui->ui_init(current_ui);
    }
    printf("ui %u->%u\r\n", last_ui_index, ui_id);
}

void delayed_close_led1(void*p)
{
    p;
    set_led1(false);
}

void delayed_close_led2(void*p)
{
    p;
    set_led2(false);
}

void flash_led(uint8 led_index, uint mss)
{
    if(led_index == 1){
        set_led1(true);
        set_delayed_work(mss, delayed_close_led1, NULL);
    }
    else{
        set_led2(true);
        set_delayed_work(mss, delayed_close_led2, NULL);
    }
}

void common_process_event(void*vp)
{
    //bool dg = g_flag_1s;
    ui_info* uif =(ui_info*)vp;
    if(cur_task_event_flag && !is_playing_music()){
        if(keyA2_up){
            play_music_note(50, 50);
        }
        if(keyA4_up){
            play_music_note(5, 50);
        }
        if(keyA1_up || keyA3_up){
            play_music_note(1, 50);
        }
        if(!(uif->time_disp_mode & NO_LED_FLASH_EVENT)){
            flash_led(2, 5);
        }
    }
    for(int8 i = 0; i < EVENT_MAX; i++){
        uint8 evt_flag=1<<i;
        //if(dg) printf("ev flag %x %x i %x\r\n", cur_task_event_flag, evt_flag, i);
        if(cur_task_event_flag & evt_flag){
            if(uif->ui_event_transfer[i]>=0){
                ui_transfer(uif->ui_event_transfer[i]);
                return;
            }
            if(uif->ui_event_transfer[i]==UI_TRANSFER_DEFAULT){
                if(evt_flag == (1<<EVENT_KEYA2_UP) &&
                        last_ui_index != cur_ui_index){
                    ui_transfer(last_ui_index);
                    return;
                }
            }
            if(uif->ui_event_transfer[i]==UI_RESET_TIMEOUT){
                if(uif->time_disp_mode & TIME_OUT_INPUT){
                    cur_task_timeout_ct = input_timeout;
                }
                else{
                    cur_task_timeout_ct = uif->timeout;
                }
            }
            //printf("ev flag %x EVUTO %x\r\n", evt_flag, EVENT_UI_TIMEOUT);
            if(evt_flag == (1<<EVENT_UI_TIMEOUT) && uif->timeout_music){
                play_music(uif->timeout_music);
            }
        }
    }
    if(led_flash_per_second && !is_playing_music()){
        if(g_flag_1s){
            flash_led(1, 5);
        }
    }
    cur_task_event_flag = 0;
}

//first
void first_init(void*vp)
{
    common_ui_init(vp);
    sprintf(disp_mem, "%s%s", VERSION, GIT_SHA1);
    sprintf(disp_mem+16, "%s", __TIME__);
    sprintf(disp_mem+21, "%s", __DATE__);
    disp_mem_update = true;
}

void first_process_event(void*vp)
{
    common_process_event(vp);
}

void delayed_ui_transfer(void*p)
{
    uint * up = (uint*)p;
    ui_transfer(*up);
}

//second
void second_init(void*vp)
{
    ui_info* uif =(ui_info*)vp;
    ui_common_uint8 = 1;//round of 5min
    common_ui_init(vp);
    memset(disp_mem, 0, 32);
    sprintf(disp_mem+12, "%u", ui_common_uint8);
    time_hms(disp_mem, uif->timeout);
    disp_mem_update = true;
    play_music(uif->timeout_music);
}

void second_process_event(void*vp)
{
    ui_info* uif =(ui_info*)vp;
    if(cur_task_event_flag & (1<<EVENT_UI_TIMEOUT)){
        cur_task_timeout_ct = uif->timeout;
        ui_common_uint8++;
        sprintf(disp_mem+12, "%u", ui_common_uint8);
    }
    if(cur_task_event_flag & (1<<EVENT_MUSIC_PLAY_END) &&
            ui_common_uint8 == 4){//15min only
        ui_common_uint8 = 9;
        set_delayed_work(1000, delayed_ui_transfer, &ui_common_uint8);
    }
    common_process_event(vp);
}

//input
void show_input_timeout()
{
    sprintf(disp_mem+16, "%u", input_timeout);
    time_hms(disp_mem, input_timeout);
    disp_mem_update = true;
}

void timeout_input_init(void*vp)
{
    ui_info* uif =(ui_info*)vp;
    common_ui_init(vp);
    cli.switch_cursor_type = SWITCH_CURSOR_BY_LEFT_KEY;
    ui_common_bit = true;//increase the uint
    sprintf(disp_mem+9, "Timeout");
    ui_common_uint = 1;
    cursor_cmd = 0x87;
    cli.factor = 60;
    cli.min_adder = 1;
    cli.max_adder = 3600;
    cli.cursor_jump = 3;
    cli.max_cursor_posi = 0x81;
    cli.min_cursor_posi = 0x87;
    show_input_timeout();
}
//timer
void timer_ui_init(void*vp)
{
    ui_info* uif =(ui_info*)vp;
    common_ui_init(vp);
    time_hms(disp_mem, input_timeout);
    disp_mem_update = true;
}

void timer_ui_quit(void*vp)
{
    vp;//fix warning
    pause_music();
}

void timer_ui_process_event(void*vp)
{
    ui_info* uif =(ui_info*)vp;

    if(keyA4_up){
        cur_task_timer_started = !cur_task_timer_started;
    }
    common_process_event(vp);
}

void timeout_input_quit(void*vp)
{
    vp;//fix warning
    cursor_cmd = 0;
}

void inc_change_level(bool increase)
{
    if(increase){
        if(ui_common_uint==cli.max_adder){
            ui_common_uint=cli.min_adder;
            cursor_cmd = cli.min_cursor_posi;
        }
        else{
            ui_common_uint*=cli.factor;
            cursor_cmd -= cli.cursor_jump;
        }
    }
    else{
        if(ui_common_uint==cli.min_adder){
            ui_common_uint=cli.max_adder;
            cursor_cmd = cli.max_cursor_posi;
        }
        else{
            ui_common_uint/=cli.factor;
            cursor_cmd += cli.cursor_jump;
        }
    }
}

void inc_uint(uint *p, bool increase)
{
    if(increase){
        if(UINT_MAX - *p>ui_common_uint){
            *p+=ui_common_uint;
        }
    }
    else{
        if(*p>ui_common_uint){
            *p-=ui_common_uint;
        }
    }
}

bool edit_uint_by_key(uint *p)
{
    bool ret = false;//if need update display
    if(cli.switch_cursor_type == SWITCH_CURSOR_BY_DOUBLE_KEY ||
            cli.switch_cursor_type == SWITCH_CURSOR_BY_LONG_PRESS){
        if(keyA1_up&& last_keyA1_down_ct<4000){
            printf("key A1 up %u\r\n", keyA1_down_ct);
            inc_uint(p, false);
            ret = true;
        }
        if(keyA3_up&& last_keyA3_down_ct<4000){
            printf("key A3 up %u\r\n", keyA3_down_ct);
            inc_uint(p, true);
            ret = true;
        }
    }
    switch(cli.switch_cursor_type){
        case SWITCH_CURSOR_BY_DOUBLE_KEY:
            if(keyA1_down && keyA3_down){
                if(g_flag_1s){
                    inc_change_level(true);
                    ret = true;
                }
            }
            if(count_10ms%10==0){
                if(keyA1_down_ct>4000){
                    inc_uint(p, false);
                    ret = true;
                }
                if(keyA3_down_ct>4000){
                    //printf("uicint %d %u\r\n", ui_common_uint, keyA3_down_ct);
                    inc_uint(p, true);
                    ret = true;
                }
            }
            break;
        case SWITCH_CURSOR_BY_LONG_PRESS:
            if(keyA1_down_ct>4000){
                if(g_flag_1s){
                    inc_change_level(true);
                    ret = true;
                }
            }
            if(keyA3_down_ct>4000){
                if(g_flag_1s){
                    inc_change_level(false);
                    ret = true;
                }
            }
            break;
        case SWITCH_CURSOR_BY_LEFT_KEY:
            if(keyA1_up && last_keyA1_down_ct<4000){
                printf("key A1 up\r\n");
                inc_change_level(true);
                ret = true;
            }
            if(keyA3_up){
                printf("key A3 up\r\n");
                inc_uint(p, ui_common_bit);
                ret = true;
            }
            if(keyA1_down_ct>4000){
                if(g_flag_1s){
                    ui_common_bit = !ui_common_bit;
                    inc_uint(p, ui_common_bit);
                    ret = true;
                }
            }
            if(keyA3_down_ct>4000){
                if(count_10ms%10==0){
                    inc_uint(p, ui_common_bit);
                    ret = true;
                }
            }
            break;
    }
    return ret;
}

void timeout_input_process_event(void*vp)
{
    ui_info* uif =(ui_info*)vp;

    if(edit_uint_by_key(&input_timeout)){
        show_input_timeout();
    }
    common_process_event(vp);
}

//lcj
void lcj_disp()
{
    ui_common_uint8 = disp_mem[16];
    float_sprintf.buf=disp_mem;
    float_sprintf.fv = speed;
    float_sprintf.number_int=2;
    float_sprintf.number_decimal=1;
    float_sprintf.follows="km/h";
    local_float_sprintf(&float_sprintf);

    if(mileage<1000000){
        //sprintf(disp_mem+10, "%03.1fm", mileage/1000);
        float_sprintf.buf=disp_mem+10;
        float_sprintf.fv = mileage/1000;
        float_sprintf.number_int=3;
        float_sprintf.number_decimal=1;
        float_sprintf.follows="m";
        local_float_sprintf(&float_sprintf);
    }
    else if(mileage < 10000000){
        //sprintf(disp_mem+10, "%1.2fkm", mileage/1000000);
        float_sprintf.buf=disp_mem+10;
        float_sprintf.fv = mileage/1000000;
        float_sprintf.number_int=1;
        float_sprintf.number_decimal=2;
        float_sprintf.follows="km";
        local_float_sprintf(&float_sprintf);
    }
    else{
        //sprintf(disp_mem+10, "%3.1fkm", mileage/1000000);
        float_sprintf.buf=disp_mem+10;
        float_sprintf.fv = mileage/1000000;
        float_sprintf.number_int=3;
        float_sprintf.number_decimal=1;
        float_sprintf.follows="km";
        local_float_sprintf(&float_sprintf);
    }
    disp_mem[16] = ui_common_uint8;
}

void lcj_compute_speed(ulong t)
{
    ui_common_ulong = t - last_saved_int_timer_ct;
    //printf("dur %d", duration);
    speed = (float)WHEEL_CIRCUMFERENCE * tcops / 1000 / (float)ui_common_ulong; //m/s
    //printf("2 m/s---%2.1f\r\n", speed);
    speed = speed * 3600 / 1000;//km/h
    //printf("saved---%lu\r\n", saved_int_timer_ct);
    //printf("last saved---%lu\r\n", last_saved_int_timer_ct);
    //printf("2---%2.1f\r\n", speed);
}

void lcj_ui_init(void*vp)
{
    ui_info* uif =(ui_info*)vp;
    common_ui_init(vp);
    speed = 0.0f;
    mileage = 0.0f;
    lcj_disp();
    disp_mem_update = true;
}

void lcj_process_event(void*vp)
{
    ui_info* uif =(ui_info*)vp;
    if(last_saved_int_timer_ct != saved_int_timer_ct){
        if(last_saved_int_timer_ct != 0){
            lcj_compute_speed(saved_int_timer_ct);
            if(speed < 100){//abnormal if speed > 100km/h, discard the data
                last_speed = speed;
                mileage += WHEEL_CIRCUMFERENCE;
                lcj_disp();
                disp_mem_update = true;
                last_saved_int_timer_ct = saved_int_timer_ct;
            }
        }
        else if(saved_int_timer_ct != 0){
            last_saved_int_timer_ct = saved_int_timer_ct;
        }
        flash_led(2, 5);
    }
    else if(g_flag_1s){
        lcj_compute_speed(timer_ct);
        if(speed < last_speed){
            lcj_disp();
            disp_mem_update = true;
            last_speed = speed;
        }
    }
    common_process_event(vp);
}

__code const ui_info all_ui[]={
    {//0 main menu
        "Menu",
        menu_ui_init,//func_p ui_init;
        menu_process_event,//func_p ui_process_event;
        NULL,//func_p ui_quit;
        0,//int timeout;
        0,//uint8 time_disp_mode;
        33,//uint8 time_position_of_dispmem;
        10,//uint8 power_position_of_dispmem;
        {-1,-1,-1,-1,-1,-1,9},//int8 ui_event_transfer[EVENT_MAX];
        NULL,//__code char*timeout_music;
    },
    {//1 second
        "5 mins",
        second_init,
        second_process_event,
        NULL,
        300,
        TIME_DISP_EN|TIME_DISP_LEFT|TIME_OUT_EN,
        16,
        27,
        {-1,0,-1,-1,-1,-1,-1},
        notice_music,
    },
    {//2 predefined timer
        "Predefined timer",
        predefined_timer_ui_init,
        predefined_timer_process_event,
        NULL,
        0,//int timeout;
        0,
        33,
        10,
        {-1,0,-1,4,-1,-1,9},
        NULL,
    },
    {//3 input timeout
        "Set timeout",
        timeout_input_init,//func_p ui_init;
        timeout_input_process_event,//func_p ui_process_event;
        timeout_input_quit,//func_p ui_quit;
        0,//int timeout;
        0,//uint8 time_disp_mode;
        33,//uint8 time_position_of_dispmem;
        27,//uint8 power_position_of_dispmem;
        {-1,0,-1,4,2,-1,9},//ui_event_transfer[EVENT_MAX];
        NULL,//__code char*timeout_music;
    },
    {//4 timer
        "Timer",
        timer_ui_init,//func_p ui_init;
        timer_ui_process_event,//func_p ui_process_event;
        timer_ui_quit,//func_p ui_quit;
        0,//int timeout;
        TIMER_TRIGGER_START|TIME_DISP_EN|TIME_DISP_LEFT|TIME_OUT_INPUT|TIME_OUT_EN,//uint8 time_disp_mode;
        16,//uint8 time_position_of_dispmem;
        27,//uint8 power_position_of_dispmem;
        {-1,UI_TRANSFER_DEFAULT,-1,-1,-1,3,-1},//int8 ui_event_transfer[EVENT_MAX];
        xianglian,//__code char*timeout_music;
    },
    {//5 lcj
        "LCJ",
        lcj_ui_init,//func_p ui_init;
        lcj_process_event,//func_p ui_process_event;
        NULL,//func_p ui_quit;
        INT_MAX,//int timeout;
        NO_LED_FLASH_EVENT |TIME_DISP_EN|TIME_OUT_EN,//uint8 time_disp_mode;
        16,//uint8 time_position_of_dispmem;
        27,//uint8 power_position_of_dispmem;
        {-1,0,-1,-1,-1,-1,-1},//int8 ui_event_transfer[EVENT_MAX];
        NULL,//__code char*timeout_music;
    },
    {//6 music menu
        "Music",
        music_ui_init,//func_p ui_init;
        music_process_event,//func_p ui_process_event;
        NULL,//func_p ui_quit;
        0,//int timeout;
        0,//uint8 time_disp_mode;
        33,//uint8 time_position_of_dispmem;
        10,//uint8 power_position_of_dispmem;
        {-1,0,-1,-1,-1,-1,9},//int8 ui_event_transfer[EVENT_MAX];
        NULL,//__code char*timeout_music;
    },
    {//7 eerom modify
        "Cali data",
        cali_ui_init,//func_p ui_init;
        cali_process_event,//func_p ui_process_event;
        NULL,//func_p ui_quit;
        0,//int timeout;
        0,//uint8 time_disp_mode;
        33,//uint8 time_position_of_dispmem;
        10,//uint8 power_position_of_dispmem;
        {-1,0,-1,-1,-1,-1,9},//int8 ui_event_transfer[EVENT_MAX];
        NULL,//__code char*timeout_music;
    },
    {//8 watch dog warning
        "Watch Dog Warn",
        wtd_ui_init,//func_p ui_init;
        wtd_ui_process_event,//func_p ui_process_event;
        NULL,//func_p ui_quit;
        1,//int timeout;
        TIME_OUT_EN,//uint8 time_disp_mode;
        33,//uint8 time_position_of_dispmem;
        27,//uint8 power_position_of_dispmem;
        {-1,0,-1,-1,-1,UI_RESET_TIMEOUT,9},//int8 ui_event_transfer[EVENT_MAX];
        warning,//__code char*timeout_music;
    },
    {//9 watch dog warning
        "Power Off",
        pwroff_ui_init,//func_p ui_init;
        pwroff_ui_process_event,//func_p ui_process_event;
        NULL,//func_p ui_quit;
        10,//int timeout;
        TIME_DISP_LEFT|TIME_DISP_EN|TIME_DISP_SECOND|TIME_OUT_EN,//uint8 time_disp_mode;
        16,//uint8 time_position_of_dispmem;
        27,//uint8 power_position_of_dispmem;
        {-1,UI_TRANSFER_DEFAULT,-1,-1,-1,-1,-1},//int8 ui_event_transfer[EVENT_MAX];
        pwroff_music,//__code char*timeout_music;
    },
    {//10 first
        "Version",
        first_init,
        first_process_event,
        NULL,
        0,//int timeout;
        0,
        0,
        33,
        {-1,0,-1,3,2,-1,9},
        NULL,
    },
#if 0
    {//n input timeout
        "",
        common_ui_init,//func_p ui_init;
        common_process_event,//func_p ui_process_event;
        NULL,//func_p ui_quit;
        0,//int timeout;
        0,//uint8 time_disp_mode;
        33,//uint8 time_position_of_dispmem;
        27,//uint8 power_position_of_dispmem;
        {-1,-1,-1,-1,-1,-1},//int8 ui_event_transfer[EVENT_MAX];
        NULL,//__code char*timeout_music;
    },
#endif
};

const char* __pdata const menu_str[]={
    all_ui[1].ui_name,
    all_ui[2].ui_name,
    all_ui[3].ui_name,
    all_ui[4].ui_name,
    all_ui[5].ui_name,
    all_ui[6].ui_name,
    all_ui[7].ui_name,
    all_ui[8].ui_name,
    all_ui[9].ui_name,
    all_ui[10].ui_name,
};
const char* __code const cali_str[]={
    "timer cal",
    "wheel cal",
};
#define NUMBER_OF_STRARR(str) (sizeof(str)/sizeof(char*))
#define MAX_UI_MENU_SIZE 7
void disp_ui_menu(const char** m_s, uint8 size, uint8 id)
{
    uint8 position;
    if(size > MAX_UI_MENU_SIZE){
        size = MAX_UI_MENU_SIZE;
    }
    for(uint8 i = 0; i< size; i++){
        disp_mem[i] = '_';
    }
    disp_mem[size] = ' ';
    position = id-1;
    if(position > MAX_UI_MENU_SIZE - 1){
        position = MAX_UI_MENU_SIZE - 1;
    }
    if(id>9){
        disp_mem[position]=id/10+0x30;
        disp_mem[position+1]=id%10+0x30;
    }
    else{
        disp_mem[position]=id+0x30;
    }
    memset(disp_mem+16, 0, 16);
    sprintf(disp_mem+16, "%s", m_s[id-1]);
    disp_mem_update = true;
}

void menu_moving(const char** m_s, uint8 numb_of_arr)
{
    if(keyA1_up){
        if(ui_common_uint8>1){
            ui_common_uint8--;
            disp_ui_menu(m_s, numb_of_arr, ui_common_uint8);
        }
        printf("key A1 up\r\n");
    }
    if(keyA3_up){
        if(ui_common_uint8<numb_of_arr){
            ui_common_uint8++;
            disp_ui_menu(m_s, numb_of_arr, ui_common_uint8);
        }
        printf("key A3 up\r\n");
    }
}

void menu_ui_init(void*vp)
{
    ui_info* uif =(ui_info*)vp;
    if(reset_flag){
        reset_flag = false;
        ui_transfer(8);
    }
    else{
        common_ui_init(vp);
        ui_common_uint8 = last_ui_index;
        disp_ui_menu(menu_str, NUMBER_OF_STRARR(menu_str), ui_common_uint8);
    }
}

void menu_process_event(void*vp)
{
    ui_info* uif =(ui_info*)vp;
    common_process_event(vp);
    menu_moving(menu_str, NUMBER_OF_STRARR(menu_str));
    if(keyA4_up){
        ui_transfer(ui_common_uint8);
        printf("key A4 up\r\n");
    }
}

void cali_ui_init(void*vp)
{
    ui_info* uif =(ui_info*)vp;
    common_ui_init(vp);
    cli.switch_cursor_type = SWITCH_CURSOR_BY_LONG_PRESS;
    ui_common_uint8 = 1;
    ui_common_int8 = 0;
    ui_tcops = tcops;
    ui_wheelr = wheelr;
    cli.factor = 10;
    cli.min_adder = 1;
    cli.max_adder = 10000;
    cli.cursor_jump = 1;
    cli.max_cursor_posi = 0x9b;
    cli.min_cursor_posi = 0x9f;
    disp_ui_menu(cali_str, 2, ui_common_uint8);
}

void cali_process_event(void*vp)
{
    ui_info* uif =(ui_info*)vp;
    common_process_event(vp);
    if(ui_common_int8 == 0){
        menu_moving(cali_str, NUMBER_OF_STRARR(cali_str));
        if(keyA2_up){
            if(ui_tcops != tcops || ui_wheelr != wheelr){
                ui_common_int8 = 2;
                sprintf(disp_mem+27, "%s", "Save?");
                disp_mem_update = true;
            }
            else{
                ui_transfer(last_ui_index);
            }
            printf("key A2 up\r\n");
        }
        if(keyA4_up){
            ui_common_int8 = 1;
            if(ui_common_uint8 == 1){
                ui_common_uint_p = &ui_tcops;
            }
            else{
                ui_common_uint_p = &ui_wheelr;
            }
            sprintf(disp_mem + 27, "%05u", *ui_common_uint_p);
            cursor_cmd = 0x9f;
            disp_mem_update = true;
            ui_common_uint = 1;
            printf("key A4 up\r\n");
        }
    }
    else if(ui_common_int8 == 1){
        if(edit_uint_by_key(ui_common_uint_p)){
            sprintf(disp_mem + 27, "%05u", *ui_common_uint_p);
            disp_mem_update = true;
        }
        if(keyA2_up){
            ui_common_int8 = 0;
            if(ui_common_uint8 == 1){
                ui_tcops = tcops;
            }
            else{
                ui_wheelr = wheelr;
            }
            memset(disp_mem + 27, ' ', 5);
            disp_mem_update = true;
            cursor_cmd = 0;
        }
        if(keyA4_up){
            ui_common_int8 = 0;
            memset(disp_mem + 27, ' ', 5);
            disp_mem_update = true;
            cursor_cmd = 0;
        }
    }
    else if(ui_common_int8 == 2){
        if(keyA4_up){
            save_cal_data(ui_tcops, ui_wheelr);
            tcops = ui_tcops;
            wheelr = ui_wheelr;
            ui_common_int8 = 0;
            disp_ui_menu(cali_str, 2, ui_common_uint8);
            sprintf(disp_mem+27, "%s", "Saved");
            disp_mem_update = true;
        }
        if(keyA2_up){
            ui_transfer(last_ui_index);
        }
    }
}

const char* __code const music_str[]={
    "ShaoLinShi",
    "XiangLian",
    "HappyBirthDay",
    "XiYouJi_1",
};
__code const signed char* __code const music_list[]={
    shaolshi,
    xianglian,
    fu,
    xiyouji1,
};
void music_ui_init(void*vp)
{
    ui_info* uif =(ui_info*)vp;
    common_ui_init(vp);
    ui_common_uint8 = 1;
    ui_common_int8 = 0;
    disp_ui_menu(music_str, NUMBER_OF_STRARR(music_str), ui_common_uint8);
}

void delayed_music(void *p)
{
    __code const signed char*music = (__code const signed char*)p;
    play_music(music);
}

void music_process_event(void*vp)
{
    ui_info* uif =(ui_info*)vp;
    menu_moving(music_str, NUMBER_OF_STRARR(music_str));
    if(keyA2_up){
        if(is_playing_music()){
            pause_music();
        }
        else{
            ui_transfer(0);
        }
        printf("key A2 up\r\n");
    }
    if(keyA4_up){
        play_music(music_list[ui_common_uint8-1]);
        printf("key A4 up\r\n");
    }
    if(cur_task_event_flag & (1<<EVENT_MUSIC_PLAY_END)){
        if(ui_common_uint8<NUMBER_OF_STRARR(music_str)){
            ui_common_uint8++;
            disp_ui_menu(music_str, NUMBER_OF_STRARR(music_str), ui_common_uint8);
            set_delayed_work(300, delayed_music, music_list[ui_common_uint8-1]);
        }
    }
    if(is_playing_music()){
        no_key_down_ct = 0;
    }
    common_process_event(vp);
}
void wtd_ui_init(void*vp)
{
    ui_info* uif =(ui_info*)vp;
    common_ui_init(vp);
    strcpy(disp_mem, "Watch Dog");
    strcpy(disp_mem+16, "Warning!");
    disp_mem_update = true;
}

void wtd_ui_process_event(void*vp)
{
    ui_info* uif =(ui_info*)vp;
    if(keyA4_up){
        stop_feed_wtd = true;
        printf("test watch dog reset\r\n");
    }
    common_process_event(vp);
}
void pwroff_ui_init(void*vp)
{
    ui_info* uif =(ui_info*)vp;
    common_ui_init(vp);
    strcpy(disp_mem, "PwrOffCountDown");
    disp_mem_update = true;
}

void pwroff_ui_process_event(void*vp)
{
    ui_info* uif =(ui_info*)vp;
    if(cur_task_event_flag & (1<<EVENT_MUSIC_PLAY_END)){
        power_off();
    }
    if(keyA2_up){
        pause_music();
    }
    if(g_flag_1s){
        play_music_note(8, 100);
    }
    common_process_event(vp);
}

const char* __code const predefined_timer_str[]={
    "5s",
    "30s",
    "1 min",
    "2 min",
    "5 min",
    "10 min",
    "15 min",
    "20 min",
    "30 min",
    "1 hour",
    "2 hour",
};
__code const uint predefined_timer_value_list[]={
    5,
    30,
    60,
    120,
    300,
    600,
    900,
    1200,
    1800,
    3600,
    7200,
};
void predefined_timer_ui_init(void*vp)
{
    ui_info* uif =(ui_info*)vp;
    common_ui_init(vp);
    ui_common_uint8 = 1;
    disp_ui_menu(predefined_timer_str, NUMBER_OF_STRARR(predefined_timer_str), ui_common_uint8);
}

void predefined_timer_process_event(void*vp)
{
    ui_info* uif =(ui_info*)vp;
    menu_moving(predefined_timer_str, NUMBER_OF_STRARR(predefined_timer_str));
    if(keyA2_up){
        ui_transfer(0);
    }
    if(keyA4_up){
        input_timeout = predefined_timer_value_list[ui_common_uint8-1];
    }
    common_process_event(vp);
}
