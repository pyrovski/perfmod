//#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sched.h>
#include <mpi.h>

void test1(int active, int core);
void test2(int active, int core);
void test3(int active, int core);

int rank, size;

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

  int active = 1;
  int core = -1;
  int opt;
  int whichTest = 1;
  int iterations = 1;
  while((opt = getopt(argc, argv, "123pc:n:")) != -1){
    switch(opt){
    case 'p':
      active = 0;
      printf("passive mode\n");
      break;
    case 'c':
      core = atoi(optarg);
      break;
    case '1':
      whichTest = 1;
      break;
    case '2':
      whichTest = 2;
      break;
    case '3':
      whichTest = 3;
      break;
    case 'n':
      iterations = atoi(optarg);
      break;
    default:
      break;
    }
  }

  cpu_set_t cpu_set;
  CPU_ZERO(&cpu_set);
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
    if(core < 0){
      printf("could not determine core association; quitting\n");
      MPI_Abort(MPI_COMM_WORLD, 1);
    }
    CPU_ZERO(&cpu_set);
    CPU_SET(core, &cpu_set);
    sched_setaffinity(0, sizeof(cpu_set_t), &cpu_set);
  }
  
  char hostname[80];
  gethostname(hostname, 80);

  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  printf("rank %d on core %d of %s\n", rank, core, hostname);

  switch(whichTest){
  case 1:
    {
      int i;
      for(i = 0; i < iterations; i++)
	test1(active, core);
    }
    break;
  case 2:
    {
      int i;
      for(i = 0; i < iterations; i++)
	test2(active, core);
    }
    break;
  case 3:
    {
      int i;
      for(i = 0; i < iterations; i++)
	test3(active, core);
    }
    break;
  default:
    printf("invalid test: %d\n", whichTest);
    break;
  }

  MPI_Finalize();
  return 0;
}

void test1(int active, int core){
  struct timeval tvBegin, tvEnd, tvDiff;
  
  uint64_t mperf_aperf_begin[2], mperf_aperf_end[2],
    tsc_begin, tsc_end;

  FILE *file = fopen("/proc/mperf_aperf", "r");
  assert(file);
  gettimeofday(&tvBegin, 0);
  int status = fread(mperf_aperf_begin, sizeof(uint64_t) * 2, 1, file);
  tsc_begin = rdtsc();
  status = fseek(file, 0, SEEK_SET);
  assert(!status);

  if(active){
    volatile uint64_t count;
    for(count = 0; count < 500000000; count++);
  } else
    sleep(1);

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
}

void test2(int active, int core){
  struct timeval tvBegin, tvEnd, tvDiff;
  
  uint64_t mperf_aperf_begin[2], mperf_aperf_end[2],
    tsc_begin, tsc_end;

  FILE *file = fopen("/proc/mperf_aperf", "r");
  assert(file);
  gettimeofday(&tvBegin, 0);
  int status = fread(mperf_aperf_begin, sizeof(uint64_t) * 2, 1, file);
  tsc_begin = rdtsc();
  status = fclose(file);
  assert(!status);

  if(active){
    volatile uint64_t count;
    for(count = 0; count < 500000000; count++);
  } else
    sleep(1);


  file = fopen("/proc/mperf_aperf", "r");
  assert(file);
  status = fread(mperf_aperf_end, sizeof(uint64_t) * 2, 1, file);
  tsc_end = rdtsc();
  gettimeofday(&tvEnd, 0);
  status = fclose(file);
  assert(!status);

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
}

void test3(int active, int core){
  const unsigned reps = 100000, delay_us = 0, delay_counts = 50000;
  struct timeval tvEnd[reps], tvDiff;
  double times[reps];

  // first 64 of every 128 bits is mperf, second is aperf
  uint64_t mperf_aperf_end[reps*2], 
    tsc_end[reps];

  FILE *file = fopen("/proc/mperf_aperf", "r");
  assert(file);
  assert(!fclose(file));
  unsigned rcount;

  if(active){
    for(rcount = 0; rcount < reps; rcount++){
      file = fopen("/proc/mperf_aperf", "r");
      fread(mperf_aperf_end+rcount*2, sizeof(uint64_t) * 2, 1, file);
      tsc_end[rcount] = rdtsc();
      gettimeofday(tvEnd + rcount, 0);
      fclose(file);
      
      volatile uint64_t count;
      for(count = 0; count < delay_counts; count++);
    }
  } else{
    for(rcount = 0; rcount < reps; rcount++){
      file = fopen("/proc/mperf_aperf", "r");
      fread(mperf_aperf_end+rcount*2, sizeof(uint64_t) * 2, 1, file);
      tsc_end[rcount] = rdtsc();
      gettimeofday(tvEnd + rcount, 0);
      fclose(file);

      if(delay_us)
	usleep(delay_us);
    }
  }
  char buffer[80];
  sprintf(buffer, "test.%03d.dat", rank);
  file = fopen(buffer, "w");
  assert(file);
  fprintf(file, "mperf\taperf\ttsc\ttime\terror\n");
  int error;
  for(rcount = 0; rcount < reps; rcount++){
    timeval_subtract(&tvDiff, &tvEnd[rcount], &tvEnd[0]);
    times[rcount] = (double)tvDiff.tv_sec + tvDiff.tv_usec / 1000000.0;
    error = 0;
    if(rcount){ // detect abnormalities
      if(mperf_aperf_end[2*rcount] <= mperf_aperf_end[2*(rcount-1)]){
	printf("rank %d mperf abnormal at index %d\n", rank, rcount);
	error = 1;
      }
      if(mperf_aperf_end[2*rcount+1] <= mperf_aperf_end[2*(rcount-1)+1]){
	printf("rank %d aperf abnormal at index %d\n", rank, rcount);
	error = 1;
      }
      if(tsc_end[rcount] <= tsc_end[rcount-1]){
	printf("rank %d tsc abnormal at index %d\n", rank, rcount);
	error = 1;
      }
      if(times[rcount] <= times[rcount-1]){
	printf("rank %d time abnormal at index %d\n", rank, rcount);
	error = 1;
      }
    }
    fprintf(file, "0x%llx\t0x%llx\t0x%llx\t%3.6le\t%d\n", 
	    mperf_aperf_end[2*rcount], 
	    mperf_aperf_end[2*rcount+1], 
	    tsc_end[rcount],
	    times[rcount], 
	    error);
  }
  fclose(file);
}
