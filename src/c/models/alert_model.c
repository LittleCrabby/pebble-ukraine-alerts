#include "alert_model.h"
#include "geo_model.h"

static AlertState s_state;

void alert_model_init(void) {
  memset(&s_state, 0, sizeof(AlertState));
  memset(s_state.region_durations, 0xFF, sizeof(s_state.region_durations));
  s_state.is_loaded = false;
  strncpy(s_state.summary_text, "...", sizeof(s_state.summary_text) - 1);
}

AlertState* alert_model_get_state(void) {
  return &s_state;
}

bool alert_model_is_region_full_alert(uint8_t region_id) {
  if (region_id >= REGION_MASK_COUNT) return false;
  return (s_state.region_mask & (1UL << region_id)) != 0;
}

bool alert_model_is_region_partial_alert(uint8_t region_id) {
  if (region_id >= REGION_MASK_COUNT) return false;
  return (s_state.partial_region_mask & (1UL << region_id)) != 0;
}

bool alert_model_is_district_alert(uint8_t district_global_id) {
  if (district_global_id >= GEO_TOTAL_DISTRICTS) return false;
  uint8_t byte_idx = district_global_id / 8;
  uint8_t bit_idx = district_global_id % 8;
  if (byte_idx >= DISTRICT_MASK_BYTES) return false;
  return (s_state.district_mask[byte_idx] & (1 << bit_idx)) != 0;
}

AlertLevel alert_model_get_district_alert_level(uint8_t district_global_id) {
  if (district_global_id >= GEO_TOTAL_DISTRICTS) return ALERT_LEVEL_NONE;

  const DistrictGeoInfo *dg = geo_model_get_district(district_global_id);
  if (dg && alert_model_is_region_full_alert(dg->region_id)) {
    return alert_model_get_region_alert_level(dg->region_id);
  }

  uint8_t byte_idx = district_global_id / 4;
  uint8_t shift = (district_global_id % 4) * 2;
  if (byte_idx >= DISTRICT_LEVELS_BYTES) return ALERT_LEVEL_NONE;
  AlertLevel lvl = (AlertLevel)((s_state.district_levels[byte_idx] >> shift) & 0x03);
  if (lvl == ALERT_LEVEL_NONE && alert_model_is_district_alert(district_global_id)) {
    return ALERT_LEVEL_RED;
  }
  return lvl;
}

AlertLevel alert_model_get_region_alert_level(uint8_t region_id) {
  if (region_id >= REGION_MASK_COUNT) return ALERT_LEVEL_NONE;

  uint8_t byte_idx = region_id / 4;
  uint8_t shift = (region_id % 4) * 2;
  if (byte_idx >= REGION_LEVELS_BYTES) return ALERT_LEVEL_NONE;
  AlertLevel lvl = (AlertLevel)((s_state.region_levels[byte_idx] >> shift) & 0x03);
  if (lvl == ALERT_LEVEL_NONE && alert_model_is_region_full_alert(region_id)) {
    return ALERT_LEVEL_RED;
  }
  return lvl;
}

GColor alert_model_get_level_color(AlertLevel level) {
  switch (level) {
    case ALERT_LEVEL_RED:
      return COLOR_ALERT_RED;
    case ALERT_LEVEL_ORANGE:
      return COLOR_ALERT_ORANGE;
    case ALERT_LEVEL_YELLOW:
      return COLOR_ALERT_YELLOW;
    default:
      return COLOR_MAP_BASE;
  }
}

uint16_t alert_model_get_region_duration(uint8_t region_id) {
  if (region_id >= REGION_MASK_COUNT) return 0xFFFF;
  return s_state.region_durations[region_id];
}

void alert_model_update(uint32_t region_mask, const uint8_t *district_mask, uint8_t reg_count, uint8_t dist_count, const char *summary) {
  s_state.region_mask = region_mask;
  if (district_mask) {
    memcpy(s_state.district_mask, district_mask, DISTRICT_MASK_BYTES);
  } else {
    memset(s_state.district_mask, 0, DISTRICT_MASK_BYTES);
  }

  // Precalculate partial_region_mask for O(1) queries
  s_state.partial_region_mask = 0;
  if (district_mask) {
    for (int d = 0; d < GEO_TOTAL_DISTRICTS; d++) {
      uint8_t byte_idx = d / 8;
      uint8_t bit_idx = d % 8;
      if (byte_idx < DISTRICT_MASK_BYTES && (s_state.district_mask[byte_idx] & (1 << bit_idx))) {
        const DistrictGeoInfo *dg = geo_model_get_district(d);
        if (dg && dg->region_id < REGION_MASK_COUNT) {
          if (!(s_state.region_mask & (1UL << dg->region_id))) {
            s_state.partial_region_mask |= (1UL << dg->region_id);
          }
        }
      }
    }
  }

  s_state.active_region_count = reg_count;
  s_state.active_district_count = dist_count;
  s_state.last_update_time = time(NULL);
  if (summary && strlen(summary) > 0) {
    strncpy(s_state.summary_text, summary, sizeof(s_state.summary_text) - 1);
    s_state.summary_text[sizeof(s_state.summary_text) - 1] = '\0';
  }
  s_state.is_loaded = true;
}

void alert_model_set_user_location(bool valid, uint16_t x, uint16_t y) {
  s_state.has_user_location = valid;
  s_state.user_loc_x = x;
  s_state.user_loc_y = y;
}

void alert_model_set_region_durations(const uint8_t *data, size_t len) {
  if (!data) return;
  size_t count = len / 2;
  if (count > REGION_MASK_COUNT) count = REGION_MASK_COUNT;
  for (size_t i = 0; i < count; i++) {
    s_state.region_durations[i] = (uint16_t)data[i * 2] | ((uint16_t)data[i * 2 + 1] << 8);
  }
}

void alert_model_set_district_levels(const uint8_t *data, size_t len) {
  if (!data) {
    memset(s_state.district_levels, 0, sizeof(s_state.district_levels));
    return;
  }
  size_t count = len < DISTRICT_LEVELS_BYTES ? len : DISTRICT_LEVELS_BYTES;
  memcpy(s_state.district_levels, data, count);
  if (count < DISTRICT_LEVELS_BYTES) {
    memset(s_state.district_levels + count, 0, DISTRICT_LEVELS_BYTES - count);
  }
}

void alert_model_set_region_levels(const uint8_t *data, size_t len) {
  if (!data) {
    memset(s_state.region_levels, 0, sizeof(s_state.region_levels));
    return;
  }
  size_t count = len < REGION_LEVELS_BYTES ? len : REGION_LEVELS_BYTES;
  memcpy(s_state.region_levels, data, count);
  if (count < REGION_LEVELS_BYTES) {
    memset(s_state.region_levels + count, 0, REGION_LEVELS_BYTES - count);
  }
}
