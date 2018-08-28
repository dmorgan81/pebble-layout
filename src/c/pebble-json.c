#include <ctype.h>
#include <limits.h>
#include <pebble.h>
#include <@smallstoneapps/linked-list/linked-list.h>
#include "jsmn/jsmn.h"
#include "pebble-json.h"

#define JSMN_PARENT_LINKS

struct Json {
    char *buf;
    bool free_buf;
    LinkedRoot *marks;
    jsmntok_t *tokens;
    int16_t num_tokens;
    int16_t index;
};

struct JsonMark {
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

static bool prv_marks_destroy(void *object, void *context) {
    free(object);
    return true;
}

void json_destroy(Json *json) {
    json->index = -1;
    json->num_tokens = -1;

    free(json->tokens);
    json->tokens = NULL;

    if (json->marks) {
        linked_list_foreach(json->marks, prv_marks_destroy, NULL);
        linked_list_clear(json->marks);
        free(json->marks);
        json->marks = NULL;
    }

    if (json->free_buf) free(json->buf);
    json->buf = NULL;

    free(json);
}

bool json_is_string(Json *json) {
    return json->tokens[json->index].type == JSMN_STRING;
}

bool json_is_primitive(Json *json) {
    return json->tokens[json->index].type == JSMN_PRIMITIVE;
}

bool json_is_array(Json *json) {
    return json->tokens[json->index].type == JSMN_ARRAY;
}

bool json_is_object(Json *json) {
    return json->tokens[json->index].type == JSMN_OBJECT;
}

bool json_has_next(Json *json) {
    return json->tokens != NULL && json->num_tokens > -1 && json->index < json->num_tokens;
}

static jsmntok_t *prv_json_next(Json *json) {
    return &json->tokens[++json->index];
}

char *json_next_string(Json *json) {
    jsmntok_t *tok = prv_json_next(json);
    if (tok->type != JSMN_STRING) return NULL;
    size_t len = tok->end - tok->start;
    char *s = malloc(sizeof(char) * (len + 1));
    memset(s, 0, len + 1);
    return strncpy(s, json->buf + tok->start, len);
}

bool json_next_bool(Json *json) {
    jsmntok_t *tok = prv_json_next(json);
    size_t len = tok->end - tok->start;
    return tok->type == JSMN_PRIMITIVE && \
        strlen("true") == len && \
        strncmp(json->buf + tok->start, "true", len) == 0;
}

int json_next_int(Json *json) {
    char *s = json_next_string(json);;
    int i = s == NULL ? 0 : atoi(s);
    free(s);
    return i;
}

static unsigned long
 prv_strtoul(const char *nptr, char **endptr, int base)
 {
     const char *s;
     unsigned long acc, cutoff;
     int c;
     int neg, any, cutlim;
     s = nptr;
     do {
         c = (unsigned char) *s++;
     } while (isspace(c));
     if (c == '-') {
         neg = 1;
         c = *s++;
     } else {
         neg = 0;
         if (c == '+')
             c = *s++;
     }
     if ((base == 0 || base == 16) &&
         c == '0' && (*s == 'x' || *s == 'X')) {
         c = s[1];
         s += 2;
         base = 16;
     }
     if (base == 0)
         base = c == '0' ? 8 : 10;
     cutoff = ULONG_MAX / (unsigned long)base;
     cutlim = ULONG_MAX % (unsigned long)base;
     for (acc = 0, any = 0;; c = (unsigned char) *s++) {
         if (isdigit(c))
             c -= '0';
         else if (isalpha(c))
             c -= isupper(c) ? 'A' - 10 : 'a' - 10;
         else
             break;
         if (c >= base)
             break;
         if (any < 0)
             continue;
         if (acc > cutoff || (acc == cutoff && c > cutlim)) {
             any = -1;
             acc = ULONG_MAX;
         } else {
             any = 1;
             acc *= (unsigned long)base;
             acc += c;
         }
     }
     if (neg && any > 0)
         acc = -acc;
     if (endptr != 0)
         *endptr = (char *) (any ? s - 1 : nptr);
     return (acc);
 }

GColor json_next_color(Json *json) {
    char *s = json_next_string(json);
    GColor color = GColorFromHEX(prv_strtoul(s + (s[0] == '#' ? 1 : 0), NULL, 16));
    free(s);
    return color;
}

size_t json_get_size(Json *json) {
    return json->tokens[json->index].size;
}

JsonMark *json_mark(Json *json) {
    if (!json->marks) json->marks = linked_list_create_root();
    JsonMark *mark = malloc(sizeof(JsonMark));
    mark->index = json->index;
    linked_list_prepend(json->marks, mark);
    return mark;
}

void json_reset(Json *json, JsonMark *mark) {
    if (!json->marks) return;
    int16_t index = linked_list_find(json->marks, mark);
    if (index > -1) {
        json->index = mark->index;
        linked_list_remove(json->marks, index);
        free(mark);

        if (linked_list_count(json->marks) == 0) {
            free(json->marks);
            json->marks = NULL;
        }
    }
}

void json_advance(Json *json) {
    json->index += 1;
}

void json_skip_tree(Json *json) {
    jsmntok_t *tok = prv_json_next(json);
    if (tok->type == JSMN_ARRAY) {
        int size = tok->size;
        for (int i = 0; i < size; i++) json_skip_tree(json);
    } else if (tok-> type == JSMN_OBJECT) {
        int size = tok->size;
        for (int i = 0; i < size; i++) {
            tok = prv_json_next(json);
            json_skip_tree(json);
        }
    }
}
