#include <stdio.h>
#include <string.h>
#include <limits.h>
#include "type.h"
#include "music.h"
#include "common.h"
#include "stc12_drv.h"
#include "ui.h"
#include "task.h"

__code struct task all_tasks[]=
{
    {
        task_ui,
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
    {
        task_music,
    },
    {
        task_power,
    },
    {
        task_lcd_bklight,
    },
    {
        task_misc,
    },
};

void main()
{
    system_init();
    ui_start();
    while(1){
        for(int i = 0; i<sizeof(all_tasks)/sizeof(struct task); i++){
            all_tasks[i].t_func(&all_tasks[i]);
        }
    }
}
