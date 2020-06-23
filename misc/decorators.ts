/** Set the script is runable in editor */
export function tool<T extends godot.Object>(target: new() => T) {
	godot.set_script_tooled(target, true);
}

/** Set the script icon */
export function icon<T extends godot.Object>(icon) {
	return function (target: new() => T) {
		godot.set_script_icon(target, icon);
	}
}

/** Register signal to godot script */
export function signal<T extends godot.Object>(name: string) {
	return function (target: new() => T) {
		godot.register_signal(target, name);
	}
}

/**
 * Register multiple signals from an array or keys of an object
 * @param signals The object or array contains signal names
 */
export function signals<T extends godot.Object>(signals: {[key: string]: any} | string[]) {
	return function (target: new() => T) {
		let keys: string[] = [];
		if (!Array.isArray) {
			keys = Object.getOwnPropertyNames(signals);
		}
		for (const signal of keys) {
			godot.register_signal(target, signal);
		}
	}
}

/**
 * Register property to godot class
 * @param value The default value of the property
 */
export function property<T extends godot.Object>(value) {
	return function (target: T, property: string, descriptor) {
		godot.register_property(target, property, value);
		return descriptor;
	}
}

/**
 * Register an enumeration property
 * @param enumeration Enumeration name list
 * @param default_value The default value of the property
 */
export function enum_property<T extends godot.Object>(enumeration: string[], default_value?: string|number) {
	return function (target: T, property: string, descriptor) {
		const pi: godot.PropertyInfo = {
			hint: godot.PropertyHint.PROPERTY_HINT_ENUM,
			type: typeof(default_value) === 'string' ? godot.TYPE_STRING : godot.TYPE_INT,
			hint_string: '',
			default: typeof(default_value) === 'string' ? default_value : 0
		};
		for (let i = 0; i < enumeration.length; i++) {
			pi.hint_string += enumeration[i];
			if (i < enumeration.length - 1) {
				pi.hint_string += ',';
			}
		}
		godot.register_property(target, property, pi);
		return descriptor;
	}
}

/**
 * Return the node with `path` if the `_onready` is called
 * @param path The path or the type to get the node
 */
export function onready<T extends godot.Node>(path: string | (new()=>godot.Node)) {
	return function (target: T, property: string, descriptor) {
		descriptor.get = function() {
			const key = `__on_ready_value:${property}`;
			if (!this[key]) {
				this[key] = (this as godot.Node).get_node(path as (new()=>godot.Node));
			}
			return this[key];
		};
		return descriptor;
	}
}
