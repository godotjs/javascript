## JavaScript language binding for godot game engine

This module implements the JavaScript script language support for godot game engine.

[QuickJS](https://bellard.org/quickjs/) is used as the ECMAScript engine.
> QuickJS is a small and embeddable Javascript engine. It supports the ES2020 specification including modules, asynchronous generators and proxies.
>
> It optionally supports mathematical extensions such as big integers (BigInt), big floating point numbers (BigFloat) and operator overloading.

It is also possible to replace the ECMAScript engine like V8 or SpiderMonkey but not in the near plan.

**This project is still in development so it may contain bugs.**

-----

### Compile
* Clone the source code of [godot](https://github.com/godotengine/godot)
* Clone this module and put it into `godot/modules/` and make sure the folder name of this module is `ECMAScript`
* [Recompile godot engine](https://docs.godotengine.org/en/3.0/development/compiling/index.html)

### Usage

##### How to export script class to godot
1. Define your ECMAScript class inhirent from godot class and export it as **default** entry
```js
// The default export entry is treat as exported class to godot
export default class MySprite extends godot.Sprite {
	
	// this is _init() in GDScript
	constructor() {
		super();
	}
	
	_ready() {
		
	}
	
	_process(delta) {
		
	}
}
```

2. Attach the script file to the node or resource object like you did with GDScript

##### How to export signals

```js
export default class MySprite extends godot.Sprite {};
// register game_over signal to MySprite class
godot.register_signal(MySprite, 'game_over');
```

##### How to export properties
```js
export default class MySprite extends godot.Sprite {
	_process(delta) {
		// Yes! We can use operators in JavaScript like GDScript
		this.position += this.direction * new godot.Vector2(delta, delta);
	}
};
// export 'direction' properties to MySprite godot inspector
godot.register_property(MySprite, 'direction', new godot.Vector2(1, 0));
```

#### About the API

All godot api are define in the `godot` namespace.

We didn't change any api name so you don't need to change your habbit at all.

GDScript | ECMAScript
---- | ---
null | null
int | number
float | number
String | string
Array | Array
Dictionary | Object
NodePath | string
Object | godot.Object
Resource | godot.Resource
Vector2 | godot.Vecotor2
Color | godot.Color
sin(v)| godot.sin(v)
print(v)| godot.print(v)
PI|godot.PI
Color.black | godot.Color.black
Control.CursorShape | godot.Control.CursorShape
Label.Align.ALIGN_LEFT | godot.Label.Align.ALIGN_LEFT

##### API specification:
- Keys of Dictionary are converted to String in JavaScript
- Signals are defined as constants to its classes
	```
	godot.Control.resized === 'resized' // true
	```
- Additional functions
  - `godot.register_signal(cls, signal_name)` to register signals
  - `godot.register_property(cls, name, default_value)` to define and export properties
  - `godot.register_class(cls, name)` to register named class manually
  - `godot.set_script_tooled(cls, tooled)` to set `tooled` of the class
  - `godot.set_script_icon(cls, path)` to set icon of the class
  - `godot.get_type(val)` Returns the internal type of the given `Variant` object, using the `godot.TYPE_*`
  - `requestAnimationFrame(callback)` to add a callback function to be called every frame
  - `cancelAnimationFrame(request_id)` to cancel an frame request previously scheduled
- Using signals in the ECMAScript way
  - Allow passing functions for `godot.Object.connect`, `godot.Object.disconnect` and `godot.Object.is_connected`
	```js
		this.panel.connect(godot.Control.resized, (size) => {
			console.log('The size of the panel changed to:', size);
		});
	```
  - Using `await` to wait signals
	```js
	await godot.yield(this.get_tree().create_timer(1), godot.SceneTreeTimer.timeout);
	console.log('After one second to show');
	```
- Preload resources with ECMAScript import statement
	```js
	import ICON from 'res://icon.png';
	```
- Multi-threading with minimal [Worker API](https://developer.mozilla.org/en-US/docs/Web/API/Worker)
	- Start a new thread with Worker
		```js
		const worker = new Worker('worker.js'); // Run worker.js in a new thread context
		worker.postMessage({type: 'load_dlc', value: 'dlc01.pck'});
		worker.onmessage = function(msg) {
			console.log("[MainThread] recived message from worker thread:", msg);
		}
		```
	- Tansfer value in different thread context with `Worker.abandonValue` and `Worker.adoptValue`
		```js
		// In worker thread
		let id = Worker.abandonValue(object);
		postMessage({ type: 'return_value', id: id });
		
		// In the host thread
		worker.onmessage = function(msg) {
			if (typeof msg === 'object' && msg.type === 'return_value') {
				let value_from_worker = Worker.adoptValue(msg.id);
			}
		}
		```
	
- TypeScript support
	- Run the menu command `Project > Tools > ECMAScript > Generate TypeScript Declaration` from godot editor to dump the api declearations
	- Run the menu command `Project > Tools > ECMAScript > Generate TypeScript Project` from godot editor to generate a TypeScript project
	

## Demo
You can try demos in the [ECMAScriptDemos](https://github.com/Geequlim/ECMAScriptDemos)
