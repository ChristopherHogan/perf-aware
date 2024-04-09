#ifndef PERFAWARE_HAVERSINE_H_
#define PERFAWARE_HAVERSINE_H_

struct Point {
  f64 x0;
  f64 y0;
  f64 x1;
  f64 y1;
};

struct PointArray {
  Point *data;
  u32 num_points;
};

#endif  // PERFAWARE_HAVERSINE_H_
