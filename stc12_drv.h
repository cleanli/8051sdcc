#ifndef _STC12_DRV_H
#define _STC12_DRV_H

void isr_pca0(void) __interrupt 7 __using 3;
void isr_int1(void) __interrupt 2 __using 2;
void isrtimer0(void) __interrupt 1 __using 1;

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
#endif
