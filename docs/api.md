All of Godot's APIs are defined within the `godot` namespace.

No API names have been renamed or changed, so you shouldn't need to change your habits.

| GDScript               | JavaScript                   |
| ---------------------- | ---------------------------- |
| null                   | null                         |
| int                    | number                       |
| float                  | number                       |
| String                 | string                       |
| Array                  | Array                        |
| Dictionary             | Object                       |
| NodePath               | string                       |
| Object                 | godot.Object                 |
| Resource               | godot.Resource               |
| Vector2                | godot.Vector2                |
| Color                  | godot.Color                  |
| sin(v)                 | godot.sin(v)                 |
| print(v)               | godot.print(v)               |
| PI                     | godot.PI                     |
| Color.black            | godot.Color.black            |
| Control.CursorShape    | godot.Control.CursorShape    |
| Label.Align.ALIGN_LEFT | godot.Label.Align.ALIGN_LEFT |

## API specification:

- Keys of Dictionary are converted to String in JavaScript
- Signals are defined as constants to their classes
  ```
  godot.Control.resized === 'resized' // true
  ```

### Additional functions

- `godot.register_signal(cls, signal_name)` to register signals
- `godot.register_property(cls, name, default_value)` to define and export properties
- `godot.register_class(cls, name)` to register named class manually
- `godot.set_script_tooled(cls, tooled)` to set `tooled` of the class
- `godot.set_script_icon(cls, path)` to set icon of the class
- `godot.get_type(val)` Returns the internal type of the given `Variant` object, using the `godot.TYPE_*`
- `godot.yield(target, signal)` Returns a Promise which will be resolved when the signal emitted
- `requestAnimationFrame(callback)` registers a callback function to be called every frame, returns a request ID.
- `cancelAnimationFrame(request_id)` to cancel a previously scheduled frame request
- `require(module_id)` to load a CommonJS module or load a resource file
- `$` is the alias of `Node.get_node`

### Using signals

Allow passing functions for `godot.Object.connect`, `godot.Object.disconnect`, and `godot.Object.is_connected`:

```js
this.panel.connect(godot.Control.resized, (size) => {
  console.log("The size of the panel changed to:", size);
});
```

Using `await` to wait for signals

```js
await godot.yield(
  this.get_tree().create_timer(1),
  godot.SceneTreeTimer.timeout
);
console.log("After one second to show");
```

Preload resources with ECMAScript import statement

```js
import ICON from "res://icon.png";
```

### Multi-threading

Multi-threading with minimal [Worker API](https://developer.mozilla.org/en-US/docs/Web/API/Worker) (**This is an experimental feature**)

Start a new thread with Worker:

```js
const worker = new Worker("worker.js"); // Run worker.js in a new thread context
worker.postMessage({ type: "load_dlc", value: "dlc01.pck" });
worker.onmessage = function (msg) {
  console.log("[MainThread] received message from worker thread:", msg);
};
```

Transfer value in different thread context with `godot.abandon_value` and `godot.adopt_value`:

```js
// In worker thread
let id = godot.abandon_value(object);
postMessage({ type: "return_value", id: id });

// In the host thread
worker.onmessage = function (msg) {
  if (typeof msg === "object" && msg.type === "return_value") {
    let value_from_worker = godot.adopt_value(msg.id);
  }
};
```
