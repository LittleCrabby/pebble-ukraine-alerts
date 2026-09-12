CC ?= gcc
CFLAGS = -Wall -Wextra -O2 -I../src/c -Imock_pebble

TEST_BINS = test_geo_math test_geo_model test_alert_model test_i18n

all: $(TEST_BINS)
	@echo "--- Running C Unit Tests ---"
	@./test_geo_math
	@./test_geo_model
	@./test_alert_model
	@./test_i18n
	@echo "--- All C Unit Tests Passed! ---"

test_geo_math: test_geo_math.c ../src/c/util/geo_math.c ../src/c/models/geo_model.c mock_pebble/pebble_mock.c
	$(CC) $(CFLAGS) -o $@ $^

test_geo_model: test_geo_model.c ../src/c/models/geo_model.c mock_pebble/pebble_mock.c
	$(CC) $(CFLAGS) -o $@ $^

test_alert_model: test_alert_model.c ../src/c/models/alert_model.c ../src/c/models/geo_model.c mock_pebble/pebble_mock.c
	$(CC) $(CFLAGS) -o $@ $^

test_i18n: test_i18n.c ../src/c/util/i18n.c ../src/c/models/geo_model.c mock_pebble/pebble_mock.c
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(TEST_BINS)

.PHONY: all clean
