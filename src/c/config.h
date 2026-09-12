#pragma once
#include <pebble.h>

// Screen dimensions
#if defined(PBL_ROUND) || defined(PBL_PLATFORM_GABBRO)
  #define SCREEN_IS_ROUND true
  #define SCREEN_WIDTH 260
  #define SCREEN_HEIGHT 260
#elif defined(PBL_PLATFORM_EMERY)
  #define SCREEN_IS_ROUND false
  #define SCREEN_WIDTH 200
  #define SCREEN_HEIGHT 228
#else
  #define SCREEN_IS_ROUND false
  #define SCREEN_WIDTH 144
  #define SCREEN_HEIGHT 168
#endif

// Map projection aspect ratio (height is ~67% of width for Ukraine coordinates)
#define UKRAINE_MAP_ASPECT_RATIO_NUM 67
#define UKRAINE_MAP_ASPECT_RATIO_DENOM 100

// Zoom settings: 1.0x, 2.0x, 3.0x, 4.0x
#define ZOOM_LEVEL_MIN 100 // 1.0x
#define ZOOM_LEVEL_MAX 400 // 4.0x
#define ZOOM_STEP 100      // 1.0x per step

// Colors: 1 gradation darker green (#55AA55) and 1 gradation darker red (#AA0000)
#define COLOR_BG GColorWhite
#define COLOR_MAP_BASE GColorMayGreen
#define COLOR_MAP_BORDER GColorDarkGray
#define COLOR_ALERT_FULL GColorDarkCandyAppleRed
#define COLOR_ALERT_PARTIAL GColorChromeYellow
#define COLOR_TEXT_DARK GColorBlack
#define COLOR_TEXT_MUTED GColorDarkGray
#define COLOR_ACCENT GColorCobaltBlue

// Bitmask helpers
#define REGION_MASK_COUNT 26
#define DISTRICT_MASK_BYTES 16 // 128 bits for 118 districts
