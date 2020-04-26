#ifndef _STC12_DRV_H
#define _STC12_DRV_H

void isr_pca0(void) __interrupt 7 __using 3;
void isr_int1(void) __interrupt 2 __using 2;
void isrtimer0(void) __interrupt 1 __using 1;

void read_cal_data();
void system_init();
void drv_update_key_status();
void drv_power_task_loop();
uint8 get_key_status_raw();
void music_led_flash();
void sound_en(bool en);
void update_led_lcj();
void update_music_note_register(uint v);
void drv_trigger_AD();
bool drv_AD_ready();
uint drv_get_AD_result();

uint8 read_rom(uint addr);
bool write_rom(uint addr, uint8 c);
bool erase_rom(uint addr);
bool write_rom_uint(uint addr, uint data);
uint read_rom_uint(uint addr);
void dump_rom();
void read_cal_data();

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
#endif
