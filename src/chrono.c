#include "chrono.h"
#include <time.h>
#include <sys/time.h>
#include <unistd.h>

int nanosleep(const struct timespec* req, struct timespec* rem);

microseconds_t get_current_microseconds() {
    struct timeval cur_time;
	gettimeofday(&cur_time, NULL);
	return (cur_time.tv_sec * 1000000l) + cur_time.tv_usec;
}

void sleep_microseconds(microseconds_t time) {
	nanosleep(&(struct timespec) {
		.tv_sec = time / 1000000l,
		.tv_nsec = (time % 1000000l) * 1000l
	}, NULL);
}