#include "perfaware_haversine.h"

const f64 kEarthRadius = 6372.8;

f64 *calculateHaversine(Arena *arena, Point *points, u64 num_points) {
  TimeFunction;
  // NOTE(chogan): Add 1 to store the avg
  f64 *haversine_answers = pushArray<f64>(arena, num_points + 1);

  u64 count = num_points;
  u64 offset = 0;
  f64 sum = 0;
  while (count--) {
    f64 answer = ReferenceHaversine(points[offset].x0, points[offset].y0,
                                    points[offset].x1, points[offset].y1, kEarthRadius);
    haversine_answers[offset] = answer;
    sum += answer;
    offset++;
  }
  haversine_answers[offset] = sum / (f64)num_points;

  return haversine_answers;
}
