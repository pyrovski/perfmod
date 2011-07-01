#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sched.h>

// result = x - y
int
timeval_subtract (result, x, y)
     struct timeval *result, *x, *y;
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
  FILE *file = fopen("/proc/mperf_aperf", "r");
  assert(file);

  cpu_set_t cpu_set;
  int core = 0;
  CPU_ZERO(&cpu_set);
  if(argc > 1){
    if(atoi(argv[1]) >= 0)
      core = atoi(argv[1]);
    else
      core = -1;
  }
  if(core >= 0){
    CPU_SET(core, &cpu_set);
    sched_setaffinity(0, sizeof(cpu_set_t), &cpu_set);
  } else {
    sched_getaffinity(0, sizeof(cpu_set_t), &cpu_set);
    int i, section;
    for(i = 0; i < sizeof(cpu_set_t) / sizeof(int); i++){
      section = *((int*)&cpu_set + i);
      if(!section)
	continue;

      int shift_count = 0;
      while(!(section & 1)){
	section >>= 1;
	shift_count++;
      }
      core = shift_count;
      break;
    }
    if(core < 0)
      printf("could not determine core association\n");
  }
  
  struct timeval tvBegin, tvEnd, tvDiff;
  
  gettimeofday(&tvBegin, 0);
  uint64_t mperf_aperf_begin[2], mperf_aperf_end[2],
    tsc_begin, tsc_end;
  int status = fread(mperf_aperf_begin, sizeof(uint64_t) * 2, 1, file);
  tsc_begin = rdtsc();
  status = fseek(file, 0, SEEK_SET);
  assert(!status);
  
  volatile uint64_t count;
  for(count = 0; count < 500000000; count++);

  status = fread(mperf_aperf_end, sizeof(uint64_t) * 2, 1, file);
  tsc_end = rdtsc();
  gettimeofday(&tvEnd, 0);
  fclose(file);

  timeval_subtract(&tvDiff, &tvEnd, &tvBegin);
  
  double elapsedTime = tvDiff.tv_sec + tvDiff.tv_usec / 1000000.0;

  printf("core %d begin mperf: 0x%llx aperf: 0x%llx\n", core,
	 mperf_aperf_begin[0], mperf_aperf_begin[1]);
  printf("core %d   end mperf: 0x%llx aperf: 0x%llx\n", core, 
	 mperf_aperf_end[0], mperf_aperf_end[1]);
  printf("core %d time: %lfs mperf rate: %lf/s aperf rate: %lf/s, tsc rate: %lf/s\n", 
	 core, elapsedTime, 
	 (mperf_aperf_end[0] - mperf_aperf_begin[0]) / elapsedTime,
	 (mperf_aperf_end[1] - mperf_aperf_begin[1]) / elapsedTime,
	 (tsc_end - tsc_begin) / elapsedTime);
  printf("core %d aperf/mperf ratio: %lf\n", core,
	 (double)(mperf_aperf_end[1] - mperf_aperf_begin[1]) / 
	 (mperf_aperf_end[0] - mperf_aperf_begin[0]));

   return 0;
}

