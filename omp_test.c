#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sched.h>
#include <omp.h>

// result = x - y
int
timeval_subtract (struct timeval *result, struct timeval *x, struct timeval *y)
{
  /* Perform the carry for the later subtraction by updating y. */
  if (x->tv_usec < y->tv_usec) {
    int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
    y->tv_usec -= 1000000 * nsec;
    y->tv_sec += nsec;
  }
  if (x->tv_usec - y->tv_usec > 1000000) {
    int nsec = (x->tv_usec - y->tv_usec) / 1000000;
    y->tv_usec += 1000000 * nsec;
    y->tv_sec -= nsec;
  }
     
  /* Compute the time remaining to wait.
     tv_usec is certainly positive. */
  result->tv_sec = x->tv_sec - y->tv_sec;
  result->tv_usec = x->tv_usec - y->tv_usec;
     
  /* Return 1 if result is negative. */
  return x->tv_sec < y->tv_sec;
}

static inline uint64_t rdtsc(void)
{
  if (sizeof(long) == sizeof(uint64_t)) {
    uint32_t lo, hi;
    asm volatile("rdtsc" : "=a" (lo), "=d" (hi));
    return ((uint64_t)(hi) << 32) | lo;
  }
  else {
    uint64_t tsc;
    asm volatile("rdtsc" : "=A" (tsc));
    return tsc;
  }
}

int main(int argc, char ** argv){
#pragma omp parallel 
  {
    int tid = omp_get_thread_num();
    FILE *fp = fopen("/proc/mperf_aperf", "r");
    if(!fp){
      fprintf(stderr, "thread %d failed to open.\n", tid);
      exit(1);
    }
    volatile int i;
#pragma omp for
    for(i = 0; i < 1000; i++){
      uint64_t mperf_aperf[2];
      int status = fread(mperf_aperf, sizeof(uint64_t) * 2, 1, fp);
      printf("t%d: %d: 0x%llx 0x%llx\n",
	     tid, status, mperf_aperf[0], mperf_aperf[1]);
    }
  }
  return 0;
}
