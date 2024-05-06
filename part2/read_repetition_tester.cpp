#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define ArrayCount(arr) (sizeof(arr) / sizeof((arr)[0]))

typedef uint8_t u8;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int32_t s32;
typedef double f64;

#include "perfaware_timer.h"
#include "perfaware_timer.cpp"
#include "perfaware_memory.h"
#include "perfaware_memory.cpp"
struct RepetitionTester {
  
};

struct ReadParams {
  u8 *dest;
  char *filename;
  u64 size;
};

void readViaRead(RepetitionTester *tester, ReadParams *params) {
  (void)tester;
  (void)params;
}

void readViaFread(RepetitionTester *tester, ReadParams *params) {
  (void)tester;
  (void)params;
}

void readViaIfStream(RepetitionTester *tester, ReadParams *params) {
  (void)tester;
  (void)params;
}

void newTestWave(RepetitionTester *tester, u64 bytes_to_process, u64 cpu_timer_freq, u32 seconds = 10) {
  
}

typedef void ReadTestFunc(RepetitionTester *tester, ReadParams *params);

struct TestFunction {
  const char *name;
  ReadTestFunc *func;
};

TestFunction test_functions[] {
  {"read", readViaRead},
  {"fread", readViaFread},
  {"ifstream", readViaIfStream},
};


int main(int argc, char **argv) {

  u64 cpu_timer_freq = estimateCPUFrequency(100);

  if (argc == 2) {
    char *filename = argv[1];

    struct stat st = {};
    if (stat(filename, &st) == 0) {
      Arena arena = {};
      initArenaAndAllocate(st.st_size);

      ReadParams params = {};
      params.dest = pushSize(&arena, st.st_size);
      params.size = st.st_size;
      params.filename = filename;

      if (params.dest) {
        RepetitionTester testers[ArrayCount(test_functions)] = {};

        for (;;) {

          for (u32 i = 0; i < ArrayCount(test_functions); ++i) {
            TestFunction *func = test_functions + i;
            RepetitionTester *tester = testers + i;

            printf("\n--- %s ---\n", func->name);
            newTestWave(tester, params.size, cpu_timer_freq);
            func->func(tester, &params);
          }
        }
      } else {
        fprintf(stderr, "ERROR: Unable to allocate memory\n");
      }

      destroyArena(&arena);
    } else {
      fprintf(stderr, "ERROR: stat failed with message: %s\n", strerror(errno));
    }
  }


  return 0;
}
