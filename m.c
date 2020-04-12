#include <stc12.h>
#include <stdio.h>
#include <string.h>

#define VERSION "V0.1"
#define WHEEL_R 197 //mm
#define WHEEL_CIRCUMFERENCE (WHEEL_R*2*3.14159f)
#define MS_COUNT 1279
#define TIMER0_COUNT_PER_SECOND 8296
#define COUNT10MS ((TIMER0_COUNT_PER_SECOND+50)/100)
#define true 1
#define false 0
#define TC0PS_EEROM_ADDR 4
typedef unsigned int uint;
typedef unsigned char uint8;
typedef unsigned long ulong;
typedef __bit bool;
volatile ulong timer_ct = 0;
volatile ulong saved_int_timer_ct = 0;
ulong last_saved_int_timer_ct = 0;
__pdata unsigned long saved_timer_ct = 0;
__pdata unsigned long saved_timer_ct_music = 0;
//__pdata static unsigned char count_10ms=0;
__pdata static unsigned int count_1s=0;
__pdata unsigned char disp_mem[33];
__pdata float mileage;
__pdata float last_speed;
__pdata uint tcops = TIMER0_COUNT_PER_SECOND;
bool flag_10ms = 0, flag_1s = 0;
bool target = 0;
bool disp_left_time=1;
uint target_hour = 0, target_minute = 1;
ulong target_seconds;
void LCD_Init();
void lcd_update(unsigned char*);
uint8 get_key_status_raw();
/*eerom*/
#define READ 1
#define WRITE 2
#define ERASE 3
#define WAIT_TIME 2

uint8 read_rom(uint addr)
{
	uint8 c;
	IAP_ADDRH = addr >> 8;
	IAP_ADDRL = addr & 0xff;
	IAP_CONTR = WAIT_TIME | 0x80;
	IAP_CMD = READ;
	IAP_TRIG = 0x5a;
	IAP_TRIG = 0xa5;
	//printf("r %04x = ", addr);
	c = IAP_DATA;
	//printf("%02x\n", (unsigned int)c);
	return c;
}

bool write_rom(uint addr, uint8 c)
{
	IAP_DATA = c;
	IAP_ADDRH = addr >> 8;
	IAP_ADDRL = addr & 0xff;
	IAP_CONTR = WAIT_TIME | 0x80;
	IAP_CMD = WRITE;
	IAP_TRIG = 0x5a;
	IAP_TRIG = 0xa5;
	//printf("w %04x = %02x\n", addr, (unsigned int)c);
	return 1;
}

bool erase_rom(uint addr)
{
	IAP_ADDRH = addr >> 8;
	IAP_ADDRL = addr & 0xff;
	IAP_CONTR = WAIT_TIME | 0x80;
	IAP_CMD = ERASE;
	IAP_TRIG = 0x5a;
	IAP_TRIG = 0xa5;
	//printf("erase %04x\n", addr & 0xfe00);
	return 1;
}
void dump_rom()
{
	uint addr = 0;
	while(addr < 0x400){
		if((addr & 0x7)== 0)
			printf("\r\n%04x:", addr);
		printf(" %02x", (uint)read_rom(addr));
		addr++;
	}
}
/*eerom*/
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
    EX1 = 1;//P3_3 interrupt enable
    IT1 = 1;//drop edge trigger interrupt
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
    //time calibration
    uint8 tmp8 = read_rom(TC0PS_EEROM_ADDR);
    if(tmp8 != 0xff){
        uint8*ui8p = (uint8*)&tcops;
        *ui8p = tmp8;
        *(ui8p+1)= read_rom(TC0PS_EEROM_ADDR+1);
    }
}

void time_update(unsigned int t)
{
	uint h, m, tm, s;

	h = t / 3600;
	tm = t - h * 3600;
	m = tm / 60;
	s = tm - m * 60;
	sprintf(&disp_mem[16], "%02u:%02u:%02u", (uint)h, (uint)m, (uint)s);
}

void time_flag()
{
    static uint last_count_1s = 0;
	flag_1s = 0;
	flag_10ms = 0;
    count_1s = (timer_ct-saved_timer_ct)/TIMER0_COUNT_PER_SECOND;
    if(count_1s != last_count_1s){
        printf("count 1s: %u\r\n", count_1s);
        if(disp_left_time){
            time_update(target_seconds-count_1s);
        }
        else{
            time_update(count_1s);
        }
        flag_1s = 1;
        lcd_update(disp_mem);
    }
    last_count_1s = count_1s;
}
#define KEY_A1 P3_2
#define KEY_A2 P0_7
#define KEY_A3 P0_6
#define KEY_A4 P0_5
#define LED1 P0_0
#define LED2 P0_1
#define BEEPER P0_4
#define NO_KEY_A1_DOWN (1<<2)
#define NO_KEY_A2_DOWN (1<<7)
#define NO_KEY_A3_DOWN (1<<6)
#define NO_KEY_A4_DOWN (1<<5)
#define NO_KEY_DOWN (NO_KEY_A1_DOWN|NO_KEY_A2_DOWN|NO_KEY_A3_DOWN|NO_KEY_A4_DOWN)

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
    1,1,4,4,5,5,8,8,0
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
			if(get_key_status_raw() != NO_KEY_DOWN){
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
    //printf("ADC_RES %x\r\n", rs);
    //printf("ADC_RESL %x\r\n", ADC_RESL);
    ret = 2.45f * 1024 / rs;
    printf("rs %u\r\n", ret);
	P1ASF &= ~0x04;//p12 recover normal IO
    ADC_CONTR = ADC_CONTR & ~0x80;//power off
    return ret;
}

struct s_lfs_data{
    char*buf;
    float fv;
    uint8 number_int;
    uint8 number_decimal;
    const char*follows;
};

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

void disp_power(bool force)
{
    struct s_lfs_data ld;
    float pv;
	if(!force && !flag_1s)
        return;
    pv = get_power_votage();
    //printf("pv %f\r\n",pv);
    ld.buf=disp_mem+27;
    ld.fv = pv;
    ld.number_int=1;
    ld.number_decimal=2;
    ld.follows="V";
    local_float_sprintf(&ld);
    lcd_update(disp_mem);
}

void timer_running(__code char* pu, char message_c)
{
    target = 0;
    saved_timer_ct = timer_ct;
    target_seconds = target_hour*3600 + target_minute*60;
    count_1s=0;
	memset(disp_mem, ' ', 32);
    sprintf(disp_mem, "%02u:%02u:00", target_hour, target_minute);
    sprintf(disp_mem+10, "SuppVo");
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
        if(count_1s >= target_seconds){
            printf("set target 1 count 1s %u\r\n", count_1s);
            target = 1;
        }
	}
    if(target){
        LED1 = 0;
        LED2 = 1;
        //memset(disp_mem, ' ', 32);
        //strcpy(disp_mem, "Playing music ...");
        //lcd_update(disp_mem);
        play_music(pu);
        printf("play end...\n");
    }
}

uint8 get_key_status_raw()
{
    return (P0 & 0xe0)|(P3 & 0x4);
}

uint8 get_key_status()
{
    uint8 ret1, ret;
    ret1 = (P0 & 0xe0)|(P3 & 0x4);
    //printf("ret1 %x\r\n", ret1);
    if(ret1 != NO_KEY_DOWN){
        ms_delay(20);
        ret = ((P0 & 0xe0)|(P3 & 0x4));
        if(ret != NO_KEY_DOWN){
            ret|=ret1;
        }
        if(ret!=NO_KEY_DOWN){
            if(!(ret & NO_KEY_A1_DOWN))printf("Key A1 down\r\n");
            if(!(ret & NO_KEY_A2_DOWN))printf("Key A2 down\r\n");
            if(!(ret & NO_KEY_A3_DOWN))printf("Key A3 down\r\n");
            if(!(ret & NO_KEY_A4_DOWN))printf("Key A4 down\r\n");
            return ret;
        }
    }
    return NO_KEY_DOWN;
}

uint8 key_down_in_time(uint8 timeout_in_20ms)
{
    uint8 ret;
    uint c = timeout_in_20ms;
    do{
        ret = get_key_status();
        if(ret!=NO_KEY_DOWN){
            return ret;
        }
        ms_delay(20);
    }
    while(c--);
    return NO_KEY_DOWN;
}

void main()
{
    uint8 uc_tmp;
    bool last_is_hour = 0;
    unsigned int delayct = 600;
    system_init();
#if 0
    dump_rom();
    if(read_rom(0x02) == 0xff){
        write_rom(0x02, 0xab);
    }
#endif
    if(get_key_status_raw() != NO_KEY_DOWN){//go test
        bool stop_disp_update = 0;
        memset(disp_mem, '-', 32);
        sprintf(disp_mem, "%s%s", VERSION, GIT_SHA1);
        lcd_update(disp_mem);
        ms_delay(1000);
        while(1){
            if(!stop_disp_update){
                saved_timer_ct = timer_ct;
                sprintf(disp_mem+16, "%lu", saved_timer_ct);
                sprintf(disp_mem+26, "%lu", saved_timer_ct/tcops);
                lcd_update(disp_mem);
            }
            if(get_key_status_raw() != NO_KEY_DOWN){
                ms_delay(400);
                memset(disp_mem, ' ', 16);
                count_1s = (saved_timer_ct+tcops/2)/tcops;
                //stop_disp_update = !stop_disp_update;
                //printf("stop_disp_update %d\r\n", stop_disp_update);
                //ms_delay(500);
disp_c1s:
                sprintf(disp_mem+0, "%lu", saved_timer_ct);
                sprintf(disp_mem+10, "%u", count_1s);
                lcd_update(disp_mem);
                while(key_down_in_time(10)==NO_KEY_DOWN);
                uint8 tmp8 = key_down_in_time(2);
                if((tmp8&NO_KEY_A4_DOWN) == 0){
                    ms_delay(400);
                    uint tmp_tcops = saved_timer_ct/count_1s;
                    memset(disp_mem, ' ', 8);
                    sprintf(disp_mem+0, "%u", tmp_tcops);
                    lcd_update(disp_mem);
                    while(key_down_in_time(10)==NO_KEY_DOWN);
                    if((key_down_in_time(200)&NO_KEY_A4_DOWN) == 0){
                        sprintf(disp_mem+8, "Write do");
                        lcd_update(disp_mem);
                        ms_delay(400);
                        printf("go write rom %u", tmp_tcops);
                    }
                }
                else if((tmp8&NO_KEY_A1_DOWN) == 0){
                    ms_delay(400);
                    count_1s--;
                    goto disp_c1s;
                }
                else if((tmp8&NO_KEY_A3_DOWN) == 0){
                    ms_delay(400);
                    count_1s++;
                    goto disp_c1s;
                }
            }
            ms_delay(400);
        }
    }

    //variable address
    printf("disp_mem %p\r\n",disp_mem);
    printf("timer_ct %p\r\n", &timer_ct);
    printf("count_1s %p\r\n", &count_1s);

    memset(disp_mem, ' ', 32);
    strcpy(disp_mem, "Press Key in 3 second to LCJ");
    lcd_update(disp_mem);
    uc_tmp = key_down_in_time(3*50);
    printf("uc_tmp %d", uc_tmp);
    if(uc_tmp==NO_KEY_DOWN){
        target_minute = 5;
        target_hour = 0;
        memset(disp_mem, ' ', 32);
        strcpy(disp_mem, "Start 3x5min timer");
        lcd_update(disp_mem);
        play_music(music);
        disp_left_time = true;
        timer_running(music, '1');
        timer_running(music, '2');
        timer_running(music, '3');
    }
    //
    memset(disp_mem, ' ', 32);
    strcpy(disp_mem, "Press Key in 3 second to timer");
    lcd_update(disp_mem);
    ms_delay(200);
    uc_tmp = key_down_in_time(3*50);
    if(uc_tmp==NO_KEY_DOWN){
        //lcj
        target_minute = 0;
        target_hour = 9999;
        disp_left_time = false;
        saved_timer_ct = timer_ct;
        count_1s=0;
        memset(disp_mem, ' ', 32);
        strcpy(disp_mem, "LCJ");
        lcd_update(disp_mem);
        while(1){
            struct s_lfs_data ld;
            disp_power(0);
            time_flag();
            if(NO_KEY_DOWN!=get_key_status_raw()){
                break;
            }
            if(last_saved_int_timer_ct != saved_int_timer_ct){
                if(last_saved_int_timer_ct != 0){
                    ulong duration = saved_int_timer_ct - last_saved_int_timer_ct;
                    float speed = (float)WHEEL_CIRCUMFERENCE * TIMER0_COUNT_PER_SECOND / 1000 / (float)duration; //m/s
                    speed = speed * 3600 / 1000;//km/h
                    //printf("saved---%lu\r\n", saved_int_timer_ct);
                    //printf("last saved---%lu\r\n", last_saved_int_timer_ct);
                    printf("1---%2.1f\r\n", speed);
                    if(speed < 100){//abnormal if speed > 100km/h, discard the data
                        last_speed = speed;
                        mileage += WHEEL_CIRCUMFERENCE;
                        //sprintf(disp_mem, "%02.1fkm/h", speed);
                        ld.buf=disp_mem;
                        ld.fv = speed;
                        ld.number_int=2;
                        ld.number_decimal=1;
                        ld.follows="km/h";
                        local_float_sprintf(&ld);
                        if(mileage<1000000){
                            //sprintf(disp_mem+10, "%03.1fm", mileage/1000);
                            ld.buf=disp_mem+10;
                            ld.fv = mileage/1000;
                            ld.number_int=3;
                            ld.number_decimal=1;
                            ld.follows="m";
                            local_float_sprintf(&ld);
                        }
                        else if(mileage < 10000000){
                            //sprintf(disp_mem+10, "%1.2fkm", mileage/1000000);
                            ld.buf=disp_mem+10;
                            ld.fv = mileage/1000000;
                            ld.number_int=1;
                            ld.number_decimal=2;
                            ld.follows="km";
                            local_float_sprintf(&ld);
                        }
                        else{
                            //sprintf(disp_mem+10, "%3.1fkm", mileage/1000000);
                            ld.buf=disp_mem+10;
                            ld.fv = mileage/1000000;
                            ld.number_int=3;
                            ld.number_decimal=1;
                            ld.follows="km";
                            local_float_sprintf(&ld);
                        }
                        lcd_update(disp_mem);
                        last_saved_int_timer_ct = saved_int_timer_ct;
                    }
                }
                else if(saved_int_timer_ct != 0){
                    last_saved_int_timer_ct = saved_int_timer_ct;
                }
            }
            else if(flag_1s){
                ulong duration = timer_ct - last_saved_int_timer_ct;
                //printf("dur %d", duration);
                float speed = (float)WHEEL_CIRCUMFERENCE * TIMER0_COUNT_PER_SECOND / 1000 / (float)duration; //m/s
                //printf("2 m/s---%2.1f\r\n", speed);
                speed = speed * 3600 / 1000;//km/h
                //printf("saved---%lu\r\n", saved_int_timer_ct);
                //printf("last saved---%lu\r\n", last_saved_int_timer_ct);
                printf("2---%2.1f\r\n", speed);
                if(speed < last_speed){
                    //sprintf(disp_mem, "%02.1fkm/h", speed);
                    ld.buf=disp_mem;
                    ld.fv = speed;
                    ld.number_int=2;
                    ld.number_decimal=1;
                    ld.follows="km/h";
                    local_float_sprintf(&ld);
                    lcd_update(disp_mem);
                    last_speed = speed;
                }
            }
            if(P3_3){
                LED1 = 1;
            }
            else{
                LED1 = 0;
            }
        }
    }

    //
    ulong last_ct;
start:
    last_ct=timer_ct;
    ms_delay(200);
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
                ms_delay(100);
            }
        }
        if(!KEY_A1){
            ms_delay(100);
            if(!KEY_A1){
                target_hour++;
                sprintf(disp_mem+17, "%u", target_hour);
                lcd_update(disp_mem);
                last_is_hour=1;
                ms_delay(100);
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
                        ms_delay(100);
                    }
                }
                else{
                    if(target_minute>1){
                        target_minute--;
                        sprintf(disp_mem+22, "%u", target_minute);
                        lcd_update(disp_mem);
                        ms_delay(100);
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
    disp_left_time = true;
    timer_running(fu, ' ');
    goto start;
}

void isr_int1(void) __interrupt 2 __using 2
{
	saved_int_timer_ct = timer_ct;
}

void isrtimer0(void) __interrupt 1 __using 1
{
	timer_ct++;
}
