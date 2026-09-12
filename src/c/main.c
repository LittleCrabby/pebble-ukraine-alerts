#include <pebble.h>
#include "models/alert_model.h"
#include "services/comm_service.h"
#include "views/map_window.h"
#include "views/alert_list_window.h"
#include "util/i18n.h"

static void on_data_updated(void) {
  map_window_mark_dirty();
  alert_list_window_refresh();
}

static void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed) {
  map_window_mark_dirty();
}

static void init(void) {
  i18n_init();
  alert_model_init();
  comm_service_init(on_data_updated);
  map_window_init();
  alert_list_window_init();
  
  // Subscribe to clock minute ticks for auto-updating time
  tick_timer_service_subscribe(MINUTE_UNIT, handle_minute_tick);
  
  // Request fresh data from JS
  comm_service_request_data();
}

static void deinit(void) {
  tick_timer_service_unsubscribe();
  alert_list_window_deinit();
  map_window_deinit();
  comm_service_deinit();
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
