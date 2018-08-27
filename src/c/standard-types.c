#include <pebble.h>
#include "standard-types.h"

static void prv_text_layer_destroy(void *object) {
    TextLayer *layer = (TextLayer *) object;
    char *buf = (char *) text_layer_get_text(layer);
    if (buf) free(buf);
    text_layer_destroy(layer);
}

static void prv_text_layer_parse(Layout *layout, Json *json, void *object) {
    TextLayer *layer = (TextLayer *) object;
    text_layer_set_background_color(layer, GColorClear);

    size_t size = json_get_size(json);
    for (size_t i = 0; i < size; i++) {
        char key[32];
        json_next_string(json, key);
        if (strncmp(key, "text", sizeof(key)) == 0) {
            char *text = json_next_string(json, NULL);
            text_layer_set_text(layer, text);
        } else if (strncmp(key, "color", sizeof(key)) == 0) {
            text_layer_set_text_color(layer, json_next_color(json));
        } else if (strncmp(key, "background", sizeof(key)) == 0) {
            text_layer_set_background_color(layer, json_next_color(json));
        } else if (strncmp(key, "alignment", sizeof(key)) == 0) {
            json_next_string(json, key);
            GTextAlignment alignment = GTextAlignmentLeft;
            if (strncmp(key, "GTextAlignmentCenter", sizeof(key)) == 0 ||
                strncmp(key, "center", sizeof(key)) == 0) alignment = GTextAlignmentCenter;
            else if (strncmp(key, "GTextAlignmentRight", sizeof(key)) == 0 ||
                        strncmp(key, "right", sizeof(key)) == 0) alignment = GTextAlignmentRight;
            text_layer_set_text_alignment(layer, alignment);
        }  else if (strncmp(key, "font", sizeof(key)) == 0) {
            json_next_string(json, key);
            GFont font = layout_get_font(layout, key);
            if (font) text_layer_set_font(layer, font);
        } else {
            json_skip_tree(json);
        }
    }
}

void add_standard_types(Layout *layout) {
    layout_add_type(layout, "Layer", (TypeFuncs) {
        .create = (TypeCreateFunc) layer_create,
        .destroy = (TypeDestroyFunc) layer_destroy
    }, NULL);

    layout_add_type(layout, "TextLayer", (TypeFuncs) {
        .create = (TypeCreateFunc) text_layer_create,
        .destroy = prv_text_layer_destroy,
        .parse = prv_text_layer_parse,
        .get_layer = (TypeGetLayerFunc) text_layer_get_layer
    }, NULL);
}
