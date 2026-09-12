#include "comm_service.h"
#include "../models/alert_model.h"

static CommServiceUpdateHandler s_update_handler = NULL;

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  Tuple *region_mask_tuple = dict_find(iterator, MESSAGE_KEY_ALERT_REGIONS_MASK);
  Tuple *dist_mask_tuple = dict_find(iterator, MESSAGE_KEY_DISTRICT_ALERTS_MASK);
  Tuple *active_cnt_tuple = dict_find(iterator, MESSAGE_KEY_ACTIVE_COUNT);
  Tuple *dist_cnt_tuple = dict_find(iterator, MESSAGE_KEY_ACTIVE_DISTRICT_COUNT);
  Tuple *summary_tuple = dict_find(iterator, MESSAGE_KEY_SUMMARY_TEXT);
  Tuple *loc_valid_tuple = dict_find(iterator, MESSAGE_KEY_USER_LOCATION_VALID);
  Tuple *loc_x_tuple = dict_find(iterator, MESSAGE_KEY_USER_LOCATION_X);
  Tuple *loc_y_tuple = dict_find(iterator, MESSAGE_KEY_USER_LOCATION_Y);
  Tuple *durations_tuple = dict_find(iterator, MESSAGE_KEY_REGION_DURATIONS);
  
  bool state_updated = false;

  if (loc_valid_tuple) {
    bool valid = loc_valid_tuple->value->int32 != 0;
    uint16_t lx = loc_x_tuple ? (uint16_t)loc_x_tuple->value->int32 : 0;
    uint16_t ly = loc_y_tuple ? (uint16_t)loc_y_tuple->value->int32 : 0;
    alert_model_set_user_location(valid, lx, ly);
    state_updated = true;
  }
  
  if (durations_tuple) {
    alert_model_set_region_durations(durations_tuple->value->data, durations_tuple->length);
    state_updated = true;
  }
  
  if (region_mask_tuple) {
    uint32_t rmask = (uint32_t)region_mask_tuple->value->int32;
    const uint8_t *dmask = dist_mask_tuple ? dist_mask_tuple->value->data : NULL;
    uint8_t reg_count = active_cnt_tuple ? (uint8_t)active_cnt_tuple->value->int32 : 0;
    uint8_t dist_count = dist_cnt_tuple ? (uint8_t)dist_cnt_tuple->value->int32 : 0;
    const char *summary = summary_tuple ? summary_tuple->value->cstring : "";
    
    // Check if new alert was added (only if already loaded, to avoid vibrating on app launch)
    AlertState *old_state = alert_model_get_state();
    bool new_alert = false;
    if (old_state->is_loaded) {
      if ((rmask & ~old_state->region_mask) != 0) {
        new_alert = true;
      } else if (dmask) {
        for (int i = 0; i < DISTRICT_MASK_BYTES; i++) {
          if ((dmask[i] & ~old_state->district_mask[i]) != 0) {
            new_alert = true;
            break;
          }
        }
      }
    }
    
    alert_model_update(rmask, dmask, reg_count, dist_count, summary);
    state_updated = true;
    
    if (new_alert) {
      vibes_short_pulse();
    }
  }

  if (state_updated && s_update_handler) {
    s_update_handler();
  }
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "AppMessage inbox dropped: %d", (int)reason);
}

void comm_service_init(CommServiceUpdateHandler on_update) {
  s_update_handler = on_update;
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  
  // 1024 inbox size for summary text, 128 outbox
  app_message_open(1024, 128);
}

void comm_service_deinit(void) {
  app_message_deregister_callbacks();
}

void comm_service_request_data(void) {
  DictionaryIterator *iter;
  if (app_message_outbox_begin(&iter) == APP_MSG_OK) {
    dict_write_uint8(iter, MESSAGE_KEY_STATUS_CODE, 1);
    app_message_outbox_send();
  }
}
