(function override_object_connect() {
	
	const godot_object_connect = godot.Object.prototype.connect;
	const GodotAnonymousConnectionManager = new(class GodotAnonymousConnectionManager extends godot.Reference {
		anonymous_connection_index = 0;

		get_anonymous_method_name() {
			return `__anonymous_method_${this.anonymous_connection_index++}`;
		}

		add_anonymous_method(method) {
			let method_name = this.get_anonymous_method_name();
			this[method_name] = method;
			return method_name;
		}
	})();
	
	// godot_connect_override(object: godot.Object, signal: string, target: godot.Object, method: string|Function, params?: any[], flags?: number)
	function godot_connect_override(...args) {
		if (args.length < 4) throw new Error('4 or more argument expected for godot.Object.connect');
		let object = args[0];
		if (!(object instanceof godot.Object)) throw new Error('godot.Object expected for argument #0 of godot.Object.connect');
		let signal = args[1];
		if (typeof signal !== 'string' || !signal) throw new Error('string expected for argument #1 of godot.Object.connect');
		if (args.length >= 4) {
			let params = args.length >= 5 ? args[4] : [];
			let flags = args.length >= 6 && typeof (args[5]) === 'number' ? args[5] : 0;
			let method_name = null;
			let target = args[2];
			if (target instanceof godot.Object) {
				let method = args[3];
				if (typeof method === 'string' && object[method] instanceof Function) {
					method_name = method;
				} else if (typeof method === 'function') {
					if (target[method.name] === method) {
						method_name = method.name;
					} else {
						method_name = GodotAnonymousConnectionManager.get_anonymous_method_name();
						target[method_name] = method;
					}
				} else {
					throw new Error('method name or function expected for argument #3 of godot.Object.connect');
				}
			} else {
				let method = args[3];
				if (typeof method === 'function') {
					method_name = GodotAnonymousConnectionManager.add_anonymous_method(function (...args) {
						method.apply(target, args);
					});
					target = GodotAnonymousConnectionManager;
				} else {
					throw new Error('function expected for argument #3 of godot.Object.connect');
				}
			}
			if (target instanceof godot.Object && method_name) {
				godot_object_connect.apply(object, [signal, target, method_name, params, flags]);
			}
		}
	}
	
	Object.defineProperty(godot.Object.prototype, 'connect', {
		value: function (...args) {
			godot_connect_override(this, ...args);
		},
		writable: false,
		configurable: true
	});
	
})();
