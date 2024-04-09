#include "perfaware_memory.h"

using std::ptrdiff_t;

bool isPowerOfTwo(size_t val) {
  bool result = (val & (val - 1)) == 0;

  return result;
}

uintptr_t alignForward(uintptr_t addr, size_t alignment) {
  assert(isPowerOfTwo(alignment));

  uintptr_t result = addr;
  uintptr_t a = (uintptr_t)alignment;
  uintptr_t remainder = addr & (a - 1);

  if (remainder != 0) {
    result += a - remainder;
  }

  return result;
}

uintptr_t alignBackward(uintptr_t addr, size_t alignment) {
  assert(isPowerOfTwo(alignment));

  uintptr_t result = addr;
  uintptr_t a = (uintptr_t)alignment;
  uintptr_t remainder = addr & (a - 1);

  if (remainder != 0) {
    result -= remainder;
  }

  return result;
}

void initArena(Arena *arena, size_t bytes, u8 *base) {
  arena->base = base;
  arena->used = 0;
  arena->capacity = bytes;
  arena->temp_count = 0;
}

Arena initArenaAndAllocate(size_t bytes) {
  Arena result = {};
  result.base = (u8 *)malloc(bytes);
  result.capacity = bytes;

  return result;
}

void destroyArena(Arena *arena) {
  // TODO(chogan): Check for temp count?
  free(arena->base);
  arena->base = 0;
  arena->used = 0;
  arena->capacity = 0;
}

size_t getRemainingCapacity(Arena *arena) {
  size_t result = arena->capacity - arena->used;

  return result;
}

void growArena(Arena *arena, size_t new_size) {
  if (new_size > arena->capacity) {
    void *new_base = (u8 *)realloc(arena->base, new_size);
    if (new_base) {
      arena->base = (u8 *)new_base;
      arena->capacity = new_size;
    } else {
      // LOG(FATAL) << "realloc failed in " << __func__ << std::endl;
    }
  } else {
    // LOG(WARNING) << __func__ << ": Not growing arena. "
    //              << "Only accepts a size greater than the current arena "
    //              << "capacity." << std::endl;
  }
}

TemporaryMemory beginTemporaryMemory(Arena *arena) {
  TemporaryMemory result = {};
  result.arena = arena;
  result.used = arena->used;
  if (++arena->temp_count > 1) {
    // HERMES_NOT_IMPLEMENTED_YET;
  }

  return result;
}

void endTemporaryMemory(TemporaryMemory *temp_memory) {
  temp_memory->arena->used = temp_memory->used;
  temp_memory->arena->temp_count--;
  assert(temp_memory->arena->temp_count >= 0);
}

u8 *pushSize(Arena *arena, size_t size, size_t alignment) {
  // TODO(chogan): Account for possible size increase due to alignment
  // bool can_fit = GetAlignedSize(arena, size, alignment);
  bool can_fit = size + arena->used <= arena->capacity;

  if (!can_fit) {
    // TODO(chogan):
  }

  assert(can_fit);

  u8 *base_result = arena->base + arena->used;
  u8 *result = (u8 *)alignForward((uintptr_t)base_result, alignment);

  if (base_result != result) {
    ptrdiff_t alignment_size = result - base_result;
    arena->used += alignment_size;
    // DLOG(INFO) << "PushSize added " << alignment_size
    //            << " bytes of padding for alignment" << std::endl;
  }
  arena->used += size;

  return result;
}

u8 *pushSizeAndClear(Arena *arena, size_t size, size_t alignment) {
  size_t previously_used = arena->used;
  u8 *result = pushSize(arena, size, alignment);
  // NOTE(chogan): Account for case when `size` is increased for alignment
  size_t bytes_to_clear = arena->used - previously_used;

  if (result) {
    memset(result, 0, bytes_to_clear);
  }

  return result;
}
