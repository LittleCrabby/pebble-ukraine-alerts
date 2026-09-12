#include <pebble.h>
#include <assert.h>
#include <stdio.h>
#include "util/geo_math.h"

static void test_camera_init_and_reset(void) {
  CameraState cam;
  GRect bounds = GRect(0, 0, 200, 228);
  geo_math_init_camera(&cam, bounds);

  assert(cam.zoom_percent == ZOOM_LEVEL_MIN);
  assert(cam.pan_x == 0);
  assert(cam.pan_y == 0);
  assert(cam.viewport.size.w == 200);
  assert(cam.viewport.size.h == 228);
  assert(cam.full_w > 0);
  assert(cam.full_h > 0);

  // Alter camera and reset
  cam.zoom_percent = 300;
  cam.pan_x = 50;
  cam.pan_y = -30;
  geo_math_reset_camera(&cam, bounds);

  assert(cam.zoom_percent == ZOOM_LEVEL_MIN);
  assert(cam.pan_x == 0);
  assert(cam.pan_y == 0);
}

static void test_projection_roundtrip(void) {
  CameraState cam;
  GRect bounds = GRect(0, 0, 200, 228);
  geo_math_init_camera(&cam, bounds);

  uint16_t test_x[] = {1000, 2500, 5000, 7500, 9000};
  uint16_t test_y[] = {1000, 3000, 5000, 7000, 9000};

  for (size_t i = 0; i < sizeof(test_x)/sizeof(test_x[0]); i++) {
    GPoint pt = geo_math_project(&cam, test_x[i], test_y[i]);
    uint16_t un_x = 0, un_y = 0;
    geo_math_unproject(&cam, pt, &un_x, &un_y);

    // Screen discretization introduces minor rounding tolerance (approx +/- 100 in 10000 space)
    int diff_x = abs((int)test_x[i] - (int)un_x);
    int diff_y = abs((int)test_y[i] - (int)un_y);
    assert(diff_x <= 100);
    assert(diff_y <= 100);
  }
}

static void test_center_on_and_clamping(void) {
  CameraState cam;
  GRect bounds = GRect(0, 0, 200, 228);
  geo_math_init_camera(&cam, bounds);

  // At min zoom, pan must remain clamped to 0
  geo_math_center_on(&cam, 2000, 8000);
  assert(cam.pan_x == 0);
  assert(cam.pan_y == 0);

  // Zoomed in to 3.0x
  cam.zoom_percent = 300;
  geo_math_center_on(&cam, 5000, 5000);
  // Centering on midpoint norm (5000, 5000) should result in near zero pan
  assert(abs(cam.pan_x) <= 2);
  assert(abs(cam.pan_y) <= 2);

  // Test extreme pan clamp
  cam.pan_x = 10000;
  cam.pan_y = -10000;
  geo_math_clamp_pan(&cam);
  assert(cam.pan_x < 10000);
  assert(cam.pan_y > -10000);
}

int main(void) {
  printf("Running test_geo_math...\n");
  test_camera_init_and_reset();
  test_projection_roundtrip();
  test_center_on_and_clamping();
  printf("test_geo_math: ALL PASSED\n");
  return 0;
}
