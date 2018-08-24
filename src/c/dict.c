#include <@smallstoneapps/linked-list/linked-list.h>
#include "dict.h"

struct Dict {
    LinkedRoot *entries;
};

struct Entry {
    char *key;
    void *value;
};

Dict *dict_create(void) {
    Dict *dict = malloc(sizeof(Dict));
    dict->entries = NULL;
    return dict;
}

static bool prv_destroy_callback(void *object, void *context) {
    free(object);
    object = NULL;
    return true;
}

void dict_destroy(Dict *dict) {
    if (dict->entries) {
        linked_list_foreach(dict->entries, prv_destroy_callback, NULL);
        linked_list_clear(dict->entries);
        free(dict->entries);
        dict->entries = NULL;
    }
    free(dict);
}

void dict_put(Dict *dict, char *key, void *value) {
    if (!dict->entries) dict->entries = linked_list_create_root();
    struct Entry *entry = malloc(sizeof(struct Entry));
    entry->key = key;
    entry->value = value;
    linked_list_append(dict->entries, entry);
}

static bool prv_find_by_key_callback(void *object1, void *object2) {
    const char *key = (const char *) object1;
    struct Entry *entry = (struct Entry *) object2;
    return strcmp(key, entry->key) == 0;
}

bool dict_contains(Dict *dict, const char *key) {
    if (!dict->entries) return false;
    return linked_list_contains_compare(dict->entries, (void *) key, prv_find_by_key_callback);
}

void *dict_get(Dict *dict, const char *key) {
    if (!dict->entries) return NULL;
    int16_t index = linked_list_find_compare(dict->entries, (void *) key, prv_find_by_key_callback);
    if (index < 0) return NULL;
    struct Entry *entry = (struct Entry *) linked_list_get(dict->entries, index);
    return entry->value;
}

void *dict_remove(Dict *dict, const char *key) {
    if (!dict->entries) return NULL;
    int16_t index = linked_list_find_compare(dict->entries, (void *) key, prv_find_by_key_callback);
    if (index < 0) return NULL;

    struct Entry *entry = (struct Entry *) linked_list_get(dict->entries, index);
    void *value = entry->value;
    linked_list_remove(dict->entries, index);
    free(entry);

    if (linked_list_count(dict->entries) == 0) {
        free(dict->entries);
        dict->entries = NULL;
    }

    return value;
}

struct ForEachData {
    DictForEachCallback callback;
    void *context;
};

static bool prv_foreach_callback(void *object, void *context) {
    struct Entry *entry = (struct Entry *) object;
    struct ForEachData *data = (struct ForEachData *) context;
    return data->callback(entry->key, entry->value, data->context);
}

void dict_foreach(Dict *dict, DictForEachCallback callback, void *context) {
    if (!dict->entries) return;
    struct ForEachData data = {
        .callback = callback,
        .context = context
    };
    linked_list_foreach(dict->entries, prv_foreach_callback, &data);
}
