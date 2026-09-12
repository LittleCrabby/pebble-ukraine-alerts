#pragma once
#include <pebble.h>
#include "../models/geo_model.h"

typedef enum {
  LANG_AUTO = 0,
  LANG_EN,
  LANG_UK,
  LANG_FR,
  LANG_DE,
  LANG_ES,
  LANG_IT,
  LANG_PT,
  LANG_COUNT
} AppLanguage;

typedef enum {
  // Map banner strings
  I18N_UKRAINE,
  I18N_LOADING,
  I18N_ALERT,
  I18N_CALM,
  I18N_DISTRICT_ALERT,
  I18N_ACTIVE_ALERTS,
  I18N_ACTIVE_SUMMARY_FMT,
  I18N_NO_ALERTS,
  I18N_MY_LOCATION,
  I18N_IN_UKRAINE,

  // List view strings
  I18N_LOCATING,
  I18N_POS_NOT_DETERMINED,
  I18N_ALL_CLEAR_CALM,
  I18N_ENTIRE_REGION,
  I18N_ENTIRE_REGION_DUR_FMT,
  I18N_DISTS_DUR_FMT,
  I18N_DISTS_COUNT_FMT,
  I18N_DUR_DAYS_FMT,
  I18N_DUR_HOURS_MINS_FMT,
  I18N_DUR_MINS_FMT,
  I18N_SEC_MY_SAFETY,
  I18N_SEC_ACTIVE_ALERTS_FMT,
  I18N_SEC_ACTIONS,
  I18N_ALL_REGIONS_CALM,
  I18N_NO_ALERTS_RECORDED,
  I18N_LANGUAGE,
  I18N_LANG_AUTO_DESC,

  I18N_KEY_COUNT
} I18nKey;

void i18n_init(void);
AppLanguage i18n_get_selected_lang(void);
AppLanguage i18n_get_effective_lang(void);
void i18n_set_lang(AppLanguage lang);
void i18n_cycle_lang(void);
const char* i18n_get_lang_display_name(AppLanguage lang);
const char* i18n_get(I18nKey key);
const char* i18n_get_region_name(uint8_t region_id);
void i18n_format_district_name(const char *src, char *dst, size_t dst_len);
