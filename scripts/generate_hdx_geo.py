#!/usr/bin/env python3
import json
import os
import subprocess
import urllib.request

HDX_GEOJSON_PATH = '/tmp/ukr_admin2.geojson'

if not os.path.exists(HDX_GEOJSON_PATH):
    print("Downloading HDX COD-AB dataset...")
    url = "https://data.humdata.org/dataset/d23f529f-31e4-4021-a65b-13987e5cfb42/resource/681beb86-391b-4a08-8140-ca52e80fcdce/download/ukr_admin_boundaries.geojson.zip"
    zip_path = "/tmp/ukr_hdx.zip"
    urllib.request.urlretrieve(url, zip_path)
    subprocess.run(f"unzip -p {zip_path} ukr_admin2.geojson > {HDX_GEOJSON_PATH}", shell=True, check=True)

with open(HDX_GEOJSON_PATH, 'r', encoding='utf-8') as f:
    hdx = json.load(f)

with open('src/pkjs/mapping.json', 'r', encoding='utf-8') as f:
    mapping = json.load(f)

states = mapping['states']

def get_state_id(adm1_name):
    adm1 = adm1_name.strip()
    if 'Крим' in adm1: return 0
    if 'Севастополь' in adm1: return 1
    if 'Київ' == adm1: return 26
    for idx, s in enumerate(states):
        core = s.replace(' область', '').replace('м. ', '').strip()
        if core in adm1:
            return idx
    return -1

def clean(s):
    if not s: return ''
    for ch in ["'", "’", "`"]:
        s = s.replace(ch, "")
    s = s.replace(' район', '').strip().lower()
    if 'володимир' in s: return 'володимир'
    if 'кам' in s and 'янськ' in s and 'подільськ' not in s: return 'камянськ'
    if 'кам' in s and 'подільськ' in s: return 'камянецьподільськ'
    if 'новомосковськ' in s or 'самар' in s: return 'самар'
    if 'сєвєродонецьк' in s or 'сіверськодонецьк' in s: return 'сіверськодонецьк'
    if 'червоноград' in s or 'шептицьк' in s: return 'шептицьк'
    if 'куп' in s and 'янськ' in s: return 'купянськ'
    if 'звягель' in s or 'новоград' in s: return 'новоград'
    if 'берестин' in s or 'красноград' in s: return 'красноград'
    return s

map_dict = {}
for d in mapping['districts']:
    sid = d['state_id']
    dname = d['district_name']
    gid = d['global_id']
    map_dict[(sid, clean(dname))] = (gid, dname)

enriched_features = []
for f in hdx['features']:
    p = f['properties']
    sid = get_state_id(p['adm1_name1'])
    dname = p.get('adm2_name1') or ''
    cd = clean(dname)
    
    if sid == 10 and 'чорнобиль' in cd:
        # Merge Chornobyl zone into Vyshhorodskyi (gid=45)
        gid = 45
        matched_name = 'Вишгородський район'
    elif (sid, cd) in map_dict:
        gid, matched_name = map_dict[(sid, cd)]
    else:
        matches = [v for k, v in map_dict.items() if k[1] == cd]
        if len(matches) == 1:
            gid, matched_name = matches[0]
        else:
            gid = 200 + sid
            matched_name = dname

    f['properties'] = {
        'state_id': sid,
        'state_name': states[sid],
        'district_name': matched_name,
        'global_id': gid
    }
    enriched_features.append(f)

with open('/tmp/hdx_final_input.geojson', 'w', encoding='utf-8') as f:
    json.dump({'type': 'FeatureCollection', 'features': enriched_features}, f)

print("Running mapshaper topology commands...")
cmd1 = "npx mapshaper /tmp/hdx_final_input.geojson -dissolve global_id copy-fields=state_id,state_name,district_name -clean -simplify 0.5% keep-shapes -o /tmp/hdx_final_dists.geojson"
cmd2 = "npx mapshaper /tmp/hdx_final_dists.geojson -dissolve state_id -o /tmp/hdx_final_obls.geojson"
cmd3 = "npx mapshaper /tmp/hdx_final_dists.geojson -innerlines where='A.state_id == B.state_id && A.state_id > 0' -o /tmp/hdx_final_lines.geojson"

subprocess.run(cmd1, shell=True, check=True)
subprocess.run(cmd2, shell=True, check=True)
subprocess.run(cmd3, shell=True, check=True)

with open('/tmp/hdx_final_dists.geojson', 'r', encoding='utf-8') as f:
    dists_geojson = json.load(f)

with open('/tmp/hdx_final_obls.geojson', 'r', encoding='utf-8') as f:
    obls_geojson = json.load(f)

with open('/tmp/hdx_final_lines.geojson', 'r', encoding='utf-8') as f:
    lines_geojson = json.load(f)

MIN_LON, MAX_LON = 22.137, 40.228
MIN_LAT, MAX_LAT = 44.386, 52.379

def to_norm(lon, lat):
    x = int((lon - MIN_LON) / (MAX_LON - MIN_LON) * 10000)
    y = int((MAX_LAT - lat) / (MAX_LAT - MIN_LAT) * 10000)
    return max(0, min(10000, x)), max(0, min(10000, y))

def poly_area(ring):
    a = 0
    for i in range(len(ring)-1):
        a += (ring[i][0] * ring[i+1][1] - ring[i+1][0] * ring[i][1])
    return abs(a) / 2.0

# 1. Process 126 Districts
dists_by_id = {}
for f in dists_geojson['features']:
    gid = f['properties']['global_id']
    if gid < 126:
        dists_by_id[gid] = f

all_district_points = []
district_meta = []

for d in mapping['districts']:
    gid = d['global_id']
    if gid >= 126 or gid not in dists_by_id: continue
    
    f = dists_by_id[gid]
    geom = f['geometry']
    coords = geom['coordinates']
    ring = coords[0] if geom['type'] == 'Polygon' else max(coords, key=lambda x: poly_area(x[0]))[0]
    
    norm_ring = [to_norm(p[0], p[1]) for p in ring]
    cleaned = [norm_ring[0]]
    for p in norm_ring[1:]:
        if p != cleaned[-1]: cleaned.append(p)
    if cleaned[0] != cleaned[-1]:
        cleaned.append(cleaned[0])
        
    start_idx = len(all_district_points)
    cnt = len(cleaned)
    min_x, max_x = 10000, 0
    min_y, max_y = 10000, 0
    sum_x, sum_y = 0, 0
    for x, y in cleaned:
        all_district_points.append((x, y))
        min_x = min(min_x, x)
        max_x = max(max_x, x)
        min_y = min(min_y, y)
        max_y = max(max_y, y)
        sum_x += x
        sum_y += y
        
    cx = sum_x // cnt
    cy = sum_y // cnt
    
    district_meta.append({
        'global_id': gid,
        'region_id': d['state_id'],
        'name': d['district_name'],
        'start_idx': start_idx,
        'num_points': cnt,
        'cx': cx,
        'cy': cy,
        'min_x': min_x,
        'max_x': max_x,
        'min_y': min_y,
        'max_y': max_y
    })

print(f"Total district points: {len(all_district_points)} across {len(district_meta)} districts")

# 2. Process Oblasts
obls_by_id = {}
for f in obls_geojson['features']:
    sid = f['properties'].get('state_id')
    if sid is not None:
        obls_by_id[sid] = f

all_region_points = []
regions_meta = []

for sid, sname in enumerate(states):
    f = obls_by_id.get(sid)
    if sid == 1:
        # Sevastopol
        cleaned = [(6441, 9898), (6232, 9698), (6302, 9561), (6298, 9385), (6323, 9328), (6374, 9404), (6349, 9505), (6433, 9538), (6376, 9646), (6401, 9733), (6481, 9723), (6517, 9840), (6441, 9898)]
    elif f:
        geom = f['geometry']
        coords = geom['coordinates']
        ring = coords[0] if geom['type'] == 'Polygon' else max(coords, key=lambda x: poly_area(x[0]))[0]
        norm_ring = [to_norm(p[0], p[1]) for p in ring]
        cleaned = [norm_ring[0]]
        for p in norm_ring[1:]:
            if p != cleaned[-1]: cleaned.append(p)
        if cleaned[0] != cleaned[-1]:
            cleaned.append(cleaned[0])
    else:
        cleaned = []
        
    start_idx = len(all_region_points)
    cnt = len(cleaned)
    min_x, max_x = 10000, 0
    min_y, max_y = 10000, 0
    sum_x, sum_y = 0, 0
    for x, y in cleaned:
        all_region_points.append((x, y))
        min_x = min(min_x, x)
        max_x = max(max_x, x)
        min_y = min(min_y, y)
        max_y = max(max_y, y)
        sum_x += x
        sum_y += y
        
    cx = sum_x // cnt if cnt > 0 else 5000
    cy = sum_y // cnt if cnt > 0 else 5000
    
    regions_meta.append({
        'id': sid,
        'name': sname,
        'start_idx': start_idx,
        'num_points': cnt,
        'min_x': min_x,
        'max_x': max_x,
        'min_y': min_y,
        'max_y': max_y,
        'cx': cx,
        'cy': cy
    })

print(f"Total region points: {len(all_region_points)} across {len(regions_meta)} regions")

# 3. Process Internal District Lines
vertex_dists = {}
dist_region = {}
dists_by_gid = {}
for d in district_meta:
    gid = d['global_id']
    dist_region[gid] = d['region_id']
    dists_by_gid[gid] = (d['region_id'], d['name'])
    for p in range(d['num_points']):
        pt = all_district_points[d['start_idx'] + p]
        if pt not in vertex_dists: vertex_dists[pt] = set()
        vertex_dists[pt].add(gid)

all_line_points = []
lines_meta = []

for g in lines_geojson['geometries']:
    ls = [g['coordinates']] if g['type'] == 'LineString' else g['coordinates']
    for seg in ls:
        norm_seg = [to_norm(p[0], p[1]) for p in seg]
        cleaned = [norm_seg[0]]
        for p in norm_seg[1:]:
            if p != cleaned[-1]: cleaned.append(p)
        if len(cleaned) < 2: continue
        
        state_counts = {}
        counts = {}
        for pt in cleaned:
            for gid in vertex_dists.get(pt, set()):
                counts[gid] = counts.get(gid, 0) + 1
                rid = dist_region[gid]
                state_counts[rid] = state_counts.get(rid, 0) + 1
                
        if not state_counts: continue
        best_rid = max(state_counts.items(), key=lambda x: x[1])[0]
        # Skip internal lines of Crimea (sid 0)
        if best_rid == 0: continue
        
        state_candidates = [(gid, c) for gid, c in counts.items() if dist_region[gid] == best_rid]
        state_candidates.sort(key=lambda x: -x[1])
        
        da = state_candidates[0][0]
        db = state_candidates[1][0] if len(state_candidates) > 1 else da
        
        start_idx = len(all_line_points)
        cnt = len(cleaned)
        min_x, max_x = 10000, 0
        min_y, max_y = 10000, 0
        for x, y in cleaned:
            all_line_points.append((x, y))
            min_x = min(min_x, x)
            max_x = max(max_x, x)
            min_y = min(min_y, y)
            max_y = max(max_y, y)
            
        lines_meta.append({
            'dist_a': da,
            'dist_b': db,
            'start_idx': start_idx,
            'num_points': cnt,
            'min_x': min_x,
            'max_x': max_x,
            'min_y': min_y,
            'max_y': max_y
        })

print(f"Total internal line points: {len(all_line_points)} across {len(lines_meta)} internal lines")

# Write geo_model.h
with open('src/c/models/geo_model.h', 'w', encoding='utf-8') as f:
    f.write(f'''#pragma once
#include <pebble.h>

#define GEO_TOTAL_REGIONS 27
#define GEO_TOTAL_DISTRICTS {len(district_meta)}
#define GEO_TOTAL_DISTRICT_LINES {len(lines_meta)}
#define GEO_COORD_MAX 10000

typedef struct {{
  uint16_t x; // 0 .. 10000
  uint16_t y; // 0 .. 10000
}} GeoPoint;

typedef struct {{
  uint8_t id;
  const char *name;
  uint16_t start_point_idx;
  uint16_t num_points;
  uint16_t min_x;
  uint16_t max_x;
  uint16_t min_y;
  uint16_t max_y;
  uint16_t cx; // Centroid X
  uint16_t cy; // Centroid Y
}} RegionGeoInfo;

typedef struct {{
  uint8_t global_id;
  uint8_t region_id;
  const char *name;
  uint16_t cx;
  uint16_t cy;
  uint16_t start_point_idx;
  uint16_t num_points;
  uint16_t min_x;
  uint16_t max_x;
  uint16_t min_y;
  uint16_t max_y;
}} DistrictGeoInfo;

typedef struct {{
  uint8_t dist_a;
  uint8_t dist_b;
  uint16_t start_point_idx;
  uint16_t num_points;
  uint16_t min_x;
  uint16_t max_x;
  uint16_t min_y;
  uint16_t max_y;
}} DistrictInternalLine;

extern const GeoPoint s_region_points[];
extern const RegionGeoInfo s_regions[GEO_TOTAL_REGIONS];
extern const GeoPoint s_district_points[];
extern const DistrictGeoInfo s_districts[GEO_TOTAL_DISTRICTS];
extern const GeoPoint s_district_line_points[];
extern const DistrictInternalLine s_district_lines[GEO_TOTAL_DISTRICT_LINES];

const RegionGeoInfo* geo_model_get_region(uint8_t index);
const DistrictGeoInfo* geo_model_get_district(uint8_t global_id);
const DistrictInternalLine* geo_model_get_district_line(uint16_t index);
int geo_model_find_region_at(uint16_t norm_x, uint16_t norm_y);
int geo_model_find_district_at(uint16_t norm_x, uint16_t norm_y);
''')

# Write geo_model.c
with open('src/c/models/geo_model.c', 'w', encoding='utf-8') as f:
    f.write('#include "geo_model.h"\n\n')
    
    # s_region_points
    f.write('const GeoPoint s_region_points[] = {\n')
    for i, (x, y) in enumerate(all_region_points):
        f.write(f'  {{{x}, {y}}},')
        if i % 6 == 5: f.write('\n')
    f.write('\n};\n\n')

    # s_regions
    f.write('const RegionGeoInfo s_regions[GEO_TOTAL_REGIONS] = {\n')
    for r in regions_meta:
        f.write(f'  [{r["id"]}] = {{ .id = {r["id"]}, .name = "{r["name"]}", .start_point_idx = {r["start_idx"]}, .num_points = {r["num_points"]}, .min_x = {r["min_x"]}, .max_x = {r["max_x"]}, .min_y = {r["min_y"]}, .max_y = {r["max_y"]}, .cx = {r["cx"]}, .cy = {r["cy"]} }},\n')
    f.write('};\n\n')

    # s_district_points
    f.write('const GeoPoint s_district_points[] = {\n')
    for i, (x, y) in enumerate(all_district_points):
        f.write(f'  {{{x}, {y}}},')
        if i % 6 == 5: f.write('\n')
    f.write('\n};\n\n')

    # s_districts
    f.write('const DistrictGeoInfo s_districts[GEO_TOTAL_DISTRICTS] = {\n')
    for d in district_meta:
        f.write(f'  [{d["global_id"]}] = {{ .global_id = {d["global_id"]}, .region_id = {d["region_id"]}, .name = "{d["name"]}", .cx = {d["cx"]}, .cy = {d["cy"]}, .start_point_idx = {d["start_idx"]}, .num_points = {d["num_points"]}, .min_x = {d["min_x"]}, .max_x = {d["max_x"]}, .min_y = {d["min_y"]}, .max_y = {d["max_y"]} }},\n')
    f.write('};\n\n')

    # s_district_line_points
    f.write('const GeoPoint s_district_line_points[] = {\n')
    for i, (x, y) in enumerate(all_line_points):
        f.write(f'  {{{x}, {y}}},')
        if i % 6 == 5: f.write('\n')
    f.write('\n};\n\n')

    # s_district_lines
    f.write(f'const DistrictInternalLine s_district_lines[GEO_TOTAL_DISTRICT_LINES] = {{\n')
    for i, l in enumerate(lines_meta):
        f.write(f'  [{i}] = {{ .dist_a = {l["dist_a"]}, .dist_b = {l["dist_b"]}, .start_point_idx = {l["start_idx"]}, .num_points = {l["num_points"]}, .min_x = {l["min_x"]}, .max_x = {l["max_x"]}, .min_y = {l["min_y"]}, .max_y = {l["max_y"]} }},\n')
    f.write('};\n\n')

    # Helper functions
    f.write('''const RegionGeoInfo* geo_model_get_region(uint8_t index) {
  if (index >= GEO_TOTAL_REGIONS) return NULL;
  return &s_regions[index];
}

const DistrictGeoInfo* geo_model_get_district(uint8_t global_id) {
  if (global_id >= GEO_TOTAL_DISTRICTS) return NULL;
  return &s_districts[global_id];
}

const DistrictInternalLine* geo_model_get_district_line(uint16_t index) {
  if (index >= GEO_TOTAL_DISTRICT_LINES) return NULL;
  return &s_district_lines[index];
}

int geo_model_find_region_at(uint16_t norm_x, uint16_t norm_y) {
  int best_id = -1;
  int32_t min_dist_sq = 0x7FFFFFFF;
  for (int i = 0; i < GEO_TOTAL_REGIONS; i++) {
    const RegionGeoInfo *r = &s_regions[i];
    if (norm_x >= r->min_x && norm_x <= r->max_x && norm_y >= r->min_y && norm_y <= r->max_y) {
      int32_t dx = (int32_t)norm_x - r->cx;
      int32_t dy = (int32_t)norm_y - r->cy;
      int32_t dist_sq = dx * dx + dy * dy;
      if (dist_sq < min_dist_sq) {
        min_dist_sq = dist_sq;
        best_id = i;
      }
    }
  }
  return best_id;
}

int geo_model_find_district_at(uint16_t norm_x, uint16_t norm_y) {
  int best_id = -1;
  int32_t min_dist_sq = 0x7FFFFFFF;
  for (int i = 0; i < GEO_TOTAL_DISTRICTS; i++) {
    const DistrictGeoInfo *d = &s_districts[i];
    if (norm_x >= d->min_x && norm_x <= d->max_x && norm_y >= d->min_y && norm_y <= d->max_y) {
      int32_t dx = (int32_t)norm_x - d->cx;
      int32_t dy = (int32_t)norm_y - d->cy;
      int32_t dist_sq = dx * dx + dy * dy;
      if (dist_sq < min_dist_sq) {
        min_dist_sq = dist_sq;
        best_id = i;
      }
    }
  }
  return best_id;
}
''')

print("Successfully regenerated src/c/models/geo_model.h and geo_model.c!")
