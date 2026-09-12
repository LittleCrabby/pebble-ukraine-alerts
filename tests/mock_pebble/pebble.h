#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <locale.h>
#include <assert.h>

#ifndef PBL_PLATFORM_EMERY
#define PBL_PLATFORM_EMERY 1
#endif

typedef int32_t status_t;

typedef struct GPoint {
  int16_t x;
  int16_t y;
} GPoint;

typedef struct GSize {
  int16_t w;
  int16_t h;
} GSize;

typedef struct GRect {
  GPoint origin;
  GSize size;
} GRect;

#define GPoint(x, y) ((GPoint){(int16_t)(x), (int16_t)(y)})
#define GSize(w, h) ((GSize){(int16_t)(w), (int16_t)(h)})
#define GRect(x, y, w, h) ((GRect){{(int16_t)(x), (int16_t)(y)}, {(int16_t)(w), (int16_t)(h)}})

typedef uint8_t GColor;
#define GColorWhite 0xFF
#define GColorBlack 0x00
#define GColorDarkGray 0x55
#define GColorLightGray 0xAA
#define GColorMayGreen 0x5A
#define GColorDarkGreen 0x05
#define GColorDarkCandyAppleRed 0xA0
#define GColorChromeYellow 0xFA
#define GColorCobaltBlue 0x0A
#define GColorOrange 0xF5

// Mock persistent storage
extern int32_t g_mock_persist_val;
extern bool g_mock_persist_exists;
extern const char *g_mock_system_locale;

static inline bool persist_exists(const uint32_t key) {
  (void)key;
  return g_mock_persist_exists;
}

static inline int32_t persist_read_int(const uint32_t key) {
  (void)key;
  return g_mock_persist_val;
}

static inline status_t persist_write_int(const uint32_t key, const int32_t value) {
  (void)key;
  g_mock_persist_val = value;
  g_mock_persist_exists = true;
  return 0;
}

static inline const char* i18n_get_system_locale(void) {
  return g_mock_system_locale ? g_mock_system_locale : "en_US";
}
