#pragma once
#include <pebble-layout.h>

GFont layout_get_font(Layout *layout, const char *name);
uint32_t *layout_get_resource(Layout *layout, const char *name);