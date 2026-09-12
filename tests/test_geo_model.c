#include <pebble.h>
#include <assert.h>
#include <stdio.h>
#include "models/geo_model.h"

static void test_geo_model_accessors(void) {
  // Test region accessors
  for (int i = 0; i < GEO_TOTAL_REGIONS; i++) {
    const RegionGeoInfo *r = geo_model_get_region(i);
    assert(r != NULL);
    assert(r->id == i);
    assert(r->name != NULL);
    assert(r->num_points > 0);
    assert(r->min_x <= r->max_x);
    assert(r->min_y <= r->max_y);
    assert(r->cx >= r->min_x && r->cx <= r->max_x);
    assert(r->cy >= r->min_y && r->cy <= r->max_y);
  }

  assert(geo_model_get_region(GEO_TOTAL_REGIONS) == NULL);
  assert(geo_model_get_region(255) == NULL);

  // Test district accessors
  for (int i = 0; i < GEO_TOTAL_DISTRICTS; i++) {
    const DistrictGeoInfo *d = geo_model_get_district(i);
    assert(d != NULL);
    assert(d->global_id == i);
    assert(d->region_id < GEO_TOTAL_REGIONS);
    assert(d->name != NULL);
    assert(d->num_points > 0);
    assert(d->min_x <= d->max_x);
    assert(d->min_y <= d->max_y);
  }

  assert(geo_model_get_district(GEO_TOTAL_DISTRICTS) == NULL);
  assert(geo_model_get_district(255) == NULL);

  // Test district lines
  for (int i = 0; i < GEO_TOTAL_DISTRICT_LINES; i++) {
    const DistrictInternalLine *line = geo_model_get_district_line(i);
    assert(line != NULL);
    assert(line->dist_a < GEO_TOTAL_DISTRICTS);
    assert(line->dist_b < GEO_TOTAL_DISTRICTS);
    assert(line->num_points >= 2);
  }
  assert(geo_model_get_district_line(GEO_TOTAL_DISTRICT_LINES) == NULL);
}

static void test_find_region_at_centroid(void) {
  // Finding a region at its own centroid should reliably resolve to itself
  for (int i = 0; i < GEO_TOTAL_REGIONS; i++) {
    const RegionGeoInfo *r = geo_model_get_region(i);
    assert(r != NULL);
    int found_id = geo_model_find_region_at(r->cx, r->cy);
    assert(found_id == i);
  }

  // Out of bounds coordinates should return -1
  assert(geo_model_find_region_at(0, 0) == -1);
  assert(geo_model_find_region_at(10000, 10000) == -1);
}

static void test_find_district_at_centroid(void) {
  for (int i = 0; i < GEO_TOTAL_DISTRICTS; i++) {
    const DistrictGeoInfo *d = geo_model_get_district(i);
    assert(d != NULL);
    int found_dist = geo_model_find_district_at(d->cx, d->cy);
    assert(found_dist == i);
  }

  // Out of bounds coordinates
  assert(geo_model_find_district_at(0, 0) == -1);
}

int main(void) {
  printf("Running test_geo_model...\n");
  test_geo_model_accessors();
  test_find_region_at_centroid();
  test_find_district_at_centroid();
  printf("test_geo_model: ALL PASSED\n");
  return 0;
}
