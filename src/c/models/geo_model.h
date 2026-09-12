#pragma once
#include <pebble.h>

#define GEO_TOTAL_REGIONS 26
#define GEO_TOTAL_DISTRICTS 118
#define GEO_TOTAL_DISTRICT_LINES 146
#define GEO_COORD_MAX 10000

typedef struct {
  uint16_t x; // 0 .. 10000
  uint16_t y; // 0 .. 10000
} GeoPoint;

typedef struct {
  uint8_t id;
  const char *name;
  uint16_t start_point_idx;
  uint16_t num_points;
  uint16_t min_x;
  uint16_t max_x;
  uint16_t min_y;
  uint16_t max_y;
  uint16_t cx; // Centroid X
  uint16_t cy; // Centroid Y
} RegionGeoInfo;

typedef struct {
  uint8_t global_id;
  uint8_t region_id;
  const char *name;
  uint16_t cx;
  uint16_t cy;
  uint16_t start_point_idx;
  uint16_t num_points;
  uint16_t min_x;
  uint16_t max_x;
  uint16_t min_y;
  uint16_t max_y;
} DistrictGeoInfo;

typedef struct {
  uint8_t dist_a;
  uint8_t dist_b;
  uint16_t start_point_idx;
  uint16_t num_points;
  uint16_t min_x;
  uint16_t max_x;
  uint16_t min_y;
  uint16_t max_y;
} DistrictInternalLine;

extern const GeoPoint s_region_points[];
extern const RegionGeoInfo s_regions[GEO_TOTAL_REGIONS];
extern const GeoPoint s_district_points[];
extern const DistrictGeoInfo s_districts[GEO_TOTAL_DISTRICTS];
extern const GeoPoint s_district_line_points[];
extern const DistrictInternalLine s_district_lines[GEO_TOTAL_DISTRICT_LINES];

const RegionGeoInfo* geo_model_get_region(uint8_t index);
const DistrictGeoInfo* geo_model_get_district(uint8_t global_id);
const DistrictInternalLine* geo_model_get_district_line(uint16_t index);
int geo_model_find_region_at(uint16_t norm_x, uint16_t norm_y);
int geo_model_find_district_at(uint16_t norm_x, uint16_t norm_y);
