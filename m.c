#include <8051.h>
int main()
{
    int t1, t2;
    P0_0 = 0;
    P0_1 = 1;
a:
    t1 = 900;
    while(t1--)
    {
        t2 = 400;
        while(t2--)
        {
        }
    }
    P0_0 = !P0_0;
    P0_1 = !P0_1;
    goto a;
}
