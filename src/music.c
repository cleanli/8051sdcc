#include <stdio.h>
#include <string.h>
#include <limits.h>
#include "type.h"
#include "common.h"
#include "music.h"

__pdata struct music_note_play_info music_note_task_play_info={
    0,
    0,
    0
};

__pdata struct music_play_info music_task_play_info={
    NULL,
    0,
    0,
    MUSIC_IDLE
};

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
    //1+++
    2093.0, 2217.5, 2349.3, 2489.0, 2637.0, 2793.8, 2960.0, 3136.0, 3322.4, 3520.0, 3729.3, 3951.05,
};
#endif

__code const uint musical_scale_regv[]=
{
    //1-   1-#    2-     2-#    3-     4-     4-#    5-     5-#    6-    6-#   7-
    //15926, 15032, 14189, 13392, 12641, 11931, 11261, 10629, 10033, 9470, 8938, 8437,
    7963,  7516,  7094,  6696,  6320,  5966,  5631,  5315,  5016,  4735, 4469, 4218,
    3982,  3758,  3547,  3348,  3160,  2983,  2815,  2657,  2508,  2367, 2235, 2109,
    1991,  1879,  1774,  1674,  1580,  1491,  1408,  1329,  1254,  1184, 1117, 1055,
    995,   939,   887,   837,   790,   746,   704,   664,   627,   592,  559,  527,
};

uint8 get_note_index(signed char value)
{
    uint8 ret;
    bool half_note = false;
    //printf("input %d\r\n", value);
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
    //printf("output %u\r\n", ret);
    return ret;
}

__code const signed char fu[200] = {5,5,6,6,5,5,8,8,7,7,0,0,5,5,6,6,5,5,9,9,8,8,0,0,
                5,5,52,52,32,32,8,8,7,7,6,6,0,0,42,42,32,32,8,8,9,9,8,8,0,0,0,0,SCORE_END};
__code const signed char shaolshi[] = {
    5,6,8,8,8,6,8,8,8,8,8,8,3,8,  7,7,7,6,  7,7,7,7,7,7,6,3,2,2,2,3,  5,5,5,5,6,2,2,60,
    1,1,3,5,6,3,5,5,  3,5,6,3,5,5,50,60,  1,1,1,5,3,3,3,2,  3,3,3,3,3,3,50,60,  1,1,1,5,2,2,2,1,
    2,2,2,2,3,5,  6,6,6,6,6,6,3,5,  1,1,1,3,2,70,60,60,  0,2,2,3,2,2,70,60,  50,50,50,50,50,50,5,6,
    12,12,12,6,   12,12,12,12,12,12,32,12,  7,7,7,6,  7,7,7,7,7,7,6,3,  2,2,2,3,7,7,7,6,  5,5,5,5,5,5,5,6,
    12,12,12,6,   12,12,12,12,12,12,32,12,  7,7,7,6,  7,7,7,7,7,7,6,3,  2,2,2,3,  5,5,5,5,0,2,2,60,  1,1,1,1,1,1,5,6,
    1,1,1,1,1,1,6,3,  2,2,2,3,  5,5,5,5,0,2,2,60,  1,1,1,1,1,1,1,1,  0,0,2,2,2,2,60,60,  1,1,1,1,1,1,1,1,  1,1,
    SCORE_END
};
__code const signed char xianglian[] = {
    3,5,5,8,32,32,22,12,  6,6,6,6,6,6,6,6,  2,4,4,5,7,7,6,5,  3,3,3,3,3,3,3,3,
    5,6,6,8,42,42,32,8,   22,22,22,22,22,22,22,22,  6,22,22,8,7,HP,8,7,DP,6,7,  5,5,5,5,5,5,5,5,
    3,5,5,8,32,32,22,8,  6,6,6,6,6,6,6,6,  2,4,4,5,7,7,6,5,  3,3,3,3,3,3,3,3,3,
    5,6,6,8,42,42,32,8,  9,9,9,9,9,9,6,6,  7,9,9,8,7,HP,8,7,DP,6,7,  5,5,5,5,5,5,5,5, 3,5,5,8,32,32,9,8,
    6,6,6,6,6,6,6,6,  2,4,4,5,7,7,6,5,  3,3,3,3,3,3,3,3,3,  5,6,6,8,42,42,32,8,
    9,9,9,9,9,9,6,6,  7,9,9,8,7,HP,8,7,DP,6,5,  8,8,8,8,8,8,8,8,
    5,6,6,8,42,42,32,8,  9,9,9,9,9,9,9,9,  9,6,6,7,6,6,5,5,  8,8,8,8,8,8,8,8,  8,8,
    SCORE_END
};
__code const signed char notice_music[] = {
    //1,1,4,4,5,5,8,8,8,8,0
    1,1,4,4,5,5,8,8,SCORE_END
};
__code const signed char xiyouji1[] = {
    HP,
    50,50,60,60,
    1,1,1,1,1,1,2,2,  3,3,3,70,60,70,50,50,  60,60,60,60,60,60,60,60,  60,60,60,60,1,1,2,2,  3,3,3,3,3,3,5,5,  6,6,1,1,2,3,4,4,  3,3,3,3,3,3,3,3,  3,3,3,3,3,3,5,5,
    6,6,6,6,6,6,5,5,  6,6,6,6,60,60,3,3,  2,2,2,2,2,2,1,1,  2,2,2,2,3,3,3,3,  50,50,50,50,50,50,60,60,  70,70,3,3,3,3,60,60,  60,60,1,1,1,1,1,1,  1,1,1,1,1,1,1,1,  0,0,5,5,5,5,6,6,  8,8,7,7,7,7,6,5,
    6,6,6,6,6,6,6,6,  6,6,6,6,6,6,6,6,  0,0,5,5,5,5,6,6,  8,8,7,7,7,7,6,5,  3,3,3,3,3,3,3,3,  3,3,3,3,50,50,60,60,  1,1,1,1,1,1,2,2,  3,3,3,70,60,70,50,50,  60,60,60,60,60,60,60,60,
    60,60,60,60,1,1,2,2,  3,3,3,3,3,3,5,5,  6,6,1,1,2,3,4,4,  3,3,3,3,3,3,3,3,  3,3,3,3,3,3,5,5,  6,6,6,6,6,6,5,5,  6,6,6,6,60,60,3,3,  2,2,2,2,2,2,1,1,  2,2,2,2,3,3,3,3,  50,50,50,50,50,50,60,60,
    70,70,3,3,3,3,60,60,  60,60,1,1,1,1,1,1,  1,1,1,1,1,1,1,1,  1,1,1,1,50,50,60,60,  60,60,1,1,1,1,1,1,  1,1,1,1,1,1,1,1,  1,1,1,1,3,3,3,3,  50,50,50,50,50,50,60,60,  70,70,3,3,3,3,60,60,  60,60,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,  70,70,70,70,70,70,70,70,  3,3,3,3,60,60,60,60,  60,60,1,1,1,1,1,1,  1,1,1,1,1,1,1,1,  1,1,1,1,1,1,1,1,  1,1,1,1,0,0,0,0,
    DP,
    SCORE_END
};
__code const signed char warning[] = {
    //1,1,4,4,5,5,8,8,8,8,0
    //1,2,3,4,5,6,7,8,  7,1,7,1,7,1,7,1,SCORE_END
    8,7,8,7,8,7,8,7,SCORE_END
};

__code const signed char pwroff_music[] = {
    8,7,6,5,4,3,2,1,SCORE_END
};

__code const signed char count_down_music[] = {
    8,0,0,0,8,0,0,0,  8,0,0,0,8,0,0,0,  8,0,0,0,8,0,0,0,
    8,0,0,0,8,0,0,0,  8,0,0,0,8,0,0,0,  8,0,0,0,8,0,0,0,
    8,0,0,0,8,0,0,0,SCORE_END
};

__code const signed char YouJianChuiYan[] = {
    HP,
    60,70,1,2,3,4,5,
    DP,
    6,6,6,6,  5,5,3,2,  1,1,2,4,  3,3,3,3,  0,2,3,1,  60,60,50,60,  1,1,1,1,  1,1,1,1,
    5,5,5,3,  6,5,3,2,  1,1,60,1, 50,50,50,50,  60,60,60,60,  50,50,1,2,  3,3,3,3,  3,3,3,3,
    5,5,5,3,  6,5,3,2,  1,1,60,1,  50,50,50,50,  5,5,5,HP,5,6,DP,  5,3,2,HP,3,2,DP,  1,1,1,1,  1,1,1,1,
    2,2,2,1,  3,3,2,2,  1,1,2,3,  5,5,5,5,  6,6,6,6,  3,3,5,6,  5,5,5,5, 5,5,5,5,
    6,6,6,6,  5,5,3,2,  0,1,2,4,  3,3,1,1,  0,2,3,1,  60,60,50,60,
    1,1,1,1,  6,6,6,6,  5,5,3,2,  0,1,2,4,  3,3,1,1,  0,2,3,1,  60,60,50,60,
    1,1,1,1,  1,1,1,1,  1,1,0,0,  50,50,50,50,  5,5,5,6,  5,5,3,2,  1,1,1,1,  1,1,1,1, 1,1,
    SCORE_END
};

