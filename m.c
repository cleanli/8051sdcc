#include <stc12.h>
#include <stdio.h>

#define MS_COUNT 687
void us_delay(unsigned int mt)
{
	while(mt--);
}

void ms_delay(unsigned int mt)
{
	while(mt--)
		us_delay(MS_COUNT);
}

#define STC
void serial_init()
{
	PCON = 0x80;
	SCON = 0x50;
	TMOD = 0x22;
	TH1 = 0xFd;
	TL1 = 0xFd;
	TR1 = 1;
	TI = 1;
	TH0 = 5;
#ifdef STC
	/*baut 9600*/
	//BRT = 0xfd;
	//AUXR = 0x11;

	/*baut 38400*/
	BRT = 0xd7;
	/*AUXR: T0x12 T1x12 UART_M0x6 BRTR ---- BRTx12 XRAM S1BRS*/
	AUXR = 0x15;
	//AUXR1 = 0x80; //mov serial to P1
#endif
	TR0 = 1;
	EA = 1;
	ET0 = 1;
}

int putchar (int c) {
    while (!TI);
    TI = 0;
    SBUF = c;
    return c;
}

void system_init()
{
	serial_init();
	//printf("p4sw is %x\n", P4SW);
	P4SW = 0x70;//open P4 io function for LCD
	ms_delay(1000);
	P1ASF = 0x04;//p12 for ADC
	AUXR1 |= 0x04;
	P0M0 = 0x10;//P04 set to 20mA
}

#define KEY_A1 P3_2
#define KEY_A2 P0_7
#define KEY_A3 P0_6
#define KEY_A4 P0_5
#define LED1 P0_0
#define LED2 P0_1
#define BEEPER P0_4

int main()
{
    unsigned int delayct = 60000;
    system_init();
    LED1 = 0;
    LED2 = 1;
a:
    if(!KEY_A2){
        if(delayct>100)
            delayct-=delayct/100;
        else
            delayct-=1;
        if(delayct==0){
            //printf("delayct = 0\r\n");
            delayct=1;
        }
        //delayct++;
        printf("Key A2 delayct %u\r\n", delayct);
    }
    if(!KEY_A3){
        if(delayct>100)
            delayct+=delayct/100;
        else
            delayct+=1;
        printf("Key A1 delayct %u\r\n", delayct);
    }
    us_delay(delayct);
    LED1 = !LED1;
    LED2 = !LED2;
    BEEPER = !BEEPER;
    goto a;
}
