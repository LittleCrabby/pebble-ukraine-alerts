#pragma once
#include <pebble.h>
#include "../config.h"
#include "../models/geo_model.h"
#include "../models/alert_model.h"
#include "../util/geo_math.h"

#define MAX_POLY_POINTS 240
#define MAX_INTERSECTIONS 32

void map_renderer_draw(GContext *ctx, const CameraState *cam, const AlertState *alerts, int selected_region_id, int selected_district_id);
