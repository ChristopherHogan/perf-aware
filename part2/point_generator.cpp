#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include <random>
#include <string>

typedef uint32_t u32;
typedef uint64_t u64;
typedef double f64;

#include "listing_0065_haversine_formula.cpp"

const f64 kEarthRadius = 6372.8;

struct Arguments {
  u64 seed;
  u64 num_points;
};

struct Point {
  f64 x0;
  f64 y0;
  f64 x1;
  f64 y1;
};

Point *generatePoints(Arguments *args) {
  Point *points = (Point*)malloc(sizeof(Point) * args->num_points);

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

  (void)ReferenceHaversine;

  return points;
}

void outputPointsToFile(Point *points, u64 num_points) {
  std::string filename = "data_" + std::to_string(num_points) + ".json";

  FILE *file = fopen(filename.c_str(), "w");
  if (file) {
    fprintf(file, "{\"pairs\":[\n");

    for (u64 i = 0; i < num_points; ++i) {
      f64 x0 = points[i].x0;
      f64 y0 = points[i].y0;
      f64 x1 = points[i].x1;
      f64 y1 = points[i].y1;

      const char *line_spearator = i == num_points - 1 ? "\n" : ",\n";
      fprintf(file, "{\"x0\":%f, \"y0\":%f, \"x1\":%f, \"y1\":%f}%s", x0, y0, x1, y1, line_spearator);
    }

    fprintf(file, "]}\n");
    fclose(file);
  } else {
    // TODO(chogan): ERROR
  }
}

int main(int argc, char **argv) {

  const int kNumArgs = 3;

  if (argc == kNumArgs) {
    Arguments args = {};
    args.seed = atoll(argv[1]);
    args.num_points = atoll(argv[2]);
    Point *points = generatePoints(&args);
    outputPointsToFile(points, args.num_points);
    free(points);
  } else {
    fprintf(stdout, "USAGE: %s \n", argv[0]);
  }

  return 0;
}
