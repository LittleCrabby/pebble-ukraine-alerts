#pragma once
#include <pebble.h>

typedef void (*CommServiceUpdateHandler)(void);

void comm_service_init(CommServiceUpdateHandler on_update);
void comm_service_deinit(void);
void comm_service_request_data(void);
