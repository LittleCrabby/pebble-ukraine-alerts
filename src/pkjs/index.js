var mapping = require('./mapping.json');
var parser = require('./alerts_parser.js');

var API_URL = 'https://vadimklimenko.com/map/statuses.json';
var FALLBACK_URL = 'http://ubilling.net.ua/aerialalerts/';
var POLL_INTERVAL_MS = 15000;
var pollTimer = null;
var userLocation = null;
var lastAlertData = null;

function updateUserLocation() {
  if (typeof navigator !== 'undefined' && navigator.geolocation) {
    navigator.geolocation.getCurrentPosition(
      function(pos) {
        if (pos && pos.coords) {
          var norm = parser.normalizeCoordinates(pos.coords.latitude, pos.coords.longitude);
          if (norm) {
            var prevLocation = userLocation;
            userLocation = norm;
            console.log('User location in Ukraine: lat=' + pos.coords.latitude + ', lon=' + pos.coords.longitude + ' -> normX=' + norm.x + ', normY=' + norm.y);
            if ((!prevLocation || prevLocation.x !== norm.x || prevLocation.y !== norm.y) && lastAlertData) {
              sendParsedAlerts(lastAlertData);
            }
            return;
          }
          console.log('User location outside Ukraine: lat=' + pos.coords.latitude + ', lon=' + pos.coords.longitude);
          if (userLocation !== null) {
            userLocation = null;
            if (lastAlertData) {
              sendParsedAlerts(lastAlertData);
            }
          }
        }
      },
      function(err) {
        console.log('Geolocation error: ' + (err ? err.message : 'unknown'));
      },
      { timeout: 10000, maximumAge: 60000, enableHighAccuracy: false }
    );
  }
}

function sendParsedAlerts(data) {
  var dict = parser.parseAlertsData(data, mapping, Date.now(), userLocation);
  Pebble.sendAppMessage(dict, function() {
    console.log('Sent alert state to Pebble successfully: active=' + dict.ACTIVE_COUNT + ' dist=' + dict.ACTIVE_DISTRICT_COUNT + ' userLoc=' + (userLocation ? 'valid' : 'none'));
  }, function(e) {
    console.log('Failed to send alert state: ' + JSON.stringify(e));
  });
}

function sendErrorStatus(code) {
  Pebble.sendAppMessage({ 'STATUS_CODE': code }, function() {
    console.log('Sent error status to Pebble: ' + code);
  }, function(e) {
    console.log('Failed to send error status: ' + JSON.stringify(e));
  });
}

function requestJson(url, callback) {
  var xhr = new XMLHttpRequest();
  xhr.open('GET', url + (url.indexOf('?') === -1 ? '?' : '&') + 't=' + Date.now(), true);
  xhr.timeout = 10000;

  xhr.onload = function() {
    if (xhr.status >= 200 && xhr.status < 300) {
      try {
        var data = JSON.parse(xhr.responseText);
        callback(null, data);
      } catch (e) {
        callback(new Error('JSON parse error: ' + e));
      }
    } else {
      callback(new Error('HTTP error: ' + xhr.status));
    }
  };

  xhr.onerror = function() {
    callback(new Error('Network error'));
  };

  xhr.ontimeout = function() {
    callback(new Error('Request timeout'));
  };

  xhr.send();
}

function fetchAlertData() {
  updateUserLocation();

  requestJson(API_URL, function(err, data) {
    if (!err && data) {
      lastAlertData = data;
      sendParsedAlerts(data);
    } else {
      console.log('Primary API failed (' + (err ? err.message : 'unknown') + '), trying fallback...');
      requestJson(FALLBACK_URL, function(fallbackErr, fallbackData) {
        if (!fallbackErr && fallbackData) {
          lastAlertData = fallbackData;
          sendParsedAlerts(fallbackData);
        } else {
          console.log('Fallback API also failed: ' + (fallbackErr ? fallbackErr.message : 'unknown'));
          if (!lastAlertData) {
            sendErrorStatus(-1);
          }
        }
      });
    }
  });
}

Pebble.addEventListener('ready', function() {
  console.log('PebbleKit JS ready for Ukraine Alerts');
  updateUserLocation();
  fetchAlertData();

  if (pollTimer) clearInterval(pollTimer);
  pollTimer = setInterval(fetchAlertData, POLL_INTERVAL_MS);
});

Pebble.addEventListener('appmessage', function(e) {
  console.log('AppMessage received from watch, refreshing data');
  fetchAlertData();
});
