#include <stdio.h>
#include <string.h>
#include <limits.h>
#include "type.h"
#include "music.h"
#include "common.h"

volatile ulong timer_ct = 0;
volatile ulong saved_int_timer_ct = 0;
__pdata unsigned char disp_mem[33];
__pdata float mileage;
__pdata float last_speed;
__pdata uint tcops = TIMER0_COUNT_PER_SECOND;
__pdata uint wheelr = WHEEL_R;
bool flag_10ms = 0, flag_1s = 0;

void local_float_sprintf(struct s_lfs_data* lfsd)
{
    if(lfsd->fv > 65535){
        strcpy(lfsd->buf, "oo");
        return;
    }
    uint tmp_int = lfsd->fv;
    sprintf(lfsd->buf, "%u", tmp_int);
    uint8 tmp=strlen(lfsd->buf);
    if(tmp<lfsd->number_int){
        memset(lfsd->buf, '0', lfsd->number_int);
        sprintf(lfsd->buf+lfsd->number_int-tmp, "%u", tmp_int);
    }
    float decimal = lfsd->fv - tmp_int + 1;
    uint8 n_dec=lfsd->number_decimal;
    while(n_dec--)decimal*=10;
    tmp_int=decimal;
    tmp=strlen(lfsd->buf);
    sprintf(lfsd->buf+tmp, "%u", tmp_int);
    lfsd->buf[tmp]='.';
    if(lfsd->follows){
        strcat(lfsd->buf, lfsd->follows);
    }
}

/////////////////////////////new architecture///////////////////////////
bool disp_mem_update = false;
bool keyA1_down = false;
bool keyA2_down = false;
bool keyA3_down = false;
bool keyA4_down = false;
bool keyA1_up = false;
bool keyA2_up = false;
bool keyA3_up = false;
bool keyA4_up = false;
bool power_meas_trigged = false;
bool g_flag_1s = false;
bool g_flag_10ms = false;
__pdata uint keyA1_down_ct;
__pdata uint keyA2_down_ct;
__pdata uint keyA3_down_ct;
__pdata uint keyA4_down_ct;
__pdata int cur_task_timeout_ct;
__pdata uint8 cur_task_event_flag;
__pdata int8 cur_ui_index = 0;
__pdata int8 last_ui_index = 0;
__pdata float power_voltage;
__pdata struct s_lfs_data float_sprintf;
__pdata uint8 ui_common_uint8 = 0;
__pdata uint ui_common_uint = 0;
__pdata int8 ui_common_int8 = 0;
__pdata int ui_common_int = 0;
__pdata ulong ui_common_ulong = 0;
__pdata int input_timeout = 30;
__pdata uint last_count_1s = 0;
__pdata uint8 last_count_10ms = 0;
__pdata uint count_1s=0;
__pdata uint8 count_10ms=0;
__pdata uint8 cursor_cmd = 0;
__pdata float speed = 0.0f;
__pdata float mileage = 0.0f;
__pdata ulong last_saved_int_timer_ct = 0;

struct task;
__code const ui_info* current_ui=NULL;

__code const ui_info all_ui[]={
    {//0 first
        first_init,
        first_process_event,
        NULL,
        3,
        0,
        0,
        33,
        {-1,4,-1,2,1,-1},
        NULL,
    },
    {//1 second
        second_init,
        second_process_event,
        NULL,
        300,
        TIME_DISP_EN|TIME_DISP_LEFT,
        16,
        27,
        {-1,-2,-1,-1,-1,-1},
        notice_music,
    },
    {//2 input timeout
        timeout_input_init,//func_p ui_init;
        timeout_input_process_event,//func_p ui_process_event;
        timeout_input_quit,//func_p ui_quit;
        TIMEOUT_DISABLE,//int timeout;
        0,//uint8 time_disp_mode;
        33,//uint8 time_position_of_dispmem;
        27,//uint8 power_position_of_dispmem;
        {-1,UI_TRANSFER_DEFAULT,-1,3,1,-1},//ui_event_transfer[EVENT_MAX];
        NULL,//__code char*timeout_music;
    },
    {//3 timer
        common_ui_init,//func_p ui_init;
        common_process_event,//func_p ui_process_event;
        NULL,//func_p ui_quit;
        TIMEOUT_INPUT,//int timeout;
        TIME_DISP_EN|TIME_DISP_LEFT,//uint8 time_disp_mode;
        16,//uint8 time_position_of_dispmem;
        27,//uint8 power_position_of_dispmem;
        {-1,-1,-1,-1,-1,2},//int8 ui_event_transfer[EVENT_MAX];
        xianglian,//__code char*timeout_music;
    },
    {//4 lcj
        lcj_ui_init,//func_p ui_init;
        lcj_process_event,//func_p ui_process_event;
        NULL,//func_p ui_quit;
        INT_MAX,//int timeout;
        TIME_DISP_EN,//uint8 time_disp_mode;
        16,//uint8 time_position_of_dispmem;
        27,//uint8 power_position_of_dispmem;
        {-1,-1,-1,-1,-1,-1},//int8 ui_event_transfer[EVENT_MAX];
        NULL,//__code char*timeout_music;
    },
    {//n input timeout
        common_ui_init,//func_p ui_init;
        common_process_event,//func_p ui_process_event;
        NULL,//func_p ui_quit;
        TIMEOUT_DISABLE,//int timeout;
        0,//uint8 time_disp_mode;
        33,//uint8 time_position_of_dispmem;
        27,//uint8 power_position_of_dispmem;
        {-1,-1,-1,-1,-1,-1},//int8 ui_event_transfer[EVENT_MAX];
        NULL,//__code char*timeout_music;
    },
};

void task_main(struct task*vp)
{
    current_ui->ui_process_event(current_ui);
}

void task_key_status(struct task*vp)
{
    update_key_status();
}

void time_hms(char*buf, uint t)
{
    uint h, m, tm, s;

    h = t / 3600;
    tm = t - h * 3600;
    m = tm / 60;
    s = tm - m * 60;
    sprintf(buf, "%02u:%02u:%02u", h, m, s);
}

void task_power(struct task*vp)
{
    if(current_ui->power_position_of_dispmem<32){
        power_task_loop();
    }
}

void task_timer(struct task*vp)
{
    g_flag_1s = false;
    count_1s = timer_ct/tcops;
    count_10ms = (timer_ct - tcops*count_1s)/COUNT10MS;
    g_flag_10ms = false;
    if(count_10ms != last_count_10ms){
        g_flag_10ms = true;
    }
    if(count_1s != last_count_1s){
        g_flag_1s = true;
        printf("task timect %u\r\n", cur_task_timeout_ct);
        if(cur_task_timeout_ct > 0){
            cur_task_timeout_ct--;
            if(cur_task_timeout_ct == 0){
                cur_task_event_flag |= 1<<EVENT_UI_TIMEOUT;
            }
            if(current_ui->time_disp_mode & TIME_DISP_EN){
                uint tmp_ct;
                if(current_ui->time_disp_mode & TIME_DISP_LEFT){
                    tmp_ct = cur_task_timeout_ct;
                }
                else{
                    tmp_ct = current_ui->timeout - cur_task_timeout_ct;
                }
                if(current_ui->time_disp_mode & TIME_DISP_SECOND){
                    sprintf(disp_mem+current_ui->time_position_of_dispmem,
                            "%u", tmp_ct);
                }
                else{
                    time_hms(disp_mem+current_ui->time_position_of_dispmem, tmp_ct);
                }
                disp_mem_update = true;
            }
        }
    }
    last_count_1s = count_1s;
    last_count_10ms = count_10ms;
}

void task_disp(struct task*vp)
{
    if(disp_mem_update){
        //saved_timer_ct = timer_ct;
        lcd_update(disp_mem);
        //printf("lcd update %lu\r\n", timer_ct -saved_timer_ct);
        lcd_cursor(cursor_cmd);
        disp_mem_update = false;
    }
}

void task_music(struct task*vp)
{
    uint8 music_note;
    uint music_register_value;
    if(music_task_play_info.pu == NULL ||
            music_task_play_info.music_status != MUSIC_PLAYING ||
            ((timer_ct - music_task_play_info.last_note_start_timerct) < 25*COUNT10MS)
      ){
        return;
    }
    music_led_flash();
    //printf("pu_index %u status %x\r\n", music_task_play_info.pu_index, music_task_play_info.music_status);
    music_note = music_task_play_info.pu[music_task_play_info.pu_index++];
    music_task_play_info.last_note_start_timerct = timer_ct;
    if(music_note==SCORE_END){
        music_task_play_info.music_status = MUSIC_END;
        cur_task_event_flag |= 1<<EVENT_MUSIC_PLAY_END;
        printf("play end\r\n");
        sound_en(0);
    }
    else if(music_note == 0){
        sound_en(0);
    }
    else{
        music_register_value = musical_scale_regv[get_note_index(music_note)];
        //printf("note %x\r\n", music_note);
        sound_en(1);
        update_music_note_register(music_register_value);
    }
}

void pause_music()
{
    if(music_task_play_info.music_status == MUSIC_PLAYING){
        sound_en(0);
        music_task_play_info.music_status = MUSIC_PAUSE;
    }
}

void play_music(__code signed char* pu)
{
    if(music_task_play_info.music_status == MUSIC_PAUSE){
        music_task_play_info.music_status = MUSIC_PLAYING;
    }
    else if(music_task_play_info.music_status == MUSIC_PLAYING){
        music_task_play_info.music_status = MUSIC_PAUSE;
        sound_en(0);
    }
    else{
        music_task_play_info.pu = pu;
        music_task_play_info.pu_index = 0;
        music_task_play_info.last_note_start_timerct = 0;
        music_task_play_info.music_status = MUSIC_PLAYING;
    }
}

void common_ui_init(void*vp)
{
    ui_info* uif =(ui_info*)vp;
    if(uif->timeout == TIMEOUT_INPUT){
        cur_task_timeout_ct = input_timeout;
    }
    else{
        cur_task_timeout_ct = uif->timeout;
    }
    printf("cur task timect---init %x\r\n", cur_task_timeout_ct);
    cur_task_event_flag = 0;
    memset(disp_mem, 0, 33);
    disp_mem_update = true;
}

void ui_transfer(uint8 ui_id)
{
    if(current_ui->ui_quit){
        current_ui->ui_quit(current_ui);
    }
    last_ui_index = cur_ui_index;
    cur_ui_index = ui_id;
    current_ui = &all_ui[cur_ui_index];
    if(current_ui->ui_init){
        current_ui->ui_init(current_ui);
    }
    printf("ui %u->%u\r\n", last_ui_index, ui_id);
}

void common_process_event(void*vp)
{
    //bool dg = g_flag_1s;
    ui_info* uif =(ui_info*)vp;
    for(int8 i = 0; i < EVENT_MAX; i++){
        uint8 evt_flag=1<<i;
        //if(dg) printf("ev flag %x %x i %x\r\n", cur_task_event_flag, evt_flag, i);
        if(cur_task_event_flag & evt_flag){
            if(uif->ui_event_transfer[i]>0){
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
            printf("ev flag %x EVUTO %x\r\n", evt_flag, EVENT_UI_TIMEOUT);
            if(evt_flag == (1<<EVENT_UI_TIMEOUT) && uif->timeout_music){
                play_music(uif->timeout_music);
            }
        }
    }
    cur_task_event_flag = 0;
}

void show_input_timeout()
{
    sprintf(disp_mem+16, "%d", input_timeout);
    time_hms(disp_mem, input_timeout);
    disp_mem_update = true;
}

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
    printf("2---%2.1f\r\n", speed);
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
    }
    else if(g_flag_1s){
        lcj_compute_speed(timer_ct);
        if(speed < last_speed){
            lcj_disp();
            disp_mem_update = true;
            last_speed = speed;
        }
    }
    update_led_lcj();
    common_process_event(vp);
}

void timeout_input_init(void*vp)
{
    ui_info* uif =(ui_info*)vp;
    common_ui_init(vp);
    sprintf(disp_mem+9, "Timeout");
    show_input_timeout();
    ui_common_int = 1;
    cursor_cmd = 0x87;
}

void timeout_input_quit(void*vp)
{
    CDB;
    cursor_cmd = 0;
}

int get_modify_speed(uint i)
{
    if(i<4000)return 1;
    if(i<10000)return 10;
    if(i<50000)return 100;
    return 1000;
}

void timeout_input_process_event(void*vp)
{
    ui_info* uif =(ui_info*)vp;
    if(keyA1_up){
        printf("key A1 up %u\r\n", keyA1_down_ct);
        if(input_timeout>ui_common_int){
            input_timeout-=ui_common_int;
        }
        show_input_timeout();
    }
    //if(keyA2_up){ }
    if(keyA3_up){
        printf("key A3 up %u\r\n", keyA3_down_ct);
        if(65535/2 - input_timeout>ui_common_int){
            input_timeout+=ui_common_int;
        }
        show_input_timeout();
    }
    //if(keyA4_up){ }
    if(keyA1_down_ct>5000){
        if(g_flag_1s){
            if(ui_common_int==3600){
                ui_common_int=1;
                cursor_cmd = 0x87;
            }
            else{
                ui_common_int*=60;
                cursor_cmd -= 3;
            }
        }
    }
    if(keyA3_down_ct>5000){
        if(g_flag_1s){
            if(ui_common_int==1){
                ui_common_int=3600;
                cursor_cmd = 0x81;
            }
            else{
                ui_common_int/=60;
                cursor_cmd += 3;
            }
        }
    }
#if 0
#if 0
    if(keyA1_down_ct == 100 || keyA3_down_ct == 100
            ||keyA1_down_ct == 1000 || keyA3_down_ct == 1000
            ||keyA1_down_ct == 10000 || keyA3_down_ct == 10000)
        printf("%u %u\r\n", keyA1_down_ct, keyA3_down_ct);
#endif
    if(count_10ms%10==0){
        if(keyA1_down_ct>1000){
            ui_common_int = get_modify_speed(keyA1_down_ct);
            //printf("uicint %d %u\r\n", ui_common_int, keyA1_down_ct);
            if(input_timeout>ui_common_int){
                input_timeout-=ui_common_int;
            }
            else{
                input_timeout=0;
            }
            show_input_timeout();
        }
        if(keyA3_down_ct>1000){
            ui_common_int = get_modify_speed(keyA3_down_ct);
            //printf("uicint %d %u\r\n", ui_common_int, keyA3_down_ct);
            if(65535/2 - input_timeout>ui_common_int){
                CDB;
                input_timeout+=ui_common_int;
            }
            else{
                CDB;
                input_timeout=65535/2;
            }
            show_input_timeout();
        }
    }
#endif
    common_process_event(vp);
}

void first_init(void*vp)
{
    common_ui_init(vp);
    sprintf(disp_mem, "%s%s", VERSION, GIT_SHA1);
    sprintf(disp_mem+16, "%s", __TIME__);
    sprintf(disp_mem+21, "%s", __DATE__);
    disp_mem_update = true;
}
void second_init(void*vp)
{
    ui_info* uif =(ui_info*)vp;
    ui_common_uint8 = 1;
    common_ui_init(vp);
    memset(disp_mem, 0, 32);
    sprintf(disp_mem+12, "%u", ui_common_uint8);
    time_hms(disp_mem, uif->timeout);
    disp_mem_update = true;
}

void first_process_event(void*vp)
{
    if(keyA1_up){
        printf("key A1 up\r\n");
        cur_task_timeout_ct += 9;
        play_music(xianglian);
    }
    if(keyA2_up){
        pause_music();
        printf("key A2 up\r\n");
    }
    if(keyA3_up){
        printf("key A3 up\r\n");
    }
    if(keyA4_up)printf("key A4 up\r\n");
    common_process_event(vp);
}

void second_process_event(void*vp)
{
    ui_info* uif =(ui_info*)vp;
    if(keyA1_up){
        printf("key A1 up\r\n");
        play_music(fu);
    }
    if(keyA4_up)printf("key A4 up\r\n");
    if(cur_task_event_flag & (1<<EVENT_UI_TIMEOUT)){
        cur_task_timeout_ct = uif->timeout;
        ui_common_uint8++;
        sprintf(disp_mem+12, "%u", ui_common_uint8);
    }
    common_process_event(vp);
}
__code struct task all_tasks[]=
{
    {
        task_main,
    },
    {
        task_key_status,
    },
    {
        task_timer,
    },
    {
        task_disp,
    },
    {
        task_music,
    },
    {
        task_power,
    },
};

void task_init()
{
    current_ui = &all_ui[0];
    current_ui->ui_init(current_ui);
}

void main()
{
    system_init();
    task_init();
#if 1
    while(1){
        for(int i = 0; i<sizeof(all_tasks)/sizeof(struct task); i++){
            all_tasks[i].t_func(&all_tasks[i]);
        }
    }
#endif
}
