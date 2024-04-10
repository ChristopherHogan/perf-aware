#ifndef PERFAWARE_MEMORY_H_
#define PERFAWARE_MEMORY_H_

#include <assert.h>
#include <string.h>

#define KILOBYTES(n) ((n) * 1024)
#define MEGABYTES(n) (KILOBYTES((n)) * 1024)
#define GIGABYTES(n) (MEGABYTES((n)) * 1024LL)

struct Arena {
  u8 *base;
  size_t used;
  size_t capacity;
  s32 temp_count;
};

struct TemporaryMemory {
  Arena *arena;
  size_t used;
};

struct ScopedTemporaryMemory {
  Arena *arena;
  size_t used;

  ScopedTemporaryMemory() = delete;
  ScopedTemporaryMemory(const ScopedTemporaryMemory &) = delete;
  ScopedTemporaryMemory& operator=(const ScopedTemporaryMemory &) = delete;

  explicit ScopedTemporaryMemory(Arena *backing_arena)
      : arena(backing_arena), used(backing_arena->used) {
    // TODO(chogan): Currently not threadsafe unless each thread has a different
    // `backing_arena`
    if (++backing_arena->temp_count > 1) {
      // HERMES_NOT_IMPLEMENTED_YET;
    }
  }

  ~ScopedTemporaryMemory() {
    assert(arena->used >= used);
    arena->used = used;
    assert(arena->temp_count > 0);
    arena->temp_count--;
  }

  operator Arena*() {
    return arena;
  }
};

TemporaryMemory beginTemporaryMemory(Arena *arena);
void endTemporaryMemory(TemporaryMemory *temp_memory);

void initArena(Arena *arena, size_t bytes, u8 *base);
Arena initArenaAndAllocate(size_t bytes);
void destroyArena(Arena *arena);
size_t getRemainingCapacity(Arena *arena);
void growArena(Arena *arena, size_t new_size);
u8 *pushSize(Arena *arena, size_t size, size_t alignment = 8);
u8 *pushSizeAndClear(Arena *arena, size_t size, size_t alignment = 8);

template<typename T>
inline T *pushStruct(Arena *arena, size_t alignment = 8) {
  T *result = reinterpret_cast<T *>(pushSize(arena, sizeof(T), alignment));

  return result;
}

template<typename T>
inline T *pushClearedStruct(Arena *arena, size_t alignment = 8) {
  T *result = reinterpret_cast<T *>(pushSizeAndClear(arena, sizeof(T),
                                                     alignment));

  return result;
}

template<typename T>
inline T *pushArray(Arena *arena, int count, size_t alignment = 8) {
  T *result = reinterpret_cast<T *>(pushSize(arena, sizeof(T) * count,
                                             alignment));

  return result;
}

template<typename T>
inline T *pushClearedArray(Arena *arena, int count, size_t alignment = 8) {
  T *result = reinterpret_cast<T *>(pushSizeAndClear(arena, sizeof(T) * count,
                                                     alignment));

  return result;
}

#endif  // PERFAWARE_MEMORY_H_
