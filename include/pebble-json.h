#pragma once
#include <pebble.h>

typedef struct Json Json;
typedef struct JsonMark JsonMark;

Json *json_create_with_resource(uint32_t resource_id);
Json *json_create(const char *s, bool free_on_destroy);
void json_destroy(Json *json);

bool json_is_string(Json *json);
bool json_is_primitive(Json *json);
bool json_is_array(Json *json);
bool json_is_object(Json *json);

bool json_has_next(Json *json);
char *json_next_string(Json *json);
bool json_next_bool(Json *json);
int json_next_int(Json *json);
GColor json_next_color(Json *json);

size_t json_get_size(Json *json);

JsonMark *json_mark(Json *json);
void json_reset(Json *json, JsonMark *mark);
void json_advance(Json *json);
void json_skip_tree(Json *json);
