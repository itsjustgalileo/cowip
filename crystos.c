#include <stdio.h>
#include <unistd.h>
#include <time.h>

#include "./board.h"

void tick(Clock *clock, update_t update, Board *b) {
    const long nanoseconds_per_second = 1000000000L;
    struct timespec sleepTime;

    // Calculate sleep time based on the frequency
    sleepTime.tv_sec = 0;
    sleepTime.tv_nsec = nanoseconds_per_second / clock->frequency;

    // Simulate a clock tick by sleeping
    nanosleep(&sleepTime, NULL);

    // clock the cpu
    update(b->c);
}
