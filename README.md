# Ukraine Alerts Map for Pebble

[![CI & Release](https://github.com/LittleCrabby/pebble-ukraine-alerts/actions/workflows/ci.yml/badge.svg)](https://github.com/LittleCrabby/pebble-ukraine-alerts/actions/workflows/ci.yml)
[![Rebble Appstore](https://img.shields.io/badge/Rebble_Appstore-Ukraine_Alerts-FF4757?logo=pebble&logoColor=white)](https://apps.repebble.com/90b69a2d24954ae184433c85)
[![GitHub Release](https://img.shields.io/github/v/release/LittleCrabby/pebble-ukraine-alerts?color=blue&label=release)](https://github.com/LittleCrabby/pebble-ukraine-alerts/releases)
[![Platforms](https://img.shields.io/badge/platforms-emery%20%7C%20gabbro-orange)](https://github.com/LittleCrabby/pebble-ukraine-alerts)

A real-time air raid alert map and situational awareness watchapp for Pebble smartwatches, visualizing active alerts across all regions and districts of Ukraine.

Built in C using the Pebble SDK with an accompanying PebbleKit JS (PKJS) companion service.

[![Ukraine Alerts on Rebble Appstore](https://apps.repebble.com/og/90b69a2d24954ae184433c85.png)](https://apps.repebble.com/90b69a2d24954ae184433c85)

> 📲 **Install on your Pebble:** [Download from Rebble Appstore](https://apps.repebble.com/90b69a2d24954ae184433c85)

---

## Features

- **On-Device Vector Map Rendering**: Custom scanline rasterizer renders vector polygons directly on Pebble hardware for all oblasts and districts.
- **Hierarchical Alert Status**:
  - 🟢 **Calm**: No active sirens recorded in region.
  - 🟡 **Partial / District Alert**: Alert triggered in specific raions (rendered with striped fill and border highlights).
  - 🔴 **Full Alert**: Entire oblast under active alert.
- **GPS Location & "My Safety"**: Translates phone GPS coordinates onto Ukraine's map grid, displays a current location pin, and prioritizes local danger status.
- **Interactive Map Controls**:
  - **1.0x – 4.0x Zoom**: Up/Down buttons or auto-focusing on selected areas.
  - **Touch Pan & Tap**: Drag to pan across the country, tap any region or district to inspect name and alert status (on touch-supported hardware).
- **Active Alerts List**:
  - Dedicated menu showing all regions with active sirens and ongoing durations (mins, hours, or days).
  - Selecting an active region jumps and centers the map directly onto it.
- **Multilingual (i18n)**:
  - Bundles a custom Cyrillic font (`Carlito`) for clear Ukrainian and Latin typography.
  - Supports quick cycling between Ukrainian, English, and additional UI languages directly from the in-app menu.
- **Resilient Polling**: Automatically refreshes every 15 seconds with primary (`vadimklimenko.com`) and secondary (`ubilling.net.ua`) API failover.

---

## Target Platforms

Configured and tested for modern Pebble platforms:
- **`emery`**: Pebble Time 2 (200×228 color display, touch support)
- **`gabbro`**: Pebble Round 2 (260×260 color round display, touch support)

---

## Controls

### Map Screen

| Control | Action |
|---|---|
| **Up Button** | Zoom in (+1.0x, up to 4.0x) |
| **Down Button** | Zoom out (-1.0x, down to 1.0x) |
| **Select Button** | Open Alerts List & Safety screen |
| **Back Button** | Deselect region/district &rarr; Reset zoom/pan &rarr; Exit app |
| **Touch Drag** | Pan map across viewport |
| **Touch Tap** | Select region or district to inspect in bottom banner |

### Alerts List Screen

| Control | Action |
|---|---|
| **Up / Down** | Scroll through list sections (*My Safety*, *Active Alerts*, *Actions*) |
| **Select on Region** | Jump back to map centered and focused on that region |
| **Select on Location** | Center map on your current GPS location |
| **Select on Language** | Cycle interface language (Ukrainian, English, etc.) |
| **Back Button** | Return to map view |

---

## Project Structure

```
├── resources/              # Cyrillic fonts (Carlito) and app menu icons
├── scripts/                # GeoJSON processing scripts generating optimized C polygons
├── src/
│   ├── c/
│   │   ├── config.h        # Platform detection, color palette, zoom thresholds
│   │   ├── main.c          # App lifecycle and tick timers
│   │   ├── models/         # Alert state model and vector polygon geography
│   │   ├── services/       # AppMessage communication service
│   │   ├── util/           # Projection math, camera clamping, i18n
│   │   └── views/          # Map rendering engine, touch handling, list window
│   └── pkjs/
│       ├── index.js        # Phone-side lifecycle, HTTP polling, geolocation
│       ├── alerts_parser.js# Alert payload parsing and bitmask packing
│       └── mapping.json    # State & district ID mappings
└── tests/                  # C unit test suite (with Pebble mocks) and Node.js tests
```

---

## Development & Building

### Prerequisites

- [Pebble SDK / Rebble / Repebble toolchain](https://developer.repebble.com/) (`pebble` CLI)
- Node.js & npm
- GCC / Make (for C unit tests)

### Build & Install

```bash
# Build binary for configured target platforms (emery, gabbro)
pebble build

# Install and run on emulator
pebble install --emulator emery
pebble install --emulator gabbro

# Install on physical watch paired to phone
pebble install --phone <PHONE_IP>
```

### Running Tests

Run the complete test suite (both native C unit tests with Pebble mocks and PKJS parser tests):

```bash
npm test
```

---

## Data Sources

- Primary Alerts API: [vadimklimenko.com](https://vadimklimenko.com/map/statuses.json)
- Secondary Alerts API: [ubilling.net.ua](http://ubilling.net.ua/aerialalerts/)
