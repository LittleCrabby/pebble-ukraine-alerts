#include "map_window.h"
#include "map_renderer.h"
#include "alert_list_window.h"
#include "../models/alert_model.h"
#include "../models/geo_model.h"
#include "../util/geo_math.h"
#include "../util/i18n.h"
#include "../config.h"

static Window *s_window;
static Layer *s_map_layer;
static CameraState s_camera;
static int s_selected_region = -1;
static int s_selected_district = -1;
static GFont s_font_cyrillic;

// 2-line bottom banner
static char s_banner_title[64] = "";
static char s_banner_status[64] = "";
static GColor s_banner_status_color;

static void map_window_update_banner(int region_id, int district_id) {
  s_selected_region = region_id;
  s_selected_district = district_id;

  if (district_id >= 0 && district_id < GEO_TOTAL_DISTRICTS) {
    const DistrictGeoInfo *d = geo_model_get_district(district_id);
    if (d) {
      i18n_format_district_name(d->name, s_banner_title, sizeof(s_banner_title));
      if (region_id < 0) region_id = d->region_id;
    } else {
      strncpy(s_banner_title, i18n_get(I18N_MY_LOCATION), sizeof(s_banner_title) - 1);
    }
    s_banner_title[sizeof(s_banner_title) - 1] = '\0';

    bool is_dist_alert = alert_model_is_district_alert(district_id);
    bool is_full_alert = (region_id >= 0) && alert_model_is_region_full_alert(region_id);
    if (is_dist_alert || is_full_alert) {
      strncpy(s_banner_status, i18n_get(I18N_ALERT), sizeof(s_banner_status) - 1);
      s_banner_status_color = COLOR_ALERT_FULL;
    } else {
      strncpy(s_banner_status, i18n_get(I18N_CALM), sizeof(s_banner_status) - 1);
      s_banner_status_color = GColorDarkGreen;
    }
  } else if (region_id >= 0 && region_id < GEO_TOTAL_REGIONS) {
    strncpy(s_banner_title, i18n_get_region_name(region_id), sizeof(s_banner_title) - 1);
    s_banner_title[sizeof(s_banner_title) - 1] = '\0';

    if (alert_model_is_region_full_alert(region_id)) {
      strncpy(s_banner_status, i18n_get(I18N_ALERT), sizeof(s_banner_status) - 1);
      s_banner_status_color = COLOR_ALERT_FULL;
    } else if (alert_model_is_region_partial_alert(region_id)) {
      strncpy(s_banner_status, i18n_get(I18N_DISTRICT_ALERT), sizeof(s_banner_status) - 1);
      s_banner_status_color = COLOR_ALERT_FULL;
    } else {
      strncpy(s_banner_status, i18n_get(I18N_CALM), sizeof(s_banner_status) - 1);
      s_banner_status_color = GColorDarkGreen;
    }
  } else {
    s_selected_region = -1;
    s_selected_district = -1;
    s_banner_title[0] = '\0';
    s_banner_status[0] = '\0';
  }
}

#if defined(PBL_TOUCH)
static GPoint s_touch_start;
static GPoint s_pan_start;
static bool s_is_touching = false;

static void touch_handler(const TouchEvent *event, void *context) {
  switch (event->type) {
    case TouchEvent_Touchdown:
      s_is_touching = true;
      s_touch_start = GPoint(event->x, event->y);
      s_pan_start = GPoint(s_camera.pan_x, s_camera.pan_y);
      break;

    case TouchEvent_PositionUpdate:
      if (s_is_touching) {
        int16_t dx = event->x - s_touch_start.x;
        int16_t dy = event->y - s_touch_start.y;
        int16_t new_pan_x = s_pan_start.x + dx;
        int16_t new_pan_y = s_pan_start.y + dy;
        if (abs(new_pan_x - s_camera.pan_x) >= 2 || abs(new_pan_y - s_camera.pan_y) >= 2) {
          s_camera.pan_x = new_pan_x;
          s_camera.pan_y = new_pan_y;
          geo_math_update_camera(&s_camera);

          // Re-align s_pan_start to clamped position to eliminate dead zone on reverse drag
          s_pan_start.x = s_camera.pan_x - dx;
          s_pan_start.y = s_camera.pan_y - dy;

          layer_mark_dirty(s_map_layer);
        }
      }
      break;

    case TouchEvent_Liftoff:
      if (s_is_touching) {
        int16_t dx = event->x - s_touch_start.x;
        int16_t dy = event->y - s_touch_start.y;
        if (abs(dx) < 16 && abs(dy) < 16) {
          // Revert micro-pan caused by finger jitter during tap
          s_camera.pan_x = s_pan_start.x;
          s_camera.pan_y = s_pan_start.y;
          geo_math_update_camera(&s_camera);

          uint16_t nx, ny;
          geo_math_unproject(&s_camera, GPoint(event->x, event->y), &nx, &ny);

          int reg_id = geo_model_find_region_at(nx, ny);
          int dist_id = geo_model_find_district_at(nx, ny);
          if (reg_id < 0 && dist_id >= 0) {
            const DistrictGeoInfo *d = geo_model_get_district(dist_id);
            if (d) reg_id = d->region_id;
          }

          if (reg_id >= 0 || dist_id >= 0) {
            bool is_part = (reg_id >= 0) && alert_model_is_region_partial_alert(reg_id);
            if (is_part && dist_id >= 0) {
              map_window_update_banner(-1, dist_id);
            } else {
              map_window_update_banner(reg_id, -1);
            }
          } else {
            map_window_update_banner(-1, -1);
          }
        }
        s_is_touching = false;
        layer_mark_dirty(s_map_layer);
      }
      break;
  }
}
#endif

static void map_layer_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);

  // Clear background
  graphics_context_set_fill_color(ctx, COLOR_BG);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  // Render map
  AlertState *alerts = alert_model_get_state();
  map_renderer_draw(ctx, &s_camera, alerts, s_selected_region, s_selected_district);

  GFont font = s_font_cyrillic ? s_font_cyrillic : fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD);

  // Top status bar: CURRENT TIME (Clock)
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  char time_str[16];
  if (clock_is_24h_style()) {
    strftime(time_str, sizeof(time_str), "%H:%M", t);
  } else {
    strftime(time_str, sizeof(time_str), "%I:%M", t);
  }

  int16_t header_y = SCREEN_IS_ROUND ? 14 : 4;
  GRect time_rect = GRect(0, header_y, bounds.size.w, 20);

  // If zoomed in, draw backdrop pill for clock for clean legibility over map graphics
  if (s_camera.zoom_percent > ZOOM_LEVEL_MIN) {
    GRect clock_bg = GRect(bounds.size.w / 2 - 28, header_y, 56, 24);
    graphics_context_set_fill_color(ctx, GColorWhite);
    graphics_fill_rect(ctx, clock_bg, 4, GCornersAll);
    graphics_context_set_stroke_color(ctx, GColorDarkGray);
    graphics_draw_round_rect(ctx, clock_bg, 4);
  }

  graphics_context_set_text_color(ctx, COLOR_TEXT_DARK);
  graphics_draw_text(ctx, time_str, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
                     time_rect, GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);

  // Bottom banner: 2 lines (Line 1: Name, Line 2: Status)
  if (s_selected_region < 0 && s_selected_district < 0 && alerts && alerts->is_loaded) {
    // Default summary when no region tapped
    if (alerts->active_region_count > 0 || alerts->active_district_count > 0) {
      snprintf(s_banner_title, sizeof(s_banner_title), "%s", i18n_get(I18N_ACTIVE_ALERTS));
      snprintf(s_banner_status, sizeof(s_banner_status), i18n_get(I18N_ACTIVE_SUMMARY_FMT), alerts->active_region_count, alerts->active_district_count);
      s_banner_status_color = COLOR_ALERT_FULL;
    } else {
      snprintf(s_banner_title, sizeof(s_banner_title), "%s", i18n_get(I18N_UKRAINE));
      snprintf(s_banner_status, sizeof(s_banner_status), "%s", i18n_get(I18N_NO_ALERTS));
      s_banner_status_color = GColorDarkGreen;
    }
  }

  if (strlen(s_banner_title) > 0) {
    int16_t bottom_y = bounds.size.h - (SCREEN_IS_ROUND ? 46 : 38);

    // Draw white backdrop card when zoomed in so text doesn't collide with map boundaries
    if (s_camera.zoom_percent > ZOOM_LEVEL_MIN) {
      int16_t pad = SCREEN_IS_ROUND ? 26 : 8;
      GRect banner_bg = GRect(pad, bottom_y - 2, bounds.size.w - 2 * pad, 38);
      graphics_context_set_fill_color(ctx, GColorWhite);
      graphics_fill_rect(ctx, banner_bg, 4, GCornersAll);
      graphics_context_set_stroke_color(ctx, GColorDarkGray);
      graphics_draw_round_rect(ctx, banner_bg, 4);
    }

    // Line 1: Title
    graphics_context_set_text_color(ctx, COLOR_TEXT_DARK);
    GRect title_rect = GRect(10, bottom_y, bounds.size.w - 20, 18);
    graphics_draw_text(ctx, s_banner_title, font,
                       title_rect, GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);

    // Line 2: Status
    graphics_context_set_text_color(ctx, s_banner_status_color);
    GRect status_rect = GRect(10, bottom_y + 18, bounds.size.w - 20, 18);
    graphics_draw_text(ctx, s_banner_status, font,
                       status_rect, GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
  }
}

static void get_zoom_focus_target(uint16_t *out_x, uint16_t *out_y, bool *has_target) {
  AlertState *alerts = alert_model_get_state();

  // If a district is selected other than the user's current district, stay centered on it
  if (s_selected_district >= 0) {
    int user_dist_id = (alerts && alerts->has_user_location) ?
        geo_model_find_district_at(alerts->user_loc_x, alerts->user_loc_y) : -1;
    if (s_selected_district != user_dist_id) {
      const DistrictGeoInfo *d = geo_model_get_district(s_selected_district);
      if (d) {
        *out_x = d->cx;
        *out_y = d->cy;
        *has_target = true;
        return;
      }
    }
  }

  // If a region is selected other than the user's current region, stay centered on it
  if (s_selected_region >= 0) {
    int user_reg_id = (alerts && alerts->has_user_location) ?
        geo_model_find_region_at(alerts->user_loc_x, alerts->user_loc_y) : -1;
    if (s_selected_region != user_reg_id) {
      const RegionGeoInfo *r = geo_model_get_region(s_selected_region);
      if (r) {
        *out_x = r->cx;
        *out_y = r->cy;
        *has_target = true;
        return;
      }
    }
  }

  // Zoom to user location if available
  if (alerts && alerts->has_user_location) {
    *out_x = alerts->user_loc_x;
    *out_y = alerts->user_loc_y;
    *has_target = true;
    return;
  }

  *has_target = false;
}

static void apply_zoom(int16_t new_zoom) {
  if (new_zoom < ZOOM_LEVEL_MIN || new_zoom > ZOOM_LEVEL_MAX) return;

  s_camera.zoom_percent = new_zoom;

  if (s_camera.zoom_percent == ZOOM_LEVEL_MIN) {
    s_camera.pan_x = 0;
    s_camera.pan_y = 0;
    geo_math_update_camera(&s_camera);
  } else {
    uint16_t target_x = 0, target_y = 0;
    bool has_target = false;
    get_zoom_focus_target(&target_x, &target_y, &has_target);

    if (has_target) {
      geo_math_center_on(&s_camera, target_x, target_y);
    } else {
      s_camera.pan_x = 0;
      s_camera.pan_y = 0;
      geo_math_update_camera(&s_camera);
    }
  }

  layer_mark_dirty(s_map_layer);
}

// Button Handlers
static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (s_camera.zoom_percent < ZOOM_LEVEL_MAX) {
    apply_zoom(s_camera.zoom_percent + ZOOM_STEP);
  }
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (s_camera.zoom_percent > ZOOM_LEVEL_MIN) {
    apply_zoom(s_camera.zoom_percent - ZOOM_STEP);
  }
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  alert_list_window_push();
}

static void back_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (s_selected_region >= 0 || s_selected_district >= 0) {
    s_selected_region = -1;
    s_selected_district = -1;
    s_banner_title[0] = '\0';
    s_banner_status[0] = '\0';
    layer_mark_dirty(s_map_layer);
    return;
  }
  if (s_camera.zoom_percent > ZOOM_LEVEL_MIN || s_camera.pan_x != 0 || s_camera.pan_y != 0) {
    geo_math_reset_camera(&s_camera, layer_get_bounds(s_map_layer));
    s_selected_region = -1;
    s_selected_district = -1;
    s_banner_title[0] = '\0';
    s_banner_status[0] = '\0';
    layer_mark_dirty(s_map_layer);
  } else {
    window_stack_pop(true);
  }
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_click_subscribe(BUTTON_ID_BACK, back_click_handler);
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  geo_math_init_camera(&s_camera, bounds);
  s_font_cyrillic = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_CYRILLIC_15));
  strncpy(s_banner_title, i18n_get(I18N_UKRAINE), sizeof(s_banner_title) - 1);
  strncpy(s_banner_status, i18n_get(I18N_LOADING), sizeof(s_banner_status) - 1);
  s_banner_status_color = GColorDarkGreen;

  s_map_layer = layer_create(bounds);
  layer_set_update_proc(s_map_layer, map_layer_update_proc);
  layer_add_child(window_layer, s_map_layer);

#if defined(PBL_TOUCH)
  app_touch_navigation_enable(true);
  touch_service_subscribe(touch_handler, NULL);
#endif
}

static void window_unload(Window *window) {
#if defined(PBL_TOUCH)
  touch_service_unsubscribe();
#endif
  if (s_font_cyrillic) fonts_unload_custom_font(s_font_cyrillic);
  layer_destroy(s_map_layer);
}

void map_window_init(void) {
  s_window = window_create();
  window_set_click_config_provider(s_window, click_config_provider);
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  window_stack_push(s_window, true);
}

void map_window_deinit(void) {
  window_destroy(s_window);
}

void map_window_mark_dirty(void) {
  if (s_map_layer) {
    layer_mark_dirty(s_map_layer);
  }
}

void map_window_focus_region(int region_id) {
  if (region_id < 0 || region_id >= GEO_TOTAL_REGIONS) return;
  const RegionGeoInfo *r = geo_model_get_region(region_id);
  if (!r) return;

  s_camera.zoom_percent = (r->num_points < 30) ? 300 : 200;
  geo_math_center_on(&s_camera, r->cx, r->cy);

  map_window_update_banner(region_id, -1);

  if (s_map_layer) layer_mark_dirty(s_map_layer);
}

void map_window_focus_user_location(void) {
  AlertState *alerts = alert_model_get_state();
  if (!alerts || !alerts->has_user_location) return;

  s_camera.zoom_percent = ZOOM_LEVEL_MAX;
  geo_math_center_on(&s_camera, alerts->user_loc_x, alerts->user_loc_y);

  int reg_id = geo_model_find_region_at(alerts->user_loc_x, alerts->user_loc_y);
  int dist_id = geo_model_find_district_at(alerts->user_loc_x, alerts->user_loc_y);

  if (dist_id >= 0) {
    map_window_update_banner(reg_id, dist_id);
  } else if (reg_id >= 0) {
    map_window_update_banner(reg_id, -1);
  } else {
    s_selected_region = -1;
    s_selected_district = -1;
    strncpy(s_banner_title, i18n_get(I18N_MY_LOCATION), sizeof(s_banner_title) - 1);
    strncpy(s_banner_status, i18n_get(I18N_IN_UKRAINE), sizeof(s_banner_status) - 1);
    s_banner_status_color = COLOR_ACCENT;
  }

  if (s_map_layer) layer_mark_dirty(s_map_layer);
}

void map_window_reset_view(void) {
  s_selected_region = -1;
  s_selected_district = -1;
  s_banner_title[0] = '\0';
  s_banner_status[0] = '\0';
  if (s_map_layer) {
    geo_math_reset_camera(&s_camera, layer_get_bounds(s_map_layer));
    layer_mark_dirty(s_map_layer);
  }
}
