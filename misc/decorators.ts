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
		godot.register_property(target, name, value);
		return descriptor;
	}
}
