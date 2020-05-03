#ifndef _COMMON_H
#define _COMMON_H

#define CDB printf("line%d\r\n", __LINE__)
#define VERSION "2.1"
#define WHEEL_R 197 //mm
#define WHEEL_CIRCUMFERENCE (wheelr*2*3.14159f)
#define TIMER0_COUNT_PER_SECOND 8303

struct s_lfs_data{
    char*buf;
    float fv;
    uint8 number_int;
    uint8 number_decimal;
    const char*follows;
};
extern __pdata struct s_lfs_data float_sprintf;
void local_float_sprintf(struct s_lfs_data* lfsd);

bool LCD_Init();
void lcd_update(unsigned char*);
void lcd_cursor(uint8 d);

extern __pdata unsigned char disp_mem[33];

#endif
