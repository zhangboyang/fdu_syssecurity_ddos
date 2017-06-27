#include "common.h"


void counter_reset(struct counter *counter, const char *unit)
{
    *counter = (struct counter) {
        .cnt = 0,
        .unit = unit,
    };
    clock_gettime(CLOCK_REALTIME, &counter->ts);
}
int counter_inc(struct counter *counter)
{
#define SHOWPERIOD 10000
    counter->cnt++;
    if (counter->cnt % SHOWPERIOD == 0) {
        
        struct timespec nts;
        clock_gettime(CLOCK_REALTIME, &nts);
        
        long long d = ts_minus(&nts, &counter->ts);
        
        printf("INFO: %llu %s sent in %.3f seconds (%.2f %s per second).\n", counter->cnt, counter->unit, (d / 1000.0), SHOWPERIOD / (d / 1000.0), counter->unit);
        
        counter->ts = nts;
        return 1;
    } else {
        return 0;
    }
}
int counter_less(struct counter *counter, unsigned long long cnt)
{
    return counter->cnt < cnt;
}



u_int32_t rand_u32()
{
    return (rand() & 0xffff) | (rand() << 16);
}

u_int16_t rand_u16()
{
    return rand() & 0xffff;
}
u_int16_t rand_port()
{
    return rand_u16();
}





void dump(void *memaddr, size_t memsize)
{
    printf( " %llu bytes of memory dump at "  "0x%016llx"  "\n", (unsigned long long) memsize, (unsigned long long) (memaddr));
    
    unsigned long long addr = (unsigned long long) (memaddr) / 16 * 16;
    unsigned long long skip = (unsigned long long) (memaddr) - addr;
    memaddr -= skip;
    unsigned long long n = memsize + skip;

    printf("                     +0 +1 +2 +3 +4 +5 +6 +7  +8 +9 +A +B +C +D +E +F"  "\n");
    //     "  AABBCCDD EEFF0011  aa bb cc dd 00 11 22 33  dd cc bb aa 33 22 11 00  ................"

    char buf[16];
    char b;
    unsigned long long m = n % 16 ? n - n % 16 + 16 : n;
    char lb[16];
    unsigned long long i, j;
    
    for (i = 0; i < m; i++) {
        if (i % 16 == 0) {
            for (j = 0; j < 16; j++)
                if (i + j >= skip && i + j < n)
                    lb[j] = *(unsigned char *)(memaddr + i + j);
                else
                    lb[j] = 0xcc;
            printf("  %08llx %08llx  " , (addr + i) >> 32, (addr + i) & ((1ULL << 32) - 1));
        }
        if (i >= skip && i < n) {
            b = lb[i % 16];
            printf("%02x ", b & 0xff);
        } else {
            b = '.';
            printf(".. ");
        }
        buf[i % 16] = (b >= ' ' && b <= '~') ? b : '.';
        if (i % 16 == 7) printf(" ");
        if (i % 16 == 15) {
            printf(" ");
            for (j = 0; j < 16; j++) {
                printf("%c", buf[j]);
                if (j == 7) printf(" ");
            }
            printf("\n");
        }
    }
    printf(" end of memory dump\n");
}

