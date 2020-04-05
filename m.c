#include <stc12.h>
#include <stdio.h>
#include <string.h>

#define MS_COUNT 687
typedef unsigned int uint;
typedef __bit bool;
volatile unsigned int timer_ct = 0;
static unsigned char count_10ms=0;
static unsigned int count_1s=0;
__xdata unsigned char disp_mem[33];
bool flag_10ms = 0, flag_1s = 0;
void LCD_Init();
void lcd_update(unsigned char*);
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
	LCD_Init();
	memcpy(disp_mem, "0123456789abcdef~@#$%^&*()_+|-=\\", 32);
	lcd_update(disp_mem);
	ms_delay(1000);
	//AUXR1 |= 0x04;//high 2 bits of ADC result in ADC_RES
	P0M0 = 0x10;//P04 set to 20mA
}

#define COUNT10MS 83
void time_update(unsigned int t)
{
	uint h, m, tm, s;

	h = t / 3600;
	tm = t - h * 3600;
	m = tm / 60;
	s = tm - m * 60;
	sprintf(&disp_mem[16], "%u:%02u:%02u", (uint)h, (uint)m, (uint)s);
	disp_mem[23] = ' ';
}

void time_flag()
{
	flag_1s = 0;
	flag_10ms = 0;
	if(timer_ct >= COUNT10MS){
		timer_ct = 0;
		count_10ms ++;
		if(count_10ms >= 100 ){
			//if(!stop){
				count_1s++;
				printf("%u\n", count_1s);
				time_update(count_1s);
			//}
			count_10ms = 0;
			flag_1s = 1;
			lcd_update(disp_mem);
		}
	}
}
#define KEY_A1 P3_2
#define KEY_A2 P0_7
#define KEY_A3 P0_6
#define KEY_A4 P0_5
#define LED1 P0_0
#define LED2 P0_1
#define BEEPER P0_4

//-7,1,2,3,4,5,6,7,1-,2-,
__code unsigned int y[16]={1390,
                          1312, 1172, 1044, 985, 877, 781, 696,
						  657, 586, 522, 493, 439, 391, 348, 0};
__code char fu[200] = {5,5,6,6,5,5,8,8,7,7,15,15,5,5,6,6,5,5,9,9,8,8,15,15,
				5,5,12,12,10,10,8,8,7,7,6,6,15,15,11,11,10,10,8,8,9,9,8,8,15,15,15,15,0};
__code char shaolshi[] = {
    5,6,8,8,  8,6,8,8,8,8,8,8,3,8,7,7,7,6,7,7,7,7,7,7,6,3,0
};

void play_music(__code char*pu)
{
    /*
    bit fff;
	unsigned int i = 0;
	unsigned int tk = 0;
    */
    bool fff = 0;
    unsigned int i = 0;
    unsigned int tk = 0;
	while(1){
		printf("%d tk %d\n", (int)pu[tk], tk);
        LED1 = !LED1;
		if(!pu[tk])break;
		i = y[pu[tk]];
		count_10ms = 0;
		while(1){
			time_flag();
			if(!KEY_A1){
				fff = !fff;
				if(!fff)
					P0M0 = 0x10;//P04 set to 20mA
				else
					P0M0 = 0x00;//P04 set to 5mA
			}
			if(i)BEEPER = !BEEPER;
			us_delay(i);
			if(count_10ms == 25){
				count_10ms = 0;
				break;
			}
		}
		if (++tk == 300 )
			tk = 0;
	}
}

unsigned int get_power_votage()
{
    unsigned int ret;
    unsigned char rs;
    //printf("ADC_CONTR %x\r\n", ADC_CONTR);
    //printf("AUXR1 %x\r\n", AUXR1);
    ADC_CONTR = ADC_CONTR | 0x80;//power on adc
    us_delay(1);
	P1ASF |= 0x04;//p12 for ADC
    ADC_CONTR=0x82;//channel p12
    us_delay(1);
    ADC_RES = 0;
    ADC_CONTR|=0x08;//start convert
    us_delay(1);
    //printf("ADC_CONTR %x\r\n", ADC_CONTR);
    while(!(ADC_CONTR & 0x10));
    //printf("ADC_CONTR %x\r\n", ADC_CONTR);
    ADC_CONTR &= ~0x18;//clear start & flag
    rs = (unsigned char)ADC_RES;
    //printf("ADC_RES %x\r\n", rs);
    ret = 5 * 128 * 10 / rs;
    printf("rs %u\r\n", (int)ret);
	P1ASF &= ~0x04;//p12 recover normal IO
    ADC_CONTR = ADC_CONTR & ~0x80;//power off
    return ret;
}

void disp_power()
{
    unsigned int pv;
	if(!flag_1s)
        return;
    pv = get_power_votage();
    sprintf(disp_mem+27, "%u", pv);
    disp_mem[27+2] = disp_mem[27+1];
    disp_mem[27+4] = 'V';
    disp_mem[27+1] = '.';
    lcd_update(disp_mem);
}

void main()
{
    unsigned int delayct = 600;
    system_init();
	memset(disp_mem, ' ', 32);
	strcpy(disp_mem, "Playing music 'Happy Birthday'");
	lcd_update(disp_mem);
    play_music(fu);
	strcpy(disp_mem, "Playing music 'Shao Lin Shi'");
	lcd_update(disp_mem);
    play_music(shaolshi);
    LED1 = 0;
    LED2 = 1;
#if 0
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
        printf("Key A2 delayct %u t %u\r\n", delayct, timer_ct);
    }
    if(!KEY_A3){
        if(delayct>100)
            delayct+=delayct/100;
        else
            delayct+=1;
        printf("Key A1 delayct %u\r\n", delayct);
    }
    ms_delay(delayct);
    LED1 = !LED1;
    LED2 = !LED2;
    pv = get_power_votage();
    sprintf(disp_mem, "%u V", pv);
    disp_mem[2] = disp_mem[1];
    disp_mem[4] = '-';
    disp_mem[1] = '.';
    lcd_update(disp_mem);
    printf("%u V", (int)pv);
    goto a;
#endif
	printf("play end...\n");
	memset(disp_mem, ' ', 32);
	memcpy(disp_mem, " time    power  ", 16);
	lcd_update(disp_mem);
	timer_ct = 0;
	while(1){
		time_flag();
		disp_power();
	}
}

void isrtimer0(void) __interrupt 1 __using 1
{
	timer_ct++;
}
