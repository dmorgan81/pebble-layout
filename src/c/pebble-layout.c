#include <pebble.h>
#include "dict.h"
#include "stack.h"
#include "pebble-json.h"
#include "pebble-layout.h"

struct Layout {
    Layer *root;
    Dict *types;
    Dict *ids;
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

static GRect prv_get_frame(Json *json) {
    if (!json_is_object(json)) return GRectZero;

    GRect rect = GRectZero;
    JsonMark *mark = json_mark(json);
    size_t size = json_get_size(json);
    for (size_t i = 0; i < size; i++) {
        char key[8];
        json_next_string(json, key);
        if (strncmp(key, "frame", sizeof(key)) == 0) {
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
                    json_next_string(json, key);
                    if (key[0] == 'x') x = json_next_int(json);
                    else if (key[0] == 'y') y = json_next_int(json);
                    else if (key[0] == 'w') w = json_next_int(json);
                    else if (key[0] == 'h') h = json_next_int(json);
                    else json_skip_tree(json);
                }
            }
            rect = GRect(x, y, w, h);
            break;
        } else {
            json_skip_tree(json);
        }
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
        char key[8];
        json_next_string(json, key);
        if (strncmp(key, "type", sizeof(key)) == 0) {
            char *type = json_next_string(json, NULL);
            if (!dict_contains(types, type)) {
                APP_LOG(APP_LOG_LEVEL_WARNING, "Type %s does not exist. Skipping layer.", type);
            }
            type_data = dict_get(types, type);
            free(type);
            break;
        } else {
            json_skip_tree(json);
        }
    }

    if (type_data == NULL) type_data = dict_get(types, "Layer");

    json_reset(json, mark);
    return type_data;
}

static Layer *prv_create_layer(Layout *layout, Json *json) {
    if (!json_is_object(json)) return NULL;

    struct TypeData *type_data = prv_get_type_data(layout->types, json);
    if (type_data == NULL) return NULL;
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
        char key[8];
        json_next_string(json, key);
        if (strncmp(key, "id", sizeof(key)) == 0) {
            char *id = json_next_string(json, NULL);
            dict_put(layout->ids, id, data->object);
        } else if (strncmp(key, "layers", sizeof(key)) == 0) {
            json_advance(json);
            size_t len = json_get_size(json);
            for (size_t j = 0; j < len; j++) {
                json_advance(json);
                Layer *child = prv_create_layer(layout, json);
                if (child) layer_add_child(layer, child);
            }
        } else if (strncmp(key, "clips", sizeof(key)) == 0) {
            layer_set_clips(layer, json_next_bool(json));
        } else if (strncmp(key, "hidden", sizeof(key)) == 0) {
            layer_set_hidden(layer, json_next_bool(json));
        } else {
            json_skip_tree(json);
        }
    }

    return layer;
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
        } else {
            json_skip_tree(json);
        }
    }
}

Layout *layout_create(void) {
    Layout *layout = malloc(sizeof(Layout));
    layout->root = NULL;
    layout->types = dict_create();
    layout->ids = dict_create();
    layout->layers = stack_create();

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

void layout_destroy(Layout *layout) {
    struct LayerData *layer_data = NULL;
    while ((layer_data = stack_pop(layout->layers)) != NULL) {
        layer_data->type_funcs.destroy(layer_data->object);
        layer_data->object = NULL;
        free(layer_data);
    }

    stack_destroy(layout->layers);
    layout->layers = NULL;

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
