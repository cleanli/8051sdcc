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
    {
        task_music,
    },
    {
        task_power,
    },
};

void main()
{
    system_init();
    task_init();
    while(1){
        for(int i = 0; i<sizeof(all_tasks)/sizeof(struct task); i++){
            all_tasks[i].t_func(&all_tasks[i]);
        }
    }
}
