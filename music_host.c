#include<stdio.h>
#include "music.h"

void main()
{
    for (int i = 0; i< sizeof(musical_scale_freq)/sizeof(float); i++){
        printf("%d, ", (int)((25.0f*1000000/12.0f/musical_scale_freq[i])+0.5));
        if((i+1)%12 == 0)printf("\r\n");
    }
}
