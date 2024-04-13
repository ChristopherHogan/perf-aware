#include <x86intrin.h>
#include <sys/time.h>

static u64 getOsTimerFreq() {
	return 1000000;
}

static u64 readOsTimer() {
	timeval value;
	gettimeofday(&value, 0);

	u64 result = getOsTimerFreq()*(u64)value.tv_sec + (u64)value.tv_usec;
	return result;
}

static u64 readCpuTimer() {
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

#ifdef PERFAWARE_PROFILE

static void printTimeElapsed(u64 total_elapsed, ProfileEntry *entry) {
  f64 percent = 100.0 * (f64)entry->elapsed_exclusive / total_elapsed;
  printf("\t%s[%lu]: %lu (%.2f%%", entry->label, entry->hit_count, entry->elapsed_exclusive, percent);

  if (entry->elapsed_inclusive != entry->elapsed_exclusive) {
    f64 percent_with_children = 100.0 * (f64)entry->elapsed_inclusive / total_elapsed;
    printf(", %.2f%% w/children", percent_with_children);
  }
  printf(")");

  if (entry->processed_bytes) {
    f64 megabyte = 1024.0 * 1024.0;
    f64 gigabyte = megabyte * 1024.0;

    f64 seconds = (f64)entry->elapsed_inclusive / (f64)global_profiler_.cpu_freq;
    f64 bytes_per_second = (f64)entry->processed_bytes / seconds;
    f64 megabytes = (f64)entry->processed_bytes / megabyte;
    f64 gigabytes_per_second = bytes_per_second / gigabyte;

    printf("\t%.3fmb at %.2fgb/s", megabytes, gigabytes_per_second);
  }
  printf("\n");
}

static void beginProfile() {
  global_profiler_.cpu_freq = estimateCPUFrequency(100);
  global_profiler_.start = readCpuTimer();
}

static void endAndPrintProfile() {
  u64 elapsed = readCpuTimer() - global_profiler_.start;
  f64 ms = elapsed / (f64)global_profiler_.cpu_freq * 1000.0;
  printf("Total time: %fms (CPU freq %lu)\n", ms, global_profiler_.cpu_freq);
  for (u32 i = 1; i < ArrayCount(global_profiler_.entries); ++i) {
    ProfileEntry *entry = global_profiler_.entries + i;
    if (entry->elapsed_exclusive) {
      printTimeElapsed(elapsed, entry);
    }
  }
}

#elif defined(PERFAWARE_PROFILE_MAIN)

static void beginProfile() {
  global_profiler_start = readCpuTimer();
}

static void endAndPrintProfile() {
  u64 elapsed = readCpuTimer() - global_profiler_start;
  u64 cpu_freq = estimateCPUFrequency(100);
  f64 ms = elapsed / (f64)cpu_freq * 1000.0;
  printf("Total time: %fms (CPU freq %lu)\n", ms, cpu_freq);
}

#endif
