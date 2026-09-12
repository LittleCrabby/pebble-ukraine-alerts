var test = require('node:test');
var assert = require('node:assert');
var parser = require('../../src/pkjs/alerts_parser.js');
var mapping = require('../../src/pkjs/mapping.json');

test('normalizeCoordinates inside Ukraine', function() {
  // Kyiv: ~50.45N, 30.52E
  var kyiv = parser.normalizeCoordinates(50.45, 30.52);
  assert.notStrictEqual(kyiv, null);
  assert(kyiv.x >= 4000 && kyiv.x <= 5500, 'Kyiv X should be near central Ukraine');
  assert(kyiv.y >= 2000 && kyiv.y <= 3000, 'Kyiv Y should be north-central');

  // Lviv: ~49.84N, 24.03E
  var lviv = parser.normalizeCoordinates(49.84, 24.03);
  assert.notStrictEqual(lviv, null);
  assert(lviv.x < kyiv.x, 'Lviv should be west of Kyiv');

  // Odesa: ~46.48N, 30.73E
  var odesa = parser.normalizeCoordinates(46.48, 30.73);
  assert.notStrictEqual(odesa, null);
  assert(odesa.y > kyiv.y, 'Odesa should be south of Kyiv');
});

test('normalizeCoordinates outside Ukraine returns null', function() {
  assert.strictEqual(parser.normalizeCoordinates(52.52, 13.40), null, 'Berlin should be null');
  assert.strictEqual(parser.normalizeCoordinates(51.50, -0.12), null, 'London should be null');
  assert.strictEqual(parser.normalizeCoordinates(null, 30.0), null);
  assert.strictEqual(parser.normalizeCoordinates(undefined, undefined), null);
  assert.strictEqual(parser.normalizeCoordinates(NaN, 30.0), null);
});

test('getDurationMinutes calculations', function() {
  var nowMs = 1700000000000;
  var tenMinAgo = new Date(nowMs - 10 * 60 * 1000).toISOString();
  assert.strictEqual(parser.getDurationMinutes(tenMinAgo, nowMs), 10);

  var twoHoursAgo = new Date(nowMs - 120 * 60 * 1000).toISOString();
  assert.strictEqual(parser.getDurationMinutes(twoHoursAgo, nowMs), 120);

  // Future date clamps to 0
  var future = new Date(nowMs + 60 * 1000).toISOString();
  assert.strictEqual(parser.getDurationMinutes(future, nowMs), 0);

  // Invalid strings return 0xFFFF
  assert.strictEqual(parser.getDurationMinutes(null, nowMs), 0xFFFF);
  assert.strictEqual(parser.getDurationMinutes('invalid-date', nowMs), 0xFFFF);
});

test('parseAlertsData with all calm', function() {
  var nowMs = 1700000000000;
  var emptyData = { states: {} };
  var res = parser.parseAlertsData(emptyData, mapping, nowMs, null);

  assert.strictEqual(res.STATUS_CODE, 0);
  assert.strictEqual(res.ALERT_REGIONS_MASK, 0);
  assert.strictEqual(res.ACTIVE_COUNT, 0);
  assert.strictEqual(res.ACTIVE_DISTRICT_COUNT, 0);
  assert.strictEqual(res.SUMMARY_TEXT, 'Повітряних тривог немає');
  assert.strictEqual(res.USER_LOCATION_VALID, 0);
  assert.strictEqual(res.REGION_DURATIONS.length, mapping.states.length * 2);
});

test('parseAlertsData with full and partial alerts', function() {
  var nowMs = 1700000000000;
  var thirtyMinAgo = new Date(nowMs - 30 * 60 * 1000).toISOString();
  var tenMinAgo = new Date(nowMs - 10 * 60 * 1000).toISOString();

  // Pick state 1 (Vinnytsia obl) for full alert
  var state1 = mapping.states[1];
  // Pick district 10 (Kryvyi Rih, Dnipropetrovsk obl) from a different state
  var dist10 = mapping.districts[10];

  var mockData = {
    states: {}
  };
  mockData.states[state1] = {
    enabled: true,
    enabled_at: thirtyMinAgo
  };
  mockData.states[dist10.state_name] = {
    enabled: false,
    districts: {}
  };
  mockData.states[dist10.state_name].districts[dist10.district_name] = {
    enabled: true,
    enabled_at: tenMinAgo
  };

  var userLoc = { x: 4650, y: 2421 };
  var res = parser.parseAlertsData(mockData, mapping, nowMs, userLoc);

  assert.strictEqual(res.ACTIVE_COUNT, 2);
  assert.strictEqual(res.ACTIVE_DISTRICT_COUNT, 1);
  assert.strictEqual(res.USER_LOCATION_VALID, 1);
  assert.strictEqual(res.USER_LOCATION_X, 4650);
  assert.strictEqual(res.USER_LOCATION_Y, 2421);

  // Region mask should have bit 1 set
  assert.strictEqual((res.ALERT_REGIONS_MASK & (1 << 1)) !== 0, true);

  // District mask should have bit (10 % 8) set in byte (10 / 8)
  var dByte = Math.floor(10 / 8);
  var dBit = 10 % 8;
  assert.strictEqual((res.DISTRICT_ALERTS_MASK[dByte] & (1 << dBit)) !== 0, true);

  // Verify duration for state 1 is 30
  var st1Offset = 1 * 2;
  var st1Dur = res.REGION_DURATIONS[st1Offset] | (res.REGION_DURATIONS[st1Offset + 1] << 8);
  assert.strictEqual(st1Dur, 30);

  // Verify district duration propagated to district's parent state
  var distStateOffset = dist10.state_id * 2;
  var distStateDur = res.REGION_DURATIONS[distStateOffset] | (res.REGION_DURATIONS[distStateOffset + 1] << 8);
  assert.strictEqual(distStateDur, 10);
});

test('parseAlertsData counts oblast when only district has alert', function() {
  var nowMs = 1700000000000;
  var tenMinAgo = new Date(nowMs - 10 * 60 * 1000).toISOString();
  var dist10 = mapping.districts[10];

  var mockData = {
    states: {}
  };
  mockData.states[dist10.state_name] = {
    enabled: false,
    districts: {}
  };
  mockData.states[dist10.state_name].districts[dist10.district_name] = {
    enabled: true,
    enabled_at: tenMinAgo
  };

  var res = parser.parseAlertsData(mockData, mapping, nowMs, null);
  // Full region mask bit is not set, but ACTIVE_COUNT must include the oblast
  assert.strictEqual((res.ALERT_REGIONS_MASK & (1 << dist10.state_id)) === 0, true);
  assert.strictEqual(res.ACTIVE_COUNT, 1);
  assert.strictEqual(res.ACTIVE_DISTRICT_COUNT, 1);
});

test('parseAlertsData counts oblast only once when multiple districts have alerts', function() {
  var nowMs = 1700000000000;
  var tenMinAgo = new Date(nowMs - 10 * 60 * 1000).toISOString();
  var dist10 = mapping.districts[10];
  var dist11 = mapping.districts[11];
  assert.strictEqual(dist10.state_name, dist11.state_name, 'Both districts must belong to the same state');

  var mockData = {
    states: {}
  };
  mockData.states[dist10.state_name] = {
    enabled: false,
    districts: {}
  };
  mockData.states[dist10.state_name].districts[dist10.district_name] = {
    enabled: true,
    enabled_at: tenMinAgo
  };
  mockData.states[dist11.state_name].districts[dist11.district_name] = {
    enabled: true,
    enabled_at: tenMinAgo
  };

  var res = parser.parseAlertsData(mockData, mapping, nowMs, null);
  assert.strictEqual(res.ACTIVE_COUNT, 1);
  assert.strictEqual(res.ACTIVE_DISTRICT_COUNT, 2);
});

test('parseAlertsData does not double-count oblast when full alert and district alert coincide', function() {
  var nowMs = 1700000000000;
  var tenMinAgo = new Date(nowMs - 10 * 60 * 1000).toISOString();
  var dist0 = mapping.districts[0];

  var mockData = {
    states: {}
  };
  mockData.states[dist0.state_name] = {
    enabled: true,
    enabled_at: tenMinAgo,
    districts: {}
  };
  mockData.states[dist0.state_name].districts[dist0.district_name] = {
    enabled: true,
    enabled_at: tenMinAgo
  };

  var res = parser.parseAlertsData(mockData, mapping, nowMs, null);
  assert.strictEqual((res.ALERT_REGIONS_MASK & (1 << dist0.state_id)) !== 0, true);
  assert.strictEqual(res.ACTIVE_COUNT, 1);
  assert.strictEqual(res.ACTIVE_DISTRICT_COUNT, 1);
});

test('parseAlertsData handles Crimea / Sevastopol merging', function() {
  var nowMs = 1700000000000;
  var mockData = {
    states: {
      'Севастополь': {
        enabled: true,
        enabled_at: new Date(nowMs - 45 * 60 * 1000).toISOString()
      }
    }
  };

  var res = parser.parseAlertsData(mockData, mapping, nowMs, null);
  // Region 0 is Crimea
  assert.strictEqual((res.ALERT_REGIONS_MASK & (1 << 0)) !== 0, true);
  assert.strictEqual(res.ACTIVE_COUNT, 1);

  var dur = res.REGION_DURATIONS[0] | (res.REGION_DURATIONS[1] << 8);
  assert.strictEqual(dur, 45);
});
