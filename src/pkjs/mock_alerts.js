// Mock alert data for generating store screenshots with full functional showcase

function getMockAlertsData(nowMs) {
  var now = typeof nowMs === 'number' ? nowMs : Date.now();
  var minAgo = function(m) { return new Date(now - m * 60000).toISOString(); };

  return {
    states: {
      "АР Крим": { enabled: true, alert_level: "red", enabled_at: minAgo(125) },
      "Севастополь": { enabled: true, alert_level: "red", enabled_at: minAgo(125) },
      "Луганська область": { enabled: true, alert_level: "red", enabled_at: minAgo(190) },
      "Донецька область": { enabled: true, alert_level: "red", enabled_at: minAgo(95) },
      "Харківська область": { enabled: true, alert_level: "red", enabled_at: minAgo(55) },
      "Запорізька область": { enabled: true, alert_level: "red", enabled_at: minAgo(40) },
      "Сумська область": { enabled: true, alert_level: "red", enabled_at: minAgo(25) },
      "м. Київ": { enabled: true, alert_level: "red", enabled_at: minAgo(18) },
      "Київська область": {
        enabled: false,
        districts: {
          "Вишгородський район": { enabled: true, alert_level: "yellow", enabled_at: minAgo(22) },
          "Броварський район": { enabled: true, alert_level: "orange", enabled_at: minAgo(32) },
          "Бориспільський район": { enabled: true, alert_level: "red", enabled_at: minAgo(18) }
        }
      },
      "Дніпропетровська область": {
        enabled: false,
        districts: {
          "Дніпровський район": { enabled: true, alert_level: "orange", enabled_at: minAgo(45) },
          "Синельниківський район": { enabled: true, alert_level: "red", enabled_at: minAgo(50) },
          "Нікопольський район": { enabled: true, alert_level: "orange", enabled_at: minAgo(65) }
        }
      },
      "Полтавська область": {
        enabled: false,
        districts: {
          "Кременчуцький район": { enabled: true, alert_level: "yellow", enabled_at: minAgo(12) }
        }
      },
      "Чернігівська область": {
        enabled: false,
        districts: {
          "Чернігівський район": { enabled: true, alert_level: "yellow", enabled_at: minAgo(20) },
          "Корюківський район": { enabled: true, alert_level: "yellow", enabled_at: minAgo(25) }
        }
      }
    }
  };
}

var MOCK_USER_LOCATION = { x: 4650, y: 2421 }; // Kyiv center

module.exports = {
  getMockAlertsData: getMockAlertsData,
  MOCK_USER_LOCATION: MOCK_USER_LOCATION
};
