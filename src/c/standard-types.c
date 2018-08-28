#include <pebble.h>
#include "layout-internals.h"
#include "standard-types.h"

#define eq(s, t) (strncmp(s, t, sizeof(s)) == 0)

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
        if (eq(key, "text")) {
            char *text = json_next_string(json, NULL);
            text_layer_set_text(layer, text);
        } else if (eq(key, "color")) {
            text_layer_set_text_color(layer, json_next_color(json));
        } else if (eq(key, "background")) {
            text_layer_set_background_color(layer, json_next_color(json));
        } else if (eq(key, "alignment")) {
            json_next_string(json, key);
            GTextAlignment alignment = GTextAlignmentLeft;
            if (eq(key, "GTextAlignmentCenter") || eq(key, "center")) alignment = GTextAlignmentCenter;
            else if (eq(key, "GTextAlignmentRight") || eq(key, "right")) alignment = GTextAlignmentRight;
            text_layer_set_text_alignment(layer, alignment);
        }  else if (eq(key, "font")) {
            json_next_string(json, key);
            GFont font = layout_get_font(layout, key);
            if (font) text_layer_set_font(layer, font);
        } else {
            json_skip_tree(json);
        }
    }
}

static void prv_bitmap_layer_parse(Layout *layout, Json *json, void *object) {
    BitmapLayer *layer = (BitmapLayer *) object;
    bitmap_layer_set_background_color(layer, GColorClear);

    size_t size = json_get_size(json);
    for (size_t i = 0; i < size; i++) {
        char key[32];
        json_next_string(json, key);
        if (eq(key, "bitmap")) {
            char *s = json_next_string(json, NULL);
            uint32_t *resource_id = layout_get_resource(layout, s);
            free(s);
            if (resource_id) {
                GBitmap *bitmap = gbitmap_create_with_resource(*resource_id);
                bitmap_layer_set_bitmap(layer, bitmap);
            }
        } else if (eq(key, "background"))  {
            GColor color = json_next_color(json);
            bitmap_layer_set_background_color(layer, color);
        }  else if (eq(key, "alignment")) {
            json_next_string(json, key);
            GAlign alignment = GAlignCenter;
            if (eq(key, "top-left") || eq(key, "GAlignTopLeft")) alignment = GAlignTopLeft;
            else if (eq(key, "top") || eq(key, "GAlignTop")) alignment = GAlignTop;
            else if (eq(key, "top-right") || eq(key, "GAlignTopRight")) alignment = GAlignTopRight;
            else if (eq(key, "left") || eq(key, "GAlignLeft")) alignment = GAlignLeft;
            else if (eq(key, "right") || eq(key, "GAlignRight")) alignment = GAlignRight;
            else if (eq(key, "bottom-left") || eq(key, "GAlignBottomLeft")) alignment = GAlignBottomLeft;
            else if (eq(key, "bottom") || eq(key, "GAlignBottom")) alignment = GAlignBottom;
            else if (eq(key, "bottom-right") || eq(key, "GAlignBottomRight")) alignment = GAlignBottomRight;
            bitmap_layer_set_alignment(layer, alignment);
        }  else if (eq(key, "compositing")) {
            json_next_string(json, key);
            GCompOp compositing = GCompOpAssign;
            if (eq(key, "inverted") || eq(key, "GCompOpAssignInverted")) compositing = GCompOpAssignInverted;
            else if(eq(key, "or") || eq(key, "GCompOpOr")) compositing = GCompOpOr;
            else if(eq(key, "and") || eq(key, "GCompOpAnd")) compositing = GCompOpAnd;
            else if(eq(key, "clear") || eq(key, "GCompOpClear")) compositing = GCompOpClear;
            else if(eq(key, "set") || eq(key, "GCompOpSet")) compositing = GCompOpSet;
            bitmap_layer_set_compositing_mode(layer, compositing);
        } else {
            json_skip_tree(json);
        }
    }
}

static void prv_bitmap_layer_destroy(void *object) {
    BitmapLayer *layer = (BitmapLayer *) object;
    GBitmap *bitmap = (GBitmap *) bitmap_layer_get_bitmap(layer);
    bitmap_layer_set_bitmap(layer, NULL);
    if (bitmap) gbitmap_destroy(bitmap);
    bitmap_layer_destroy(layer);
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

    layout_add_type(layout, "BitmapLayer", (TypeFuncs) {
        .create = (TypeCreateFunc) bitmap_layer_create,
        .destroy = prv_bitmap_layer_destroy,
        .parse = prv_bitmap_layer_parse,
        .get_layer = (TypeGetLayerFunc) bitmap_layer_get_layer
    }, NULL);
}
