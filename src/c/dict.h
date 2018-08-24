#pragma once

typedef struct Dict Dict;

typedef bool (*DictForEachCallback)(char *key, void *value, void *context);

Dict *dict_create(void);
void dict_destroy(Dict *dict);
void dict_put(Dict *dict, char *key, void *value);
bool dict_contains(Dict *dict, const char *key);
void *dict_get(Dict *dict, const char *key);
void *dict_remove(Dict *dict, const char *key);
void dict_foreach(Dict *dict, DictForEachCallback callback, void *context);
