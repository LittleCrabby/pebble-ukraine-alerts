#include "i18n.h"

#define PERSIST_KEY_LANGUAGE 100

static AppLanguage s_selected_lang = LANG_AUTO;

// Strings table: [lang_index][key]
// lang_index = effective_lang - 1 (0: EN, 1: UK, 2: FR, 3: DE, 4: ES, 5: IT, 6: PT)
static const char * const s_strings[LANG_COUNT - 1][I18N_KEY_COUNT] = {
  // LANG_EN
  {
    [I18N_UKRAINE] = "Ukraine",
    [I18N_LOADING] = "Loading...",
    [I18N_ALERT] = "ALERT!",
    [I18N_CALM] = "Clear",
    [I18N_DISTRICT_ALERT] = "District alert",
    [I18N_ACTIVE_ALERTS] = "Active alerts",
    [I18N_ACTIVE_SUMMARY_FMT] = "%d reg. / %d dist.",
    [I18N_NO_ALERTS] = "No alerts",
    [I18N_MY_LOCATION] = "My location",
    [I18N_IN_UKRAINE] = "In Ukraine",
    [I18N_LOCATING] = "Locating...",
    [I18N_POS_NOT_DETERMINED] = "Position not found",
    [I18N_ALL_CLEAR_CALM] = "All clear / Calm",
    [I18N_ENTIRE_REGION] = "Entire region",
    [I18N_ENTIRE_REGION_DUR_FMT] = "Entire region • %s",
    [I18N_DISTS_DUR_FMT] = "%d dist. • %s",
    [I18N_DISTS_COUNT_FMT] = "%d districts",
    [I18N_DUR_DAYS_FMT] = "%dd",
    [I18N_DUR_HOURS_MINS_FMT] = "%dh %dm",
    [I18N_DUR_MINS_FMT] = "%d min",
    [I18N_SEC_MY_SAFETY] = "MY SAFETY",
    [I18N_SEC_ACTIVE_ALERTS_FMT] = "ACTIVE ALERTS (%d)",
    [I18N_SEC_ACTIONS] = "ACTIONS",
    [I18N_ALL_REGIONS_CALM] = "All regions calm",
    [I18N_NO_ALERTS_RECORDED] = "No alerts recorded",
    [I18N_LANGUAGE] = "Language",
    [I18N_LANG_AUTO_DESC] = "Auto (Pebble OS)"
  },
  // LANG_UK
  {
    [I18N_UKRAINE] = "Україна",
    [I18N_LOADING] = "Завантаження...",
    [I18N_ALERT] = "ТРИВОГА!",
    [I18N_CALM] = "Спокійно",
    [I18N_DISTRICT_ALERT] = "Тривога в районі",
    [I18N_ACTIVE_ALERTS] = "Активні тривоги",
    [I18N_ACTIVE_SUMMARY_FMT] = "%d обл. / %d р-н.",
    [I18N_NO_ALERTS] = "Тривог немає",
    [I18N_MY_LOCATION] = "Моя локація",
    [I18N_IN_UKRAINE] = "В Україні",
    [I18N_LOCATING] = "Визначення...",
    [I18N_POS_NOT_DETERMINED] = "Позиція ще не визначена",
    [I18N_ALL_CLEAR_CALM] = "Відбій / Спокійно",
    [I18N_ENTIRE_REGION] = "Вся область",
    [I18N_ENTIRE_REGION_DUR_FMT] = "Вся область • %s",
    [I18N_DISTS_DUR_FMT] = "%d р-н • %s",
    [I18N_DISTS_COUNT_FMT] = "%d районів",
    [I18N_DUR_DAYS_FMT] = "%d дн",
    [I18N_DUR_HOURS_MINS_FMT] = "%dг %dхв",
    [I18N_DUR_MINS_FMT] = "%d хв",
    [I18N_SEC_MY_SAFETY] = "МОЯ БЕЗПЕКА",
    [I18N_SEC_ACTIVE_ALERTS_FMT] = "АКТИВНІ ТРИВОГИ (%d)",
    [I18N_SEC_ACTIONS] = "ДІЇ",
    [I18N_ALL_REGIONS_CALM] = "Всі області спокійні",
    [I18N_NO_ALERTS_RECORDED] = "Тривог не зафіксовано",
    [I18N_LANGUAGE] = "Мова",
    [I18N_LANG_AUTO_DESC] = "Авто (Pebble OS)"
  },
  // LANG_FR
  {
    [I18N_UKRAINE] = "Ukraine",
    [I18N_LOADING] = "Chargement...",
    [I18N_ALERT] = "ALERTE !",
    [I18N_CALM] = "Calme",
    [I18N_DISTRICT_ALERT] = "Alerte de district",
    [I18N_ACTIVE_ALERTS] = "Alertes actives",
    [I18N_ACTIVE_SUMMARY_FMT] = "%d rég. / %d dist.",
    [I18N_NO_ALERTS] = "Aucune alerte",
    [I18N_MY_LOCATION] = "Ma position",
    [I18N_IN_UKRAINE] = "En Ukraine",
    [I18N_LOCATING] = "Localisation...",
    [I18N_POS_NOT_DETERMINED] = "Position non définie",
    [I18N_ALL_CLEAR_CALM] = "Fin d'alerte / Calme",
    [I18N_ENTIRE_REGION] = "Toute la région",
    [I18N_ENTIRE_REGION_DUR_FMT] = "Toute la région • %s",
    [I18N_DISTS_DUR_FMT] = "%d dist. • %s",
    [I18N_DISTS_COUNT_FMT] = "%d districts",
    [I18N_DUR_DAYS_FMT] = "%dj",
    [I18N_DUR_HOURS_MINS_FMT] = "%dh %dm",
    [I18N_DUR_MINS_FMT] = "%d min",
    [I18N_SEC_MY_SAFETY] = "MA SÉCURITÉ",
    [I18N_SEC_ACTIVE_ALERTS_FMT] = "ALERTES ACTIVES (%d)",
    [I18N_SEC_ACTIONS] = "ACTIONS",
    [I18N_ALL_REGIONS_CALM] = "Toutes les régions calmes",
    [I18N_NO_ALERTS_RECORDED] = "Aucune alerte signalée",
    [I18N_LANGUAGE] = "Langue",
    [I18N_LANG_AUTO_DESC] = "Auto (Pebble OS)"
  },
  // LANG_DE
  {
    [I18N_UKRAINE] = "Ukraine",
    [I18N_LOADING] = "Laden...",
    [I18N_ALERT] = "ALARM!",
    [I18N_CALM] = "Entwarnung",
    [I18N_DISTRICT_ALERT] = "Bezirksalarm",
    [I18N_ACTIVE_ALERTS] = "Aktive Alarme",
    [I18N_ACTIVE_SUMMARY_FMT] = "%d Reg. / %d Bez.",
    [I18N_NO_ALERTS] = "Keine Alarme",
    [I18N_MY_LOCATION] = "Mein Standort",
    [I18N_IN_UKRAINE] = "In der Ukraine",
    [I18N_LOCATING] = "Standortsuche...",
    [I18N_POS_NOT_DETERMINED] = "Standort unbekannt",
    [I18N_ALL_CLEAR_CALM] = "Entwarnung / Ruhig",
    [I18N_ENTIRE_REGION] = "Ganze Region",
    [I18N_ENTIRE_REGION_DUR_FMT] = "Ganze Region • %s",
    [I18N_DISTS_DUR_FMT] = "%d Bez. • %s",
    [I18N_DISTS_COUNT_FMT] = "%d Bezirke",
    [I18N_DUR_DAYS_FMT] = "%dT",
    [I18N_DUR_HOURS_MINS_FMT] = "%dh %dm",
    [I18N_DUR_MINS_FMT] = "%d Min",
    [I18N_SEC_MY_SAFETY] = "MEINE SICHERHEIT",
    [I18N_SEC_ACTIVE_ALERTS_FMT] = "AKTIVE ALARME (%d)",
    [I18N_SEC_ACTIONS] = "AKTIONEN",
    [I18N_ALL_REGIONS_CALM] = "Alle Regionen ruhig",
    [I18N_NO_ALERTS_RECORDED] = "Keine Alarme erfasst",
    [I18N_LANGUAGE] = "Sprache",
    [I18N_LANG_AUTO_DESC] = "Auto (Pebble OS)"
  },
  // LANG_ES
  {
    [I18N_UKRAINE] = "Ucrania",
    [I18N_LOADING] = "Cargando...",
    [I18N_ALERT] = "¡ALARMA!",
    [I18N_CALM] = "Tranquilo",
    [I18N_DISTRICT_ALERT] = "Alerta de distrito",
    [I18N_ACTIVE_ALERTS] = "Alertas activas",
    [I18N_ACTIVE_SUMMARY_FMT] = "%d reg. / %d dist.",
    [I18N_NO_ALERTS] = "Sin alertas",
    [I18N_MY_LOCATION] = "Mi ubicación",
    [I18N_IN_UKRAINE] = "En Ucrania",
    [I18N_LOCATING] = "Ubicando...",
    [I18N_POS_NOT_DETERMINED] = "Ubicación desconocida",
    [I18N_ALL_CLEAR_CALM] = "Despejado / Seguro",
    [I18N_ENTIRE_REGION] = "Toda la región",
    [I18N_ENTIRE_REGION_DUR_FMT] = "Toda la región • %s",
    [I18N_DISTS_DUR_FMT] = "%d dist. • %s",
    [I18N_DISTS_COUNT_FMT] = "%d distritos",
    [I18N_DUR_DAYS_FMT] = "%dd",
    [I18N_DUR_HOURS_MINS_FMT] = "%dh %dm",
    [I18N_DUR_MINS_FMT] = "%d min",
    [I18N_SEC_MY_SAFETY] = "MI SEGURIDAD",
    [I18N_SEC_ACTIVE_ALERTS_FMT] = "ALERTAS ACTIVAS (%d)",
    [I18N_SEC_ACTIONS] = "ACCIONES",
    [I18N_ALL_REGIONS_CALM] = "Todas las regiones en calma",
    [I18N_NO_ALERTS_RECORDED] = "Sin alertas registradas",
    [I18N_LANGUAGE] = "Idioma",
    [I18N_LANG_AUTO_DESC] = "Auto (Pebble OS)"
  },
  // LANG_IT
  {
    [I18N_UKRAINE] = "Ucraina",
    [I18N_LOADING] = "Caricamento...",
    [I18N_ALERT] = "ALLERTA!",
    [I18N_CALM] = "Tranquillo",
    [I18N_DISTRICT_ALERT] = "Allerta distretto",
    [I18N_ACTIVE_ALERTS] = "Allerte attive",
    [I18N_ACTIVE_SUMMARY_FMT] = "%d reg. / %d dist.",
    [I18N_NO_ALERTS] = "Nessuna allerta",
    [I18N_MY_LOCATION] = "La mia posizione",
    [I18N_IN_UKRAINE] = "In Ucraina",
    [I18N_LOCATING] = "Rilevamento...",
    [I18N_POS_NOT_DETERMINED] = "Posizione non definita",
    [I18N_ALL_CLEAR_CALM] = "Cessato allarme",
    [I18N_ENTIRE_REGION] = "Tutta la regione",
    [I18N_ENTIRE_REGION_DUR_FMT] = "Tutta la regione • %s",
    [I18N_DISTS_DUR_FMT] = "%d dist. • %s",
    [I18N_DISTS_COUNT_FMT] = "%d distretti",
    [I18N_DUR_DAYS_FMT] = "%dg",
    [I18N_DUR_HOURS_MINS_FMT] = "%dh %dm",
    [I18N_DUR_MINS_FMT] = "%d min",
    [I18N_SEC_MY_SAFETY] = "LA MIA SICUREZZA",
    [I18N_SEC_ACTIVE_ALERTS_FMT] = "ALLERTE ATTIVE (%d)",
    [I18N_SEC_ACTIONS] = "AZIONI",
    [I18N_ALL_REGIONS_CALM] = "Tutte le regioni calme",
    [I18N_NO_ALERTS_RECORDED] = "Nessun allarme registrato",
    [I18N_LANGUAGE] = "Lingua",
    [I18N_LANG_AUTO_DESC] = "Auto (Pebble OS)"
  },
  // LANG_PT
  {
    [I18N_UKRAINE] = "Ucrânia",
    [I18N_LOADING] = "Carregando...",
    [I18N_ALERT] = "ALARME!",
    [I18N_CALM] = "Calmo",
    [I18N_DISTRICT_ALERT] = "Alerta distrital",
    [I18N_ACTIVE_ALERTS] = "Alarmes ativos",
    [I18N_ACTIVE_SUMMARY_FMT] = "%d reg. / %d dist.",
    [I18N_NO_ALERTS] = "Sem alarmes",
    [I18N_MY_LOCATION] = "Minha posição",
    [I18N_IN_UKRAINE] = "Na Ucrânia",
    [I18N_LOCATING] = "Localizando...",
    [I18N_POS_NOT_DETERMINED] = "Posição indefinida",
    [I18N_ALL_CLEAR_CALM] = "Tudo limpo / Calmo",
    [I18N_ENTIRE_REGION] = "Toda a região",
    [I18N_ENTIRE_REGION_DUR_FMT] = "Toda a região • %s",
    [I18N_DISTS_DUR_FMT] = "%d dist. • %s",
    [I18N_DISTS_COUNT_FMT] = "%d distritos",
    [I18N_DUR_DAYS_FMT] = "%dd",
    [I18N_DUR_HOURS_MINS_FMT] = "%dh %dm",
    [I18N_DUR_MINS_FMT] = "%d min",
    [I18N_SEC_MY_SAFETY] = "MINHA SEGURANÇA",
    [I18N_SEC_ACTIVE_ALERTS_FMT] = "ALARMES ATIVOS (%d)",
    [I18N_SEC_ACTIONS] = "AÇÕES",
    [I18N_ALL_REGIONS_CALM] = "Todas as regiões calmas",
    [I18N_NO_ALERTS_RECORDED] = "Nenhum alarme registrado",
    [I18N_LANGUAGE] = "Idioma",
    [I18N_LANG_AUTO_DESC] = "Auto (Pebble OS)"
  }
};

// 26 Region names: [lang_index][region_id]
static const char * const s_region_names[LANG_COUNT - 1][GEO_TOTAL_REGIONS] = {
  // EN
  {
    "Crimea", "Vinnytsia obl.", "Volyn obl.", "Dnipropetrovsk obl.",
    "Donetsk obl.", "Zhytomyr obl.", "Zakarpattia obl.", "Zaporizhzhia obl.", "Ivano-Frankivsk obl.",
    "Kyiv obl.", "Kirovohrad obl.", "Luhansk obl.", "Lviv obl.", "Mykolaiv obl.",
    "Odesa obl.", "Poltava obl.", "Rivne obl.", "Sumy obl.", "Ternopil obl.",
    "Kharkiv obl.", "Kherson obl.", "Khmelnytskyi obl.", "Cherkasy obl.", "Chernivtsi obl.",
    "Chernihiv obl.", "Kyiv"
  },
  // UK
  {
    "АР Крим", "Вінницька обл.", "Волинська обл.", "Дніпропетровська обл.",
    "Донецька обл.", "Житомирська обл.", "Закарпатська обл.", "Запорізька обл.", "Івано-Франківська обл.",
    "Київська обл.", "Кіровоградська обл.", "Луганська обл.", "Львівська обл.", "Миколаївська обл.",
    "Одеська обл.", "Полтавська обл.", "Рівненська обл.", "Сумська обл.", "Тернопільська обл.",
    "Харківська обл.", "Херсонська обл.", "Хмельницька обл.", "Черкаська обл.", "Чернівецька обл.",
    "Чернігівська обл.", "м. Київ"
  },
  // FR
  {
    "Crimée", "Obl. de Vinnytsia", "Obl. de Volhynie", "Obl. de Dnipropetrovsk",
    "Obl. de Donetsk", "Obl. de Jytomyr", "Obl. de Transcarpatie", "Obl. de Zaporijia", "Obl. d'Ivano-Frankivsk",
    "Obl. de Kyïv", "Obl. de Kirovohrad", "Obl. de Louhansk", "Obl. de Lviv", "Obl. de Mykolaïv",
    "Obl. d'Odessa", "Obl. de Poltava", "Obl. de Rivne", "Obl. de Soumy", "Obl. de Ternopil",
    "Obl. de Kharkiv", "Obl. de Kherson", "Obl. de Khmelnytskyï", "Obl. de Tcherkassy", "Obl. de Tchernivtsi",
    "Obl. de Tchernihiv", "Kyïv"
  },
  // DE
  {
    "Krim", "Obl. Winnyzja", "Obl. Wolyn", "Obl. Dnipropetrowsk",
    "Obl. Donezk", "Obl. Schytomyr", "Obl. Transkarpatien", "Obl. Saporischschja", "Obl. Iwano-Frankiwsk",
    "Obl. Kiew", "Obl. Kirowohrad", "Obl. Luhansk", "Obl. Lwiw", "Obl. Mykolajiw",
    "Obl. Odessa", "Obl. Poltawa", "Obl. Riwne", "Obl. Sumy", "Obl. Ternopil",
    "Obl. Charkiw", "Obl. Cherson", "Obl. Chmelnyzkyj", "Obl. Tscherkassy", "Obl. Tscherniwzi",
    "Obl. Tschernihiw", "Kiew"
  },
  // ES
  {
    "Crimea", "Óbl. de Vínnitsa", "Óbl. de Volinia", "Óbl. de Dnipró",
    "Óbl. de Donetsk", "Óbl. de Zhytómyr", "Óbl. de Transcarpacia", "Óbl. de Zaporiyia", "Óbl. de Ivano-Frankivsk",
    "Óbl. de Kiev", "Óbl. de Kirovogrado", "Óbl. de Lugansk", "Óbl. de Leópolis", "Óbl. de Mykoláiv",
    "Óbl. de Odesa", "Óbl. de Poltava", "Óbl. de Rivne", "Óbl. de Sumy", "Óbl. de Ternópil",
    "Óbl. de Járkov", "Óbl. de Jersón", "Óbl. de Jmelnitski", "Óbl. de Cherkasy", "Óbl. de Chernivtsí",
    "Óbl. de Chernígov", "Kiev"
  },
  // IT
  {
    "Crimea", "Obl. di Vinnycja", "Obl. di Volinia", "Obl. di Dnipropetrovs'k",
    "Obl. di Donec'k", "Obl. di Zytomyr", "Obl. della Transcarpazia", "Obl. di Zaporizzja", "Obl. di Ivano-Frankivs'k",
    "Obl. di Kiev", "Obl. di Kirovohrad", "Obl. di Luhans'k", "Obl. di Leopoli", "Obl. di Mykolaïv",
    "Obl. di Odessa", "Obl. di Poltava", "Obl. di Rivne", "Obl. di Sumy", "Obl. di Ternopil'",
    "Obl. di Charkiv", "Obl. di Cherson", "Obl. di Chmel'nyc'kyj", "Obl. di Cerkasy", "Obl. di Cernivci",
    "Obl. di Cernihiv", "Kiev"
  },
  // PT
  {
    "Crimeia", "Óbl. de Vinnytsia", "Óbl. de Volínia", "Óbl. de Dnipropetrovsk",
    "Óbl. de Donetsk", "Óbl. de Jytomyr", "Óbl. de Transcarpátia", "Óbl. de Zaporíjia", "Óbl. de Ivano-Frankivsk",
    "Óbl. de Kiev", "Óbl. de Kirovogrado", "Óbl. de Luhansk", "Óbl. de Lviv", "Óbl. de Mykolaiv",
    "Óbl. de Odessa", "Óbl. de Poltava", "Óbl. de Rivne", "Óbl. de Sumy", "Óbl. de Ternopil",
    "Óbl. de Kharkiv", "Óbl. de Kherson", "Óbl. de Khmelnytskyi", "Óbl. de Cherkasy", "Óbl. de Chernivtsi",
    "Óbl. de Chernihiv", "Kiev"
  }
};

void i18n_init(void) {
  setlocale(LC_ALL, "");
  if (persist_exists(PERSIST_KEY_LANGUAGE)) {
    s_selected_lang = (AppLanguage)persist_read_int(PERSIST_KEY_LANGUAGE);
    if (s_selected_lang >= LANG_COUNT) {
      s_selected_lang = LANG_AUTO;
    }
  } else {
    s_selected_lang = LANG_AUTO;
  }
}

AppLanguage i18n_get_selected_lang(void) {
  return s_selected_lang;
}

AppLanguage i18n_get_effective_lang(void) {
  if (s_selected_lang != LANG_AUTO) {
    return s_selected_lang;
  }
  const char *locale = i18n_get_system_locale();
  if (!locale) return LANG_EN;
  if (strncmp(locale, "uk", 2) == 0) return LANG_UK;
  if (strncmp(locale, "fr", 2) == 0) return LANG_FR;
  if (strncmp(locale, "de", 2) == 0) return LANG_DE;
  if (strncmp(locale, "es", 2) == 0) return LANG_ES;
  if (strncmp(locale, "it", 2) == 0) return LANG_IT;
  if (strncmp(locale, "pt", 2) == 0) return LANG_PT;
  return LANG_EN;
}

void i18n_set_lang(AppLanguage lang) {
  if (lang >= LANG_COUNT) lang = LANG_AUTO;
  s_selected_lang = lang;
  persist_write_int(PERSIST_KEY_LANGUAGE, (int32_t)lang);
}

void i18n_cycle_lang(void) {
  AppLanguage next = s_selected_lang + 1;
  if (next >= LANG_COUNT) {
    next = LANG_AUTO;
  }
  i18n_set_lang(next);
}

const char* i18n_get_lang_display_name(AppLanguage lang) {
  switch (lang) {
    case LANG_AUTO: return i18n_get(I18N_LANG_AUTO_DESC);
    case LANG_EN: return "English";
    case LANG_UK: return "Українська";
    case LANG_FR: return "Français";
    case LANG_DE: return "Deutsch";
    case LANG_ES: return "Español";
    case LANG_IT: return "Italiano";
    case LANG_PT: return "Português";
    default: return "";
  }
}

const char* i18n_get(I18nKey key) {
  if (key >= I18N_KEY_COUNT) return "";
  AppLanguage eff = i18n_get_effective_lang();
  int idx = (int)eff - 1;
  if (idx < 0 || idx >= (LANG_COUNT - 1)) idx = 0; // Default to EN
  const char *str = s_strings[idx][key];
  return str ? str : "";
}

const char* i18n_get_region_name(uint8_t region_id) {
  if (region_id >= GEO_TOTAL_REGIONS) return "";
  AppLanguage eff = i18n_get_effective_lang();
  int idx = (int)eff - 1;
  if (idx < 0 || idx >= (LANG_COUNT - 1)) idx = 0;
  return s_region_names[idx][region_id];
}

static bool str_ends_with(const char *str, const char *suffix, size_t *out_prefix_len) {
  size_t slen = strlen(str);
  size_t ulen = strlen(suffix);
  if (slen >= ulen && strcmp(str + slen - ulen, suffix) == 0) {
    if (out_prefix_len) *out_prefix_len = slen - ulen;
    return true;
  }
  return false;
}

void i18n_format_district_name(const char *src, char *dst, size_t dst_len) {
  if (!src || !dst || dst_len == 0) return;
  if (src[0] == '\0') {
    dst[0] = '\0';
    return;
  }
  AppLanguage lang = i18n_get_effective_lang();
  if (lang == LANG_UK) {
    strncpy(dst, src, dst_len - 1);
    dst[dst_len - 1] = '\0';
    size_t prefix_len;
    if (str_ends_with(dst, " район", &prefix_len)) {
      if (prefix_len + 8 < dst_len) {
        dst[prefix_len] = '\0';
        strcat(dst, " р-н");
      }
    }
    // Clean trailing single quote if any
    size_t len = strlen(dst);
    if (len > 0 && dst[len - 1] == '\'') {
      dst[len - 1] = '\0';
    }
    return;
  }

  // Western languages: transliterate Ukrainian Cyrillic to Latin
  const char *suffix = (lang == LANG_DE) ? " Bez." : " dist.";

  // Find where " район" or " р-н" begins to truncate before transliterating
  char src_clean[96];
  strncpy(src_clean, src, sizeof(src_clean) - 1);
  src_clean[sizeof(src_clean) - 1] = '\0';

  size_t prefix_len;
  if (str_ends_with(src_clean, " район", &prefix_len) || str_ends_with(src_clean, " р-н", &prefix_len)) {
    src_clean[prefix_len] = '\0';
  }

  size_t d_idx = 0;
  const uint8_t *p = (const uint8_t *)src_clean;

  while (*p && d_idx + 6 < dst_len) {
    uint8_t b1 = *p++;
    if (b1 < 0x80) {
      if (b1 != '\'' && b1 != '`') {
        dst[d_idx++] = (char)b1;
      }
    } else if (b1 == 0xD0) {
      if (!*p) break;
      uint8_t b2 = *p++;
      switch (b2) {
        case 0x84: dst[d_idx++] = 'Y'; dst[d_idx++] = 'e'; break; // Є
        case 0x86: dst[d_idx++] = 'I'; break;                      // І
        case 0x87: dst[d_idx++] = 'Y'; dst[d_idx++] = 'i'; break; // Ї
        case 0x90: dst[d_idx++] = 'A'; break;
        case 0x91: dst[d_idx++] = 'B'; break;
        case 0x92: dst[d_idx++] = 'V'; break;
        case 0x93: dst[d_idx++] = 'H'; break;
        case 0x94: dst[d_idx++] = 'D'; break;
        case 0x95: dst[d_idx++] = 'E'; break;
        case 0x96: dst[d_idx++] = 'Z'; dst[d_idx++] = 'h'; break;
        case 0x97: dst[d_idx++] = 'Z'; break;
        case 0x98: dst[d_idx++] = 'Y'; break;
        case 0x99: dst[d_idx++] = 'Y'; break;
        case 0x9A: dst[d_idx++] = 'K'; break;
        case 0x9B: dst[d_idx++] = 'L'; break;
        case 0x9C: dst[d_idx++] = 'M'; break;
        case 0x9D: dst[d_idx++] = 'N'; break;
        case 0x9E: dst[d_idx++] = 'O'; break;
        case 0x9F: dst[d_idx++] = 'P'; break;
        case 0xA0: dst[d_idx++] = 'R'; break;
        case 0xA1: dst[d_idx++] = 'S'; break;
        case 0xA2: dst[d_idx++] = 'T'; break;
        case 0xA3: dst[d_idx++] = 'U'; break;
        case 0xA4: dst[d_idx++] = 'F'; break;
        case 0xA5: dst[d_idx++] = 'K'; dst[d_idx++] = 'h'; break;
        case 0xA6: dst[d_idx++] = 'T'; dst[d_idx++] = 's'; break;
        case 0xA7: dst[d_idx++] = 'C'; dst[d_idx++] = 'h'; break;
        case 0xA8: dst[d_idx++] = 'S'; dst[d_idx++] = 'h'; break;
        case 0xA9: dst[d_idx++] = 'S'; dst[d_idx++] = 'h'; dst[d_idx++] = 'c'; dst[d_idx++] = 'h'; break;
        case 0xAE: dst[d_idx++] = 'Y'; dst[d_idx++] = 'u'; break;
        case 0xAF: dst[d_idx++] = 'Y'; dst[d_idx++] = 'a'; break;
        case 0xB0: dst[d_idx++] = 'a'; break;
        case 0xB1: dst[d_idx++] = 'b'; break;
        case 0xB2: dst[d_idx++] = 'v'; break;
        case 0xB3: dst[d_idx++] = 'h'; break;
        case 0xB4: dst[d_idx++] = 'd'; break;
        case 0xB5: dst[d_idx++] = 'e'; break;
        case 0xB6: dst[d_idx++] = 'z'; dst[d_idx++] = 'h'; break;
        case 0xB7: dst[d_idx++] = 'z'; break;
        case 0xB8: dst[d_idx++] = 'y'; break;
        case 0xB9: dst[d_idx++] = 'i'; break;
        case 0xBA: dst[d_idx++] = 'k'; break;
        case 0xBB: dst[d_idx++] = 'l'; break;
        case 0xBC: dst[d_idx++] = 'm'; break;
        case 0xBD: dst[d_idx++] = 'n'; break;
        case 0xBE: dst[d_idx++] = 'o'; break;
        case 0xBF: dst[d_idx++] = 'p'; break;
        default: break;
      }
    } else if (b1 == 0xD1) {
      if (!*p) break;
      uint8_t b2 = *p++;
      switch (b2) {
        case 0x80: dst[d_idx++] = 'r'; break;
        case 0x81: dst[d_idx++] = 's'; break;
        case 0x82: dst[d_idx++] = 't'; break;
        case 0x83: dst[d_idx++] = 'u'; break;
        case 0x84: dst[d_idx++] = 'f'; break;
        case 0x85: dst[d_idx++] = 'k'; dst[d_idx++] = 'h'; break;
        case 0x86: dst[d_idx++] = 't'; dst[d_idx++] = 's'; break;
        case 0x87: dst[d_idx++] = 'c'; dst[d_idx++] = 'h'; break;
        case 0x88: dst[d_idx++] = 's'; dst[d_idx++] = 'h'; break;
        case 0x89: dst[d_idx++] = 's'; dst[d_idx++] = 'h'; dst[d_idx++] = 'c'; dst[d_idx++] = 'h'; break;
        case 0x8C: break; // Soft sign omitted in standard Latin transliteration
        case 0x8E: dst[d_idx++] = 'i'; dst[d_idx++] = 'u'; break;
        case 0x8F: dst[d_idx++] = 'i'; dst[d_idx++] = 'a'; break;
        case 0x94: dst[d_idx++] = 'i'; dst[d_idx++] = 'e'; break; // є
        case 0x96: dst[d_idx++] = 'i'; break;                      // і
        case 0x97: dst[d_idx++] = 'i'; break;                      // ї
        default: break;
      }
    } else if (b1 == 0xD2) {
      if (!*p) break;
      uint8_t b2 = *p++;
      if (b2 == 0x90) dst[d_idx++] = 'G';
      else if (b2 == 0x91) dst[d_idx++] = 'g';
    } else if (b1 == 0xE2) {
      // e.g. UTF-8 for ’ (\xE2\x80\x99)
      if (*p && *(p + 1)) {
        p += 2;
      } else {
        break;
      }
    }
  }

  dst[d_idx] = '\0';

  // Append suffix if space remains
  if (suffix && d_idx + strlen(suffix) < dst_len) {
    strncat(dst, suffix, dst_len - d_idx - 1);
  }
}
