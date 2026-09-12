#include <pebble.h>
#include <assert.h>
#include <stdio.h>
#include "util/i18n.h"

static void test_i18n_languages(void) {
  i18n_init();
  assert(i18n_get_selected_lang() == LANG_AUTO);

  // Set English
  i18n_set_lang(LANG_EN);
  assert(i18n_get_selected_lang() == LANG_EN);
  assert(i18n_get_effective_lang() == LANG_EN);
  assert(strcmp(i18n_get(I18N_ALERT), "ALERT!") == 0);
  assert(strcmp(i18n_get_region_name(25), "Kyiv") == 0);

  // Set Ukrainian
  i18n_set_lang(LANG_UK);
  assert(i18n_get_selected_lang() == LANG_UK);
  assert(i18n_get_effective_lang() == LANG_UK);
  assert(strcmp(i18n_get(I18N_ALERT), "ТРИВОГА!") == 0);
  assert(strcmp(i18n_get_region_name(25), "м. Київ") == 0);

  // Cycle language
  i18n_cycle_lang(); // UK -> FR
  assert(i18n_get_effective_lang() == LANG_FR);
  assert(strcmp(i18n_get(I18N_ALERT), "ALERTE !") == 0);
}

static void test_format_district_name_uk(void) {
  i18n_set_lang(LANG_UK);
  char buf[64];

  i18n_format_district_name("Броварський район", buf, sizeof(buf));
  assert(strcmp(buf, "Броварський р-н") == 0);

  i18n_format_district_name("Львівський р-н", buf, sizeof(buf));
  assert(strcmp(buf, "Львівський р-н") == 0);
}

static void test_format_district_name_translit(void) {
  char buf[96];

  // English transliteration
  i18n_set_lang(LANG_EN);
  i18n_format_district_name("Броварський район", buf, sizeof(buf));
  assert(strcmp(buf, "Brovarskyi dist.") == 0);

  // German transliteration
  i18n_set_lang(LANG_DE);
  i18n_format_district_name("Броварський район", buf, sizeof(buf));
  assert(strcmp(buf, "Brovarskyi Bez.") == 0);

  // Special Cyrillic characters: Є, І, Ї, Щ, apostrophe
  i18n_set_lang(LANG_EN);
  i18n_format_district_name("Щастинський район", buf, sizeof(buf));
  assert(strcmp(buf, "Shchastynskyi dist.") == 0);

  i18n_format_district_name("Ізмаїльський район", buf, sizeof(buf));
  assert(strcmp(buf, "Izmailskyi dist.") == 0);
}

static void test_format_district_buffer_safety(void) {
  i18n_set_lang(LANG_EN);
  char small_buf[10];

  // Must truncate safely without crash or buffer overflow
  i18n_format_district_name("Дуже Довга Назва Району район", small_buf, sizeof(small_buf));
  assert(strlen(small_buf) < sizeof(small_buf));

  // Empty string
  small_buf[0] = '\0';
  i18n_format_district_name("", small_buf, sizeof(small_buf));
  assert(small_buf[0] == '\0');

  // Null pointer safety
  i18n_format_district_name(NULL, small_buf, sizeof(small_buf));
  i18n_format_district_name("Test", NULL, 0);
}

int main(void) {
  printf("Running test_i18n...\n");
  test_i18n_languages();
  test_format_district_name_uk();
  test_format_district_name_translit();
  test_format_district_buffer_safety();
  printf("test_i18n: ALL PASSED\n");
  return 0;
}
