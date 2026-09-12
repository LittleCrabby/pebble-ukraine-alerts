#include "map_renderer.h"

static inline bool is_bbox_visible(const CameraState *cam, uint16_t min_x, uint16_t max_x, uint16_t min_y, uint16_t max_y) {
  int16_t sx_min = cam->origin_x + ((int32_t)min_x * cam->full_w) / GEO_COORD_MAX;
  int16_t sx_max = cam->origin_x + ((int32_t)max_x * cam->full_w) / GEO_COORD_MAX;
  int16_t sy_min = cam->origin_y + ((int32_t)min_y * cam->full_h) / GEO_COORD_MAX;
  int16_t sy_max = cam->origin_y + ((int32_t)max_y * cam->full_h) / GEO_COORD_MAX;
  
  if (sx_max < -4 || sx_min >= cam->viewport.size.w + 4 ||
      sy_max < -4 || sy_min >= cam->viewport.size.h + 4) {
    return false;
  }
  return true;
}

static void draw_polygon(GContext *ctx, const GPoint *pts, int count, GColor fill_color, bool striped, GRect viewport) {
  if (count < 3) return;
  
  int16_t min_y = pts[0].y;
  int16_t max_y = pts[0].y;
  int16_t min_x = pts[0].x;
  int16_t max_x = pts[0].x;
  for (int i = 1; i < count; i++) {
    if (pts[i].y < min_y) min_y = pts[i].y;
    if (pts[i].y > max_y) max_y = pts[i].y;
    if (pts[i].x < min_x) min_x = pts[i].x;
    if (pts[i].x > max_x) max_x = pts[i].x;
  }
  
  int16_t vp_top = viewport.origin.y;
  int16_t vp_bottom = viewport.origin.y + viewport.size.h - 1;
  int16_t vp_left = viewport.origin.x;
  int16_t vp_right = viewport.origin.x + viewport.size.w - 1;

  if (min_y < vp_top) min_y = vp_top;
  if (max_y > vp_bottom) max_y = vp_bottom;
  if (min_y > max_y) return;
  if (max_x < vp_left || min_x > vp_right) return;
  
  graphics_context_set_stroke_color(ctx, fill_color);
  
  for (int16_t y = min_y; y <= max_y; y++) {
    // For striped pattern, draw 3 scanlines on, 3 scanlines off
    if (striped && (y % 6 >= 3)) continue;
    
    int16_t nodes[MAX_INTERSECTIONS];
    int node_count = 0;
    
    int j = count - 1;
    for (int i = 0; i < count; i++) {
      if ((pts[i].y < y && pts[j].y >= y) || (pts[j].y < y && pts[i].y >= y)) {
        if (node_count < MAX_INTERSECTIONS) {
          int32_t dy = pts[j].y - pts[i].y;
          if (dy != 0) {
            int16_t node_x = pts[i].x + ((y - pts[i].y) * (pts[j].x - pts[i].x)) / dy;
            nodes[node_count++] = node_x;
          }
        }
      }
      j = i;
    }
    
    for (int i = 1; i < node_count; i++) {
      int16_t key = nodes[i];
      int k = i - 1;
      while (k >= 0 && nodes[k] > key) {
        nodes[k + 1] = nodes[k];
        k--;
      }
      nodes[k + 1] = key;
    }
    
    for (int i = 0; i < node_count; i += 2) {
      if (i + 1 < node_count) {
        int16_t x1 = nodes[i];
        int16_t x2 = nodes[i + 1];
        if (x1 < vp_left) x1 = vp_left;
        if (x2 > vp_right) x2 = vp_right;
        if (x1 <= x2) {
          graphics_draw_line(ctx, GPoint(x1, y), GPoint(x2, y));
        }
      }
    }
  }
}

void map_renderer_draw(GContext *ctx, const CameraState *cam, const AlertState *alerts, int selected_region_id, int selected_district_id) {
  GPoint screen_pts[MAX_POLY_POINTS];
  
  // 1. Fill oblast base polygons (culled by bounding box)
  for (int i = 0; i < GEO_TOTAL_REGIONS; i++) {
    const RegionGeoInfo *r = geo_model_get_region(i);
    if (!r) continue;
    if (!is_bbox_visible(cam, r->min_x, r->max_x, r->min_y, r->max_y)) continue;
    
    int pt_count = r->num_points;
    if (pt_count > MAX_POLY_POINTS) pt_count = MAX_POLY_POINTS;
    
    for (int p = 0; p < pt_count; p++) {
      const GeoPoint *gp = &s_region_points[r->start_point_idx + p];
      screen_pts[p] = geo_math_project(cam, gp->x, gp->y);
    }
    
    bool is_full = alerts && alert_model_is_region_full_alert(i);
    if (is_full) {
      // Entire oblast in full alert: fill with solid dark red
      draw_polygon(ctx, screen_pts, pt_count, COLOR_ALERT_FULL, false, cam->viewport);
    } else {
      // Calm or partially alerted oblast: fill with base green map color
      draw_polygon(ctx, screen_pts, pt_count, COLOR_MAP_BASE, false, cam->viewport);
    }
  }
  
  // 1b. Stripe ONLY the specific districts where an alert is active (culled by bounding box)
  if (alerts) {
    for (int d = 0; d < GEO_TOTAL_DISTRICTS; d++) {
      if (alert_model_is_district_alert(d)) {
        const DistrictGeoInfo *dist = geo_model_get_district(d);
        if (!dist || dist->num_points < 3) continue;
        if (alert_model_is_region_full_alert(dist->region_id)) continue;
        if (!is_bbox_visible(cam, dist->min_x, dist->max_x, dist->min_y, dist->max_y)) continue;
        
        int dpt_count = dist->num_points;
        if (dpt_count > MAX_POLY_POINTS) dpt_count = MAX_POLY_POINTS;
        for (int p = 0; p < dpt_count; p++) {
          const GeoPoint *gp = &s_district_points[dist->start_point_idx + p];
          screen_pts[p] = geo_math_project(cam, gp->x, gp->y);
        }
        draw_polygon(ctx, screen_pts, dpt_count, COLOR_ALERT_FULL, true, cam->viewport);
      }
    }
  }
  
  // 2. Draw internal district boundaries when zoomed in (>= 3.0x / 4.0x) - THIN 1px line (culled by bounding box)
  if (cam->zoom_percent >= 300) {
    graphics_context_set_stroke_width(ctx, 1);
    for (int l = 0; l < GEO_TOTAL_DISTRICT_LINES; l++) {
      const DistrictInternalLine *line = geo_model_get_district_line(l);
      if (!line || line->num_points < 2) continue;
      if (!is_bbox_visible(cam, line->min_x, line->max_x, line->min_y, line->max_y)) continue;
      
      bool alert_a = alerts && alert_model_is_district_alert(line->dist_a);
      bool alert_b = alerts && alert_model_is_district_alert(line->dist_b);
      if (alert_a || alert_b) {
        graphics_context_set_stroke_color(ctx, COLOR_ALERT_FULL);
      } else {
        graphics_context_set_stroke_color(ctx, GColorDarkGreen);
      }
      
      GPoint prev = geo_math_project(cam, s_district_line_points[line->start_point_idx].x,
                                          s_district_line_points[line->start_point_idx].y);
      for (int p = 1; p < line->num_points; p++) {
        GPoint cur = geo_math_project(cam, s_district_line_points[line->start_point_idx + p].x,
                                           s_district_line_points[line->start_point_idx + p].y);
        graphics_draw_line(ctx, prev, cur);
        prev = cur;
      }
    }
  }
  
  // 3. Draw standard oblast boundaries on top of district lines (culled by bounding box):
  uint8_t oblast_stroke = (cam->zoom_percent >= 300) ? 3 : 1;
  graphics_context_set_stroke_width(ctx, oblast_stroke);
  graphics_context_set_stroke_color(ctx, (cam->zoom_percent >= 300) ? GColorBlack : COLOR_MAP_BORDER);
  for (int i = 0; i < GEO_TOTAL_REGIONS; i++) {
    const RegionGeoInfo *r = geo_model_get_region(i);
    if (!r || r->num_points < 3) continue;
    if (!is_bbox_visible(cam, r->min_x, r->max_x, r->min_y, r->max_y)) continue;
    
    GPoint p0 = geo_math_project(cam, s_region_points[r->start_point_idx].x,
                                      s_region_points[r->start_point_idx].y);
    GPoint prev = p0;
    for (int p = 1; p < r->num_points; p++) {
      GPoint cur = geo_math_project(cam, s_region_points[r->start_point_idx + p].x,
                                         s_region_points[r->start_point_idx + p].y);
      graphics_draw_line(ctx, prev, cur);
      prev = cur;
    }
    graphics_draw_line(ctx, prev, p0);
  }
  
  // 4. Highlight Selected District or Selected Oblast (Drawn ON TOP of all borders in Cobalt Blue)
  if (selected_district_id >= 0 && selected_district_id < GEO_TOTAL_DISTRICTS) {
    const DistrictGeoInfo *dist = geo_model_get_district(selected_district_id);
    if (dist && dist->num_points > 2) {
      graphics_context_set_stroke_width(ctx, 3);
      graphics_context_set_stroke_color(ctx, GColorCobaltBlue);
      GPoint p0 = geo_math_project(cam, s_district_points[dist->start_point_idx].x,
                                        s_district_points[dist->start_point_idx].y);
      GPoint prev = p0;
      for (int p = 1; p < dist->num_points; p++) {
        GPoint cur = geo_math_project(cam, s_district_points[dist->start_point_idx + p].x,
                                           s_district_points[dist->start_point_idx + p].y);
        graphics_draw_line(ctx, prev, cur);
        prev = cur;
      }
      graphics_draw_line(ctx, prev, p0);
    }
  } else if (selected_region_id >= 0 && selected_region_id < GEO_TOTAL_REGIONS) {
    const RegionGeoInfo *r = geo_model_get_region(selected_region_id);
    if (r && r->num_points > 2) {
      graphics_context_set_stroke_width(ctx, 3);
      graphics_context_set_stroke_color(ctx, GColorCobaltBlue);
      GPoint p0 = geo_math_project(cam, s_region_points[r->start_point_idx].x,
                                        s_region_points[r->start_point_idx].y);
      GPoint prev = p0;
      for (int p = 1; p < r->num_points; p++) {
        GPoint cur = geo_math_project(cam, s_region_points[r->start_point_idx + p].x,
                                           s_region_points[r->start_point_idx + p].y);
        graphics_draw_line(ctx, prev, cur);
        prev = cur;
      }
      graphics_draw_line(ctx, prev, p0);
    }
  }
  
  // Reset stroke width to 1
  graphics_context_set_stroke_width(ctx, 1);
  
  // 5. User location marker (if available and within Ukraine)
  if (alerts && alerts->has_user_location) {
    GPoint up = geo_math_project(cam, alerts->user_loc_x, alerts->user_loc_y);
    int uradius = (cam->zoom_percent >= 300) ? 5 : 4;
    
    // Outer border & fill
    graphics_context_set_stroke_color(ctx, GColorWhite);
    graphics_context_set_stroke_width(ctx, 1);
    graphics_context_set_fill_color(ctx, GColorCobaltBlue);
    graphics_fill_circle(ctx, up, uradius);
    graphics_draw_circle(ctx, up, uradius);
    
    // Inner white center pin
    graphics_context_set_fill_color(ctx, GColorWhite);
    graphics_fill_circle(ctx, up, (uradius >= 5) ? 2 : 1);
  }
}
