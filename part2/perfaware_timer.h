#ifndef PERFAWARE_TIMER_H_
#define PERFAWARE_TIMER_H_

#ifdef PERFAWARE_PROFILE

#define NameConcat2(a, b) a##b
#define NameConcat(a, b) NameConcat2(a, b)
#define TimeBlock(name) TimeBandwidth(name, 0)
#define TimeBandwidth(name, byte_count) \
  Timer NameConcat(Block, __LINE__)(name, __COUNTER__ + 1, byte_count)
#define TimeFunction TimeBlock(__func__)
#define BeginProfile beginProfile()
#define EndAndPrintProfile endAndPrintProfile()
#define ProfilerEndOfCompilationUnit \
  static_assert(__COUNTER__ < ArrayCount(Profiler::entries), \
                "Number of profile points exceeds size of Profiler::entries array")

const u32 kMaxProfileEntries = 64;

struct ProfileEntry {
  u64 elapsed_exclusive;
  u64 elapsed_inclusive;
  u64 hit_count;
  u64 processed_bytes;
  const char *label;
};

struct Profiler {
  ProfileEntry entries[kMaxProfileEntries];
  u64 cpu_freq;
  u64 start;
};

static Profiler global_profiler_;
static u32 global_profiler_parent_;

static u64 readCpuTimer();

struct Timer {
  u64 start;
  u64 old_elapsed_inclusive;
  const char *label;
  u32 index;
  u32 parent_index;

  Timer(const char *name_, u32 index_, u64 processed_bytes_) {
    parent_index = global_profiler_parent_;
    index = index_;
    label = name_;

    ProfileEntry *entry = global_profiler_.entries + index;
    old_elapsed_inclusive = entry->elapsed_inclusive;
    entry->processed_bytes += processed_bytes_;

    global_profiler_parent_ = index;
    start = readCpuTimer();
  }

  ~Timer() {
    u64 end = readCpuTimer();
    global_profiler_parent_ = parent_index;
    u64 elapsed = end - start;

    ProfileEntry *cur = global_profiler_.entries + index;
    ProfileEntry *parent = global_profiler_.entries + parent_index;

    parent->elapsed_exclusive -= elapsed;
    cur->elapsed_exclusive += elapsed;
    cur->elapsed_inclusive = old_elapsed_inclusive + elapsed;
    cur->label = label;
    cur->hit_count++;
  }
};

#elif defined(PERFAWARE_PROFILE_MAIN)

static u64 global_profiler_start;

#define TimeFunction
#define TimeBlock(...)
#define TimeBlockHelper(...)
#define BeginProfile beginProfile()
#define EndAndPrintProfile endAndPrintProfile()
#define ProfilerEndOfCompilationUnit
#else
#define TimeFunction
#define TimeBlock(...)
#define TimeBlockHelper(...)
#define BeginProfile
#define EndAndPrintProfile
#define ProfilerEndOfCompilationUnit
#endif

#endif  // PERFAWARE_TIMER_H_
