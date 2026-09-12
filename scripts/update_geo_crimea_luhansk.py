#!/usr/bin/env python3
"""
Regenerates geo_model.h, geo_model.c, and mapping.json:
- Merges Crimea and Sevastopol into one region ("АР Крим")
- Removes district subdivision in Luhansk oblast (keeping solid oblast)
"""
import json
import re
import subprocess

def main():
    # 1. Load current geo_model.c
    with open('src/c/models/geo_model.c', 'r', encoding='utf-8') as f:
        c_code = f.read()

    reg_pts_match = re.search(r'const GeoPoint s_region_points\[\] = \{([\s\S]*?)\};', c_code)
    all_reg_pts = [(int(x), int(y)) for x, y in re.findall(r'\{(\d+),\s*(\d+)\}', reg_pts_match.group(1))]
    reg_matches = re.findall(r'\[(\d+)\] = \{\s*\.id = (\d+),\s*\.name = \"([^\"]+)\",\s*\.start_point_idx = (\d+),\s*\.num_points = (\d+),\s*\.min_x = (\d+),\s*\.max_x = (\d+),\s*\.min_y = (\d+),\s*\.max_y = (\d+),\s*\.cx = (\d+),\s*\.cy = (\d+)\s*\}', c_code)

    dist_pts_match = re.search(r'const GeoPoint s_district_points\[\] = \{([\s\S]*?)\};', c_code)
    all_dist_pts = [(int(x), int(y)) for x, y in re.findall(r'\{(\d+),\s*(\d+)\}', dist_pts_match.group(1))]
    dist_matches = re.findall(r'\[(\d+)\] = \{\s*\.global_id = (\d+),\s*\.region_id = (\d+),\s*\.name = \"([^\"]+)\",\s*\.cx = (\d+),\s*\.cy = (\d+),\s*\.start_point_idx = (\d+),\s*\.num_points = (\d+),\s*\.min_x = (\d+),\s*\.max_x = (\d+),\s*\.min_y = (\d+),\s*\.max_y = (\d+)\s*\}', c_code)

    line_pts_match = re.search(r'const GeoPoint s_district_line_points\[\] = \{([\s\S]*?)\};', c_code)
    all_line_pts = [(int(x), int(y)) for x, y in re.findall(r'\{(\d+),\s*(\d+)\}', line_pts_match.group(1))]
    line_matches = re.findall(r'\[(\d+)\] = \{\s*\.dist_a = (\d+),\s*\.dist_b = (\d+),\s*\.start_point_idx = (\d+),\s*\.num_points = (\d+),\s*\.min_x = (\d+),\s*\.max_x = (\d+),\s*\.min_y = (\d+),\s*\.max_y = (\d+)\s*\}', c_code)

    print(f"Loaded: {len(reg_matches)} regions, {len(dist_matches)} districts, {len(line_matches)} lines.")

if __name__ == '__main__':
    main()
