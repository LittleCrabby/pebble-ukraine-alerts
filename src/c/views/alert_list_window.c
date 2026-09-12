#include "alert_list_window.h"
#include "map_window.h"
#include "../models/alert_model.h"
#include "../models/geo_model.h"
#include "../util/i18n.h"
#include "../config.h"

static Window *s_window = NULL;
static MenuLayer *s_menu_layer = NULL;
static GFont s_font_cyrillic = NULL;

typedef struct {
  uint8_t region_id;
  uint8_t active_dists;
  bool is_full;
} ActiveAlertRow;

static ActiveAlertRow s_alert_rows[GEO_TOTAL_REGIONS];
static uint16_t s_alert_count = 0;

// User safety row
static char s_user_title[64] = "";
static char s_user_subtitle[64] = "";
static GColor s_user_color;

static void format_duration(uint16_t dur, char *out_buf, size_t buf_len) {
  if (dur == 0xFFFF) {
    out_buf[0] = '\0';
    return;
  }
  if (dur >= 1440) {
    snprintf(out_buf, buf_len, i18n_get(I18N_DUR_DAYS_FMT), dur / 1440);
  } else if (dur >= 60) {
    snprintf(out_buf, buf_len, i18n_get(I18N_DUR_HOURS_MINS_FMT), dur / 60, dur % 60);
  } else {
    snprintf(out_buf, buf_len, i18n_get(I18N_DUR_MINS_FMT), dur);
  }
}

static void rebuild_menu_items(void) {
  AlertState *st = alert_model_get_state();

  // 1. Build User Location status
  if (st->has_user_location) {
    int reg_id = geo_model_find_region_at(st->user_loc_x, st->user_loc_y);
    int dist_id = geo_model_find_district_at(st->user_loc_x, st->user_loc_y);

    const DistrictGeoInfo *d = (dist_id >= 0) ? geo_model_get_district(dist_id) : NULL;

    if (d) {
      i18n_format_district_name(d->name, s_user_title, sizeof(s_user_title));
    } else if (reg_id >= 0) {
      strncpy(s_user_title, i18n_get_region_name(reg_id), sizeof(s_user_title) - 1);
    } else {
      strncpy(s_user_title, i18n_get(I18N_MY_LOCATION), sizeof(s_user_title) - 1);
    }
    s_user_title[sizeof(s_user_title) - 1] = '\0';

    bool is_full = (reg_id >= 0) && alert_model_is_region_full_alert(reg_id);
    bool is_dist = (dist_id >= 0) && alert_model_is_district_alert(dist_id);

    if (is_full || is_dist) {
      s_user_color = COLOR_ALERT_FULL;
      uint16_t dur = (reg_id >= 0) ? alert_model_get_region_duration(reg_id) : 0xFFFF;
      char dur_str[16] = "";
      format_duration(dur, dur_str, sizeof(dur_str));
      if (strlen(dur_str) > 0) {
        snprintf(s_user_subtitle, sizeof(s_user_subtitle), "%s (%s)", i18n_get(I18N_ALERT), dur_str);
      } else {
        strncpy(s_user_subtitle, i18n_get(I18N_ALERT), sizeof(s_user_subtitle) - 1);
      }
    } else {
      s_user_color = GColorDarkGreen;
      strncpy(s_user_subtitle, i18n_get(I18N_ALL_CLEAR_CALM), sizeof(s_user_subtitle) - 1);
    }
  } else {
    s_user_color = GColorCobaltBlue;
    strncpy(s_user_title, i18n_get(I18N_MY_LOCATION), sizeof(s_user_title) - 1);
    s_user_title[sizeof(s_user_title) - 1] = '\0';
    strncpy(s_user_subtitle, i18n_get(I18N_POS_NOT_DETERMINED), sizeof(s_user_subtitle) - 1);
  }
  s_user_subtitle[sizeof(s_user_subtitle) - 1] = '\0';

  // 2. Build Active Alert rows
  s_alert_count = 0;
  if (st->is_loaded) {
    for (int i = 0; i < GEO_TOTAL_REGIONS; i++) {
      const RegionGeoInfo *reg = geo_model_get_region(i);
      if (!reg) continue;

      bool is_full = alert_model_is_region_full_alert(i);
      bool is_part = alert_model_is_region_partial_alert(i);

      if (is_full || is_part) {
        ActiveAlertRow *row = &s_alert_rows[s_alert_count];
        row->region_id = (uint8_t)i;
        row->is_full = is_full;
        row->active_dists = 0;

        if (!is_full) {
          for (int d = 0; d < GEO_TOTAL_DISTRICTS; d++) {
            const DistrictGeoInfo *dg = geo_model_get_district(d);
            if (dg && dg->region_id == i && alert_model_is_district_alert(d)) {
              row->active_dists++;
            }
          }
        }
        s_alert_count++;
      }
    }
  }
}

static uint16_t menu_get_num_sections_callback(MenuLayer *menu_layer, void *data) {
  return 3;
}

static uint16_t menu_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
  switch (section_index) {
    case 0: return 1; // My Safety
    case 1: return (s_alert_count > 0) ? s_alert_count : 1; // Active Alerts or Calm
    case 2: return 1; // Refresh
    default: return 0;
  }
}

static int16_t menu_get_header_height_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
  return 22;
}

static void menu_draw_header_callback(GContext *ctx, const Layer *cell_layer, uint16_t section_index, void *data) {
  GRect bounds = layer_get_bounds(cell_layer);
  graphics_context_set_fill_color(ctx, GColorDarkGray);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);
  graphics_context_set_text_color(ctx, GColorWhite);

  char header_text[40] = "";
  if (section_index == 0) {
    strncpy(header_text, i18n_get(I18N_SEC_MY_SAFETY), sizeof(header_text) - 1);
  } else if (section_index == 1) {
    AlertState *st = alert_model_get_state();
    snprintf(header_text, sizeof(header_text), i18n_get(I18N_SEC_ACTIVE_ALERTS_FMT), (int)st->active_region_count);
  } else {
    strncpy(header_text, i18n_get(I18N_SEC_ACTIONS), sizeof(header_text) - 1);
  }

  graphics_draw_text(ctx, header_text, s_font_cyrillic,
                     GRect(8, 2, bounds.size.w - 16, 18),
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
}

static int16_t menu_get_cell_height_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
  return 46;
}

static void menu_draw_row_callback(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data) {
  GRect bounds = layer_get_bounds(cell_layer);
  bool is_highlighted = menu_cell_layer_is_highlighted(cell_layer);

  if (is_highlighted) {
    graphics_context_set_fill_color(ctx, GColorCobaltBlue);
    graphics_fill_rect(ctx, bounds, 0, GCornerNone);
  }

  GColor text_color = is_highlighted ? GColorWhite : GColorBlack;
  GColor sub_color = is_highlighted ? GColorLightGray : GColorDarkGray;

  const char *title = "";
  const char *subtitle = "";
  char sub_buf[48] = "";
  GColor dot_color = GColorDarkGray;
  bool is_pin = false;

  if (cell_index->section == 0) {
    title = s_user_title;
    subtitle = s_user_subtitle;
    dot_color = s_user_color;
    is_pin = true;
  } else if (cell_index->section == 1) {
    if (s_alert_count == 0) {
      title = i18n_get(I18N_ALL_REGIONS_CALM);
      subtitle = i18n_get(I18N_NO_ALERTS_RECORDED);
      dot_color = GColorDarkGreen;
    } else if (cell_index->row < s_alert_count) {
      ActiveAlertRow *row = &s_alert_rows[cell_index->row];
      title = i18n_get_region_name(row->region_id);

      uint16_t dur = alert_model_get_region_duration(row->region_id);
      char dur_str[16] = "";
      format_duration(dur, dur_str, sizeof(dur_str));

      if (row->is_full) {
        dot_color = COLOR_ALERT_FULL;
        if (strlen(dur_str) > 0) {
          snprintf(sub_buf, sizeof(sub_buf), i18n_get(I18N_ENTIRE_REGION_DUR_FMT), dur_str);
        } else {
          strncpy(sub_buf, i18n_get(I18N_ENTIRE_REGION), sizeof(sub_buf) - 1);
        }
      } else {
        dot_color = GColorOrange;
        if (strlen(dur_str) > 0) {
          snprintf(sub_buf, sizeof(sub_buf), i18n_get(I18N_DISTS_DUR_FMT), (int)row->active_dists, dur_str);
        } else {
          snprintf(sub_buf, sizeof(sub_buf), i18n_get(I18N_DISTS_COUNT_FMT), (int)row->active_dists);
        }
      }
      subtitle = sub_buf;
    }
  } else if (cell_index->section == 2) {
    if (cell_index->row == 0) {
      title = i18n_get(I18N_LANGUAGE);
      subtitle = i18n_get_lang_display_name(i18n_get_selected_lang());
      dot_color = GColorChromeYellow;
    }
  }

  // Indicator dot at x = 11, y = bounds.size.h / 2
  GPoint dot_center = GPoint(11, bounds.size.h / 2);
  graphics_context_set_fill_color(ctx, dot_color);
  graphics_fill_circle(ctx, dot_center, 5);
  graphics_context_set_stroke_color(ctx, is_highlighted ? GColorCobaltBlue : GColorWhite);
  graphics_draw_circle(ctx, dot_center, 5);
  if (is_pin) {
    graphics_context_set_fill_color(ctx, GColorWhite);
    graphics_fill_circle(ctx, dot_center, 2);
  }

  // Title & Subtitle using font
  graphics_context_set_text_color(ctx, text_color);
  graphics_draw_text(ctx, title, s_font_cyrillic,
                     GRect(22, 3, bounds.size.w - 26, 19),
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);

  graphics_context_set_text_color(ctx, sub_color);
  graphics_draw_text(ctx, subtitle, s_font_cyrillic,
                     GRect(22, 23, bounds.size.w - 26, 19),
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
}

static void menu_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
  if (cell_index->section == 0) {
    AlertState *st = alert_model_get_state();
    if (st->has_user_location) {
      map_window_focus_user_location();
      window_stack_pop(true);
    } else {
      vibes_double_pulse();
    }
  } else if (cell_index->section == 1) {
    if (s_alert_count > 0 && cell_index->row < s_alert_count) {
      int reg_id = s_alert_rows[cell_index->row].region_id;
      map_window_focus_region(reg_id);
      window_stack_pop(true);
    }
  } else if (cell_index->section == 2) {
    if (cell_index->row == 0) {
      i18n_cycle_lang();
      vibes_short_pulse();
      rebuild_menu_items();
      menu_layer_reload_data(s_menu_layer);
      map_window_mark_dirty();
    }
  }
}

// Cyclic button click handlers
static void menu_up_click_handler(ClickRecognizerRef recognizer, void *context) {
  MenuIndex cur = menu_layer_get_selected_index(s_menu_layer);
  if (cur.section == 0 && cur.row == 0) {
    // Jump from top item to bottom item (Section 2, Row 0)
    MenuIndex last = MenuIndex(2, 0);
    menu_layer_set_selected_index(s_menu_layer, last, MenuRowAlignBottom, true);
  } else {
    menu_layer_set_selected_next(s_menu_layer, true, MenuRowAlignCenter, true);
  }
}

static void menu_down_click_handler(ClickRecognizerRef recognizer, void *context) {
  MenuIndex cur = menu_layer_get_selected_index(s_menu_layer);
  if (cur.section == 2 && cur.row == 0) {
    // Jump from bottom item to top item (Section 0, Row 0)
    MenuIndex first = MenuIndex(0, 0);
    menu_layer_set_selected_index(s_menu_layer, first, MenuRowAlignTop, true);
  } else {
    menu_layer_set_selected_next(s_menu_layer, false, MenuRowAlignCenter, true);
  }
}

static void menu_select_click_handler(ClickRecognizerRef recognizer, void *context) {
  MenuIndex idx = menu_layer_get_selected_index(s_menu_layer);
  menu_select_callback(s_menu_layer, &idx, NULL);
}

static void menu_click_config_provider(void *context) {
  window_single_repeating_click_subscribe(BUTTON_ID_UP, 150, menu_up_click_handler);
  window_single_repeating_click_subscribe(BUTTON_ID_DOWN, 150, menu_down_click_handler);
  window_single_click_subscribe(BUTTON_ID_SELECT, menu_select_click_handler);
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_font_cyrillic = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_CYRILLIC_15));

  rebuild_menu_items();

  s_menu_layer = menu_layer_create(bounds);
  menu_layer_set_callbacks(s_menu_layer, NULL, (MenuLayerCallbacks) {
    .get_num_sections = menu_get_num_sections_callback,
    .get_num_rows = menu_get_num_rows_callback,
    .get_header_height = menu_get_header_height_callback,
    .get_cell_height = menu_get_cell_height_callback,
    .draw_header = menu_draw_header_callback,
    .draw_row = menu_draw_row_callback,
    .select_click = menu_select_callback,
  });

  window_set_click_config_provider(window, menu_click_config_provider);
  layer_add_child(window_layer, menu_layer_get_layer(s_menu_layer));
}

static void window_unload(Window *window) {
  menu_layer_destroy(s_menu_layer);
  s_menu_layer = NULL;
  if (s_font_cyrillic) {
    fonts_unload_custom_font(s_font_cyrillic);
    s_font_cyrillic = NULL;
  }
}

void alert_list_window_init(void) {
  if (!s_window) {
    s_window = window_create();
    window_set_window_handlers(s_window, (WindowHandlers) {
      .load = window_load,
      .unload = window_unload,
    });
  }
}

void alert_list_window_deinit(void) {
  if (s_window) {
    window_destroy(s_window);
    s_window = NULL;
  }
}

void alert_list_window_push(void) {
  if (!s_window) {
    alert_list_window_init();
  }
  window_stack_push(s_window, true);
}

void alert_list_window_refresh(void) {
  if (s_menu_layer && s_window && window_is_loaded(s_window)) {
    rebuild_menu_items();
    menu_layer_reload_data(s_menu_layer);
  }
}
