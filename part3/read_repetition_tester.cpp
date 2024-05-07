#define _CRT_SECURE_NO_WARNINGS

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <windows.h>
#include <fcntl.h>
#include <io.h>

// #include <unistd.h>

#define ArrayCount(arr) (sizeof(arr) / sizeof((arr)[0]))

typedef uint8_t u8;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int32_t s32;
typedef double f64;
typedef uint32_t b32;

#include "perfaware_timer.h"
#include "perfaware_timer.cpp"
#include "perfaware_memory.h"
#include "perfaware_memory.cpp"
#include "listing_0103_repetition_tester.cpp"

struct ReadParams {
  u8 *dest;
  char *filename;
  u64 size;
};

void readViaFread(repetition_tester *tester, ReadParams *params) {

	while (IsTesting(tester)) {
		FILE *file = fopen(params->filename, "rb");
		if (file) {
			u8 *dest = params->dest;

			BeginTime(tester);
			size_t result = fread(dest, params->size, 1, file);
			EndTime(tester);

			if (result == 1) {
			  CountBytes(tester, params->size);
			} else {
				Error(tester, "fread failed");
			}
			fclose(file);
		} else {
			Error(tester, "fopen failed");
		}
	}
}

void readViaRead(repetition_tester *tester, ReadParams *params) {
	while (IsTesting(tester)) {
		int file = _open(params->filename, _O_BINARY | _O_RDONLY);
		if (file != -1) {
			u8 *dest = params->dest;
			u64 size_remaining = params->size;

			while (size_remaining) {
				u32 read_size = INT_MAX;
				if ((u64)read_size > size_remaining) {
					read_size = (u32)size_remaining;
				}

				BeginTime(tester);
				int result = _read(file, dest, read_size);
				EndTime(tester);

				if (result == (int)read_size) {
					CountBytes(tester, read_size);
				} else {
					Error(tester, "_read failed");
					break;
				}

				size_remaining -= read_size;
				dest += read_size;
			}
			_close(file);
		} else {
			Error(tester, "_open failed");
		}
	}
}

void readViaReadFile(repetition_tester *tester, ReadParams *params) {
	while (IsTesting(tester)) {
		HANDLE file = CreateFileA(params->filename, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, 0,
															OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
		if (file != INVALID_HANDLE_VALUE) {
			u8 *dest = params->dest;
			u64 size_remaining = params->size;
			while (size_remaining) {
				u32 read_size = (u32)-1;
				if ((u64)read_size > size_remaining) {
					read_size = (u32)size_remaining;
				}

				DWORD bytes_read = 0;
				BeginTime(tester);
				BOOL result = ReadFile(file, dest, read_size, &bytes_read, 0);
				EndTime(tester);

				if (result && (bytes_read == read_size)) {
					CountBytes(tester, read_size);
				} else {
					Error(tester, "ReadFile failed");
				}

				size_remaining -= read_size;
				dest += read_size;
			}
			CloseHandle(file);
		} else {
			Error(tester, "CreateFileA failed");
		}
	}
}

typedef void ReadTestFunc(repetition_tester *tester, ReadParams *params);

struct TestFunction {
  const char *name;
  ReadTestFunc *func;
};

TestFunction test_functions[] {
  {"fread", readViaFread},
  {"_read", readViaRead},
  {"ReadFile", readViaReadFile},
};


int main(int argc, char **argv) {

  u64 cpu_timer_freq = estimateCPUFrequency();

  if (argc == 2) {
    char *filename = argv[1];

    struct __stat64 st = {};
    if (_stat64(filename, &st) == 0) {
      Arena arena = initArenaAndAllocate(st.st_size);

      ReadParams params = {};
      params.dest = pushSize(&arena, st.st_size);
      params.size = st.st_size;
      params.filename = filename;

      if (params.dest) {
        repetition_tester testers[ArrayCount(test_functions)] = {};

        for (;;) {

          for (u32 i = 0; i < ArrayCount(test_functions); ++i) {
            TestFunction *func = test_functions + i;
            repetition_tester *tester = testers + i;

            printf("\n--- %s ---\n", func->name);
            NewTestWave(tester, params.size, cpu_timer_freq);
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
