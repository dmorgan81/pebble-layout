#include <pebble.h>
#include "dict.h"
#include "stack.h"
#include "standard-types.h"
#include "layout-internals.h"
#include "pebble-json.h"
#include "pebble-layout.h"

#define eq(s, t) (strncmp(s, t, strlen(s)) == 0)

struct Layout {
    Layer *root;
    Dict *types;
    Dict *ids;
    Dict *fonts;
    Dict *resource_ids;
    Stack *layers;
};

struct LayerData {
    TypeFuncs type_funcs;
    void *object;
};

struct TypeData {
    TypeFuncs type_funcs;
    const char *parent_type;
};

struct FontInfo {
    GFont font;
    bool system;
};

static struct TypeData NO_TYPE_SENTINAL;

static GRect prv_get_frame(Json *json) {
    if (!json_is_object(json)) return GRectZero;

    GRect rect = GRectZero;
    JsonMark *mark = json_mark(json);
    size_t size = json_get_size(json);
    for (size_t i = 0; i < size; i++) {
        char *key = json_next_string(json);
        if (eq(key, "frame")) {
            json_advance(json);
            int x = 0, y = 0, w = 0, h = 0;
            if (json_is_array(json)) {
                x = json_next_int(json);
                y = json_next_int(json);
                w = json_next_int(json);
                h = json_next_int(json);
            } else if (json_is_object(json)) {
                size_t len = json_get_size(json);
                for (size_t j = 0; j < len; j++) {
                    char *value = json_next_string(json);
                    if (value[0] == 'x') x = json_next_int(json);
                    else if (value[0] == 'y') y = json_next_int(json);
                    else if (value[0] == 'w') w = json_next_int(json);
                    else if (value[0] == 'h') h = json_next_int(json);
                    else json_skip_tree(json);
                    free(value);
                }
            }
            rect = GRect(x, y, w, h);
            break;
        } else {
            json_skip_tree(json);
        }
        free(key);
    }

    json_reset(json, mark);
    return rect;
}

static struct TypeData *prv_get_type_data(Dict *types, Json *json) {
    if (!json_is_object(json)) return NULL;

    struct TypeData *type_data = NULL;
    JsonMark *mark = json_mark(json);
    size_t size = json_get_size(json);
    for (size_t i = 0; i < size; i++) {
        char *key = json_next_string(json);
        if (eq(key, "type")) {
            char *type = json_next_string(json);
            if (!dict_contains(types, type)) {
                APP_LOG(APP_LOG_LEVEL_WARNING, "Type %s does not exist. Skipping layer.", type);
                type_data = &NO_TYPE_SENTINAL;
            } else {
                type_data = dict_get(types, type);
            }
            free(type);
        } else {
            json_skip_tree(json);
        }
        free(key);
    }

    if (type_data == NULL) type_data = dict_get(types, "Layer");

    json_reset(json, mark);
    return type_data;
}

static Layer *prv_create_layer(Layout *layout, Json *json) {
    if (!json_is_object(json)) return NULL;

    struct TypeData *type_data = prv_get_type_data(layout->types, json);
    if (type_data == &NO_TYPE_SENTINAL) return NULL;
    TypeFuncs type_funcs = type_data->type_funcs;

    struct LayerData *data = malloc(sizeof(struct LayerData));
    stack_push(layout->layers, data);
    data->type_funcs = type_funcs;

    GRect frame = prv_get_frame(json);
    data->object = type_funcs.create(frame);

    if (type_data->parent_type) {
        JsonMark *mark = json_mark(json);
        struct TypeData *parent_type = dict_get(layout->types, type_data->parent_type);
        if (parent_type) {
            parent_type->type_funcs.parse(layout, json, type_data->type_funcs.cast(data->object));
        }
        json_reset(json, mark);
    }

    if (type_funcs.parse) {
        JsonMark *mark = json_mark(json);
        type_funcs.parse(layout, json, data->object);
        json_reset(json, mark);
    }

    Layer *layer = NULL;
    if (type_funcs.get_layer) layer = type_funcs.get_layer(data->object);
    else layer = (Layer *) data->object; // The object is a Layer

    size_t size = json_get_size(json);
    for (size_t i = 0; i < size; i++) {
        char *key = json_next_string(json);
        if (eq(key, "id")) {
            char *id = json_next_string(json);
            dict_put(layout->ids, id, data->object);
        } else if (eq(key, "layers")) {
            json_advance(json);
            size_t len = json_get_size(json);
            for (size_t j = 0; j < len; j++) {
                json_advance(json);
                Layer *child = prv_create_layer(layout, json);
                if (child) layer_add_child(layer, child);
            }
        } else if (eq(key, "clips")) {
            layer_set_clips(layer, json_next_bool(json));
        } else if (eq(key, "hidden")) {
            layer_set_hidden(layer, json_next_bool(json));
        } else {
            json_skip_tree(json);
        }
        free(key);
    }

    return layer;
}

static void prv_add_system_font(Layout *layout, char *name, const char *font_key) {
    FontInfo *font_info = malloc(sizeof(FontInfo));
    font_info->font = fonts_get_system_font(font_key);
    font_info->system = true;
    dict_put(layout->fonts, name, font_info);
}

Layout *layout_create(void) {
    Layout *layout = malloc(sizeof(Layout));
    layout->root = NULL;
    layout->types = dict_create();
    layout->ids = dict_create();
    layout->fonts = dict_create();
    layout->resource_ids = dict_create();
    layout->layers = stack_create();

    add_standard_types(layout);

    prv_add_system_font(layout, "GOTHIC_18_BOLD", FONT_KEY_GOTHIC_18_BOLD);
    prv_add_system_font(layout, "GOTHIC_24", FONT_KEY_GOTHIC_24);
    prv_add_system_font(layout, "GOTHIC_09", FONT_KEY_GOTHIC_09);
    prv_add_system_font(layout, "GOTHIC_14", FONT_KEY_GOTHIC_14);
    prv_add_system_font(layout, "GOTHIC_14_BOLD", FONT_KEY_GOTHIC_14_BOLD);
    prv_add_system_font(layout, "GOTHIC_18", FONT_KEY_GOTHIC_18);
    prv_add_system_font(layout, "GOTHIC_24_BOLD", FONT_KEY_GOTHIC_24_BOLD);
    prv_add_system_font(layout, "GOTHIC_28", FONT_KEY_GOTHIC_28);
    prv_add_system_font(layout, "GOTHIC_28_BOLD", FONT_KEY_GOTHIC_28_BOLD);
    prv_add_system_font(layout, "BITHAM_30_BLACK", FONT_KEY_BITHAM_30_BLACK);
    prv_add_system_font(layout, "BITHAM_42_BOLD", FONT_KEY_BITHAM_42_BOLD);
    prv_add_system_font(layout, "BITHAM_42_LIGHT", FONT_KEY_BITHAM_42_LIGHT);
    prv_add_system_font(layout, "BITHAM_42_MEDIUM_NUMBERS", FONT_KEY_BITHAM_42_MEDIUM_NUMBERS);
    prv_add_system_font(layout, "BITHAM_34_MEDIUM_NUMBERS", FONT_KEY_BITHAM_34_MEDIUM_NUMBERS);
    prv_add_system_font(layout, "BITHAM_34_LIGHT_SUBSET", FONT_KEY_BITHAM_34_LIGHT_SUBSET);
    prv_add_system_font(layout, "BITHAM_18_LIGHT_SUBSET", FONT_KEY_BITHAM_18_LIGHT_SUBSET);
    prv_add_system_font(layout, "ROBOTO_CONDENSED_21", FONT_KEY_ROBOTO_CONDENSED_21);
    prv_add_system_font(layout, "ROBOTO_BOLD_SUBSET_49", FONT_KEY_ROBOTO_BOLD_SUBSET_49);
    prv_add_system_font(layout, "DROID_SERIF_28_BOLD", FONT_KEY_DROID_SERIF_28_BOLD);
    prv_add_system_font(layout, "LECO_20_BOLD_NUMBERS", FONT_KEY_LECO_20_BOLD_NUMBERS);
    prv_add_system_font(layout, "LECO_26_BOLD_NUMBERS_AM_PM", FONT_KEY_LECO_26_BOLD_NUMBERS_AM_PM);
    prv_add_system_font(layout, "LECO_32_BOLD_NUMBERS", FONT_KEY_LECO_32_BOLD_NUMBERS);
    prv_add_system_font(layout, "LECO_36_BOLD_NUMBERS", FONT_KEY_LECO_36_BOLD_NUMBERS);
    prv_add_system_font(layout, "LECO_38_BOLD_NUMBERS", FONT_KEY_LECO_38_BOLD_NUMBERS);
    prv_add_system_font(layout, "LECO_42_NUMBERS", FONT_KEY_LECO_42_NUMBERS);
    prv_add_system_font(layout, "LECO_28_LIGHT_NUMBERS", FONT_KEY_LECO_28_LIGHT_NUMBERS);

    return layout;
}

static void prv_parse(Layout *layout, Json *json) {
    if (json_has_next(json) && json_is_object(json)) {
        layout->root = prv_create_layer(layout, json);
        GRect frame = layer_get_frame(layout->root);
        if (grect_equal(&frame, &GRectZero)) {
            layer_set_frame(layout->root, GRect(0, 0, PBL_DISPLAY_WIDTH, PBL_DISPLAY_HEIGHT));
        }
    } else {
        APP_LOG(APP_LOG_LEVEL_ERROR, "layout is not valid");
    }
}

void layout_parse_resource(Layout *layout, uint32_t resource_id) {
    Json *json = json_create_with_resource(resource_id);
    prv_parse(layout, json);
    json_destroy(json);
}

void layout_parse(Layout *layout, const char *s) {
    Json *json = json_create(s, false);
    prv_parse(layout, json);
    json_destroy(json);
}

static bool prv_key_destroy_callback(char *key, void *value, void *context) {
    free(key);
    key = NULL;
    return true;
}

static bool prv_value_destroy_callback(char *key, void *value, void *context) {
    free(value);
    value = NULL;
    return true;
}

static bool prv_fonts_destroy_callback(char *key, void *value, void *context) {
    FontInfo *font_info = (FontInfo *) value;
    if (!font_info->system) fonts_unload_custom_font(font_info->font);
    font_info->font = NULL;
    free(font_info);
    return true;
}

void layout_destroy(Layout *layout) {
    struct LayerData *layer_data = NULL;
    while ((layer_data = stack_pop(layout->layers)) != NULL) {
        layer_data->type_funcs.destroy(layer_data->object);
        layer_data->object = NULL;
        free(layer_data);
    }

    stack_destroy(layout->layers);
    layout->layers = NULL;

    dict_foreach(layout->resource_ids, prv_value_destroy_callback, NULL);
    dict_destroy(layout->resource_ids);
    layout->resource_ids = NULL;

    dict_foreach(layout->fonts, prv_fonts_destroy_callback, NULL);
    dict_destroy(layout->fonts);
    layout->fonts = NULL;

    dict_foreach(layout->ids, prv_key_destroy_callback, NULL);
    dict_destroy(layout->ids);
    layout->ids = NULL;

    dict_foreach(layout->types, prv_value_destroy_callback, NULL);
    dict_destroy(layout->types);
    layout->types = NULL;

    free(layout);
}

void layout_add_type(Layout *layout, const char *type, TypeFuncs type_funcs, const char *parent_type) {
    struct TypeData *data = malloc(sizeof(struct TypeData));
    memcpy(&data->type_funcs, &type_funcs, sizeof(TypeFuncs));
    data->parent_type = parent_type;
    dict_put(layout->types, (char *) type, data);
}

Layer *layout_get_layer(Layout *layout) {
    return layout->root;
}

void *layout_find_by_id(Layout *layout, const char *id) {
    return dict_get(layout->ids, id);
}

void layout_add_font(Layout *layout, char *name, uint32_t resource_id) {
    FontInfo *font_info = malloc(sizeof(FontInfo));
    font_info->font = fonts_load_custom_font(resource_get_handle(resource_id));
    font_info->system = false;
    dict_put(layout->fonts, name, font_info);
}

GFont layout_get_font(Layout *layout, const char *name) {
    FontInfo *font_info = dict_get(layout->fonts, name);
    return font_info ? font_info->font : NULL;
}

void layout_add_resource(Layout *layout, char *name, uint32_t resource_id) {
    uint32_t *rid = malloc(sizeof(uint32_t));
    memcpy(rid, &resource_id, sizeof(uint32_t));
    dict_put(layout->resource_ids, name, rid);
}

uint32_t *layout_get_resource(Layout *layout, const char *name) {
    return dict_get(layout->resource_ids, name);
}
