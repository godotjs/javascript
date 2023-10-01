(function start_script_binding() {
	
	const GodotAnonymousConnectionManager = new(class GodotAnonymousConnectionManager extends godot.Reference {
		anonymous_connection_index = 0;
		
		get_slot_id(emitter, signal, target) {
			return `__${emitter.get_instance_id()}:${signal}:${target.get_instance_id()}__`;
		}

		get_anonymous_method_name() {
			return `__anonymous_slot:${this.anonymous_connection_index++}`;
		}
	})();
	
	const godot_object_connect = godot.Object.prototype.connect;
	Object.defineProperty(godot.Object.prototype, 'connect', {
		// Object.prototype.connect(signal_name, target, method, params, flags)
		value: function connect(...args) {
			if (args.length < 2) throw new Error('2 or more arguments expected');
			let signal = args[0];
			if (typeof signal !== 'string' || !signal) throw new Error('signal name expected for argument #0');
			
			let offset = 0;
			let target = null;
			let method = null;
			let method_name = null;
			let target_caller = null;
			let params = [];
			let flags = 0;
			
			if (typeof args[1] === 'function') {
				offset = -1;
				target = GodotAnonymousConnectionManager;
				method = args[1];
			} else if (args.length >= 3) {
				target_caller = args[1];
				target = target_caller || GodotAnonymousConnectionManager;
				if (!(target instanceof godot.Object)) {
					new Error('godot.Object expected for signal target');
				}
				method = args[2];
			} else {
				throw new Error('method name or function expected for signal method');
			}
			if (args.length > 3 + offset) {
				let p = args[3 + offset];
				if (p && p.length)
					params = p;
			}
			if (args.length > 4 + offset) {
				let p = args[4 + offset];
				if (typeof p === 'number')
					flags = p;
			}
			
			if (typeof method === 'string' && method.length) {
				method_name = method;
			} else if (typeof method === 'function') {
				const slot_id = GodotAnonymousConnectionManager.get_slot_id(this, signal, target);
				if (target[method.name] === method) {
					method_name = method.name;
				} else {
					method_name = GodotAnonymousConnectionManager.get_anonymous_method_name();
					function callback(...args) { method.apply(target_caller, args); };
					target[method_name] = callback;
					callback[slot_id] = method_name;
				}
				method[slot_id] = method_name;
			} else {
				throw new Error('method name or function expected for signal method');
			}
			
			// console.log("connect", this, signal, target, method_name, params, flags);
			return godot_object_connect.apply(this, [signal, target, method_name, params, flags]);
		},
		writable: false,
		configurable: true
	});
	
	const godot_object_disconnect = godot.Object.prototype.disconnect;
	Object.defineProperty(godot.Object.prototype, 'disconnect', {
		// Object.prototype.disconnect(signal_name, target, method)
		value: function disconnect(...args) {
			if (args.length < 2) throw new Error('2 or more arguments expected');
			let signal = args[0];
			if (typeof signal !== 'string' || !signal) throw new Error('signal name expected for argument #0');
			
			let target = null;
			let method_name = null;
			let method = null;
			let slot_id = undefined;
			
			if (typeof args[1] === 'function') {
				target = GodotAnonymousConnectionManager;
				method = args[1];
			} else if (args.length >= 3) {
				target = args[1] || GodotAnonymousConnectionManager;
				if (!(target instanceof godot.Object)) {
					new Error('godot.Object expected for signal target');
				}
				method = args[2];
			} else {
				throw new Error('method name or function expected');
			}
			
			if (typeof method === 'string' && method) {
				method_name = method;
			} else if (typeof method === 'function') {
				slot_id = GodotAnonymousConnectionManager.get_slot_id(this, signal, target);
				method_name = method[slot_id];
				method[slot_id] = undefined;
			} else {
				throw new Error('method name or function expected');
			}
			
			if (slot_id) {
				if (typeof target[method_name] !== 'undefined') {
					target[method_name] = undefined;
					delete target[method_name];
				}
				delete method[slot_id];
			}
			// console.log("disconnect", this, signal, target, method_name);
			godot_object_disconnect.apply(this, [signal, target, method_name]);
		},
		writable: false,
		configurable: true
	});
	
	const godot_object_is_connected = godot.Object.prototype.is_connected;
	Object.defineProperty(godot.Object.prototype, 'is_connected', {
		// Object.prototype.is_connected(signal_name, target, method)
		value: function godot_is_connected_override(...args) {
			if (args.length < 3) throw new Error('3 arguments expected');
			let signal = args[0];
			if (typeof signal !== 'string' || !signal) throw new Error('signal name expected for argument #1');
			
			let target = null;
			let method = null;
			let method_name = null;
			let slot_id = undefined;
			
			if (typeof args[1] === 'function') {
				target = GodotAnonymousConnectionManager;
				method = args[1];
			} else if (args.length >= 3) {
				target = args[1] || GodotAnonymousConnectionManager;
				if (!(target instanceof godot.Object)) {
					new Error('godot.Object expected for signal target');
				}
				method = args[2];
			} else {
				throw new Error('method name or function expected for argument #2');
			}
			
			if (typeof method === 'string') {
				method_name = method;
			} else if (typeof method === 'function') {
				slot_id = GodotAnonymousConnectionManager.get_slot_id(this, signal, target);
				method_name = method[slot_id];
			} else {
				throw new Error('method name or function expected');
			}
			
			return godot_object_is_connected.apply(this, [signal, target, method_name]);
		},
		writable: false,
		configurable: true
	});
	
	function godot_yield(target, signal) {
		return new Promise(function(resolve, reject) {
			function callback(...args) {
				const slot_id = GodotAnonymousConnectionManager.get_slot_id(target, signal, GodotAnonymousConnectionManager);
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


	const godot_node_get_node = godot.Node.prototype.get_node;
	Object.defineProperty(godot.Node.prototype, 'get_node', {
		value: function(arg) {
			if (typeof arg === 'string') {
				return godot_node_get_node.call(this, arg);
			} else if (typeof arg === 'function') {
				for (let n of this.get_children()) {
					if (n instanceof arg) {
						return n;
					}
				}
			}
			return null;
		},
		writable: false,
		configurable: true
	});

	Object.defineProperty(godot.Node.prototype, '$', {
		value: godot.Node.prototype.get_node,
		writable: false,
		configurable: true
	});

	const godot_vector2 = godot.Vector2;
	Object.defineProperty(godot_vector2, "Axis", {
		value: {
			AXIS_X: 0,
			AXIS_Y: 1,
		},
		writable: false,
		configurable: false,
	});

	const godot_vector3 = godot.Vector3;
	Object.defineProperty(godot_vector3, "Axis", {
		value: {
			AXIS_X: 0,
			AXIS_Y: 1,
			AXIS_Z: 2,
		},
		writable: false,
		configurable: false,
	});

	// iterator of Pool*Vector
	const pool_classes = [ godot.PoolByteArray, godot.PoolIntArray, godot.PoolRealArray, godot.PoolStringArray, godot.PoolVector2Array, godot.PoolVector3Array, godot.PoolColorArray ];
	for (const pool_class of pool_classes) {
		Object.defineProperty(pool_class.prototype, Symbol.iterator, {value: function iterator() {
			let next_index = 0;
			return {
				next: ()=> {
					if (next_index < this.size()) {
						return { value: this.get(next_index++), done: false }
					} else {
						return { done: true };
					}
				},
			};
		}});
	}

	return godot.TOOLS_ENABLED ? {
		"removed": {
			"Rect2": ["end", "grow_margin"],
			"Color": ["h", "s", "v", "r8", "g8", "b8", "a8"],
			"Transform2D": ["xform", "xform_inv"],
			"Basis": ["is_equal_approx"],
			"Plane": ["intersects_segment", "intersects_ray", "intersect_3"],
			"AABB": ["end"],
			"Transform": ["xform", "xform_inv"],
		},
		"added": {
			"Object": [
				"/** Connect the `method` of `target` to the `signal`*/",
				"//@ts-ignore",
				"connect(signal: string, target: Object, method: Function, binds: any[] = [], flags: number = 0) : number;",
				"",
				"/** Connect the `method` to the `signal`. The caller of the method will be `null` */",
				"//@ts-ignore",
				"connect(signal: string, method: Function, binds: any[] = [], flags: number = 0) : number;",
				"",
				"/** Returns `true` if a connection exists for a given `signal`, `target`, and `method`. */",
				"is_connected(signal: string, target: Object, method: Function) : boolean;",
				"/** Returns `true` if a connection exists for a given `signal` and `method`. */",
				"is_connected(signal: string, method: Function) : boolean;",
				"",
				"/** Disconnects a `signal` from the `method` on the given `target`. */",
				"disconnect(signal: string, target: Object, method: Function) : void;",
				"/** Disconnects a `signal` from the `method` */",
				"disconnect(signal: string, method: Function) : void;"
			],
			"Node": [
				"/** Get first node with the class `cls` */",
				"get_node<T extends godot.Node>(cls: new()=>T): T;",
				"",
				"/** Get node with the path */",
				"$(path: string): godot.Node;",
				"/** Get first node with the class `cls` */",
				"$<T extends godot.Node>(cls: new()=>T): T;",
			]
		},
	} : undefined;
})();
