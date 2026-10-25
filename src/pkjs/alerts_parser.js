// Pure data parser and converter functions for Ukraine Alerts

var UKRAINE_BOUNDS = {
  MIN_LAT: 44.3,
  MAX_LAT: 52.4,
  MIN_LON: 22.0,
  MAX_LON: 40.3
};

function normalizeCoordinates(lat, lon) {
  if (typeof lat !== 'number' || typeof lon !== 'number') return null;
  if (isNaN(lat) || isNaN(lon)) return null;

  // Broad bounding box around Ukraine
  if (lat >= 44.0 && lat <= 52.5 && lon >= 22.0 && lon <= 40.5) {
    var normX = Math.round(((lon - UKRAINE_BOUNDS.MIN_LON) / (UKRAINE_BOUNDS.MAX_LON - UKRAINE_BOUNDS.MIN_LON)) * 10000);
    var normY = Math.round(((UKRAINE_BOUNDS.MAX_LAT - lat) / (UKRAINE_BOUNDS.MAX_LAT - UKRAINE_BOUNDS.MIN_LAT)) * 10000);

    if (normX < 0) normX = 0;
    if (normX > 10000) normX = 10000;
    if (normY < 0) normY = 0;
    if (normY > 10000) normY = 10000;

    return { x: normX, y: normY };
  }
  return null;
}

function getDurationMinutes(isoString, nowMs) {
  if (!isoString) return 0xFFFF;
  var start = Date.parse(isoString);
  if (isNaN(start)) return 0xFFFF;
  var current = typeof nowMs === 'number' ? nowMs : Date.now();
  var diffMin = Math.floor((current - start) / 60000);
  if (diffMin < 0) diffMin = 0;
  return diffMin > 65534 ? 65534 : diffMin;
}

function getAlertLevelCode(alertLevelStr, isEnabled) {
  if (!isEnabled) return 0; // ALERT_LEVEL_NONE
  if (!alertLevelStr) return 3; // ALERT_LEVEL_RED (default for active alert)
  var s = String(alertLevelStr).toLowerCase();
  if (s === 'yellow') return 1;
  if (s === 'orange') return 2;
  if (s === 'red') return 3;
  return 3;
}

function set2BitLevel(array, index, level) {
  var byteIdx = Math.floor(index / 4);
  var shift = (index % 4) * 2;
  array[byteIdx] = (array[byteIdx] & ~(0x03 << shift)) | ((level & 0x03) << shift);
}

function get2BitLevel(array, index) {
  var byteIdx = Math.floor(index / 4);
  var shift = (index % 4) * 2;
  return (array[byteIdx] >> shift) & 0x03;
}

function parseAlertsData(data, mapping, nowMs, userLocation) {
  if (!data) data = {};
  var statesData = data.states || {};
  var currentMs = typeof nowMs === 'number' ? nowMs : Date.now();

  var regionMask = 0;
  var districtMask = [];
  for (var b = 0; b < 16; b++) districtMask[b] = 0;

  var districtLevels = [];
  for (var b = 0; b < 32; b++) districtLevels[b] = 0;

  var regionLevels = [];
  for (var b = 0; b < 8; b++) regionLevels[b] = 0;

  var activeStateCount = 0;
  var activeDistCount = 0;
  var summaryParts = [];
  var regionDurations = [];

  var stateList = (mapping && mapping.states) ? mapping.states : [];
  var districtList = (mapping && mapping.districts) ? mapping.districts : [];

  // 1. Process regions (26 regions)
  for (var sIdx = 0; sIdx < stateList.length; sIdx++) {
    var sName = stateList[sIdx];
    var sObj = statesData[sName];
    var dur = 0xFFFF;
    var isStateActive = false;

    if (sIdx === 0) {
      // Merged Crimea: check both "АР Крим" and "Севастополь"
      var crimeaObj = statesData['АР Крим'];
      var sevObj = statesData['Севастополь'] || statesData["Севастополь'"];
      var cActive = !!(crimeaObj && (crimeaObj.enabled || crimeaObj.alertnow));
      var sActive = !!(sevObj && (sevObj.enabled || sevObj.alertnow));
      isStateActive = cActive || sActive;

      if (isStateActive) {
        var cLvl = cActive ? getAlertLevelCode(crimeaObj ? crimeaObj.alert_level : null, true) : 0;
        var sLvl = sActive ? getAlertLevelCode(sevObj ? sevObj.alert_level : null, true) : 0;
        set2BitLevel(regionLevels, 0, Math.max(cLvl, sLvl));

        if (cActive && crimeaObj.enabled_at) {
          dur = getDurationMinutes(crimeaObj.enabled_at, currentMs);
        }
        if (sActive && sevObj.enabled_at) {
          var sDur = getDurationMinutes(sevObj.enabled_at, currentMs);
          if (dur === 0xFFFF || sDur > dur) {
            dur = sDur;
          }
        }
      }
    } else if (sObj) {
      isStateActive = !!(sObj.enabled || sObj.alertnow);
      if (isStateActive) {
        var sLvl = getAlertLevelCode(sObj.alert_level, true);
        set2BitLevel(regionLevels, sIdx, sLvl);
        if (sObj.enabled_at) {
          dur = getDurationMinutes(sObj.enabled_at, currentMs);
        }
      }
    }

    if (isStateActive) {
      regionMask |= (1 << sIdx);
      activeStateCount++;
      summaryParts.push(sName + ': Вся область');
    }
    regionDurations.push(dur & 0xFF);
    regionDurations.push((dur >> 8) & 0xFF);
  }

  // 2. Process districts (118 districts)
  var partialRegionMask = 0;
  for (var dIdx = 0; dIdx < districtList.length; dIdx++) {
    var dMeta = districtList[dIdx];
    var stObj = statesData[dMeta.state_name];
    if (stObj && stObj.districts) {
      var dObj = stObj.districts[dMeta.district_name];
      if (dObj && (dObj.enabled || dObj.alertnow)) {
        var byteIdx = Math.floor(dIdx / 8);
        var bitIdx = dIdx % 8;
        districtMask[byteIdx] |= (1 << bitIdx);
        activeDistCount++;

        var dLvl = getAlertLevelCode(dObj.alert_level, true);
        set2BitLevel(districtLevels, dIdx, dLvl);

        if (!(regionMask & (1 << dMeta.state_id))) {
          if (!(partialRegionMask & (1 << dMeta.state_id))) {
            partialRegionMask |= (1 << dMeta.state_id);
            activeStateCount++;
          }
          summaryParts.push(dMeta.state_name + ': ' + dMeta.district_name);
          // If state is not fully active, update region duration from active district
          if (dObj.enabled_at) {
            var distDur = getDurationMinutes(dObj.enabled_at, currentMs);
            var stOffset = dMeta.state_id * 2;
            var curDur = regionDurations[stOffset] | (regionDurations[stOffset + 1] << 8);
            if (curDur === 0xFFFF || distDur > curDur) {
              regionDurations[stOffset] = distDur & 0xFF;
              regionDurations[stOffset + 1] = (distDur >> 8) & 0xFF;
            }
          }
          // Update partial region's level to maximum of its active districts
          var curRegLvl = get2BitLevel(regionLevels, dMeta.state_id);
          if (dLvl > curRegLvl) {
            set2BitLevel(regionLevels, dMeta.state_id, dLvl);
          }
        }
      }
    }
  }

  // Ensure districts in fully alerted regions inherit the region's level
  for (var dIdx = 0; dIdx < districtList.length; dIdx++) {
    var dMeta = districtList[dIdx];
    if (regionMask & (1 << dMeta.state_id)) {
      var rLvl = get2BitLevel(regionLevels, dMeta.state_id);
      var curDLvl = get2BitLevel(districtLevels, dIdx);
      if (rLvl > curDLvl) {
        set2BitLevel(districtLevels, dIdx, rLvl);
      }
    }
  }

  var summaryText = summaryParts.slice(0, 10).join('; ');
  if (summaryParts.length === 0) {
    summaryText = 'Повітряних тривог немає';
  }

  return {
    'STATUS_CODE': 0,
    'ALERT_REGIONS_MASK': regionMask,
    'DISTRICT_ALERTS_MASK': districtMask,
    'ACTIVE_COUNT': activeStateCount,
    'ACTIVE_DISTRICT_COUNT': activeDistCount,
    'TIMESTAMP': Math.floor(currentMs / 1000),
    'SUMMARY_TEXT': summaryText,
    'USER_LOCATION_VALID': userLocation ? 1 : 0,
    'USER_LOCATION_X': userLocation ? userLocation.x : 0,
    'USER_LOCATION_Y': userLocation ? userLocation.y : 0,
    'REGION_DURATIONS': regionDurations,
    'DISTRICT_ALERT_LEVELS': districtLevels,
    'REGION_ALERT_LEVELS': regionLevels
  };
}

module.exports = {
  UKRAINE_BOUNDS: UKRAINE_BOUNDS,
  normalizeCoordinates: normalizeCoordinates,
  getDurationMinutes: getDurationMinutes,
  getAlertLevelCode: getAlertLevelCode,
  set2BitLevel: set2BitLevel,
  get2BitLevel: get2BitLevel,
  parseAlertsData: parseAlertsData
};
