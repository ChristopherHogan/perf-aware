#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include <random>
#include <string>

typedef uint8_t u8;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int32_t s32;
typedef double f64;

#include "perfaware_timer.h"
#include "perfaware_haversine.h"
#include "perfaware_memory.h"
#include "listing_0065_haversine_formula.cpp"
#include "perfaware_haversine.cpp"
#include "perfaware_memory.cpp"
#include "perfaware_timer.cpp"

struct Arguments {
  u64 seed;
  u64 num_points;
};

Point *generatePoints(Arena *arena, Arguments *args) {
  Point *points = pushArray<Point>(arena, args->num_points);

  std::uniform_real_distribution<f64> unifX(-180, 180);
  std::uniform_real_distribution<f64> unifY(-90, 90);
  std::default_random_engine re(args->seed);

  u64 count = args->num_points;
  u64 offset = 0;
  while (count--) {
    points[offset].x0 = unifX(re);
    points[offset].y0 = unifY(re);
    points[offset].x1 = unifX(re);
    points[offset].y1 = unifY(re);
    offset++;
  }

  return points;
}

void outputPointsToFile(Point *points, u64 num_points) {
  std::string filename = "data_" + std::to_string(num_points) + ".json";

  FILE *file = fopen(filename.c_str(), "w");
  if (file) {
    fprintf(file, "{\"pairs\":[\n");
    const char *fmt_string = "\t{\"x0\":%.16f, \"y0\":%.16f, \"x1\":%.16f, \"y1\":%.16f}%s";

    for (u64 i = 0; i < num_points; ++i) {
      f64 x0 = points[i].x0;
      f64 y0 = points[i].y0;
      f64 x1 = points[i].x1;
      f64 y1 = points[i].y1;

      const char *line_spearator = i == num_points - 1 ? "\n" : ",\n";
      fprintf(file, fmt_string, x0, y0, x1, y1, line_spearator);
    }

    fprintf(file, "]}\n");
    fclose(file);
  } else {
    fprintf(stderr, "ERROR: Couldn't open file %s\n", filename.c_str());
  }
}
void outputAnswersToFile(f64 *answers, u64 num_points) {
  std::string filename = "answers_" + std::to_string(num_points) + ".f64";

  FILE *file = fopen(filename.c_str(), "w");
  if (file) {
    u64 count = num_points + 1;
    u64 offset = 0;
    while (count--) {
      fprintf(file, "%.16f\n", answers[offset++]);
    }
    fclose(file);
  } else {
    fprintf(stderr, "ERROR: Couldn't open file %s\n", filename.c_str());
  }
}

int main(int argc, char **argv) {

  const int kNumArgs = 3;

  if (argc == kNumArgs) {
    BeginProfile;
    Arena arena = initArenaAndAllocate(KILOBYTES(64));

    Arguments args = {};
    args.seed = atoll(argv[1]);
    args.num_points = atoll(argv[2]);

    Point *points = generatePoints(&arena, &args);
    f64 *answers = calculateHaversine(&arena, points, args.num_points);

    outputPointsToFile(points, args.num_points);
    outputAnswersToFile(answers, args.num_points);

    destroyArena(&arena);

    EndAndPrintProfile;
  } else {
    fprintf(stdout, "USAGE: %s <seed> <num_points>\n", argv[0]);
  }

  return 0;
}
