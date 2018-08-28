#include <pebble.h>
#include "layout-internals.h"
#include "standard-types.h"

#define eq(s, t) (strncmp(s, t, strlen(s)) == 0)

struct DefaultLayerData {
    GColor color;
};

static void prv_default_update_proc(Layer *layer, GContext *ctx) {
    struct DefaultLayerData *data = layer_get_data(layer);
    if (!gcolor_equal(data->color, GColorClear)) {
        graphics_context_set_fill_color(ctx, data->color);
        graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornerNone);
    }
}

static void *prv_default_layer_create(GRect frame) {
    Layer *layer = layer_create_with_data(frame, sizeof(struct DefaultLayerData));
    layer_set_update_proc(layer, prv_default_update_proc);
    struct DefaultLayerData *data = layer_get_data(layer);
    data->color = GColorClear;
    return layer;
}

static void prv_default_layer_parse(Layout *layout, Json *json, void *object) {
    Layer *layer = (Layer *) object;
    struct DefaultLayerData *data = layer_get_data(layer);

    size_t size = json_get_size(json);
    for (size_t i = 0; i < size; i++) {
        char *key = json_next_string(json);
        if (eq(key, "background")) {
            data->color = json_next_color(json);
        } else {
            json_skip_tree(json);
        }
        free(key);
    }
}

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
        char *key = json_next_string(json);
        if (eq(key, "text")) {
            char *text = json_next_string(json);
            text_layer_set_text(layer, text);
        } else if (eq(key, "color")) {
            text_layer_set_text_color(layer, json_next_color(json));
        } else if (eq(key, "background")) {
            text_layer_set_background_color(layer, json_next_color(json));
        } else if (eq(key, "alignment")) {
            char *value = json_next_string(json);
            GTextAlignment alignment = GTextAlignmentLeft;
            if (eq(value, "GTextAlignmentCenter") || eq(value, "center")) alignment = GTextAlignmentCenter;
            else if (eq(value, "GTextAlignmentRight") || eq(value, "right")) alignment = GTextAlignmentRight;
            text_layer_set_text_alignment(layer, alignment);
            free(value);
        }  else if (eq(key, "font")) {
            char *value = json_next_string(json);
            GFont font = layout_get_font(layout, value);
            if (font) text_layer_set_font(layer, font);
            free(value);
        } else {
            json_skip_tree(json);
        }
        free(key);
    }
}

static void prv_bitmap_layer_parse(Layout *layout, Json *json, void *object) {
    BitmapLayer *layer = (BitmapLayer *) object;
    bitmap_layer_set_background_color(layer, GColorClear);

    size_t size = json_get_size(json);
    for (size_t i = 0; i < size; i++) {
        char *key = json_next_string(json);
        if (eq(key, "bitmap")) {
            char *value = json_next_string(json);
            uint32_t *resource_id = layout_get_resource(layout, value);
            if (resource_id) {
                GBitmap *bitmap = gbitmap_create_with_resource(*resource_id);
                bitmap_layer_set_bitmap(layer, bitmap);
            }
            free(value);
        } else if (eq(key, "background"))  {
            GColor color = json_next_color(json);
            bitmap_layer_set_background_color(layer, color);
        }  else if (eq(key, "alignment")) {
            char *value = json_next_string(json);
            GAlign alignment = GAlignCenter;
            if (eq(value, "top-left") || eq(value, "GAlignTopLeft")) alignment = GAlignTopLeft;
            else if (eq(value, "top") || eq(value, "GAlignTop")) alignment = GAlignTop;
            else if (eq(value, "top-right") || eq(value, "GAlignTopRight")) alignment = GAlignTopRight;
            else if (eq(value, "left") || eq(value, "GAlignLeft")) alignment = GAlignLeft;
            else if (eq(value, "right") || eq(value, "GAlignRight")) alignment = GAlignRight;
            else if (eq(value, "bottom-left") || eq(value, "GAlignBottomLeft")) alignment = GAlignBottomLeft;
            else if (eq(value, "bottom") || eq(value, "GAlignBottom")) alignment = GAlignBottom;
            else if (eq(value, "bottom-right") || eq(value, "GAlignBottomRight")) alignment = GAlignBottomRight;
            bitmap_layer_set_alignment(layer, alignment);
            free(value);
        }  else if (eq(key, "compositing")) {
            char *value = json_next_string(json);
            GCompOp compositing = GCompOpAssign;
            if (eq(value, "inverted") || eq(value, "GCompOpAssignInverted")) compositing = GCompOpAssignInverted;
            else if(eq(value, "or") || eq(value, "GCompOpOr")) compositing = GCompOpOr;
            else if(eq(value, "and") || eq(value, "GCompOpAnd")) compositing = GCompOpAnd;
            else if(eq(value, "clear") || eq(value, "GCompOpClear")) compositing = GCompOpClear;
            else if(eq(value, "set") || eq(value, "GCompOpSet")) compositing = GCompOpSet;
            bitmap_layer_set_compositing_mode(layer, compositing);
            free(value);
        } else {
            json_skip_tree(json);
        }
        free(key);
    }
}

static void prv_bitmap_layer_destroy(void *object) {
    BitmapLayer *layer = (BitmapLayer *) object;
    GBitmap *bitmap = (GBitmap *) bitmap_layer_get_bitmap(layer);
    bitmap_layer_set_bitmap(layer, NULL);
    if (bitmap) gbitmap_destroy(bitmap);
    bitmap_layer_destroy(layer);
}

static void *prv_status_bar_layer_create(GRect frame) {
    StatusBarLayer *layer = status_bar_layer_create();
    if (!grect_equal(&frame, &GRectZero)) layer_set_frame(status_bar_layer_get_layer(layer), frame);
    return layer;
}

static void prv_status_bar_layer_parse(Layout *layout, Json *json, void *object) {
    StatusBarLayer *layer = (StatusBarLayer *) object;

    GColor background = status_bar_layer_get_background_color(layer);
    GColor foreground = status_bar_layer_get_foreground_color(layer);

    size_t size = json_get_size(json);
    for (size_t i = 0; i < size; i++) {
        char *key = json_next_string(json);
        if (eq(key, "background")) {
            background = json_next_color(json);
        } else if (eq(key, "foreground")) {
            foreground = json_next_color(json);
        }  else if (eq(key, "separator")) {
            char *value = json_next_string(json);
            StatusBarLayerSeparatorMode mode = StatusBarLayerSeparatorModeNone;
            if (eq(value, "dotted") || eq(value, "StatusBarLayerSeparatorModeDotted")) {
                mode = StatusBarLayerSeparatorModeDotted;
            }
            status_bar_layer_set_separator_mode(layer, mode);
            free(value);
        } else {
            json_skip_tree(json);
        }
        free(key);
    }

    status_bar_layer_set_colors(layer, background, foreground);
}

void add_standard_types(Layout *layout) {
    layout_add_type(layout, "Layer", (TypeFuncs) {
        .create = prv_default_layer_create,
        .destroy = (TypeDestroyFunc) layer_destroy,
        .parse = prv_default_layer_parse
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

    layout_add_type(layout, "StatusBarLayer", (TypeFuncs) {
        .create = prv_status_bar_layer_create,
        .destroy = (TypeDestroyFunc) status_bar_layer_destroy,
        .parse = prv_status_bar_layer_parse,
        .get_layer = (TypeGetLayerFunc) status_bar_layer_get_layer
    }, NULL);
}
