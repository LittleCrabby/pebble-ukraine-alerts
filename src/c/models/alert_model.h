#pragma once
#include <pebble.h>
#include "../config.h"

typedef enum {
  ALERT_LEVEL_NONE = 0,
  ALERT_LEVEL_YELLOW = 1,
  ALERT_LEVEL_ORANGE = 2,
  ALERT_LEVEL_RED = 3
} AlertLevel;

typedef struct {
  uint32_t region_mask;            // 26 bits for 26 regions
  uint32_t partial_region_mask;    // 26 bits for regions with active district(s) but not full alert
  uint8_t district_mask[DISTRICT_MASK_BYTES]; // 118 bits for districts (16 bytes = 128 bits)
  uint8_t district_levels[DISTRICT_LEVELS_BYTES]; // 2 bits per district (32 bytes = 128 districts)
  uint8_t region_levels[REGION_LEVELS_BYTES];     // 2 bits per region (8 bytes = 32 regions)
  uint8_t active_region_count;
  uint8_t active_district_count;
  time_t last_update_time;
  char summary_text[512];
  bool is_loaded;
  bool has_user_location;
  uint16_t user_loc_x;
  uint16_t user_loc_y;
  uint16_t region_durations[REGION_MASK_COUNT]; // Duration in minutes for each region
} AlertState;

void alert_model_init(void);
AlertState* alert_model_get_state(void);
bool alert_model_is_region_full_alert(uint8_t region_id);
bool alert_model_is_region_partial_alert(uint8_t region_id);
bool alert_model_is_district_alert(uint8_t district_global_id);
AlertLevel alert_model_get_district_alert_level(uint8_t district_global_id);
AlertLevel alert_model_get_region_alert_level(uint8_t region_id);
GColor alert_model_get_level_color(AlertLevel level);
uint16_t alert_model_get_region_duration(uint8_t region_id);
void alert_model_update(uint32_t region_mask, const uint8_t *district_mask, uint8_t reg_count, uint8_t dist_count, const char *summary);
void alert_model_set_user_location(bool valid, uint16_t x, uint16_t y);
void alert_model_set_region_durations(const uint8_t *data, size_t len);
void alert_model_set_district_levels(const uint8_t *data, size_t len);
void alert_model_set_region_levels(const uint8_t *data, size_t len);
