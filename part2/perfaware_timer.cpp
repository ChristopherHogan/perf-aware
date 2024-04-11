#include <x86intrin.h>
#include <sys/time.h>

static u64 getOsTimerFreq(void) {
	return 1000000;
}

static u64 readOsTimer(void) {
	timeval value;
	gettimeofday(&value, 0);

	u64 result = getOsTimerFreq()*(u64)value.tv_sec + (u64)value.tv_usec;
	return result;
}

static u64 readCpuTimer(void) {
	return __rdtsc();
}

[[maybe_unused]] static u64 estimateCPUFrequency(u64 ms) {
	u64 milliseconds_to_wait = ms;
	u64 os_freq = getOsTimerFreq();

	u64 cpu_start = readCpuTimer();
	u64 os_start = readOsTimer();
	u64 os_end = 0;
	u64 os_elapsed = 0;
	u64 os_wait_time = os_freq * milliseconds_to_wait / 1000;

	while(os_elapsed < os_wait_time) {
		os_end = readOsTimer();
		os_elapsed = os_end - os_start;
	}

	u64 cpu_end = readCpuTimer();
	u64 cpu_elapsed = cpu_end - cpu_start;
	u64 cpu_freq = 0;
	if(os_elapsed) {
		cpu_freq = os_freq * cpu_elapsed / os_elapsed;
	}

  return cpu_freq;
}

void printTimeElapsed(const char *label, u64 total_elapsed, u64 elapsed) {
  printf("\t%s: %lu (%.2f%%)\n", label, elapsed, 100.0 * (f64)elapsed / total_elapsed);
}

Timer::Timer(const char *name, u32 index) : start(0), index(index) {
  global_profiler_.measurement_labels[index] = name;
  start = readCpuTimer();
}

Timer::~Timer() {
  u64 end = readCpuTimer();
  u64 elapsed = end - start;
  global_profiler_.measurements[index] = elapsed;
}
