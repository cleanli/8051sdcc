#include <stc12.h>
#include <stdio.h>
#include <string.h>
#include "type.h"

#define VERSION "0.1"
#define WHEEL_R 197 //mm
#define WHEEL_CIRCUMFERENCE (wheelr*2*3.14159f)
#define MS_COUNT 1279
#define TIMER0_COUNT_PER_SECOND 8296
#define COUNT10MS ((TIMER0_COUNT_PER_SECOND+50)/100)
#define true 1
#define false 0
#define TC0PS_EEROM_ADDR 4
#define WHEEL_R_EEROM_ADDR 6
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
__pdata uint wheelr = WHEEL_R;
bool flag_10ms = 0, flag_1s = 0;
bool target = 0;
bool disp_left_time=1;
uint target_hour = 0, target_minute = 1;
ulong target_seconds;
void LCD_Init();
void lcd_update(unsigned char*);
uint8 get_key_status_raw();
uint8 key_down_in_time(uint8 timeout_in_20ms);
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
    printf("r %04x = ", addr);
    c = IAP_DATA;
    printf("%02x iap_contr %02x\r\n", (unsigned int)c, IAP_CONTR);
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
    printf("w %04x = %02x\r\n", addr, (unsigned int)c);
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
    printf("erase %04x\r\n", addr);
    return 1;
}
bool write_rom_uint(uint addr, uint data)
{
    uint d = data;
    uint8 tmp8_2= read_rom(addr);
    if(tmp8_2 != 0xff){
        erase_rom(addr);
    }
    tmp8_2= read_rom(addr+1);
    if(tmp8_2 != 0xff){
        erase_rom(addr+1);
    }
    uint8*ui8p = (uint8*)&d;
    write_rom(addr, *ui8p);
    write_rom(addr+1, *(ui8p+1));
    return true;
}
uint read_rom_uint(uint addr)
{
    uint d;
    uint8*ui8p = (uint8*)&d;
    *ui8p= read_rom(addr);
    *(ui8p+1)= read_rom(addr+1);
    return d;
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

void pca_init()
{
    CMOD = 0x8c;//stop count in idle;source=fosc/6;disable PCA overflow int
    CCON = 0;//clear flag;
    CL = 0;
    CH = 0;
    CCAP0L = 0x55;
    CCAP0H = 0x05;
    CCAPM0 = 0x49;//PCA module 0 is 16 bit timer;ECCF0=1 enable interrupt
    //CR = 1;//enable counter
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
    //PCA init
    pca_init();
    //time calibration
    uint tmp16 = read_rom_uint(TC0PS_EEROM_ADDR);
    if(tmp16 != 0xffff){
        tcops = tmp16;
    }
    tmp16 = read_rom_uint(WHEEL_R_EEROM_ADDR);
    if(tmp16 != 0xffff){
        wheelr = tmp16;
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
    count_1s = (timer_ct-saved_timer_ct)/tcops;
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

#if 0
float musical_scale_freq[]=
{
    //1-    1-#     2-      2-#     3-      4-      4-#     5-      5-#     6-      6-#     7-
    130.81, 138.59, 146.83, 155.56, 164.81, 174.61, 185.00, 196.00, 207.65, 220.00, 233.08, 246.94,
    //1
    261.63, 277.18, 293.66, 311.13, 329.63, 349.23, 369.99, 392.00, 415.30, 440.00, 466.16, 493.88,
    //1+
    523.25, 554.37, 587.33, 622.25, 659.26, 698.46, 739.99, 783.99, 830.61, 880.00, 932.33, 987.77,
    //1++
    1046.5, 1108.7, 1174.7, 1244.5, 1318.5, 1396.9, 1480.0, 1568.0, 1661.2, 1760.0, 1864.7, 1975.53,
};
#endif

__code uint musical_scale_regv[]=
{
    //1-   1-#    2-     2-#    3-     4-     4-#    5-     5-#    6-    6-#   7-
    15926, 15032, 14189, 13392, 12641, 11931, 11261, 10629, 10033, 9470, 8938, 8437,
    7963,  7516,  7094,  6696,  6320,  5966,  5631,  5315,  5016,  4735, 4469, 4218,
    3982,  3758,  3547,  3348,  3160,  2983,  2815,  2657,  2508,  2367, 2235, 2109,
    1991,  1879,  1774,  1674,  1580,  1491,  1408,  1329,  1254,  1184, 1117, 1055,
};

uint8 get_note_index(signed char value)
{
    uint8 ret;
    bool half_note = false;
    printf("input %d\r\n", value);
    if(value < 0){//half note: #1 #2...
        value = 0 - value;
        half_note = true;
    }
    if(value == 8)value=12;
    if(value == 9)value=22;
    if(value<8){
        value=value*10+1;
    }
    ret = (value%10) * 12;//base
    value /= 10;
    ret += value*2-2;
    if(value>=4)ret--;
    if(half_note)ret++;
    printf("output %u\r\n", ret);
    return ret;
}

#define END 127
//-7,1,2,3,4,5,6,7,1-,2-,
__code unsigned int y[16]={1390,
                          1312, 1172, 1044, 985, 877, 781, 696,
                          657, 586, 522, 493, 439, 391, 348, 0};
__code char fu[200] = {5,5,6,6,5,5,8,8,7,7,0,0,5,5,6,6,5,5,9,9,8,8,0,0,
                5,5,52,52,32,32,8,8,7,7,6,6,0,0,42,42,32,32,8,8,9,9,8,8,0,0,0,0,END};
__code char shaolshi[] = {
    5,6,8,8,8,6,8,8,8,8,8,8,3,8,  7,7,7,6,  7,7,7,7,7,7,6,3,2,2,2,3,  5,5,5,5,6,2,2,60,
    1,1,3,5,6,3,5,5,  3,5,6,3,5,5,50,60,  1,1,1,5,3,3,3,2,  3,3,3,3,3,3,50,60,  1,1,1,5,2,2,2,1,
    2,2,2,2,3,5,  6,6,6,6,6,6,3,5,  1,1,1,3,2,70,60,60,  0,2,2,3,2,2,70,60,  50,50,50,50,50,50,5,6,
    12,12,12,6,   12,12,12,12,12,12,32,12,  7,7,7,6,  7,7,7,7,7,7,6,3,  2,2,2,3,7,7,7,6,  5,5,5,5,5,5,5,6,
    12,12,12,6,   12,12,12,12,12,12,32,12,  7,7,7,6,  7,7,7,7,7,7,6,3,  2,2,2,3,  5,5,5,5,0,2,2,60,  1,1,1,1,1,1,5,6,
    1,1,1,1,1,1,6,3,  2,2,2,3,  5,5,5,5,0,2,2,60,  1,1,1,1,1,1,1,1,  0,0,2,2,2,2,60,60,  1,1,1,1,1,1,1,1,  1,1,
    END
};
__code char music[] = {
    //1,1,4,4,5,5,8,8,8,8,0
    1,1,4,4,5,5,8,8,END
};
__code char testmu[] = {
    //1,1,4,4,5,5,8,8,8,8,0
    1,2,3,4,5,6,7,8,END
};

void play_music(__code char*pu)
{
    /*
    bit fff;
    unsigned int i = 0;
    unsigned int tk = 0;
    */
    CR = 0;//enable counter
    bool fff = 0;
    unsigned int i = 0;
    unsigned int tk = 0;
    while(1){
        printf("%d tk %d\r\n", (int)pu[tk], tk);
        LED1 = !LED1;
        LED2 = !LED2;
        if(pu[tk]==END)break;
        if(pu[tk]==0){
            CR=0;
        }
        else{
            CR=1;
            i = musical_scale_regv[get_note_index(pu[tk])];
            CCAP0L = 0xff & i;
            CCAP0H = i>>8;
        }
        saved_timer_ct_music = timer_ct;
        while(1){
            //time_flag();
            if(get_key_status_raw() != NO_KEY_DOWN){
                return;
            }
            //if(i)BEEPER = !BEEPER;
            us_delay(i);
            if(timer_ct - saved_timer_ct_music >= 25*COUNT10MS){
                break;
            }
        }
        if (++tk == 300 )
            tk = 0;
    }
    CR = 0;
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

void cal_to_rom(uint addr, char*message)
{
    uint dd = 1;
    bool last_isadd = true;
    ms_delay(400);
    memset(disp_mem, ' ', 33);
    uint tmp_data = read_rom_uint(addr);
    sprintf(disp_mem+0, message);
    while(1){
        sprintf(disp_mem+16, "%u", tmp_data);
        lcd_update(disp_mem);
        while(key_down_in_time(10)==NO_KEY_DOWN);
        uint8 tmp8 = key_down_in_time(2);
        if((tmp8&NO_KEY_A4_DOWN) == 0){
            ms_delay(400);
            memset(disp_mem, ' ', 8);
            sprintf(disp_mem+0, "Confirm:");
            lcd_update(disp_mem);
            while(key_down_in_time(10)==NO_KEY_DOWN);
            if((key_down_in_time(200)&NO_KEY_A4_DOWN) == 0){
                ms_delay(400);
                printf("go write rom %u addr %x\r\n", tmp_data, addr);
                write_rom_uint(addr, tmp_data);
                sprintf(disp_mem+5, "Write done");
                lcd_update(disp_mem);
                ms_delay(1000);
                break;
            }
        }
        else if((tmp8&NO_KEY_A1_DOWN) == 0){
            ms_delay(200);
            if(!last_isadd){
                if(dd<30000) dd*=2;
            }
            else{
                dd = 1;
            }
            last_isadd = false;
            tmp_data-=dd;
        }
        else if((tmp8&NO_KEY_A3_DOWN) == 0){
            ms_delay(200);
            if(last_isadd){
                if(dd<30000) dd*=2;
            }
            else{
                dd = 1;
            }
            last_isadd = true;
            tmp_data+=dd;
        }
        else{
            break;
        }
    }
}
/////////////////////////////new architecture///////////////////////////
bool disp_mem_update = false;
struct task;
typedef void (*task_func)(struct task*);
typedef void (*func_p)(void*);
struct task {
    task_func t_func;
    //char flag_1s;
};
typedef struct ui_info_ {
    func_p ui_init;
    func_p ui_process_event;
    func_p ui_quit;
} ui_info;

void first_init(void*vp)
{
    sprintf(disp_mem, "%s%s", VERSION, GIT_SHA1);
    sprintf(disp_mem+16, "%s", __TIME__);
    sprintf(disp_mem+21, "%s", __DATE__);
    disp_mem_update = true;
}
void first_process_event(void*vp)
{
}
ui_info all_ui[]={
    {
        first_init,
        first_process_event,
        NULL,
    },
};

ui_info* current_ui;

bool g_flag_1s = false;
void task_main(struct task*vp)
{
    current_ui->ui_process_event(NULL);
}

void task_key_status(struct task*vp)
{
    if(get_key_status_raw()!=NO_KEY_DOWN
            && g_flag_1s)
        printf("key pressed\r\n");
}

void task_timer(struct task*vp)
{
    static uint last_count_1s = 0;
    g_flag_1s = false;
    count_1s = timer_ct/tcops;
    if(count_1s != last_count_1s){
        g_flag_1s = true;
    }
    last_count_1s = count_1s;
}

void task_disp(struct task*vp)
{
    if(disp_mem_update){
        saved_timer_ct = timer_ct;
        lcd_update(disp_mem);
        printf("lcd update %lu\r\n", timer_ct -saved_timer_ct);
        disp_mem_update = false;
    }
}

struct task all_tasks[]=
{
    {
        task_main,
    },
    {
        task_key_status,
    },
    {
        task_timer,
    },
    {
        task_disp,
    },
};

void task_init()
{
    current_ui = &all_ui[0];
    current_ui->ui_init(NULL);
}

void main()
{
    system_init();
    task_init();
#if 1
    while(1){
        for(int i = 0; i<sizeof(all_tasks)/sizeof(struct task); i++){
            all_tasks[i].t_func(&all_tasks[i]);
        }
    }
#endif
}

void isr_pca0(void) __interrupt 7 __using 3
{
    //printf("ccon %02x\r\n", CCON);//0x41
    CCF0 = 0;
    CH = 0;
    CL = 0;
    BEEPER = !BEEPER;
}

void isr_int1(void) __interrupt 2 __using 2
{
    saved_int_timer_ct = timer_ct;
}

void isrtimer0(void) __interrupt 1 __using 1
{
    timer_ct++;
}
