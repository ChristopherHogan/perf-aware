#ifndef PERFAWARE_TIMER_H_
#define PERFAWARE_TIMER_H_

#define TimeFunction \
  global_profiler_.count++; \
  Timer timer_(__FUNCTION__, __COUNTER__)

#define TimeBlock(name) TimeBlockHelper(name, __COUNTER__)

#define TimeBlockHelper(name, counter) \
  global_profiler_.count++; \
  Timer time_block_timer_##counter(#name, counter)

#define BeginProfile                                      \
  global_profiler_.cpu_freq = estimateCPUFrequency(100);  \
  global_profiler_.start = readCpuTimer()

#define EndAndPrintProfile                                              \
  u64 global_profiler_elapsed_ = readCpuTimer() - global_profiler_.start; \
  f64 global_profiler_ms_ = global_profiler_elapsed_ / (f64)global_profiler_.cpu_freq * 1000.0; \
  printf("Total time: %fms (CPU freq %lu)\n", global_profiler_ms_, global_profiler_.cpu_freq); \
  for (u32 gp_i_ = 0; gp_i_ < global_profiler_.count - 1; ++gp_i_) {    \
    const char *gp_label_ = global_profiler_.measurement_labels[gp_i_]; \
    u64 gp_elapsed_ = global_profiler_.measurements[gp_i_]; \
    printTimeElapsed(gp_label_, global_profiler_elapsed_, gp_elapsed_); \
  }

const u32 kMaxMeasurments = 64;

struct ProfilerInfo {
  u64 measurements[kMaxMeasurments];
  const char *measurement_labels[kMaxMeasurments];
  u64 cpu_freq;
  u64 start;
  u32 count;
};

static ProfilerInfo global_profiler_;

struct Timer {
  u64 start;
  u32 index;

  Timer(const char *name, u32 index);
  ~Timer();
};

#endif  // PERFAWARE_TIMER_H_
