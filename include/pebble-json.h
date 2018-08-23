#pragma once
#include <pebble.h>

typedef struct Json Json;

Json *json_create_with_resource(uint32_t resource_id);
Json *json_create(const char *s, bool free_on_destroy);
void json_destroy(Json *json);

bool json_has_next(Json *json);
char *json_next_string(Json *json, char *s);
bool json_next_bool(Json *json);
int json_next_int(Json *json);

int16_t json_get_index(const Json *json);
void json_set_index(Json *json,  int16_t index);
void json_skip_next(Json *json);
