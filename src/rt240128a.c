#include "rt240128a.h"
#include <stc12.h>
#include "type.h"
#include <stdio.h>
#include <string.h>
#include "stc12_drv.h"
#include "common.h"

#if 0
#define DBG(fmt,arg...)
#else
#define DBG printf
#endif
void us_delay(unsigned int mt);
void ms_delay(unsigned int mt);

#if 0//old test board
#define RT21_RESET P4_5 
#define RT21_CMD_DATA P4_6 
#define RT21_WR P0_5 
#define RT21_RD P0_6 
#define RT21_EN P0_7 
uint8 read_cmd(bool cmd)
{
    uint8 ret=0;
    uint8 vp2,vp4;
    P3_7 = 0;
    RT21_EN = 1;
    RT21_RD = 1;
    RT21_WR = 1;
    RT21_CMD_DATA = cmd;//cmd
    us_delay(1);
    P2 = 0xff;
    P4_4 = 1;
    RT21_EN = 0;
    RT21_RD = 0;
    us_delay(1);
    vp2 = P2;
    vp4 = P4;
    RT21_RD = 1;
    RT21_EN = 1;
    DBG("P2 %02x\r\n", vp2);
    DBG("P4 %02x\r\n", vp4);
    ret |= (vp4&0x10)>>4;
    ret |= (vp2&0x80)>>6;
    ret |= (vp2&0x40)>>4;
    ret |= (vp2&0x20)>>2;
    ret |= (vp2&0x10);
    ret |= (vp2&0x08)<<2;
    ret |= (vp2&0x04)<<4;
    ret |= (vp2&0x02)<<6;
    if(cmd)DBG("rcmd %02x\r\n", ret);
    else DBG("rdat %02x\r\n", ret);
    return ret;
}

void write_cmd(uint8 c, bool cmd)
{
    uint8 ret=0;
    uint8 vp2,vp4;
    P3_7 = 0;
    RT21_EN = 1;
    RT21_RD = 1;
    RT21_WR = 1;
    RT21_CMD_DATA = cmd;//cmd
    //setup data
    P4_4 = c&0x01;
    P2_1 = c&0x80;
    P2_2 = c&0x40;
    P2_3 = c&0x20;
    P2_4 = c&0x10;
    P2_5 = c&0x08;
    P2_6 = c&0x04;
    P2_7 = c&0x02;
    us_delay(1);
    RT21_EN = 0;
    RT21_WR = 0;
    us_delay(1);
    RT21_WR = 1;
    RT21_EN = 1;
    vp2 = P2;
    vp4 = P4;
    DBG("P2 %02x\r\n", vp2);
    DBG("P4 %02x\r\n", vp4);
    if(cmd)DBG("wcmd %02x\r\n", c);
    else DBG("wdat %02x\r\n", c);
}
#endif

#define RT21_RESET P4_5
#define RT21_CMD_DATA P4_6
#define RT21_WR P1_1
#define RT21_RD P1_6
#define RT21_EN P4_4
#define RT21_FS P1_5
#define LCD_Out P2
#define LCD_In P2

uint8 read_cmd(bool cmd)
{
    uint8 ret=0;
    RT21_EN = 1;
    RT21_RD = 1;
    RT21_WR = 1;
    RT21_CMD_DATA = cmd;//cmd
    us_delay(1);
    P2 = 0xff;
    RT21_EN = 0;
    RT21_RD = 0;
    us_delay(1);
    ret = P2;
    RT21_RD = 1;
    RT21_EN = 1;
    if(cmd)DBG("rcmd %02x\r\n", ret);
    else DBG("rdat %02x\r\n", ret);
    return ret;
}

void write_cmd(uint8 c, bool cmd)
{
    uint8 ret=0;
    RT21_EN = 1;
    RT21_RD = 1;
    RT21_WR = 1;
    RT21_CMD_DATA = cmd;//cmd
    //setup data
    P2 = c;
    us_delay(1);
    RT21_EN = 0;
    RT21_WR = 0;
    us_delay(1);
    RT21_WR = 1;
    RT21_EN = 1;
    if(cmd)DBG("wcmd %02x\r\n", c);
    else DBG("wdat %02x\r\n", c);
}

void lcd_rt240128a_init()
{
    CDB;
    while(1){
        DBG("stat %02x\r\n", read_cmd(true));
        ms_delay(1000);
    }
}
