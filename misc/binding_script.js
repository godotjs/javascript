(function start_script_binding() {
	
	const GodotAnonymousConnectionManager = new(class GodotAnonymousConnectionManager extends godot.Reference {
		anonymous_connection_index = 0;
		
		get_slot_id(emitter, signal) {
			return `__slot_${emitter.get_instance_id()}_${signal}`;
		}

		get_anonymous_method_name() {
			return `__anonymous_method_${this.anonymous_connection_index++}`;
		}

		add_anonymous_method(method) {
			let method_name = this.get_anonymous_method_name();
			this[method_name] = method;
			return method_name;
		}
	})();
	
	const godot_object_connect = godot.Object.prototype.connect;
	function godot_connect_override(...args) {
		if (args.length < 4) throw new Error('4 or more arguments expected');
		let object = args[0];
		if (!(object instanceof godot.Object)) throw new Error('godot.Object expected for argument #0');
		let signal = args[1];
		if (typeof signal !== 'string' || !signal) throw new Error('signal name expected for argument #1');
		if (args.length >= 4) {
			let params = args.length >= 5 && args[4].length ? args[4] : [];
			let flags = args.length >= 6 && typeof (args[5]) === 'number' ? args[5] : 0;
			let method_name = null;
			let target = args[2] || GodotAnonymousConnectionManager;
			if (target instanceof godot.Object) {
				let method = args[3];
				if (typeof method === 'string') {
					method_name = method;
				} else if (typeof method === 'function') {
					if (target[method.name] === method) {
						method_name = method.name;
					} else {
						method_name = GodotAnonymousConnectionManager.get_anonymous_method_name();
						target[method_name] = method;
					}
					method[GodotAnonymousConnectionManager.get_slot_id(object, signal)] = method_name;
				} else {
					throw new Error('method name or function expected for argument #3');
				}
			} else {
				let method = args[3];
				if (typeof method === 'function') {
					const slot_id = GodotAnonymousConnectionManager.get_slot_id(object, signal);
					function callback(...args) { method.apply(target, args); };
					method_name = GodotAnonymousConnectionManager.add_anonymous_method();
					method[slot_id] = method_name;
					callback[slot_id] = method_name;
					target = GodotAnonymousConnectionManager;
				} else {
					throw new Error('function expected for argument #3');
				}
			}
			if (target instanceof godot.Object && method_name) {
				return godot_object_connect.apply(object, [signal, target, method_name, params, flags]);
			} else {
				throw new Error('Invalid arguments');
			}
		}
		return godot.FAILED;
	};
	
	Object.defineProperty(godot.Object.prototype, 'connect', {
		value: function (...args) {
			return godot_connect_override(this, ...args);
		},
		writable: false,
		configurable: true
	});
	
	
	const godot_object_disconnect = godot.Object.prototype.disconnect;
	function godot_disconnect_override(...args) {
		if (args.length < 4) throw new Error('4 arguments expected');
		let object = args[0];
		if (!(object instanceof godot.Object)) throw new Error('godot.Object expected for argument #0');
		let signal = args[1];
		if (typeof signal !== 'string' || !signal) throw new Error('signal name expected for argument #1');
		let target = args[2] || GodotAnonymousConnectionManager;
		let method = args[3];
		let method_name = null;
		let slot_id = undefined;
		if (typeof method === 'string') {
			method_name = method;
		} else if (typeof method === 'function') {
			slot_id = GodotAnonymousConnectionManager.get_slot_id(object, signal);
			method_name = method[slot_id];
			method[slot_id] = undefined;
		} else {
			throw new Error('method name or function expected for argument #3');
		}
		
		
		if (object instanceof godot.Object && target instanceof godot.Object && method_name) {
			if (slot_id) {
				if (typeof target[method_name] === 'function') {
					target[method_name] = undefined;
					delete target[method_name];
				}
				delete method[slot_id];
			}
			godot_object_disconnect.apply(object, [signal, target, method_name]);
		} else {
			throw new Error('Invalid arguments');
		}
	};
	
	Object.defineProperty(godot.Object.prototype, 'disconnect', {
		value: function (...args) {
			godot_disconnect_override(this, ...args);
		},
		writable: false,
		configurable: true
	});
	
	const godot_object_is_connected = godot.Object.prototype.is_connected;
	function godot_is_connected_override(...args) {
		if (args.length < 4) throw new Error('4 arguments expected');
		let object = args[0];
		if (!(object instanceof godot.Object)) throw new Error('godot.Object expected for argument #0');
		let signal = args[1];
		if (typeof signal !== 'string' || !signal) throw new Error('signal name expected for argument #1');
		let target = args[2] || GodotAnonymousConnectionManager;
		let method = args[3];
		let method_name = null;
		let slot_id = undefined;
		if (typeof method === 'string') {
			method_name = method;
		} else if (typeof method === 'function') {
			slot_id = GodotAnonymousConnectionManager.get_slot_id(object, signal);
			method_name = method[slot_id];
		} else {
			throw new Error('method name or function expected for argument #3');
		}
		
		if (object instanceof godot.Object && target instanceof godot.Object && method_name) {
			return godot_object_is_connected.apply(object, [signal, target, method_name]);
		} else {
			throw new Error('Invalid arguments');
		}
		return false;
	};
	
	Object.defineProperty(godot.Object.prototype, 'is_connected', {
		value: function (...args) {
			return godot_is_connected_override(this, ...args);
		},
		writable: false,
		configurable: true
	});
	
	function godot_yield(target, signal) {
		return new Promise(function(resolve, reject) {
			function callback(...args) {
				const slot_id = GodotAnonymousConnectionManager.get_slot_id(target, signal);
				const method_name = callback[slot_id];
				GodotAnonymousConnectionManager[method_name] = undefined;
				delete GodotAnonymousConnectionManager[method_name];
				delete callback[slot_id];
				resolve(args);
			};
			target.connect(signal, GodotAnonymousConnectionManager, callback, [], godot.Object.CONNECT_ONESHOT);
		});
	}
	
	Object.defineProperty(godot, 'yield', {
		value: godot_yield,
		writable: false,
		configurable: true
	});
	
})();
