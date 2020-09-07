/**
 * Expose as an ECMAClass.
 * An ECMAScript object is created and attached automaticly when construct an instance from this class
 */
export function gdclass<T extends godot.Object>(target: new() => T) {
    const id = gdclass['internal_class_id'] = gdclass['internal_class_id'] ? gdclass['internal_class_id'] + 1 : 1;
	const class_name = `AnonymousECMAClass${id}`;
	godot.register_class(target, class_name);
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

function make_rpc_mode_config_decorator(mode: godot.MultiplayerAPI.RPCMode) {
	return function <T extends godot.Node>(target: T, property: string, descriptor?: any) {
		const is_method = typeof target[property] === 'function';
		const original_ready = target._ready;
		target._ready = function (this: godot.Node) {
			if (is_method) this.rpc_config(property, mode);
			else this.rset_config(property, mode);
			if (original_ready) return original_ready.call(this);
		};
		return descriptor;
	}
}

/**
 * Used with `Node.rpc_config` or `Node.rset_config` to set a method to be called or
 * a property to be changed only on the remote end, not locally.
 * Analogous to the `remote` keyword. Calls and property changes are accepted from all
 * remote peers, no matter if they are node's master or puppets.
 */
export const remote = make_rpc_mode_config_decorator(godot.MultiplayerAPI.RPC_MODE_REMOTE)
/**
 * Used with `Node.rpc_config` or `Node.rset_config` to set a method to be called or
 * a property to be changed only on the network master for this node.
 * Analogous to the `master` keyword. Only accepts calls or property changes from the
 * node's network puppets, see `Node.set_network_master`.
 */
export const master = make_rpc_mode_config_decorator(godot.MultiplayerAPI.RPC_MODE_MASTER)
/**
 * Used with `Node.rpc_config` or `Node.rset_config` to set a method to be called or
 * a property to be changed only on puppets for this node.
 * Analogous to the `puppet` keyword. Only accepts calls or property changes from the
 * node's network master, see `Node.set_network_master`.
 */
export const puppet = make_rpc_mode_config_decorator(godot.MultiplayerAPI.RPC_MODE_PUPPET)
/**
 * @deprecated Use `RPC_MODE_PUPPET` (@puppet) instead. Analogous to the `slave` keyword.
 */
export const slave = make_rpc_mode_config_decorator(godot.MultiplayerAPI.RPC_MODE_SLAVE)
/**
 * Behave like `RPC_MODE_REMOTE` (@remote) but also make the call or property change locally.
 * Analogous to the `remotesync` keyword.
 */
export const remote_sync = make_rpc_mode_config_decorator(godot.MultiplayerAPI.RPC_MODE_REMOTESYNC)
/**
 * @deprecated Use `RPC_MODE_REMOTESYNC` (@remote_sync) instead. Analogous to the `sync` keyword
 */
export const sync = make_rpc_mode_config_decorator(godot.MultiplayerAPI.RPC_MODE_REMOTESYNC)
/**
 * Behave like `RPC_MODE_MASTER` (@master) but also make the call or property change locally.
 * Analogous to the `mastersync` keyword.
 */
export const master_sync = make_rpc_mode_config_decorator(godot.MultiplayerAPI.RPC_MODE_MASTERSYNC)
/**
 * Behave like `RPC_MODE_PUPPET` (@puppet) but also make the call or property change locally.
 * Analogous to the `puppetsync` keyword.
 */
export const puppet_sync = make_rpc_mode_config_decorator(godot.MultiplayerAPI.RPC_MODE_PUPPETSYNC)
