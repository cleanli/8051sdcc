#include "stc12.h"
#include <stdio.h>
// this file for MCU I/O port or the orther`s hardware config
// for LCD Display
// Define for the port use by LCD Driver
#define LCD_EP P4_4
#define LCD_RW P4_5
#define LCD_RS P4_6
#define LCD_Data_BUS_Out P2
#define LCD_Data_BUS_In P2
__code unsigned char LCD_InitialCode[]={0x30,0x30,0x30,0x38,0x01,0x06,0x0c};

unsigned char sw_byte(unsigned char a)
{
/*
	unsigned char b=0,i;
	for (i=0;i<8;i++){
		b<<=1;
		b += a & 1;
		a>>=1;
	}
	return b;
*/
	return a;
}
//========================================================================
// 函数: void LCD_DataWrite(unsigned char Data)
// 描述: 写一个字节的显示数据至LCD中的显示缓冲RAM当中
// 参数: Data 写入的数据
// 返回: 无
//========================================================================
void LCD_DataWrite(unsigned char Data)
{
unsigned int Read_Dat=0;
LCD_EP = 0; //EP、RS端口为低，RW为高
LCD_RS = 0;
LCD_RW = 1;
do{
LCD_Data_BUS_In = 0xff;
LCD_EP = 1;
Read_Dat = sw_byte(LCD_Data_BUS_In)&0x80;
LCD_EP = 0;
}while(Read_Dat!=0); //读状态字并判断是否可进行读写操作
LCD_RW = 0; //EP RW to Low
LCD_RS = 1; //RS Hight
LCD_Data_BUS_Out = sw_byte(Data);
LCD_EP = 1; //EP to Hight
LCD_EP = 0; //EP to low
}
//========================================================================
// 函数: void LCD_RegWrite(unsigned char Command)
// 描述: 写一个字节的数据至LCD中的控制寄存器当中
// 参数: Command 写入的数据（byte）
// 返回: 无
//========================================================================
void LCD_RegWrite(unsigned char Command)
{
unsigned int Read_Dat=0;
LCD_EP = 0; //EP、RS置低，RW置高，表为读状态字
LCD_RS = 0;
LCD_RW = 1;
do{
LCD_Data_BUS_In = 0xff;
LCD_EP = 1;
Read_Dat = sw_byte(LCD_Data_BUS_In)&0x80;
LCD_EP = 0;
}while(Read_Dat!=0); //读状态字并判断是否可进行读写操作
LCD_RW = 0; //RW to Low，表为写指令
LCD_Data_BUS_Out = sw_byte(Command);
LCD_EP = 1; //EP to Hight
LCD_EP = 0;
}
//========================================================================
// 函数: unsigned char LCD_DataRead(void)
// 描述: 从LCD中的显示缓冲RAM当中读一个字节的显示数据
// 参数: 无
// 返回: 读出的数据，低八位有效（byte）
//========================================================================
unsigned char LCD_DataRead(void)
{
unsigned char Read_Dat=0;
LCD_EP = 0; //EP、RS置低，RW置高，表为读状态字
LCD_RS = 0;
LCD_RW = 1;
do{
LCD_Data_BUS_In = 0xff;
LCD_EP = 1;
Read_Dat = sw_byte(LCD_Data_BUS_In)&0x80;
LCD_EP = 0;
}while(Read_Dat!=0); //读状态字并判断是否可进行读写操作
LCD_RS = 1; //RS置高，表为读数据
LCD_EP = 1; //EP to Hight
Read_Dat = sw_byte(LCD_Data_BUS_In); //读出数据
LCD_EP = 0;
return Read_Dat;
}
//========================================================================
// 函数: unsigned char LCD_StatusRead(void)
// 描述: 从LCD中的显示缓冲RAM当中读一个字节的显示数据
// 参数: 无
// 返回: 读出的数据，低八位有效（byte）
//========================================================================
unsigned char LCD_StatusRead(void)
{
unsigned char Read_Dat=0;
LCD_EP = 0; //EP、RS置低，RW置高，表为读状态字
LCD_RS = 0;
LCD_RW = 1;
LCD_Data_BUS_In = 0xff;
LCD_EP = 1;
Read_Dat = sw_byte(LCD_Data_BUS_In); //读状态字
LCD_EP = 0;
return Read_Dat;
}
//========================================================================
// 函数: void LCD_Init(void)
// 描述: LCD初始化程序，在里面会完成LCD初始所需要设置的许多寄存器，具体如果
// 用户想了解，建议查看DataSheet当中各个寄存器的意义
// 参数: 无
// 返回: 无
// 备注:
// 版本:
// 2007/11/14 First version
//========================================================================
//延时程序

#define Nop __asm \
nop \
__endasm

void TimeDelay(int Time)
{
int i;
while(Time > 0)
{
for(i = 0;i < 800;i++)
{
Nop;
}
Time --;
}
}
void LCD_Init(void)
{
unsigned char uiTemp=0,i;
unsigned char * Point;
//LCD驱动所使用到的端口的初始化
Point = (unsigned char *)LCD_InitialCode; //获取初始化序列数据的首地址
LCD_EP = 0;
LCD_RS = 0;
LCD_RW = 0;
for(i=0;i<4;i++)
{
uiTemp = *Point++;
LCD_Data_BUS_Out = sw_byte(uiTemp);
LCD_EP = 1; //EP to Hight
TimeDelay(4); //延一定的时间，一般要求4.5ms以上就可以，没有那么严格的了
LCD_EP = 0; //EP to Hight
}
LCD_RegWrite(*Point++);
LCD_RegWrite(*Point++);
LCD_RegWrite(*Point++);
}
void lcd_update(unsigned char * s)
{
unsigned char uiTemp=0;
unsigned char * String_s;

uiTemp = LCD_StatusRead(); //无意义，只是测试读状态字的子程序
//printf("read %02x\n", (int)uiTemp);
LCD_RegWrite(0x80); //设置地址为第一行第一个字符的位置
uiTemp = 16;
String_s = s;
while(uiTemp--) //显示字符串
{
if(*String_s == '\n'){
	String_s++;
	break;
}
if(*String_s == 0)*String_s = ' ';
LCD_DataWrite(*String_s);
String_s++;
}
uiTemp = 16;
LCD_RegWrite(0xc0); //设置地址为第二行第一个字符的位置
while(uiTemp--)
{
LCD_DataWrite(*String_s);
String_s++;
}
//以下仅为测试使用，测试读数据程序的功能
uiTemp = LCD_DataRead(); //读数据
//printf("read %02x\n", (int)uiTemp);
LCD_RegWrite(0x80); //设置地址后再读数据
uiTemp = LCD_DataRead();
//printf("read %02x\n", (int)uiTemp);
}
