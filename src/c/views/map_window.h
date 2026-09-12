#pragma once
#include <pebble.h>

void map_window_init(void);
void map_window_deinit(void);
void map_window_mark_dirty(void);
void map_window_focus_region(int region_id);
void map_window_focus_user_location(void);
void map_window_reset_view(void);
