#include <math.h>
#include <stdint.h>
#include <stdlib.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;
typedef double f64;

#include "perfaware_haversine.h"
#include "perfaware_memory.h"
#include "listing_0065_haversine_formula.cpp"
#include "perfaware_haversine.cpp"
#include "perfaware_memory.cpp"
#include "perfaware_json_parser.cpp"

PointArray parseJson(Arena *arena, const char *file_path) {
  // TODO(chogan): temp arena for file
  EntireFile file = readEntireFile(arena, file_path);
  TokenArray tokens = tokenize(arena, &file);
  PointArray result = parseTokens(arena, &tokens);

  return result;
}

int main(int argc, char **argv) {

  if (argc == 2) {
    const char *file_path = argv[1];
    Arena arena = initArenaAndAllocate(KILOBYTES(64));
    PointArray points = parseJson(&arena, file_path);
    f64 *answers = calculateHaversine(&arena, points.data, points.num_points);

    // check against reference
    (void)answers;

    destroyArena(&arena);
  } else {
    fprintf(stderr, "USAGE: %s [] <>\n", argv[0]);
  }

  return 0;
}
