#ifndef _STC12_DRV_H
#define _STC12_DRV_H

#define MS_COUNT 1279
#define COUNT10MS ((tcops+50)/100)
#define TC0PS_EEROM_ADDR 4
#define WHEEL_R_EEROM_ADDR 6

extern __pdata uint tcops;
extern __pdata uint wheelr;
extern bool reset_flag;

extern volatile ulong timer_ct;
extern volatile ulong saved_int_timer_ct;
void isr_pca0(void) __interrupt 7 __using 3;
void isr_int1(void) __interrupt 2 __using 2;
void isrtimer0(void) __interrupt 1 __using 1;

void read_cal_data();
void save_cal_data(uint, uint);
void feed_watch_dog();
void system_init();
void drv_update_key_status();
bool hw_key_A1_down();
bool hw_key_A2_down();
bool hw_key_A3_down();
bool hw_key_A4_down();
void drv_power_task_loop();
uint8 get_key_status_raw();
void sound_en(bool en);
void update_music_note_register(uint v);
void drv_trigger_AD();
bool drv_AD_ready();
uint drv_get_AD_result();
void set_led1(bool light);
void set_led2(bool light);
bool get_led1();
bool get_led2();
bool get_lcj_signal();
void power_off();

uint8 read_rom(uint addr);
bool write_rom(uint addr, uint8 c);
bool erase_rom(uint addr);
bool write_rom_uint(uint addr, uint data);
uint read_rom_uint(uint addr);
void dump_rom();
void read_cal_data();

#endif
