#include <pebble.h>
#include "jsmn/jsmn.h"
#include "pebble-json.h"

#define JSMN_PARENT_LINKS

struct Json {
    char *buf;
    bool free_buf;
    jsmntok_t *tokens;
    int16_t num_tokens;
    int16_t index;
};

Json *json_create_with_resource(uint32_t resource_id) {
    ResHandle handle = resource_get_handle(resource_id);
    size_t size = resource_size(handle);

    char *json = malloc(sizeof(char) * (size + 1));
    memset(json, 0, size + 1);
    resource_load(handle, (uint8_t *) json, size);

    return json_create(json, true);
}

Json *json_create(const char *s, bool free_on_destroy) {
    Json *json = malloc(sizeof(Json));
    json->buf = (char *) s;
    json->free_buf = free_on_destroy;

    jsmn_parser parser;
    jsmn_init(&parser);

    size_t s_len = strlen(s);
    int num_tokens = jsmn_parse(&parser, s, s_len, NULL, 0);
    json->tokens = malloc(sizeof(jsmntok_t) * num_tokens);

    jsmn_init(&parser); // reset parser for actual pass
    json->num_tokens = jsmn_parse(&parser, s, s_len, json->tokens, num_tokens);
    json->index = 0;

    return json;
}

void json_destroy(Json *json) {
    json->index = -1;
    json->num_tokens = -1;

    free(json->tokens);
    json->tokens = NULL;

    if (json->free_buf) free(json->buf);
    json->buf = NULL;

    free(json);
}

bool json_has_next(Json *json) {
    return json->tokens != NULL && json->num_tokens > -1 && json->index < json->num_tokens;
}

static jsmntok_t *prv_json_next(Json *json) {
    return &json->tokens[json->index++];
}

static char * prv_json_string(Json *json, jsmntok_t *tok, char *dest) {
    char *st = dest;
    if (st == NULL) {
        size_t len = (tok->end - tok->start) + 1;
        st = malloc(sizeof(char) * len);
        memset(st, 0, len);
    }
    return strncpy(st, json->buf + tok->start, tok->end - tok->start);
}

char *json_next_string(Json *json, char *dest) {
    jsmntok_t *tok = prv_json_next(json);
    if (tok->type != JSMN_STRING) return NULL;
    return prv_json_string(json, tok, dest);
}

bool json_next_bool(Json *json) {
    jsmntok_t *tok = prv_json_next(json);
    size_t len = tok->end - tok->start;
    return tok->type == JSMN_PRIMITIVE && \
        strlen("true") == len && \
        strncmp(json->buf + tok->start, "true", len) == 0;
}

int json_next_int(Json *json) {
    jsmntok_t *tok = prv_json_next(json);
    size_t len = (tok->end - tok->start) + 1;
    char *s = malloc(sizeof(char) * len);
    s = prv_json_string(json, tok, s);
    int i = s == NULL ? 0 : atoi(s);
    free(s);
    return i;
}

int16_t json_get_index(const Json *json) {
    return json->index;
}

void json_set_index(Json *json, int16_t index) {
    json->index = index;
}

void json_skip_next(Json *json) {
    jsmntok_t *tok = prv_json_next(json);
    if (tok->type == JSMN_ARRAY) {
        int size = tok->size;
        for (int i = 0; i < size; i++) json_skip_next(json);
    } else if (tok-> type == JSMN_OBJECT) {
        int size = tok->size;
        for (int i = 0; i < size; i++) {
            tok = prv_json_next(json);
            json_skip_next(json);
        }
    }
}
