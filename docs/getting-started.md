### How to export script class to Godot

1. Define your JavaScript class and inherit from a Godot class, then export it as the **default** entry:

```javascript title="my-sprite.mjs"
// The default export entry is treated as an exported class to Godot
export default class MySprite extends godot.Sprite {
	// this is _init() in GDScript
	constructor() {
		super();
	}

	_ready() {}

	_process(delta) {}
}
```

2. Save the script with extension `.mjs`
3. Attach the script file to the node or resource object like you do with GDScript

### How to export signals

```javascript title="my-sprite.mjs"
export default class MySprite extends godot.Sprite {}
// register game_over signal to MySprite class
godot.register_signal(MySprite, "game_over");
```

### How to export properties

```javascript title="my-sprite.mjs"
export default class MySprite extends godot.Sprite {
	_process(delta) {
		// Yes! We can use operators in JavaScript like GDScript
		this.position += this.direction * delta;
	}
}
// export 'direction' properties to MySprite Godot inspector
godot.register_property(MySprite, "direction", new godot.Vector2(1, 0));
```

There are 2 ways of using the `godot.register_property`. The third parameter can either be a default value for the property you're trying to export or an object giving a more detailed description of how the editor should show it.

```js
function register_property(target: GodotClass | godot.Object, name: string, value: any);
function register_property(target: GodotClass | godot.Object, name: string, info: PropertyInfo);
```

So calling the `register_property` like this:

```js
godot.register_property(MyClass, "number_value", 3.14);
```

Is the simplified version of:

```js
godot.register_property(MyClass, "number_value", {
	type: godot.TYPE_REAL,
	hint: godot.PropertyHint.PROPERTY_HINT_NONE,
	hint_string: "",
	default: 3.14,
});
```

For more detail on how to use it, [click here](https://github.com/Geequlim/ECMAScript/issues/24#issuecomment-655584829).
