/**
 * Export the class to godot
 * @param name The name of the class in godot
 * @param tool Does this script runs in godot editor
 * @param icon The icon of the class in godot
 */
export function gdclass(name:string, tool?: boolean, icon?:string) {
	return function(target) {
		godot.register_class(target, name, tool, icon);
	}
}

/** Register the method as godot signal */
export function signal(target, property, descriptor) {
	descriptor.writable = false;
	godot.register_signal(target, property);
}

/**
 * Register property to godot class
 * @param value The default value of the property
 */
export function property(value) {
	return function (target, name, descriptor) {
		var type = godot.TYPE_NIL;
		switch (typeof (value)) {
			case 'number':
				type = godot.TYPE_REAL;
				break;
			case 'string':
				type = godot.TYPE_STRING;
				break;
			case 'boolean':
				type = godot.TYPE_BOOL;
				break;
			default:
				break;
		}
		if (type === godot.TYPE_NIL && typeof(value) === 'object') {
			if (value instanceof godot.Vector2) {
				type = godot.TYPE_VECTOR2;
			} else if (value instanceof godot.Color) {
				type = godot.TYPE_COLOR;
			} else if (value instanceof godot.Rect2) {
				type = godot.TYPE_RECT2;
			} else {
				type = godot.TYPE_DICTIONARY;
			}
		}
		godot.register_property(target, name, type, value);
		return descriptor;
	}
}