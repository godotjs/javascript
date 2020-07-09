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
export function signal(target: godot.Object | (new() => godot.Object), property: string, descriptor?: any) {
	var constructor: Function = typeof(target) === 'function' ? target : target.constructor;
	var prototype: object = constructor.prototype;
	godot.register_signal(target, property);

	descriptor = descriptor || {};
	(descriptor as PropertyDescriptor).value = property;
	(descriptor as PropertyDescriptor).writable = false;

	Object.defineProperty(constructor, property, descriptor);
	Object.defineProperty(prototype, property, descriptor);
}

/**
 * Register property to godot class
 * @param value The default value of the property
 */
export function property<T extends godot.Object>(info: godot.PropertyInfo) {
	return function (target: T, property: string, descriptor?: any) {
		info = info || {};
		godot.register_property(target, property, info);
		return descriptor;
	}
}

/**
 * Return the node with `path` if the `_onready` is called
 * @param path The path or the type to get the node
 */
export function onready<T extends godot.Node>(path: string | (new()=>godot.Node)) {
	return function (target: T, property: string, descriptor?: any) {
		const key = `$onready:${property}`;
		descriptor = descriptor || {};
		descriptor.set = function(v) { this[key] = v; };
		descriptor.get = function() {
			let v = this[key];
			if (!v) {
				v = (this as godot.Node).get_node(path as (new()=>godot.Node));
				this[key] = v;
			}
			return v;
		};
		return descriptor;
	}
}

/**
 * Register the member as a node property  
 * **Note: The value is null before current node is ready**
 * @param path The default path name of the node
 */
export function node<T extends godot.Node>(target: T, property: string, descriptor?: any) {
	const key = `$onready:${property}`;
	const path_key = `${property} `; // <-- a space at the end
	descriptor = descriptor || {};
	descriptor.set = function(v) { this[key] = v; };
	descriptor.get = function() {
		let v = this[key];
		if (!v) {
			v = (this as godot.Node).get_node(this[path_key]);
			this[key] = v;
		}
		return v;
	};
	godot.register_property(target, path_key, { type: godot.TYPE_NODE_PATH });
	return descriptor;
}

/**
 * Register an enumeration property
 * @param enumeration Enumeration name list
 * @param default_value The default value of the property
 */
export function enumeration<T extends godot.Object>(enumeration: string[], default_value?: string|number) {
	return function (target: T, property: string, descriptor?: any) {
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
