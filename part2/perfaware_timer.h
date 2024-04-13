#ifndef PERFAWARE_TIMER_H_
#define PERFAWARE_TIMER_H_

#ifdef PERFAWARE_PROFILE

#define TimeBlock(name) TimeBlockHelper(name, __COUNTER__)
#define TimeBlockHelper(name, counter) \
  Timer time_block_timer_##counter(name, counter + 1)

#define TimeFunction TimeBlock(__func__)
#define BeginProfile beginProfile()
#define EndAndPrintProfile endAndPrintProfile()

const u32 kMaxProfileEntries = 64;

struct ProfileEntry {
  u64 elapsed_exclusive;
  u64 elapsed_inclusive;
  u64 hit_count;
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

  Timer(const char *name_, u32 index_) {
    parent_index = global_profiler_parent_;
    index = index_;
    label = name_;

    ProfileEntry *entry = global_profiler_.entries + index;
    old_elapsed_inclusive = entry->elapsed_inclusive;

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

#else
#define TimeFunction
#define TimeBlock(name)
#define TimeBlockHelper(name, counter)
#define BeginProfile
#define EndAndPrintProfile
#endif

#endif  // PERFAWARE_TIMER_H_
