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
bool music_flash = false;
bool cur_task_timer_started = false;
bool g_flag_1s = false;
bool g_flag_10ms = false;
__pdata float power_voltage;
__pdata uint cur_task_timeout_ct;
__pdata uint8 cur_task_event_flag;
__pdata uint last_count_1s = 0;
__pdata uint8 last_count_10ms = 0;
__pdata uint count_1s=0;
__pdata uint8 count_10ms=0;
__pdata uint8 cursor_cmd = 0;
__pdata uint default_music_note_period = DEFAULT_MUSIC_NOTE_PERIOD;
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
bool save_power_mode = false;
__pdata uint last_keyA1_down_ct;
__pdata uint last_keyA2_down_ct;
__pdata uint last_keyA3_down_ct;
__pdata uint last_keyA4_down_ct;
__pdata uint keyA1_down_ct;
__pdata uint keyA2_down_ct;
__pdata uint keyA3_down_ct;
__pdata uint keyA4_down_ct;
__pdata uint8 keyA1_up_ct;
__pdata uint8 keyA2_up_ct;
__pdata uint8 keyA3_up_ct;
__pdata uint8 keyA4_up_ct;
__pdata uint no_key_down_ct = 0;
__pdata uint no_key_down_ct_lcd = 0;
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
        if(key##AN##_up_ct<255)key##AN##_up_ct++; \
        if(key##AN##_up_ct > KEY_CONFIRM_TIMER_CT){ \
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
    } \
    if(key##AN##_down_ct > KEY_CONFIRM_TIMER_CT){ \
        if(key##AN##_down==false){ \
            key##AN##_up_ct = 0; \
            key##AN##_down=true; \
        } \
    } \
    else{ \
        key##AN##_down=false; \
    }

void enable_power_save(bool en)
{
    save_power_mode = en;
    enable_lcd_bklight(!en);
}

void task_key_status(struct task*vp)
{
    vp;//fix unused variable warning
    drv_update_key_status();
    if(!hw_no_key_down()){
        no_key_down_ct = 0;
        no_key_down_ct_lcd = 0;
        enable_power_save(false);
    }
    else if(g_flag_1s){
        no_key_down_ct++;
        if(save_power_mode){
            no_key_down_ct_lcd++;
        }
        //printf("nokeydown %u\r\n", no_key_down_ct);
        if(no_key_down_ct > NO_KEY_DOWN_PWSAVE_MAX){
            if(!save_power_mode){
                enable_power_save(true);
            }
        }
        if(no_key_down_ct > NO_KEY_DOWN_CT_MAX){
            cur_task_event_flag |= 1<<EVENT_NOKEYCT_MAXREACHED;
            no_key_down_ct = 0;
        }
    }
#if 0
    if(g_flag_1s){
        printf("%u\r\n", keyA1_down_ct);
        printf("%u\r\n", keyA1_up_ct);
    }
#endif
    CHECK_KEY(A1)
    CHECK_KEY(A2)
    CHECK_KEY(A3)
    CHECK_KEY(A4)
}

void task_lcd_bklight(struct task*vp)
{
    vp;//fix unused variable warning
    if(save_power_mode){
        if(get_lcd_bklight()){
            if(no_key_down_ct_lcd > (LCD_POWER_SAVE_CYCLE/LCD_POWER_SAVE_RATIO)){
                printf("lcd_\r\n");
                no_key_down_ct_lcd = 0;
                toggle_lcd_bklight();
            }
        }
        else{
            if(no_key_down_ct_lcd > (LCD_POWER_SAVE_CYCLE)){
                printf("lcd^\r\n");
                no_key_down_ct_lcd = 0;
                toggle_lcd_bklight();
            }
        }
    }
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
        //printf("task timect %u\r\n", cur_task_timeout_ct);
        if(cur_task_timeout_ct > 0 && (current_ui->time_disp_mode & TIME_OUT_EN)){
            if(cur_task_timer_started){
                cur_task_timeout_ct--;
                if(cur_task_timeout_ct == 0){
                    cur_task_event_flag |= 1<<EVENT_UI_TIMEOUT;
                }
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
    if(lcd_detected && disp_mem_update){
        //saved_timer_ct = timer_ct;
        lcd_update(disp_mem);
        //printf("lcd update %lu\r\n", timer_ct -saved_timer_ct);
        lcd_cursor(cursor_cmd);
        disp_mem_update = false;
    }
}

void reset_music_note()
{
    music_note_task_play_info.music_note = 0;
    music_note_task_play_info.note_start_timerct = 0;
    music_note_task_play_info.period_ms_ct = 0;
}

bool is_music_idle()
{
    return (music_note_task_play_info.period_ms_ct == 0);
}

bool play_music_note(int8 note, uint period)
{
    if(music_note_task_play_info.period_ms_ct == 0){
        music_note_task_play_info.music_note = note;
        music_note_task_play_info.note_start_timerct = 0;
        music_note_task_play_info.period_ms_ct = period;
        return true;
    }
    return false;
}

void task_music(struct task*vp)
{
    vp;//fix unused variable warning
    int8 music_note;
    uint music_register_value;
    if(music_task_play_info.pu != NULL &&
            music_task_play_info.music_status == MUSIC_PLAYING &&
            is_music_idle()){
        music_flash = !music_flash;
        set_led1(music_flash);
        set_led2(!music_flash);
        //printf("pu_index %u status %x\r\n", music_task_play_info.pu_index, music_task_play_info.music_status);
        music_note = music_task_play_info.pu[music_task_play_info.pu_index++];
        if(music_note==SCORE_END){
            music_task_play_info.music_status = MUSIC_END;
            cur_task_event_flag |= 1<<EVENT_MUSIC_PLAY_END;
            set_led1(false);
            set_led2(false);
            printf("play end\r\n");
            sound_en(0);
            set_music_note_period(DEFAULT_MUSIC_NOTE_PERIOD);//recover default note period
        }
        else if(music_note==HALF_PERIOD){
            default_music_note_period /= 2;
        }
        else if(music_note==DOUBLE_PERIOD){
            default_music_note_period *= 2;
        }
        else if(music_note==DIVERT){
            if(music_task_play_info.divert_index != NO_DIVERT){
                music_task_play_info.pu_index = music_task_play_info.divert_index;
                music_task_play_info.divert_index = NO_DIVERT;
            }
        }
        else if(music_note==GOSTART){
                music_task_play_info.divert_index = music_task_play_info.pu_index;
                music_task_play_info.pu_index = music_task_play_info.restart_index;
        }
        else if(music_note==FLAGSTART){
            music_task_play_info.restart_index = music_task_play_info.pu_index;
        }
        else{
            play_music_note(music_note, default_music_note_period);
        }

    }

    if(music_note_task_play_info.period_ms_ct){
        if(music_note_task_play_info.note_start_timerct == 0){
            music_note_task_play_info.note_start_timerct = timer_ct;
            if(music_note_task_play_info.music_note == 0){
                sound_en(0);
            }
            else{
                music_register_value =
                    musical_scale_regv[get_note_index(music_note_task_play_info.music_note)];
                sound_en(1);
                update_music_note_register(music_register_value);
            }
        }
        else if((timer_ct - music_note_task_play_info.note_start_timerct) >=
                (ulong)music_note_task_play_info.period_ms_ct*COUNT10MS/10){
            music_note_task_play_info.period_ms_ct = 0;
            if(!is_playing_music()){
                sound_en(0);
            }
        }
    }
    else{
        sound_en(0);
    }
}

void set_music_note_period(uint p)
{
    default_music_note_period = p;
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

void play_music(__code const signed char* pu, uint note_period)
{
    music_task_play_info.pu = pu;
    music_task_play_info.pu_index = 0;
    music_task_play_info.music_status = MUSIC_PLAYING;
    music_task_play_info.divert_index = NO_DIVERT;
    music_task_play_info.restart_index = 0;
    if(note_period == 0){
        note_period = DEFAULT_MUSIC_NOTE_PERIOD;
    }
    set_music_note_period(note_period);
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
    vp;//fix unused variable warning
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
