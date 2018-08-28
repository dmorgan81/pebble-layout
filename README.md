# pebble-layout

pebble-layout provides a separation between your code and your view. Writing and maintaining a UI layout in code is tedious; pebble-layout moves all that layout code into a familiar JSON format. Standard layer types like TextLayer, BitmapLayer, and StatusBarLayer are provided. Custom types can be added to extend pebble-layout's functionality.

# Installation

```
pebble package install pebble-layout
```

You must be using a new-style project; install the latest pebble tool and SDK and run `pebble convert-project` on your app if you aren't.

# Usage

```c
// This is not a complete example, but should demonstrate the basic usage

#include <pebble-layout/pebble-layout.h>

...
static Layout *s_layout;
...

static void prv_window_load(Window *window) {
    s_layout = layout_create();
    layout_parse(s_layout, RESOURCE_ID_LAYOUT);
    layer_add_child(window_get_root_layer(window), layout_get_layer(s_layout));
    ...
}

static void prv_window_unload(Window *window) {
  layout_destroy(s_layout);
  ...
}
```

# JSON format

The format of a layout file is pretty simple. It's just standard JSON. For example:

```json
{
    "id": "root",
    "background": "#000000",
    "layers": [
        {
            "frame": [20, 10, 124, 128],
            "background": "#FF0000",
            "layers": [
                {
                    "id": "hello",
                    "type": "TextLayer",
                    "frame": [0, 0, 124, 128],
                    "text": "Hello World!",
                    "color": "#FFFFFF",
                    "alignment": "center",
                    "font": "GOTHIC_28_BOLD"
                }
            ]
        },
        {
            "frame": [0, 120, 144, 40],
            "background": "#00FF00"
        }
    ] 
}
```

The layout above will be two colored rectangles of different sizes with the text "Hello World!" centered along the top of the first rectangle. Two layers have IDs; we could use `layout_find_by_id()` to get a pointer to these layers if we needed to.

The only property that is required is `frame`. Without it a layer's frame defaults to GRectZero, which isn't useful since it won't draw anything. Layers can have IDs, as shown.

Untyped layers default to basic layers. An untyped layer can have child layers (the `layers` property). a background color which defaults to GColorClear if not specified, a `clips` boolean property which acts just like `layer_set_clips()`, and a `hidden` boolean property which will hide the layer if true.

TextLayers can have the following properties:

| Property | Pebble API equivalent |
|----------|-----------------------|
| text | `text_layer_set_text()` |
| background | `text_layer_set_background_color()` |
| color | `text_layer_set_text_color()` |
| alignment | `text_layer_set_text_alignment()` |
| overflow | `text_layer_set_overflow_mode()` |

Anything that takes an enum value takes the value as a string, like GTextAlignmentCenter or GTextOverflowModeFill.

BitmapLayers can have the following properties:

| Property | Pebble API equivalent | Notes |
|----------|-----------------------|-------|
| bitmap | `bitmap_layer_set_bitmap()` | Should be a name that was registered with `layout_add_resource()`
| background | `bitmap_layer_set_background_color()` |
| alignment | `bitmap_layer_set_alignment()` |
| compositing | `bitmap_layer_set_compositing_mode()` |

# pebble-layout API

| Method | Description |
|--------|---------|
| `Layout *layout_create(void)` | Create and initialize a Layout. No parsing has been done at this point.|
| `void layout_parse_resource(Layout *layout, uint32_t resource_id)` | Parse a JSON resource into a tree of layers.|
| `void layout_parse(Layout *layout, char *json)` | Parse a JSON string into a tree of layers.|
| `void layout_destroy(Layout *layout)` | Destroy a layout, including all parsed layers.|
| `Layer *layout_get_layer(Layout *layout)` | Get the root layer of the layout. If parsing has not been done or parsing failed, this returns `NULL`.|
| `void *layout_find_by_id(Layout *layout, char *id)` | Return a layer by its ID. The caller is responsible for casting to the correct type. If no layer exists with that ID, `NULL` is returned. |
| `void layout_add_font(Layout *layout, char *name, uint32_t resource_id)` | Add a custom font that can referenced during parsing. The font will be loaded and unloaded automatically. Calling this function after parsing will have no effect.|
| `void layout_add_resource(Layout *layout, char *name, uint32_t resource_id)` | Add a resource by its ID that can be referenced during parsing. Calling this function after parsing will have no effect.|
| `void layout_add_type(Layout *layout, char *type, TypeFuncs type_funcs, const char *parent_type)` | Add a custom type that can be used during parsing. See the section below on [custom types](#custom-types).|

# Custom types

pebble-layout can be extended by adding custom types before parsing. During parsing any layer with its `type` property set to a string you specify will be constructed/destroyed using the functions you specify.

Adding a type requires implementing or aliases some functions:
* `create`: `void* (GRect frame)` - Anything can be returned from this function. The result will be passed around to the other custom type functions so it's a good idea to make it a struct that holds everything you might need.
* `parse`: `void (Layout *layout, Json *json, void *object);` - Handle setting additional properties on your type by parsing the JSON. See the section on the [JSON API](#json-api) for how to use `json`.
* `destroy`: `void (void *object)` - Standard cleanup. Destroy child layers, unload resources, free allocated memory, etc.
* `get_layer`: `Layer* (void *object)` - Must return a layer to add to the layer heirarchy.
* `cast`: `void *(void *object)` - Return something that parent parsing can handle. If you add a type with `layout_add_type()` and specify `parent_type` then before your type is parsed the parent type will parse the JSON. This is useful for extending something like TextLayer to handle all the standard text attributes.

# JSON API

pebble-layout includes a simple JSON API that uses [Jsmn](https://github.com/zserge/jsmn) to handle parsing/tokenizing. The API iterates through the JSON structure, converting tokens into types automatically.

You will need to use the JSON API when implementing a custom type. The custom type create function gives you a `Json` object, which you will pass along to the various API functions. The standard template for processing fields is as follows:

```c
static void prv_my_custom_type_parse(Layout *this, Json *json, void *object) {
    // object is what was created in the create function.
    // For example, it might be a simple Layer
    Layer *layer = (Layer *) object;

    size_t size = json_get_size(json);
    for (size_t i = 0; i < size; i++) {
        char *key = json_next_string(json);
        if (strncmp(key, "color", strlen(key))) {
            GColor color = json_next_color(json); // Advance and parse the token.
            ... // Do something with color.
        } else if (strncmp(key, "foobar", strlen(key))) {
            json_advance(json);
            if (json_is_array(json)) {
                size_t len = json_get_size(json)
                for (size_t j = 0; j < len; j++) {
                    ... // Do something with each nested token.
                }
            }
        } else {
            json_skip_tree(json); // Skip any properties we don't care about.
            // This is important to keep iteration in sync and prevent later parsing
            // from blowing up. So always end your if-else chain with json_skip_tree().
        }
        free(key); // Make sure to free any strings you get
    }

    ...
}
```
