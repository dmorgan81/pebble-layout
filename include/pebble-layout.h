#pragma once
#include <pebble.h>
#include "pebble-json.h"

typedef struct Layout Layout;

typedef void* (*TypeCreateFunc)(GRect frame);
typedef void (*TypeDestroyFunc)(void *object);
typedef void (*TypeParseFunc)(Layout *layout, Json *json, void *object);
typedef Layer* (*TypeGetLayerFunc)(void *object);

typedef struct {
    TypeCreateFunc create;
    TypeDestroyFunc destroy;
    TypeParseFunc parse;
    TypeGetLayerFunc get_layer;
} TypeFuncs;

Layout *layout_create(void);
void layout_parse_resource(Layout *layout, uint32_t resource_id);
void layout_parse(Layout *layout, const char *s);
void layout_destroy(Layout *layout);
void layout_add_type(Layout *layout, const char *type, TypeFuncs type_funcs, const char *parent_type);
Layer *layout_get_layer(Layout *layout);
void *layout_find_by_id(Layout *layout, const char *id);
