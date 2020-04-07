#include <stc12.h>
#include <stdio.h>
#include <string.h>

#define MS_COUNT 1279
typedef unsigned int uint;
typedef unsigned char uint8;
typedef unsigned long ulong;
typedef __bit bool;
volatile unsigned long timer_ct = 0;
__idata unsigned long saved_timer_ct = 0;
__idata unsigned long saved_timer_ct_music = 0;
static __idata unsigned char count_10ms=0;
static __idata unsigned int count_1s=0;
__xdata unsigned char disp_mem[33];
bool flag_10ms = 0, flag_1s = 0;
bool target = 0;
__idata uint target_hour = 0, target_minute = 1;
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
    if(h == target_hour && m == target_minute){
        printf("set target 1 t %u\r\n", t);
        target = 1;
    }
}

void time_flag()
{
    static uint last_count_1s = 0;
	flag_1s = 0;
	flag_10ms = 0;
    count_10ms = (timer_ct-saved_timer_ct)%COUNT10MS;
    count_1s = (timer_ct-saved_timer_ct)/100/COUNT10MS;
    if(count_1s != last_count_1s){
        time_update(count_1s);
        flag_1s = 1;
        lcd_update(disp_mem);
    }
    last_count_1s = count_1s;
    /*
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
    */
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
__code char music[] = {
    //1,1,4,4,5,5,8,8,8,8,0
    1,4,5,8,8,8,8,8,0
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
		printf("%d tk %d\r\n", (int)pu[tk], tk);
        LED1 = !LED1;
        LED2 = !LED2;
		if(!pu[tk])break;
		i = y[pu[tk]];
		saved_timer_ct_music = timer_ct;
		while(1){
			//time_flag();
			if(!KEY_A4){
                return;
			}
			if(i)BEEPER = !BEEPER;
			us_delay(i);
			if(timer_ct - saved_timer_ct_music >= 25*COUNT10MS){
				break;
			}
		}
		if (++tk == 300 )
			tk = 0;
	}
}

float get_power_votage()
{
    float ret;
    uint rs;
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
    rs = (unsigned char)ADC_RES<<2;
    rs |= ADC_RESL;
    printf("ADC_RES %x\r\n", rs);
    printf("ADC_RESL %x\r\n", ADC_RESL);
    ret = 2.45f * 1024 / rs;
    //printf("rs %u\r\n", (int)ret);
	P1ASF &= ~0x04;//p12 recover normal IO
    ADC_CONTR = ADC_CONTR & ~0x80;//power off
    return ret;
}

void disp_power(bool force)
{
    float pv;
	if(!force && !flag_1s)
        return;
    pv = get_power_votage();
    sprintf(disp_mem+27, "%1.2fV", pv);
    lcd_update(disp_mem);
}

void timer_running(__code char* pu, char message_c)
{
    target = 0;
    saved_timer_ct = timer_ct;
    count_1s=0;
    count_10ms=0;
	memset(disp_mem, ' ', 32);
    sprintf(disp_mem, "%u:%02u:00", target_hour, target_minute);
    sprintf(disp_mem+8, "SupplyVo");
    disp_mem[25]=message_c;
	lcd_update(disp_mem);
	while(!target){
		disp_power(0);
		time_flag();
        if(!KEY_A2){
            ms_delay(100);
            if(!KEY_A2){
                sprintf(disp_mem+16, "Cancelled", target_hour, target_minute);
                lcd_update(disp_mem);
                ms_delay(2000);
                break;
            }
        }
	}
    if(target){
        LED1 = 0;
        LED2 = 1;
        memset(disp_mem, ' ', 32);
        strcpy(disp_mem, "Playing music ...");
        lcd_update(disp_mem);
        play_music(pu);
        printf("play end...\n");
    }
}

#define KEY_NONE_A1_DOWN (1<<2)
#define KEY_NONE_A2_DOWN (1<<7)
#define KEY_NONE_A3_DOWN (1<<6)
#define KEY_NONE_A4_DOWN (1<<5)
#define KEY_NONE_DOWN (KEY_NONE_A1_DOWN|KEY_NONE_A2_DOWN|KEY_NONE_A3_DOWN|KEY_NONE_A4_DOWN)
uint8 get_key_status()
{
    uint8 ret1, ret;
    ret1 = (P0 & 0xe0)|(P3 & 0x4);
    //printf("ret1 %x\r\n", ret1);
    if(ret1 != KEY_NONE_DOWN){
        ms_delay(20);
        ret = ((P0 & 0xe0)|(P3 & 0x4));
        if(ret != KEY_NONE_DOWN){
            ret|=ret1;
        }
        if(ret!=KEY_NONE_DOWN){
            if(!(ret & KEY_NONE_A1_DOWN))printf("Key A1 down\r\n");
            if(!(ret & KEY_NONE_A2_DOWN))printf("Key A2 down\r\n");
            if(!(ret & KEY_NONE_A3_DOWN))printf("Key A3 down\r\n");
            if(!(ret & KEY_NONE_A4_DOWN))printf("Key A4 down\r\n");
            return ret;
        }
    }
    return KEY_NONE_DOWN;
}

uint8 key_down_in_time(uint8 timeout)
{
    uint8 ret;
    uint c = timeout * 50;
    do{
        ret = get_key_status();
        if(ret!=KEY_NONE_DOWN){
            return ret;
        }
        ms_delay(20);
    }
    while(c--);
    return KEY_NONE_DOWN;
}

void main()
{
    uint8 uc_tmp;
    bool last_is_hour = 0;
    unsigned int delayct = 600;
    system_init();

    //variable address
    printf("disp_mem %p\r\n",disp_mem);
    printf("timer_ct %p\r\n", timer_ct);
    printf("count_1s %p\r\n", count_1s);

    memset(disp_mem, ' ', 32);
    strcpy(disp_mem, "Press Key in 3 second to LCJ");
    lcd_update(disp_mem);
    uc_tmp = key_down_in_time(3);
    printf("uc_tmp %d", uc_tmp);
    if(uc_tmp==KEY_NONE_DOWN){
        target_minute = 5;
        target_hour = 0;
        memset(disp_mem, ' ', 32);
        strcpy(disp_mem, "Start 3x5min timer");
        lcd_update(disp_mem);
        play_music(music);
        timer_running(music, '1');
        timer_running(music, '2');
        timer_running(music, '3');
    }
    //
    memset(disp_mem, ' ', 32);
    strcpy(disp_mem, "Press Key in 3 second to timer");
    lcd_update(disp_mem);
    ms_delay(1000);
    uc_tmp = key_down_in_time(3);
    if(uc_tmp==KEY_NONE_DOWN){
        //lcj
        target_minute = 0;
        target_hour = 9999;
        saved_timer_ct = timer_ct;
        count_1s=0;
        count_10ms=0;
        memset(disp_mem, ' ', 32);
        strcpy(disp_mem, "LCJ");
        lcd_update(disp_mem);
        while(1){
            disp_power(0);
            time_flag();
            if(KEY_NONE_DOWN!=get_key_status()){
                break;
            }
        }
    }

    //
    ulong last_ct;
start:
    last_ct=timer_ct;
    ms_delay(1000);
    printf("1s %lu\r\n", timer_ct-last_ct);
    last_ct=timer_ct;
    ms_delay(1000);
    printf("1s %lu\r\n", timer_ct-last_ct);
    target_minute = 1;
    target_hour = 0;
	memset(disp_mem, ' ', 32);
	memcpy(disp_mem, "hour  minute    ", 16);
    sprintf(disp_mem+17, "%u", target_hour);
    sprintf(disp_mem+22, "%u", target_minute);
    lcd_update(disp_mem);
    while(1){
        if(!KEY_A4){
            ms_delay(100);
            if(!KEY_A4){
                break;
            }
        }
        if(!KEY_A3){
            ms_delay(100);
            if(!KEY_A3){
                target_minute++;
                if(target_minute == 60)
                    target_minute = 1;
                sprintf(disp_mem+22, "%u", target_minute);
                lcd_update(disp_mem);
                last_is_hour=0;
                ms_delay(500);
            }
        }
        if(!KEY_A1){
            ms_delay(100);
            if(!KEY_A1){
                target_hour++;
                sprintf(disp_mem+17, "%u", target_hour);
                lcd_update(disp_mem);
                last_is_hour=1;
                ms_delay(500);
            }
        }
        if(!KEY_A2){
            ms_delay(100);
            if(!KEY_A2){
                if(last_is_hour){
                    if(target_hour>0){
                        target_hour--;
                        sprintf(disp_mem+17, "%u", target_hour);
                        lcd_update(disp_mem);
                        ms_delay(500);
                    }
                }
                else{
                    if(target_minute>1){
                        target_minute--;
                        sprintf(disp_mem+22, "%u", target_minute);
                        lcd_update(disp_mem);
                        ms_delay(500);
                    }
                }
            }
        }
        ms_delay(50);
        static int l_pc = 0;
        if(l_pc++ > 20){
            disp_power(1);
            l_pc = 0;
        }
    }
    timer_running(fu, ' ');
    goto start;
}

void isrtimer0(void) __interrupt 1 __using 1
{
	timer_ct++;
}
