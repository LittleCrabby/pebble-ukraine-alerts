import urllib.request, json, math

print("1. Fetching TopoJSON for oblasts...")
req_topo = urllib.request.Request('https://vadimklimenko.com/map/ukraine.topojson?v=4', headers={'User-Agent': 'Mozilla/5.0'})
topo = json.loads(urllib.request.urlopen(req_topo).read().decode('utf-8'))
scale = topo['transform']['scale']
translate = topo['transform']['translate']
raw_arcs = topo['arcs']

MIN_LON = 22.0
MAX_LON = 40.3
MIN_LAT = 44.3
MAX_LAT = 52.4

norm_arcs = []
for arc in raw_arcs:
    curr = [0, 0]
    decoded = []
    for pt in arc:
        curr[0] += pt[0]
        curr[1] += pt[1]
        lon = curr[0] * scale[0] + translate[0]
        lat = curr[1] * scale[1] + translate[1]
        nx = int((lon - MIN_LON) / (MAX_LON - MIN_LON) * 10000)
        ny = int((MAX_LAT - lat) / (MAX_LAT - MIN_LAT) * 10000)
        nx = max(0, min(10000, nx))
        ny = max(0, min(10000, ny))
        decoded.append((nx, ny))
    norm_arcs.append(decoded)

def p_dist(p, p1, p2):
    x0, y0 = p
    x1, y1 = p1
    x2, y2 = p2
    dx, dy = x2 - x1, y2 - y1
    if dx == 0 and dy == 0: return math.hypot(x0 - x1, y0 - y1)
    return abs(dy * x0 - dx * y0 + x2 * y1 - y2 * x1) / math.hypot(dx, dy)

def simplify_arc(pts, tol):
    if len(pts) <= 2: return pts
    dmax, idx = 0, 0
    for i in range(1, len(pts) - 1):
        d = p_dist(pts[i], pts[0], pts[-1])
        if d > dmax: idx, dmax = i, d
    if dmax > tol:
        rec1 = simplify_arc(pts[:idx+1], tol)
        rec2 = simplify_arc(pts[idx:], tol)
        return rec1[:-1] + rec2
    else:
        return [pts[0], pts[-1]]

def simplify_points(pts, tolerance):
    if len(pts) <= 2: return pts
    dmax, idx = 0, 0
    for i in range(1, len(pts) - 1):
        d = p_dist(pts[i], pts[0], pts[-1])
        if d > dmax: idx, dmax = i, d
    if dmax > tolerance:
        rec1 = simplify_points(pts[:idx+1], tolerance)
        rec2 = simplify_points(pts[idx:], tolerance)
        return rec1[:-1] + rec2
    else:
        return [pts[0], pts[-1]]

# Simplify arcs with tol = 90
sim_arcs = [simplify_arc(a, 90) for a in norm_arcs]

def get_ring_coords(ring_indices):
    coords = []
    for idx in ring_indices:
        if idx >= 0:
            coords.extend(sim_arcs[idx][:-1])
        else:
            coords.extend(list(reversed(sim_arcs[~idx]))[:-1])
    if coords:
        coords.append(coords[0])
    return coords

with open('src/pkjs/mapping.json', 'r', encoding='utf-8') as f:
    mapping = json.load(f)
state_names = mapping['states']
districts = mapping['districts']
geoms_by_name = {g['properties']['NAME_1']: g for g in topo['objects']['UKR_adm1']['geometries']}

all_region_points = []
regions_meta = []

for s_idx, name in enumerate(state_names):
    g = geoms_by_name[name]
    if g['type'] == 'Polygon':
        coords = get_ring_coords(g['arcs'][0])
    else:
        best_ring = []
        for poly in g['arcs']:
            r = get_ring_coords(poly[0])
            if len(r) > len(best_ring): best_ring = r
        coords = best_ring
    
    # Ensure closed
    if coords[0] != coords[-1]:
        coords.append(coords[0])
        
    start_idx = len(all_region_points)
    cnt = len(coords)
    min_x, max_x = 10000, 0
    min_y, max_y = 10000, 0
    sum_x, sum_y = 0, 0
    
    for x, y in coords:
        all_region_points.append((x, y))
        min_x = min(min_x, x)
        max_x = max(max_x, x)
        min_y = min(min_y, y)
        max_y = max(max_y, y)
        sum_x += x
        sum_y += y
        
    cx = sum_x // cnt
    cy = sum_y // cnt
    
    regions_meta.append({
        'id': s_idx,
        'name': name,
        'start_idx': start_idx,
        'num_points': cnt,
        'min_x': min_x, 'max_x': max_x,
        'min_y': min_y, 'max_y': max_y,
        'cx': cx, 'cy': cy
    })

print(f"Total regions: {len(regions_meta)}, total region points: {len(all_region_points)}")

print("2. Fetching rayony.geojson for districts...")
req_rayons = urllib.request.Request('https://raw.githubusercontent.com/slawomirmatuszak/ukrainian_geodata/master/rayony.geojson', headers={'User-Agent': 'Mozilla/5.0'})
raw_rayons = json.loads(urllib.request.urlopen(req_rayons).read().decode('utf-8'))

def norm(s):
    if not s: return ''
    s = s.replace('’', "'").replace('`', "'").replace(' район', '').strip().lower()
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

features_by_norm = {norm(f['properties'].get('rayon', '')): f for f in raw_rayons['features']}

all_district_points = []
district_meta = []

for d in districts:
    n = norm(d['district_name'])
    f = features_by_norm.get(n)
    if not f:
        for k, v in features_by_norm.items():
            if n in k or k in n:
                f = v
                break
    
    pts = []
    if f:
        geom = f['geometry']
        if geom['type'] == 'Polygon':
            pts = geom['coordinates'][0]
        elif geom['type'] == 'MultiPolygon':
            best_ring = []
            for poly in geom['coordinates']:
                if len(poly[0]) > len(best_ring):
                    best_ring = poly[0]
            pts = best_ring
            
    sim = simplify_points(pts, tolerance=0.08) if pts else []
    if sim and sim[0] != sim[-1]:
        sim.append(sim[0])
        
    start_idx = len(all_district_points)
    cnt = len(sim)
    min_x, max_x = 10000, 0
    min_y, max_y = 10000, 0
    sum_x, sum_y = 0, 0
    
    for lon, lat in sim:
        nx = int((lon - MIN_LON) / (MAX_LON - MIN_LON) * 10000)
        ny = int((MAX_LAT - lat) / (MAX_LAT - MIN_LAT) * 10000)
        nx = max(0, min(10000, nx))
        ny = max(0, min(10000, ny))
        all_district_points.append((nx, ny))
        min_x = min(min_x, nx)
        max_x = max(max_x, nx)
        min_y = min(min_y, ny)
        max_y = max(max_y, ny)
        sum_x += nx
        sum_y += ny
        
    if cnt > 0:
        cx = sum_x // cnt
        cy = sum_y // cnt
    else:
        reg = regions_meta[d['state_id']]
        cx, cy = reg['cx'], reg['cy']
        min_x, max_x = reg['min_x'], reg['max_x']
        min_y, max_y = reg['min_y'], reg['max_y']
        
    district_meta.append({
        'global_id': d['global_id'],
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

print(f"Total districts: {len(district_meta)}, total district points: {len(all_district_points)}")

# Write to src/c/models/geo_model.c
with open('src/c/models/geo_model.c', 'w', encoding='utf-8') as f:
    f.write('#include "geo_model.h"\n\n')
    
    # 1. s_region_points
    f.write('const GeoPoint s_region_points[] = {\n')
    for i, (x, y) in enumerate(all_region_points):
        f.write(f'  {{{x}, {y}}},')
        if i % 6 == 5: f.write('\n')
    f.write('\n};\n\n')

    # 2. s_regions
    f.write('const RegionGeoInfo s_regions[GEO_TOTAL_REGIONS] = {\n')
    for r in regions_meta:
        f.write(f'  [{r["id"]}] = {{ .id = {r["id"]}, .name = "{r["name"]}", .start_point_idx = {r["start_idx"]}, .num_points = {r["num_points"]}, .min_x = {r["min_x"]}, .max_x = {r["max_x"]}, .min_y = {r["min_y"]}, .max_y = {r["max_y"]}, .cx = {r["cx"]}, .cy = {r["cy"]} }},\n')
    f.write('};\n\n')

    # 3. s_district_points
    f.write('const GeoPoint s_district_points[] = {\n')
    for i, (x, y) in enumerate(all_district_points):
        f.write(f'  {{{x}, {y}}},')
        if i % 6 == 5: f.write('\n')
    f.write('\n};\n\n')

    # 4. s_districts
    f.write('const DistrictGeoInfo s_districts[GEO_TOTAL_DISTRICTS] = {\n')
    for d in district_meta:
        f.write(f'  [{d["global_id"]}] = {{ .global_id = {d["global_id"]}, .region_id = {d["region_id"]}, .name = "{d["name"]}", .cx = {d["cx"]}, .cy = {d["cy"]}, .start_point_idx = {d["start_idx"]}, .num_points = {d["num_points"]}, .min_x = {d["min_x"]}, .max_x = {d["max_x"]}, .min_y = {d["min_y"]}, .max_y = {d["max_y"]} }},\n')
    f.write('};\n\n')

    # 5. Functions
    f.write('''const RegionGeoInfo* geo_model_get_region(uint8_t index) {
  if (index >= GEO_TOTAL_REGIONS) return NULL;
  return &s_regions[index];
}

const DistrictGeoInfo* geo_model_get_district(uint8_t global_id) {
  if (global_id >= GEO_TOTAL_DISTRICTS) return NULL;
  return &s_districts[global_id];
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

print("Successfully written to src/c/models/geo_model.c!")
