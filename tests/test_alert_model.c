#include <pebble.h>
#include <assert.h>
#include <stdio.h>
#include "models/alert_model.h"
#include "models/geo_model.h"

static void test_alert_model_init(void) {
  alert_model_init();
  AlertState *st = alert_model_get_state();
  assert(st != NULL);
  assert(st->is_loaded == false);
  assert(st->region_mask == 0);
  assert(st->partial_region_mask == 0);
  assert(st->active_region_count == 0);
  assert(st->active_district_count == 0);
  assert(st->has_user_location == false);
}

static void test_alert_model_full_vs_partial(void) {
  alert_model_init();

  // Let district 0 be in some region
  const DistrictGeoInfo *d0 = geo_model_get_district(0);
  assert(d0 != NULL);
  uint8_t d0_reg = d0->region_id;

  // Let's choose a different region for full alert
  uint8_t full_reg = (d0_reg + 1) % GEO_TOTAL_REGIONS;

  // Set full alert on full_reg, and district alert on district 0
  uint32_t rmask = (1UL << full_reg);
  uint8_t dmask[DISTRICT_MASK_BYTES];
  memset(dmask, 0, sizeof(dmask));
  dmask[0 / 8] |= (1 << (0 % 8));

  alert_model_update(rmask, dmask, 2, 1, "Test Alert Summary");

  AlertState *st = alert_model_get_state();
  assert(st->is_loaded == true);
  assert(st->active_region_count == 2);
  assert(st->active_district_count == 1);
  assert(strcmp(st->summary_text, "Test Alert Summary") == 0);

  // full_reg should be full alert, not partial
  assert(alert_model_is_region_full_alert(full_reg) == true);
  assert(alert_model_is_region_partial_alert(full_reg) == false);

  // d0_reg should be partial alert, not full
  assert(alert_model_is_region_full_alert(d0_reg) == false);
  assert(alert_model_is_region_partial_alert(d0_reg) == true);

  // District 0 should be alerted
  assert(alert_model_is_district_alert(0) == true);
  assert(alert_model_is_district_alert(1) == false);

  // If full_reg is updated to also contain all districts, full_reg still reports full alert, not partial
  rmask |= (1UL << d0_reg);
  alert_model_update(rmask, dmask, 2, 1, "Both Alerted");
  assert(alert_model_is_region_full_alert(d0_reg) == true);
  assert(alert_model_is_region_partial_alert(d0_reg) == false);
}

static void test_alert_model_durations(void) {
  alert_model_init();

  // 26 regions -> 52 bytes
  uint8_t dur_data[GEO_TOTAL_REGIONS * 2];
  for (int i = 0; i < GEO_TOTAL_REGIONS; i++) {
    uint16_t val = (uint16_t)(i * 15);
    dur_data[i * 2] = val & 0xFF;
    dur_data[i * 2 + 1] = (val >> 8) & 0xFF;
  }

  alert_model_set_region_durations(dur_data, sizeof(dur_data));

  for (int i = 0; i < GEO_TOTAL_REGIONS; i++) {
    uint16_t dur = alert_model_get_region_duration(i);
    assert(dur == (uint16_t)(i * 15));
  }

  assert(alert_model_get_region_duration(GEO_TOTAL_REGIONS) == 0xFFFF);
}

static void test_alert_model_user_location(void) {
  alert_model_init();
  alert_model_set_user_location(true, 4650, 2421);

  AlertState *st = alert_model_get_state();
  assert(st->has_user_location == true);
  assert(st->user_loc_x == 4650);
  assert(st->user_loc_y == 2421);

  alert_model_set_user_location(false, 0, 0);
  assert(st->has_user_location == false);
}

static void test_alert_model_levels(void) {
  alert_model_init();

  // Test colors
  assert(alert_model_get_level_color(ALERT_LEVEL_NONE) == COLOR_MAP_BASE);
  assert(alert_model_get_level_color(ALERT_LEVEL_YELLOW) == COLOR_ALERT_YELLOW);
  assert(alert_model_get_level_color(ALERT_LEVEL_ORANGE) == COLOR_ALERT_ORANGE);
  assert(alert_model_get_level_color(ALERT_LEVEL_RED) == COLOR_ALERT_RED);

  // Set up district levels
  // District 0: Yellow (1) -> bits 0..1 = 1
  // District 1: Orange (2) -> bits 2..3 = 2 (0b10 << 2 = 8)
  // District 2: Red (3)    -> bits 4..5 = 3 (0b11 << 4 = 48)
  // Byte 0 = 1 | 8 | 48 = 57
  uint8_t dist_levels[DISTRICT_LEVELS_BYTES];
  memset(dist_levels, 0, sizeof(dist_levels));
  dist_levels[0] = 1 | (2 << 2) | (3 << 4);

  alert_model_set_district_levels(dist_levels, sizeof(dist_levels));

  assert(alert_model_get_district_alert_level(0) == ALERT_LEVEL_YELLOW);
  assert(alert_model_get_district_alert_level(1) == ALERT_LEVEL_ORANGE);
  assert(alert_model_get_district_alert_level(2) == ALERT_LEVEL_RED);
  assert(alert_model_get_district_alert_level(3) == ALERT_LEVEL_NONE);

  // Set region level: Region 1 = Orange (2)
  uint8_t reg_levels[REGION_LEVELS_BYTES];
  memset(reg_levels, 0, sizeof(reg_levels));
  reg_levels[0] = (2 << 2); // region 1: bits 2..3
  alert_model_set_region_levels(reg_levels, sizeof(reg_levels));
  assert(alert_model_get_region_alert_level(1) == ALERT_LEVEL_ORANGE);

  // If region 1 is full alert, districts belonging to region 1 inherit region 1's level
  // Find a district in region 1
  int dist_in_reg1 = -1;
  for (int d = 0; d < GEO_TOTAL_DISTRICTS; d++) {
    const DistrictGeoInfo *dg = geo_model_get_district(d);
    if (dg && dg->region_id == 1 && d > 2) {
      dist_in_reg1 = d;
      break;
    }
  }
  assert(dist_in_reg1 >= 0);

  // Before full alert on region 1, dist_in_reg1 has no alert
  assert(alert_model_get_district_alert_level(dist_in_reg1) == ALERT_LEVEL_NONE);

  // Put region 1 in full alert
  uint32_t rmask = (1UL << 1);
  alert_model_update(rmask, NULL, 1, 0, "Region 1 alert");
  assert(alert_model_is_region_full_alert(1) == true);
  // Now dist_in_reg1 inherits region 1's level (ALERT_LEVEL_ORANGE)
  assert(alert_model_get_district_alert_level(dist_in_reg1) == ALERT_LEVEL_ORANGE);

  // Test fallback: if district mask is set but district_levels was 0, it falls back to RED
  alert_model_init();
  uint8_t dmask[DISTRICT_MASK_BYTES];
  memset(dmask, 0, sizeof(dmask));
  dmask[0] = (1 << 5); // district 5 alerted
  alert_model_update(0, dmask, 1, 1, "Fallback test");
  assert(alert_model_is_district_alert(5) == true);
  assert(alert_model_get_district_alert_level(5) == ALERT_LEVEL_RED);
}

int main(void) {
  printf("Running test_alert_model...\n");
  test_alert_model_init();
  test_alert_model_full_vs_partial();
  test_alert_model_durations();
  test_alert_model_user_location();
  test_alert_model_levels();
  printf("test_alert_model: ALL PASSED\n");
  return 0;
}
