/** Register signal to godot script */
export function signal<T extends godot.Object>(name: string) {
	return function (target: new() => T) {
		godot.register_signal(target, name);
	}
}

/**
 * Register property to godot class
 * @param value The default value of the property
 */
export function property<T extends godot.Object>(value) {
	return function (target: T, property, descriptor) {
		godot.register_property(target, property, value);
		return descriptor;
	}
}

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
