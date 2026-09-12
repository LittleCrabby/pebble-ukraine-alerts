#include "geo_math.h"

void geo_math_clamp_pan(CameraState *cam) {
  int16_t vw = cam->viewport.size.w;
  int16_t vh = cam->viewport.size.h;
  
  if (cam->zoom_percent <= ZOOM_LEVEL_MIN) {
    cam->pan_x = 0;
    cam->pan_y = 0;
    return;
  }
  
  // Padding beyond map edge so borders remain easily accessible and visible
  int16_t pad_x = SCREEN_IS_ROUND ? 30 : 20;
  int16_t pad_y = SCREEN_IS_ROUND ? 30 : 20;
  
  int32_t max_pan_x = (cam->full_w - vw) / 2 + pad_x;
  int32_t max_pan_y = (cam->full_h - vh) / 2 + pad_y;
  
  if (max_pan_x < 0) max_pan_x = 0;
  if (max_pan_y < 0) max_pan_y = 0;
  
  if (cam->pan_x > max_pan_x) cam->pan_x = max_pan_x;
  if (cam->pan_x < -max_pan_x) cam->pan_x = -max_pan_x;
  if (cam->pan_y > max_pan_y) cam->pan_y = max_pan_y;
  if (cam->pan_y < -max_pan_y) cam->pan_y = -max_pan_y;
}

void geo_math_update_camera(CameraState *cam) {
  int16_t vw = cam->viewport.size.w;
  int16_t vh = cam->viewport.size.h;
  
  int16_t margin = SCREEN_IS_ROUND ? 26 : 8;
  int16_t base_w = vw - (margin * 2);
  int16_t base_h = (base_w * UKRAINE_MAP_ASPECT_RATIO_NUM) / UKRAINE_MAP_ASPECT_RATIO_DENOM;
  
  cam->full_w = (int32_t)base_w * cam->zoom_percent / 100;
  cam->full_h = (int32_t)base_h * cam->zoom_percent / 100;
  
  geo_math_clamp_pan(cam);
  
  int16_t cx = cam->viewport.origin.x + vw / 2;
  int16_t cy = cam->viewport.origin.y + vh / 2;
  
  cam->origin_x = (cx - cam->full_w / 2) + cam->pan_x;
  cam->origin_y = (cy - cam->full_h / 2) + cam->pan_y;
}

void geo_math_init_camera(CameraState *cam, GRect bounds) {
  cam->zoom_percent = ZOOM_LEVEL_MIN;
  cam->pan_x = 0;
  cam->pan_y = 0;
  cam->viewport = bounds;
  geo_math_update_camera(cam);
}

void geo_math_reset_camera(CameraState *cam, GRect bounds) {
  cam->zoom_percent = ZOOM_LEVEL_MIN;
  cam->pan_x = 0;
  cam->pan_y = 0;
  cam->viewport = bounds;
  geo_math_update_camera(cam);
}

GPoint geo_math_project(const CameraState *cam, uint16_t norm_x, uint16_t norm_y) {
  int16_t screen_x = cam->origin_x + ((int32_t)norm_x * cam->full_w) / GEO_COORD_MAX;
  int16_t screen_y = cam->origin_y + ((int32_t)norm_y * cam->full_h) / GEO_COORD_MAX;
  return GPoint(screen_x, screen_y);
}

void geo_math_unproject(const CameraState *cam, GPoint screen_pt, uint16_t *out_norm_x, uint16_t *out_norm_y) {
  if (cam->full_w <= 0 || cam->full_h <= 0) {
    *out_norm_x = 0;
    *out_norm_y = 0;
    return;
  }
  
  int32_t rel_x = screen_pt.x - cam->origin_x;
  int32_t rel_y = screen_pt.y - cam->origin_y;
  
  int32_t nx = (rel_x * GEO_COORD_MAX) / cam->full_w;
  int32_t ny = (rel_y * GEO_COORD_MAX) / cam->full_h;
  
  if (nx < 0) nx = 0;
  if (nx > GEO_COORD_MAX) nx = GEO_COORD_MAX;
  if (ny < 0) ny = 0;
  if (ny > GEO_COORD_MAX) ny = GEO_COORD_MAX;
  
  *out_norm_x = (uint16_t)nx;
  *out_norm_y = (uint16_t)ny;
}

void geo_math_center_on(CameraState *cam, uint16_t norm_x, uint16_t norm_y) {
  if (cam->full_w <= 0 || cam->full_h <= 0) return;
  cam->pan_x = (int16_t)((cam->full_w / 2) - ((int32_t)norm_x * cam->full_w) / GEO_COORD_MAX);
  cam->pan_y = (int16_t)((cam->full_h / 2) - ((int32_t)norm_y * cam->full_h) / GEO_COORD_MAX);
  geo_math_update_camera(cam);
}

