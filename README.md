## JavaScript language binding for godot game engine

This module implements JavaScript/TypeScript language support for the godot game engine. [QuickJS](https://bellard.org/quickjs/) is used as the ECMAScript engine.

-----

### Features
- Almost complete ES2020 support
- All godot api avaliable
- Operator overriding for builtin types (Vector3, Color, etc)
- TypeScript support 
- [Using thirdpart libraries from npm](https://github.com/GodotExplorer/ECMAScriptDemos/tree/master/npm_module)
- Multi-thread support with Worker API
- Full code completion support for all godot APIs including signals and enumerations
- Debug in Visual Studio Code with the [plugin](https://marketplace.visualstudio.com/items?itemName=geequlim.godot-javascript-debug)

### Download
You can try the pre-compiled binaries from the [release page](https://github.com/GodotExplorer/ECMAScript/releases)  
You can also get the binaries with lastest commits from the [github build action result](https://github.com/GodotExplorer/ECMAScript/actions)

### Compilation
* Clone the source code of [godot](https://github.com/godotengine/godot)
* Clone this module and put it into `godot/modules/` and make sure the folder name of this module is `ECMAScript`
* [Recompile the godot engine](https://docs.godotengine.org/en/3.2/development/compiling/index.html) <b>(Only MinGW is supported on Windows for now!)</b>

![Build Godot with ECMAScript](https://github.com/GodotExplorer/ECMAScript/workflows/Build%20Godot%20with%20ECMAScript/badge.svg)

### Usage

##### How to export script class to godot
1. Define your ECMAScript class and inherit from a godot class then export it as the **default** entry
```js
// The default export entry is treated as an exported class to godot
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
2. Save the script with extension `.jsx`
3. Attach the script file to the node or resource object like you do with GDScript

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
		this.position += this.direction * delta;
	}
};
// export 'direction' properties to MySprite godot inspector
godot.register_property(MySprite, 'direction', new godot.Vector2(1, 0));
```

#### About the API

All of godots api's are defined within the `godot` namespace.

No API names have been renamed or changed so you shouldn't need to change your habits.

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
  - `godot.yield(target, signal)` Returns a Promise which will be resolved when the signal emitted
  - `requestAnimationFrame(callback)` registers a callback function to be called every frame, returns a request ID.
  - `cancelAnimationFrame(request_id)` to cancel a previously scheduled frame request
  - `require(module_id)` to load a CommonJS module or load a resource file
  - `$` is the alias of `Node.get_node`
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
- Multi-threading with minimal [Worker API](https://developer.mozilla.org/en-US/docs/Web/API/Worker) (**This is an experimental feature**)
	- Start a new thread with Worker
		```js
		const worker = new Worker('worker.js'); // Run worker.js in a new thread context
		worker.postMessage({type: 'load_dlc', value: 'dlc01.pck'});
		worker.onmessage = function(msg) {
			console.log("[MainThread] received message from worker thread:", msg);
		}
		```
	- Transfer value in different thread context with `godot.abandon_value` and `godot.adopt_value`
		```js
		// In worker thread
		let id = godot.abandon_value(object);
		postMessage({ type: 'return_value', id: id });
		
		// In the host thread
		worker.onmessage = function(msg) {
			if (typeof msg === 'object' && msg.type === 'return_value') {
				let value_from_worker = godot.adopt_value(msg.id);
			}
		}
		```
	
### TypeScript support and JSX code completion
- Run the menu command `Project > Tools > ECMAScript > Generate TypeScript Project` from the godot editor to generate a TypeScript project
- Run `tsc -w -p .` under your project folder in the terminal to compile scripts

#### Code completion
- Code completion in TSX will automatically work once the godot.d.ts file is generated by the above steps.
- Code completion in JSX in VS Code can be achieved by adding `"types": "./godot.d.ts"` to your package.json file, as long as the godot.d.ts file is above or in a sibling folder of the .jsx source files in the directory structure.

#### Example TypeScript Usage

Make sure the file with extension '.tsx' so it can be compiled to a `.jsx` file then we can attach it to a node in godot editor.

Most of the `register` functions are available as various decorators as seen below.

```ts
import { signal, property, tool, onready, node } from "./decorators";

@tool // make the script runnable in godot editor
export default class InputLine extends godot.HBoxContainer {

	// define a signal
	@signal
	static readonly OnTextChanged: string;

	// expose a node property
	@node
	icon: godot.Sprite;

	// register offset property with the godot inspector with default value of Vector2(0, 0)
	@property({ default: godot.Vector2.ZERO })
	offset: godot.Vector2;
	
	// register properties for godot editor inspector
	@property({ type: godot.VariantType.TYPE_STRING })
	get title() { return this._title; }
	set title(v: string) {
		this._title = v;
		if (this._label) {
			this._label.text = v;
		}
	}
	private _title: string;

	@property({ default: "Input text here" })
	get hint() { return this._hint; }
	set hint(v: string) {
		this._hint = v;
		if (this.edit) {
			this.edit.hint_tooltip = v;
			this.edit.placeholder_text = v;
		}
	}
	private _hint: string;

	get label(): godot.Label { return this._label; }
	protected _label: godot.Label;

	// call get_node('LineEdit') and assign the returned value to 'this.edit' automatically when the node is ready
	@onready('LineEdit')
	edit: godot.LineEdit;

	get text(): string {
		return this.edit?.text;
	}

	_ready() {
		// get first child with the type of godot.Label
		this._label = this.get_node(godot.Label);
		
		// Apply the inspector filled values with property setters
		this.title = this.title;
		this.hint = this.hint;

		this.edit.connect(godot.LineEdit.text_changed, (text: string)=>{
			this.emit_signal(InputLine.OnTextChanged, text);
		});
	}
}
```

## Demo
You can try demos in the [ECMAScriptDemos](https://github.com/Geequlim/ECMAScriptDemos)
