#include <stc12.h>
#include "type.h"
#include <stdio.h>
#include <string.h>
#include "common.h"

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

uint8 get_key_status_raw()
{
    return (P0 & 0xe0)|(P3 & 0x4);
}

#define KEY_CONFIRM_TIMER_CT 160
#define CHECK_KEY(AN) \
    if((ksts&NO_KEY_##AN##_DOWN) == 0){ \
        if(key##AN##_down_ct<65535)key##AN##_down_ct++; \
    } \
    else{ \
        key##AN##_down_ct = 0; \
        if(key##AN##_down==true){ \
            key##AN##_down=false; \
            key##AN##_up=true; \
            cur_task_event_flag |= 1<<EVENT_KEY##AN##_UP; \
        } \
        else{ \
            key##AN##_up=false; \
        } \
    } \
    if(key##AN##_down_ct > KEY_CONFIRM_TIMER_CT){ \
        key##AN##_down=true; \
    } \
    else{ \
        key##AN##_down=false; \
    }
void update_key_status()
{
    uint8 ksts = get_key_status_raw();
    CHECK_KEY(A1)
    CHECK_KEY(A2)
    CHECK_KEY(A3)
    CHECK_KEY(A4)
}

void power_task_loop()
{
    if(!power_meas_trigged){
        if(g_flag_1s){
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
            power_meas_trigged = true;
        }
    }
    else{//trigged
        if(ADC_CONTR & 0x10){//A/D transfer ready
            uint rs;
            //printf("ADC_CONTR %x\r\n", ADC_CONTR);
            ADC_CONTR &= ~0x18;//clear start & flag
            rs = (unsigned char)ADC_RES<<2;
            rs |= ADC_RESL;
            //printf("ADC_RES %x\r\n", rs);
            //printf("ADC_RESL %x\r\n", ADC_RESL);
            power_voltage = 2.45f * 1024 / rs;
            P1ASF &= ~0x04;//p12 recover normal IO
            ADC_CONTR = ADC_CONTR & ~0x80;//power off
            power_meas_trigged = false;
            //disp power
            float_sprintf.buf=
                disp_mem+current_ui->power_position_of_dispmem;
            float_sprintf.fv = power_voltage;
            float_sprintf.number_int=1;
            float_sprintf.number_decimal=2;
            float_sprintf.follows="V";
            local_float_sprintf(&float_sprintf);
            disp_mem_update = true;
        }
    }
}

void music_led_flash()
{
    LED1 = !LED1;
    LED2 = !LED1;
}

void sound_en(bool en)
{
    CR = en;
}

void update_freq(uint v)
{
    CCAP0L = 0xff & v;
    CCAP0H = v>>8;
}

void update_led_lcj()
{
    if(P3_3){
        LED1 = 1;
    }
    else{
        LED1 = 0;
    }
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
