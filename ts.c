#include "common.h"

void ts_plus(struct timespec *tsp2, const struct timespec *tsp1, long long ms)
{
    /* tsp2 = tsp1 + ms
     * note: tsp1 may equal to tsp2 */
    int nsec = tsp1->tv_nsec + (ms % 1000) * 1000000;
    tsp2->tv_sec = tsp1->tv_sec + ms / 1000 + nsec / 1000000000;
    tsp2->tv_nsec = nsec % 1000000000;
}
long long ts_minus(const struct timespec *tsp1, const struct timespec *tsp2)
{
    /* tsp1 - tsp2 (in ms)
     * note: pay attation to rounding method */
    return (tsp1->tv_sec - tsp2->tv_sec) * 1000 +
           (tsp1->tv_nsec - tsp2->tv_nsec) / 1000000;
}
int ts_less(const struct timespec *tsp1, const struct timespec *tsp2)
{
    /* use this function to check deadline:
     *   tsp1 -> deadline
     *   tsp2 -> current time
     * note: should not use ts_minus(tsp1, tsp2) < 0, since it's unit is ms */
    return tsp1->tv_sec < tsp2->tv_sec ||
           (tsp1->tv_sec == tsp2->tv_sec && tsp1->tv_nsec <= tsp2->tv_nsec);
}
