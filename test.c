#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <stdint.h>

int main(int argc, char ** argv){
  FILE *file = fopen("/proc/mperf_aperf", "r");
  assert(file);
  
  uint64_t mperf_aperf[2];
  fread(mperf_aperf, sizeof(uint64_t) * 2, 1, file);

  printf("mperf: 0x%llx aperf: 0x%llx\n", mperf_aperf[0], mperf_aperf[1]);
  
  return 0;
}

