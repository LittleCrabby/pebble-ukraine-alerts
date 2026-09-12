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

int main(void) {
  printf("Running test_alert_model...\n");
  test_alert_model_init();
  test_alert_model_full_vs_partial();
  test_alert_model_durations();
  test_alert_model_user_location();
  printf("test_alert_model: ALL PASSED\n");
  return 0;
}
