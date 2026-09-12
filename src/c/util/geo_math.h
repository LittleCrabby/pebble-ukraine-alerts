#pragma once
#include <pebble.h>
#include "../config.h"
#include "../models/geo_model.h"

typedef struct {
  int16_t zoom_percent; // 100 = 1.0x, 150 = 1.5x, etc.
  int16_t pan_x;        // screen pixel offset
  int16_t pan_y;        // screen pixel offset
  GRect viewport;       // drawing area
  
  // Fast projection cache
  int32_t origin_x;
  int32_t origin_y;
  int32_t full_w;
  int32_t full_h;
} CameraState;

void geo_math_init_camera(CameraState *cam, GRect bounds);
void geo_math_reset_camera(CameraState *cam, GRect bounds);
void geo_math_update_camera(CameraState *cam);
void geo_math_clamp_pan(CameraState *cam);
GPoint geo_math_project(const CameraState *cam, uint16_t norm_x, uint16_t norm_y);
void geo_math_unproject(const CameraState *cam, GPoint screen_pt, uint16_t *out_norm_x, uint16_t *out_norm_y);
void geo_math_center_on(CameraState *cam, uint16_t norm_x, uint16_t norm_y);
