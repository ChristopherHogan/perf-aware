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

PointArray parseJson(Arena *arena, Arena *scratch, const char *file_path) {
  ScopedTemporaryMemory scratch_memory(scratch);
  EntireFile file = readEntireFile(scratch_memory, file_path);
  TokenArray tokens = tokenize(scratch_memory, &file);
  PointArray result = parseTokens(arena, &tokens);

  return result;
}

bool verifyHaversine(Arena *scratch, f64 *answers, const char *filename, u32 num_points) {
  ScopedTemporaryMemory file_memory(scratch);
  EntireFile file = readEntireFile(file_memory, filename);
  u8 *at = file.data;
  bool result = true;

  for (u32 i = 0; i < num_points + 1; ++i) {
    f64 answer = atof((const char *)at);
    while (*at++ != '\n');
    if (abs(answer - answers[i]) > 0.00000001) {
      fprintf(stderr, "%.16f != %.16f\n", answer, answers[i]);
      result = false;
      break;
    }
  }

  return result;
}

int main(int argc, char **argv) {

  if (argc == 3) {
    const char *file_path = argv[1];
    const char *answers_filename = argv[2];

    Arena arena = initArenaAndAllocate(GIGABYTES(1));
    Arena scratch = initArenaAndAllocate(GIGABYTES(6));

    PointArray points = parseJson(&arena, &scratch, file_path);
    f64 *answers = calculateHaversine(&arena, points.data, points.num_points);

    if (!verifyHaversine(&scratch, answers, answers_filename, points.num_points)) {
      fprintf(stderr, "Test failed\n");
      exit(1);
    }

    destroyArena(&arena);
  } else {
    fprintf(stderr, "USAGE: %s <JSON_path> <haversine_answers_path>\n", argv[0]);
  }

  return 0;
}
