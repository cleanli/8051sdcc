#include <stdio.h>
#include <string.h>
#include <limits.h>
#include "type.h"
#include "music.h"
#include "common.h"
#include "stc12_drv.h"
#include "ui.h"
#include "task.h"

bool power_meas_trigged = false;
bool stop_feed_wtd = false;
__pdata float power_voltage;
__pdata uint cur_task_timeout_ct;
__pdata uint8 cur_task_event_flag;
__pdata uint last_count_1s = 0;
__pdata uint8 last_count_10ms = 0;
__pdata uint count_1s=0;
__pdata uint8 count_10ms=0;
__pdata uint8 cursor_cmd = 0;
__pdata struct delay_work_info delayed_works[]={
    {
        NULL,
        0,
        NULL
    },
    {
        NULL,
        0,
        NULL
    }
};
#define NUMBER_OF_DELAYED_WORKS \
    (sizeof(delayed_works)/sizeof(struct delay_work_info))

bool keyA1_down = false;
bool keyA2_down = false;
bool keyA3_down = false;
bool keyA4_down = false;
bool keyA1_up = false;
bool keyA2_up = false;
bool keyA3_up = false;
bool keyA4_up = false;
__pdata uint last_keyA1_down_ct;
__pdata uint last_keyA2_down_ct;
__pdata uint last_keyA3_down_ct;
__pdata uint last_keyA4_down_ct;
__pdata uint keyA1_down_ct;
__pdata uint keyA2_down_ct;
__pdata uint keyA3_down_ct;
__pdata uint keyA4_down_ct;
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

void time_hms(char*buf, uint t)
{
    uint h, m, tm, s;

    h = t / 3600;
    tm = t - h * 3600;
    m = tm / 60;
    s = tm - m * 60;
    sprintf(buf, "%02u:%02u:%02u", h, m, s);
}

void task_ui(struct task*vp)
{
    vp;//fix unused variable warning
    current_ui->ui_process_event(current_ui);
}

#define KEY_CONFIRM_TIMER_CT 160
#define CHECK_KEY(AN) \
    if(hw_key_##AN##_down()){ \
        if(key##AN##_down_ct<65535)key##AN##_down_ct++; \
    } \
    else{ \
        if(key##AN##_down==true){ \
            key##AN##_down=false; \
            key##AN##_up=true; \
            last_key##AN##_down_ct = key##AN##_down_ct; \
            cur_task_event_flag |= 1<<EVENT_KEY##AN##_UP; \
        } \
        else{ \
            key##AN##_up=false; \
        } \
        key##AN##_down_ct = 0; \
    } \
    if(key##AN##_down_ct > KEY_CONFIRM_TIMER_CT){ \
        key##AN##_down=true; \
    } \
    else{ \
        key##AN##_down=false; \
    }

void task_key_status(struct task*vp)
{
    vp;//fix unused variable warning
    drv_update_key_status();
    CHECK_KEY(A1)
    CHECK_KEY(A2)
    CHECK_KEY(A3)
    CHECK_KEY(A4)
}

void task_power(struct task*vp)
{
    vp;
    if(current_ui->power_position_of_dispmem<32){
        if(!power_meas_trigged){
            if(g_flag_1s){
                drv_trigger_AD();
                power_meas_trigged = true;
            }
        }
        else{//trigged
            if(drv_AD_ready()){//A/D transfer ready
                uint rs = drv_get_AD_result();
                power_voltage = 2.45f * 1024 / rs;
                //disp power
                float_sprintf.buf=
                    disp_mem+current_ui->power_position_of_dispmem;
                float_sprintf.fv = power_voltage;
                float_sprintf.number_int=1;
                float_sprintf.number_decimal=2;
                float_sprintf.follows="V";
                local_float_sprintf(&float_sprintf);
                power_meas_trigged = false;
                disp_mem_update = true;
            }
        }
    }
}

void task_timer(struct task*vp)
{
    vp;//fix unused variable warning
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
        if(cur_task_timeout_ct > 0 && (current_ui->time_disp_mode & TIME_OUT_EN)){
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
    vp;//fix unused variable warning
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
    vp;//fix unused variable warning
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

bool is_playing_music()
{
    return (music_task_play_info.music_status == MUSIC_PLAYING);
}

void pause_music()
{
    if(music_task_play_info.music_status == MUSIC_PLAYING){
        sound_en(0);
        music_task_play_info.music_status = MUSIC_PAUSE;
    }
}

void continue_music()
{
    if(music_task_play_info.music_status == MUSIC_PAUSE){
        music_task_play_info.music_status = MUSIC_PLAYING;
    }
}

void play_music(__code const signed char* pu)
{
    music_task_play_info.pu = pu;
    music_task_play_info.pu_index = 0;
    music_task_play_info.last_note_start_timerct = 0;
    music_task_play_info.music_status = MUSIC_PLAYING;
}

void set_delayed_work(uint tct, func_p f, void*pa)
{
    for(int8 i = 0; i<NUMBER_OF_DELAYED_WORKS; i++){
        if(delayed_works[i].function == NULL){
            delayed_works[i].function = f;
            delayed_works[i].ct_10ms = tct;
            delayed_works[i].para = pa;
            break;
        }
    }
}

void task_misc(struct task*vp)
{
    if(!stop_feed_wtd){
        feed_watch_dog();
    }
    if(g_flag_10ms){
        for(int8 i = 0; i<NUMBER_OF_DELAYED_WORKS; i++){
            if(delayed_works[i].ct_10ms > 0){
                delayed_works[i].ct_10ms--;
                if(delayed_works[i].ct_10ms==0 && delayed_works[i].function != NULL){
                    delayed_works[i].function(delayed_works[i].para);
                    delayed_works[i].function = NULL;
                }
            }
        }
    }
}

void task_init()
{
    current_ui = &all_ui[0];
    current_ui->ui_init(current_ui);
}
