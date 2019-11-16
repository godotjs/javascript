#include "quickjs_binder.h"
#include "../ecmascript_language.h"
#include "core/engine.h"
#include "core/global_constants.h"
#include "core/math/expression.h"

HashMap<JSContext *, QuickJSBinder *, QuickJSBinder::PtrHasher> QuickJSBinder::context_binders;
HashMap<JSRuntime *, JSContext *, QuickJSBinder::PtrHasher> QuickJSBinder::runtime_context_map;

static JSValue console_log_function(JSContext *ctx, JSValue this_val, int argc, JSValue *argv) {
	PoolStringArray args;
	args.resize(argc);
	for (int i = 0; i < argc; ++i) {
		size_t size;
		const char *utf8 = JS_ToCStringLen(ctx, &size, *(argv + i));
		args.write()[i].parse_utf8(utf8, size);
		JS_FreeCString(ctx, utf8);
	}
	print_line(args.join(" "));
	return JS_UNDEFINED;
}

void QuickJSBinder::add_global_console() {
	JSValue console = JS_NewObject(ctx);
	JSValue log = JS_NewCFunction(ctx, console_log_function, "log", 0);
	JS_DefinePropertyValueStr(ctx, global_object, "console", console, PROP_DEF_DEFAULT);
	JS_DefinePropertyValueStr(ctx, console, "log", log, PROP_DEF_DEFAULT);
}

JSValue QuickJSBinder::object_method(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv, int method_id) {

	ECMAScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, this_val);
	ERR_FAIL_NULL_V(bind, JS_ThrowReferenceError(ctx, "Call native method without native binding data"));
	ERR_FAIL_NULL_V(bind->godot_object, JS_ThrowReferenceError(ctx, "Call native method without native object caller"));

	Object *obj = bind->get_godot_object();
	MethodBind *mb = get_context_binder(ctx)->godot_methods[method_id];

	if (!mb->is_vararg()) {
		argc = MIN(argc, mb->get_argument_count());
	}

	const Variant **args = memnew_arr(const Variant *, argc);
	Vector<Variant> vargs;
	vargs.resize(argc);
	for (int i = 0; i < argc; ++i) {
		vargs.write[i] = var_to_variant(ctx, *(argv + i));
		args[i] = (vargs.ptr() + i);
	}

	Variant::CallError call_err;
	Variant ret_val = mb->call(obj, args, argc, call_err);
	if (args != NULL) {
		memdelete_arr(args);
	}

	CharString err_message;
	switch (call_err.error) {
		case Variant::CallError::CALL_ERROR_INVALID_ARGUMENT: {
			String err = "Invalid type for argument #" + itos(call_err.error) + ", expected '" + Variant::get_type_name(call_err.expected) + "'.";
			err_message = err.utf8();
		} break;
		case Variant::CallError::CALL_ERROR_INVALID_METHOD: {
			String err = "Invalid method '" + String(mb->get_name()) + "' for type '" + obj->get_class_name() + "'.";
			err_message = err.utf8();
		} break;
		case Variant::CallError::CALL_ERROR_TOO_MANY_ARGUMENTS: {
			String err = "Too many arguments for method '" + String(mb->get_name()) + "'";
			err_message = err.utf8();
		} break;
		default: {
		}
	}
	ERR_FAIL_COND_V((call_err.error != Variant::CallError::CALL_OK), JS_ThrowTypeError(ctx, err_message.ptr()));
	return variant_to_var(ctx, ret_val);
}

JSValue QuickJSBinder::variant_to_var(JSContext *ctx, const Variant p_var) {
	switch (p_var.get_type()) {
		case Variant::BOOL:
			return ((bool)p_var) ? JS_TRUE : JS_FALSE;
		case Variant::INT:
			return JS_NewInt32(ctx, int32_t(p_var));
		case Variant::REAL:
			return JS_NewFloat64(ctx, (double)(p_var));
		case Variant::STRING:
			return godot_string_to_jsvalue(ctx, p_var);
		case Variant::OBJECT: {
			Object *obj = p_var;
			ECMAScriptGCHandler *data = BINDING_DATA_FROM_GD(obj);
			ERR_FAIL_NULL_V(data, JS_UNDEFINED);
			ERR_FAIL_NULL_V(data->ecma_object, JS_UNDEFINED);
			JSValue js_obj = JS_MKPTR(JS_TAG_OBJECT, data->ecma_object);
			JS_DupValue(ctx, js_obj);
			return js_obj;
		}
		case Variant::ARRAY: {
			Array arr = p_var;
			JSValue js_arr = JS_NewArray(ctx);
			for (uint32_t i = 0; i < arr.size(); i++) {
				JS_SetPropertyUint32(ctx, js_arr, i, variant_to_var(ctx, arr[int(i)]));
			}
			return js_arr;
		} break;
		case Variant::DICTIONARY: {
			Dictionary dict = p_var;
			JSValue obj = JS_NewObject(ctx);
			// TODO
			return obj;
		} break;
		case Variant::NIL:
			return JS_NULL;
		default:
			return QuickJSBuiltinBinder::new_object_from(ctx, p_var);
	}
}

Variant QuickJSBinder::var_to_variant(JSContext *ctx, JSValue p_val) {

	switch (JS_VALUE_GET_TAG(p_val)) {
		case JS_TAG_INT:
			return Variant(JS_VALUE_GET_INT(p_val));
		case JS_TAG_BOOL:
			return Variant(bool(JS_VALUE_GET_INT(p_val)));
		case JS_TAG_FLOAT64:
			return Variant(real_t(JS_VALUE_GET_FLOAT64(p_val)));
		case JS_TAG_STRING: {
			return js_string_to_godot_string(ctx, p_val);
		}
		case JS_TAG_OBJECT: {
			if (int length = get_js_array_length(ctx, p_val) >= 0) { // Array
				Array arr;
				arr.resize(length);
				for (uint32_t i = 0; i < length; i++) {
					JSValue val = JS_GetPropertyUint32(ctx, p_val, i);
					arr[int(i)] = var_to_variant(ctx, val);
					JS_FreeValue(ctx, val);
				}
				return arr;
			} else if (ECMAScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, p_val)) { // Binding object
				ERR_FAIL_NULL_V(bind, Variant());
				ERR_FAIL_NULL_V(bind->godot_object, Variant());
				return bind->get_value();
			} else { // Plain Object
				Dictionary dict;
				// TODO
				return dict;
			}
		}
		default:
			return Variant();
	}
}

JSAtom QuickJSBinder::get_atom(JSContext *ctx, const StringName &p_key) {
	String name = p_key;
	CharString name_str = name.utf8();
	JSAtom atom = JS_NewAtom(ctx, name_str.ptr());
	return atom;
}

JSValue QuickJSBinder::godot_string_to_jsvalue(JSContext *ctx, const String &text) {
	CharString utf8 = text.utf8();
	return JS_NewStringLen(ctx, utf8.ptr(), utf8.length());
}

String QuickJSBinder::js_string_to_godot_string(JSContext *ctx, JSValue p_val) {
	String ret;
	size_t len = 0;
	const char *utf8 = JS_ToCStringLen(ctx, &len, p_val);
	ret.parse_utf8(utf8, len);
	JS_FreeCString(ctx, utf8);
	return ret;
}

JSValue QuickJSBinder::godot_to_string(JSContext *ctx, JSValue this_val, int argc, JSValue *argv) {
	String str = var_to_variant(ctx, this_val);
	CharString utf8 = str.utf8();
	return JS_NewStringLen(ctx, utf8.ptr(), utf8.length());
}

JSClassID QuickJSBinder::register_class(const ClassDB::ClassInfo *p_cls) {

	ClassBindData data;
	data.class_id = 0;
	data.base_class = NULL;
	data.class_name = String(p_cls->name).utf8();
	data.jsclass.class_name = data.class_name.ptr();
	data.jsclass.exotic = NULL;
	data.jsclass.gc_mark = NULL;
	data.jsclass.call = NULL;
	data.gdclass = p_cls;

	data.prototype = JS_NewObject(ctx);

	// methods
	Map<StringName, JSValue> methods;
	{
		godot_methods.resize(internal_godot_method_id + p_cls->method_map.size());
		const StringName *key = p_cls->method_map.next(NULL);
		while (key) {
			MethodBind *mb = p_cls->method_map.get(*key);
			godot_methods.write[internal_godot_method_id] = mb;

			CharString name = String(*key).utf8();
			JSValue method = JS_NewCFunctionMagic(ctx, &QuickJSBinder::object_method, name.ptr(), mb->get_argument_count(), JS_CFUNC_generic_magic, internal_godot_method_id);
			JS_DefinePropertyValueStr(ctx, data.prototype, name.ptr(), method, PROP_DEF_DEFAULT);

			methods.insert(*key, method);

			++internal_godot_method_id;
			key = p_cls->method_map.next(key);
		}

		if (p_cls->name == "Object") {
			// toString()
			JSValue to_string_func = JS_NewCFunction(ctx, godot_to_string, TO_STRING_LITERAL, 0);
			JS_DefinePropertyValueStr(ctx, data.prototype, TO_STRING_LITERAL, to_string_func, PROP_DEF_DEFAULT);
			// free()
			const char *free_func_name = "free";
			JSValue free_func = JS_NewCFunction(ctx, object_free, free_func_name, 0);
			JS_DefinePropertyValueStr(ctx, data.prototype, free_func_name, free_func, PROP_DEF_DEFAULT);
		}
	}

	// properties
	{
		const StringName *key = p_cls->property_setget.next(NULL);
		while (key) {
			const StringName &prop_name = *key;
			const ClassDB::PropertySetGet &prop = p_cls->property_setget[prop_name];
			JSValue *setter = NULL;
			JSValue *getter = NULL;

			if (Map<StringName, JSValue>::Element *E = methods.find(prop.setter)) {
				setter = &(E->get());
				JS_DupValue(ctx, *setter);
			}
			if (Map<StringName, JSValue>::Element *E = methods.find(prop.getter)) {
				getter = &(E->get());
				JS_DupValue(ctx, *getter);
			}
			JSAtom atom = get_atom(ctx, prop_name);
			JS_DefinePropertyGetSet(ctx, data.prototype, atom, getter ? *getter : JS_UNDEFINED, setter ? *setter : JS_UNDEFINED, PROP_DEF_DEFAULT);
			JS_FreeAtom(ctx, atom);
			key = p_cls->property_setget.next(key);
		}
	}

	JS_NewClassID(&data.class_id);
	JS_NewClass(JS_GetRuntime(ctx), data.class_id, &data.jsclass);
	JS_SetClassProto(ctx, data.class_id, data.prototype);

	data.constructor = JS_NewCFunctionMagic(ctx, object_constructor, data.jsclass.class_name, data.class_name.size(), JS_CFUNC_constructor_magic, (int)data.class_id);
	JS_SetConstructor(ctx, data.constructor, data.prototype);
	JS_DefinePropertyValue(ctx, data.prototype, js_key_godot_classid, JS_NewInt32(ctx, data.class_id), PROP_DEF_DEFAULT);
	JS_DefinePropertyValue(ctx, data.constructor, js_key_godot_classid, JS_NewInt32(ctx, data.class_id), PROP_DEF_DEFAULT);

	// constants
	{
		const StringName *key = p_cls->constant_map.next(NULL);
		while (key) {

			int value = p_cls->constant_map.get(*key);
			JSAtom atom = get_atom(ctx, *key);
			JS_DefinePropertyValue(ctx, data.constructor, atom, JS_NewInt32(ctx, value), PROP_DEF_DEFAULT);
			JS_FreeAtom(ctx, atom);

			key = p_cls->constant_map.next(key);
		}
	}

	// enumeration
	{
		const StringName *key = p_cls->enum_map.next(NULL);
		while (key) {

			JSValue enum_obj = JS_NewObject(ctx);
			JSAtom atom = get_atom(ctx, *key);

			const List<StringName> &const_keys = p_cls->enum_map.get(*key);
			for (const List<StringName>::Element *E = const_keys.front(); E; E = E->next()) {
				int value = p_cls->constant_map.get(E->get());
				JSAtom atom_key = get_atom(ctx, E->get());
				JS_DefinePropertyValue(ctx, enum_obj, atom_key, JS_NewInt32(ctx, value), PROP_DEF_DEFAULT);
				JS_FreeAtom(ctx, atom_key);
			}

			JS_DefinePropertyValue(ctx, data.constructor, atom, enum_obj, PROP_DEF_DEFAULT);
			JS_FreeAtom(ctx, atom);

			key = p_cls->enum_map.next(key);
		}
	}

	// signals
	{
		const StringName *key = p_cls->signal_map.next(NULL);
		while (key) {

			JSAtom atom = get_atom(ctx, *key);
			JS_DefinePropertyValue(ctx, data.constructor, atom, godot_string_to_jsvalue(ctx, *key), PROP_DEF_DEFAULT);
			JS_FreeAtom(ctx, atom);

			key = p_cls->signal_map.next(key);
		}
	}

	class_bindings.set(data.class_id, data);
	classname_bindings.set(p_cls->name, class_bindings.getptr(data.class_id));

	return data.class_id;
}

void QuickJSBinder::add_godot_origin() {
	godot_origin_class.class_id = 0;
	godot_origin_class.class_name = "GodotOrigin";
	godot_origin_class.jsclass.class_name = godot_origin_class.class_name.ptr();
	godot_origin_class.jsclass.finalizer = origin_finalizer;
	godot_origin_class.jsclass.exotic = NULL;
	godot_origin_class.jsclass.call = NULL;
	godot_origin_class.jsclass.gc_mark = NULL;
	godot_origin_class.prototype = JS_NewObject(ctx);
	godot_origin_class.gdclass = NULL;
	godot_origin_class.base_class = NULL;
	JS_NewClassID(&godot_origin_class.class_id);
	JS_NewClass(JS_GetRuntime(ctx), godot_origin_class.class_id, &godot_origin_class.jsclass);
	JS_SetClassProto(ctx, godot_origin_class.class_id, godot_origin_class.prototype);
	godot_origin_class.constructor = JS_NewCFunction2(ctx, js_empty_consturctor, godot_origin_class.jsclass.class_name, 0, JS_CFUNC_constructor, 0);
	JS_SetConstructor(ctx, godot_origin_class.constructor, godot_origin_class.prototype);
	JS_DefinePropertyValueStr(ctx, godot_object, godot_origin_class.class_name, godot_origin_class.constructor, PROP_DEF_DEFAULT);
}

void QuickJSBinder::add_godot_classes() {

	Map<const ClassDB::ClassInfo *, JSClassID> gdclass_jsmap;
	// register classes
	const StringName *key = ClassDB::classes.next(NULL);
	while (key) {
		const ClassDB::ClassInfo *cls = ClassDB::classes.getptr(*key);
		gdclass_jsmap.insert(cls, register_class(cls));
		key = ClassDB::classes.next(key);
	}

	// Setup inherits chain
	for (Map<const ClassDB::ClassInfo *, JSClassID>::Element *E = gdclass_jsmap.front(); E; E = E->next()) {
		const ClassDB::ClassInfo *gdcls = E->key();
		ClassBindData &bind = class_bindings.get(E->get());
		if (gdcls->inherits_ptr) {
			if (Map<const ClassDB::ClassInfo *, JSClassID>::Element *base = gdclass_jsmap.find(gdcls->inherits_ptr)) {
				bind.base_class = class_bindings.getptr(base->get());
			}
		}
	}

	// Setup the prototype chain
	const JSClassID *id = class_bindings.next(NULL);
	while (id) {

		const ClassBindData &data = class_bindings.get(*id);
		JS_DefinePropertyValueStr(ctx, godot_object, data.jsclass.class_name, data.constructor, PROP_DEF_DEFAULT);
		if (data.base_class) {
			JS_SetPrototype(ctx, data.prototype, data.base_class->prototype);
		} else {
			JS_SetPrototype(ctx, data.prototype, godot_origin_class.prototype);
		}

		id = class_bindings.next(id);
	}
}

void QuickJSBinder::add_godot_globals() {
	JSValue js_func_register_class = JS_NewCFunction(ctx, godot_register_emca_class, "register_class", 4);
	JS_DefinePropertyValueStr(ctx, godot_object, "register_class", js_func_register_class, PROP_DEF_DEFAULT);

	// Singletons
	List<Engine::Singleton> singletons;
	Engine::get_singleton()->get_singletons(&singletons);
	for (List<Engine::Singleton>::Element *E = singletons.front(); E; E = E->next()) {
		const Engine::Singleton &s = E->get();

		ERR_CONTINUE(s.ptr == NULL);

		const ClassBindData **cls_ptr = classname_bindings.getptr(s.ptr->get_class_name());
		ERR_CONTINUE(cls_ptr == NULL);
		const ClassBindData *cls = *cls_ptr;

		JSValue obj = JS_NewObjectProtoClass(ctx, cls->prototype, get_origin_class_id());
		ECMAScriptGCHandler *data = memnew(ECMAScriptGCHandler);
		data->ecma_object = JS_VALUE_GET_PTR(obj);
		data->godot_object = s.ptr;
		data->type = Variant::OBJECT;
		data->flags = ECMAScriptGCHandler::FLAG_OBJECT;
		JS_SetOpaque(obj, data);

		JSAtom singleton_name = get_atom(ctx, s.name);
		JSValue last_obj = JS_GetProperty(ctx, godot_object, singleton_name);
		if (!JS_IsUndefined(last_obj)) {
			JSValue prototype = JS_GetPrototype(ctx, last_obj);
			JS_SetPrototype(ctx, obj, prototype);
			JS_FreeValue(ctx, last_obj);
		}

		JS_DupValue(ctx, obj);
		JS_DefinePropertyValue(ctx, godot_object, singleton_name, obj, QuickJSBinder::PROP_DEF_DEFAULT);
		JS_FreeAtom(ctx, singleton_name);
		godot_singletons.push_back(obj);

		if (ClassDB::ClassInfo *cls = ClassDB::classes.getptr(s.ptr->get_class_name())) {
			// constants
			for (const StringName *const_key = cls->constant_map.next(NULL); const_key; const_key = cls->constant_map.next(const_key)) {
				JSAtom const_name = get_atom(ctx, *const_key);
				JS_DefinePropertyValue(ctx, obj, const_name, variant_to_var(ctx, cls->constant_map.get(*const_key)), QuickJSBinder::PROP_DEF_DEFAULT);
				JS_FreeAtom(ctx, const_name);
			}
			// enumrations
			for (const StringName *enum_key = cls->enum_map.next(NULL); enum_key; enum_key = cls->enum_map.next(enum_key)) {
				const List<StringName> &consts = cls->enum_map.get(*enum_key);
				JSValue enum_obj = JS_NewObject(ctx);
				for (const List<StringName>::Element *E = consts.front(); E; E = E->next()) {
					JSAtom const_name = get_atom(ctx, E->get());
					JS_DefinePropertyValue(ctx, enum_obj, const_name, variant_to_var(ctx, cls->constant_map.get(E->get())), QuickJSBinder::PROP_DEF_DEFAULT);
					JS_FreeAtom(ctx, const_name);
				}
				JSAtom enum_name = get_atom(ctx, *enum_key);
				JS_DefinePropertyValue(ctx, obj, enum_name, enum_obj, QuickJSBinder::PROP_DEF_DEFAULT);
				JS_FreeAtom(ctx, enum_name);
			}
		}
	}

	// global constants
	HashMap<StringName, HashMap<StringName, int> > global_constants;
	for (int i = 0; i < GlobalConstants::get_global_constant_count(); ++i) {

		StringName enum_name = GlobalConstants::get_global_constant_enum(i);
		const char *const_name = GlobalConstants::get_global_constant_name(i);
		const int value = GlobalConstants::get_global_constant_value(i);

		JSAtom js_const_name = JS_NewAtom(ctx, const_name);
		JS_DefinePropertyValue(ctx, godot_object, js_const_name, JS_MKVAL(JS_TAG_INT, value), QuickJSBinder::PROP_DEF_DEFAULT);
		JS_FreeAtom(ctx, js_const_name);

		if (HashMap<StringName, int> *consts = global_constants.getptr(enum_name)) {
			consts->set(const_name, value);
		} else {
			HashMap<StringName, int> enum_;
			enum_.set(const_name, value);
			global_constants.set(enum_name, enum_);
		}
	}

	// global enums
	for (const StringName *enum_name = global_constants.next(NULL); enum_name; enum_name = global_constants.next(enum_name)) {
		if (String(*enum_name).empty()) continue;

		JSAtom atom_enum_name = get_atom(ctx, *enum_name);
		JSValue enum_object = JS_NewObject(ctx);
		JS_DefinePropertyValue(ctx, godot_object, atom_enum_name, enum_object, QuickJSBinder::PROP_DEF_DEFAULT);
		JS_FreeAtom(ctx, atom_enum_name);

		const HashMap<StringName, int> &enum_ = global_constants.get(*enum_name);
		for (const StringName *const_name = enum_.next(NULL); const_name; const_name = enum_.next(const_name)) {
			JSAtom js_const_name = get_atom(ctx, *const_name);
			const int value = enum_.get(*const_name);
			JS_DefinePropertyValue(ctx, enum_object, js_const_name, JS_MKVAL(JS_TAG_INT, value), QuickJSBinder::PROP_DEF_DEFAULT);
			JS_FreeAtom(ctx, js_const_name);
		}
	}

	// buitin functions
	for (int i = 0; i < Expression::FUNC_MAX; ++i) {
		Expression::BuiltinFunc func = (Expression::BuiltinFunc)i;
		String name = Expression::get_func_name(func);
		JSValue js_func = JS_NewCFunctionMagic(ctx, godot_builtin_function, name.utf8().ptr(), 0, JS_CFUNC_generic_magic, i);
		JSAtom atom = get_atom(ctx, name);
		JS_DefinePropertyValue(ctx, godot_object, atom, js_func, QuickJSBinder::PROP_DEF_DEFAULT);
		JS_FreeAtom(ctx, atom);
	}
}

QuickJSBinder::QuickJSBinder() {
	internal_godot_method_id = 0;
	godot_allocator.js_malloc = QuickJSBinder::js_malloc;
	godot_allocator.js_free = QuickJSBinder::js_free;
	godot_allocator.js_realloc = QuickJSBinder::js_realloc;
	godot_allocator.js_malloc_usable_size = NULL;
}

void QuickJSBinder::initialize() {
	// create runtime and context for the binder
	runtime = JS_NewRuntime2(&godot_allocator, this);
	ctx = JS_NewContext(runtime);
	context_binders.set(ctx, this);
	runtime_context_map.set(runtime, ctx);

	empty_function = JS_NewCFunction(ctx, js_empty_func, "virtual_fuction", 0);
	// global = globalThis
	global_object = JS_GetGlobalObject(ctx);
	// global.console
	add_global_console();
	// global.godot
	godot_object = JS_NewObject(ctx);
	js_key_godot_classid = JS_NewAtom(ctx, JS_HIDDEN_SYMBOL("cls"));
	JS_DefinePropertyValueStr(ctx, global_object, "godot", godot_object, PROP_DEF_DEFAULT);
	// godot.GodotOrigin
	add_godot_origin();
	// godot.Vector2 godot.Color ...
	builtin_binder.initialize(ctx, this);
	// godot.Object godot.Node godot.Theme ...
	add_godot_classes();
	// godot.print godot.sin ...
	add_godot_globals();
}

void QuickJSBinder::uninitialize() {

	builtin_binder.uninitialize();

	// Free singletons
	for (int i = 0; i < godot_singletons.size(); i++) {
		ECMAScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, godot_singletons[i]);
		if (bind) {
			memdelete(bind);
			JS_SetOpaque(godot_singletons[i], NULL);
		}
		JS_FreeValue(ctx, godot_singletons[i]);
	}

	// Free registered js classes
	const StringName *key = ecma_classes.next(NULL);
	while (key) {
		const ECMAClassInfo &ecma_class = ecma_classes.get(*key);
		JSValue class_func = JS_MKPTR(JS_TAG_OBJECT, ecma_class.ecma_class_function.ecma_object);
		JS_FreeValue(ctx, class_func);
		key = ecma_classes.next(key);
	}
	JS_FreeAtom(ctx, js_key_godot_classid);
	JS_FreeValue(ctx, empty_function);
	JS_FreeValue(ctx, global_object);
	JS_FreeContext(ctx);
	JS_FreeRuntime(runtime);

	context_binders.erase(ctx);
	runtime_context_map.erase(runtime);
	ctx = NULL;
	runtime = NULL;
}

Error QuickJSBinder::eval_string(const String &p_source) {
	String error;
	return safe_eval_text(p_source, error);
}

Error QuickJSBinder::safe_eval_text(const String &p_source, String &r_error) {
	CharString utf8_str = p_source.utf8();
	JSValue ret = JS_Eval(ctx, utf8_str.ptr(), utf8_str.size(), "", JS_EVAL_TYPE_GLOBAL | JS_EVAL_FLAG_STRICT);
	if (JS_IsException(ret)) {
		JSValue e = JS_GetException(ctx);
		const char *message = JS_ToCString(ctx, e);
		r_error = message;
		JS_FreeCString(ctx, message);
		JS_FreeValue(ctx, e);
	}
	return OK;
}

/************************* Memory Management ******************************/

void *QuickJSBinder::alloc_object_binding_data(Object *p_object) {
	if (const ClassBindData **bind_ptr = classname_bindings.getptr(p_object->get_class_name())) {
		JSValue obj = JS_NewObjectProtoClass(ctx, (*bind_ptr)->prototype, get_origin_class_id());
		ECMAScriptGCHandler *data = memnew(ECMAScriptGCHandler);
		data->ecma_object = JS_VALUE_GET_PTR(obj);
		data->godot_object = p_object;
		data->type = Variant::OBJECT;
		data->flags = ECMAScriptGCHandler::FLAG_OBJECT;
		if (Reference *ref = Object::cast_to<Reference>(p_object)) {
			data->flags |= ECMAScriptGCHandler::FLAG_REFERENCE;
			ref->reference();
			union {
				REF *ref;
				struct {
					Reference *ref;
				} * r;
			} u;
			u.ref = memnew(REF);
			u.r->ref = ref;
			data->godot_reference = u.ref;
		}
		JS_SetOpaque(obj, data);
		return data;
	}
	return NULL;
}

void QuickJSBinder::free_object_binding_data(void *p_gc_handle) {
	ECMAScriptGCHandler *bind = (ECMAScriptGCHandler *)p_gc_handle;
	if (bind->is_object()) {
		JSValue js_obj = JS_MKPTR(JS_TAG_OBJECT, bind->ecma_object);
		JS_FreeValue(ctx, js_obj);
	}
	memdelete(bind);
}

void QuickJSBinder::godot_refcount_incremented(Reference *p_object) {
	ECMAScriptGCHandler *bind = BINDING_DATA_FROM_GD(p_object);
	JSValue js_obj = JS_MKPTR(JS_TAG_OBJECT, bind->ecma_object);
	if (!(bind->flags & ECMAScriptGCHandler::FLAG_HOLDING_SCRIPT_REF)) {
		JS_DupValue(ctx, js_obj); // JS ref_count ++
		bind->flags |= ECMAScriptGCHandler::FLAG_HOLDING_SCRIPT_REF;
	}
}

bool QuickJSBinder::godot_refcount_decremented(Reference *p_object) {
	ECMAScriptGCHandler *bind = BINDING_DATA_FROM_GD(p_object);
	if (bind->flags & ECMAScriptGCHandler::FLAG_HOLDING_SCRIPT_REF) {
		bind->flags ^= ECMAScriptGCHandler::FLAG_HOLDING_SCRIPT_REF;
		if (!(bind->flags & ECMAScriptGCHandler::FLAG_SCRIPT_FINALIZED)) {
			JS_FreeValue(ctx, JS_MKPTR(JS_TAG_OBJECT, bind->ecma_object));
			return false;
		}
	}
	return true;
}

JSValue QuickJSBinder::object_constructor(JSContext *ctx, JSValueConst new_target, int argc, JSValueConst *argv, int class_id) {
	const ClassBindData &cls = QuickJSBinder::get_context_binder(ctx)->class_bindings.get(class_id);
	ECMAScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, new_target);
	JSValue js_obj;
	if (bind) {
		bind->flags |= ECMAScriptGCHandler::FLAG_FROM_NATIVE;
		js_obj = new_target;
	} else {
		Object *gd_obj = cls.gdclass->creation_func();
		bind = BINDING_DATA_FROM_GD(gd_obj);
		bind->flags |= ECMAScriptGCHandler::FLAG_FROM_SCRIPT;
		js_obj = JS_MKPTR(JS_TAG_OBJECT, bind->ecma_object);

		if (bind->is_reference()) {
			bind->godot_reference->ptr()->unreference();
		} else if (bind->is_object()) {
			JS_DupValue(ctx, js_obj);
		}
	}

	return js_obj;
}

void QuickJSBinder::object_finalizer(ECMAScriptGCHandler *p_bind) {
	if (p_bind->is_reference()) {
		memdelete(p_bind->godot_reference);
		p_bind->flags ^= (ECMAScriptGCHandler::FLAG_OBJECT | ECMAScriptGCHandler::FLAG_REFERENCE);
		p_bind->clear();
	}
}

void QuickJSBinder::origin_finalizer(JSRuntime *rt, JSValue val) {
	JSContext *ctx = runtime_context_map.get(rt);
	ECMAScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, val);
	if (bind) {
		bind->flags |= ECMAScriptGCHandler::FLAG_SCRIPT_FINALIZED;
		if (bind->type == Variant::OBJECT) {
			object_finalizer(bind);
		} else {
			QuickJSBuiltinBinder::builtin_finalizer(bind);
		}
	}
}

JSValue QuickJSBinder::object_free(JSContext *ctx, JSValue this_val, int argc, JSValue *argv) {

	ECMAScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, this_val);
	ERR_FAIL_NULL_V(bind, JS_ThrowReferenceError(ctx, "The object already be freed"));
	ERR_FAIL_NULL_V(bind->godot_object, JS_ThrowReferenceError(ctx, "The object already be freed"));
	ERR_FAIL_COND_V((bind->is_reference()), JS_ThrowReferenceError(ctx, "Call free to Reference object is not allowed"));

	memdelete(bind->godot_object);
	JS_SetOpaque(this_val, NULL);
	return JS_UNDEFINED;
}

JSValue QuickJSBinder::godot_builtin_function(JSContext *ctx, JSValue this_val, int argc, JSValue *argv, int magic) {

	Variant ret;
	Variant::CallError err;
	String err_msg;

	const Variant **args = memnew_arr(const Variant *, argc);
	Vector<Variant> vargs;
	vargs.resize(argc);
	for (int i = 0; i < argc; ++i) {
		vargs.write[i] = var_to_variant(ctx, argv[0]);
		args[i] = (vargs.ptr() + i);
	}

	Expression::BuiltinFunc func = (Expression::BuiltinFunc)magic;
	Expression::exec_func(func, args, &ret, err, err_msg);
	if (args != NULL) {
		memdelete_arr(args);
	}

	if (err.error != Variant::CallError::CALL_OK) {
		CharString msg = err_msg.utf8();
		return JS_ThrowTypeError(ctx, msg.ptr());
	}

	return variant_to_var(ctx, ret);
}

/************************* END Memory Management ******************************/

/********************************* Script --> C++ ****************************/
JSValue QuickJSBinder::godot_register_emca_class(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
	ERR_FAIL_COND_V(!JS_IsFunction(ctx, *argv), JS_ThrowTypeError(ctx, "Class function expected"));
	QuickJSBinder *binder = get_context_binder(ctx);
	JSValue prototype = JS_GetProperty(ctx, *argv, QuickJSBinder::JS_ATOM_prototype);
	JSValue classid = JS_GetProperty(ctx, prototype, binder->js_key_godot_classid);
	JSClassID id = JS_VALUE_GET_INT(classid);

	if (const ClassBindData *bind = binder->class_bindings.getptr(id)) {

		ECMAClassInfo ecma_class;
		ecma_class.native_class = bind->gdclass;

		// Add reference to the class function
		ecma_class.ecma_class_function.ecma_object = JS_VALUE_GET_PTR(*argv);
		JS_DupValue(ctx, *argv);

		ecma_class.ecma_prototype.ecma_object = JS_VALUE_GET_PTR(prototype);

		StringName class_name;
		if (argc > 1 && JS_IsString(*(argv + 1))) {
			class_name = js_string_to_godot_string(ctx, *(argv + 1));
		} else {
			JSValue name = JS_GetProperty(ctx, *argv, JS_ATOM_name);
			class_name = js_string_to_godot_string(ctx, name);
			JS_FreeValue(ctx, name);
		}
		ecma_class.class_name = class_name;

		// TODO
		ecma_class.icon_path = "";
		ecma_class.tool = false;
		binder->ecma_classes.set(class_name, ecma_class);

		Ref<ECMAScript> script;
		script.instance();
		script->class_name = class_name;
		script->library = ECMAScriptLibraryResourceLoader::get_loading_library();
		ECMAScriptLanguage::get_singleton()->script_classes.set(class_name, script);
	}

	JS_FreeValue(ctx, prototype);
	JS_FreeValue(ctx, classid);

	return JS_UNDEFINED;
}

int QuickJSBinder::get_js_array_length(JSContext *ctx, JSValue p_val) {
	if (!JS_IsArray(ctx, p_val)) return -1;
	JSValue ret = JS_GetProperty(ctx, p_val, JS_ATOM_length);
	int32_t length = -1;
	if (JS_ToInt32(ctx, &length, ret)) return -1;
	return length;
}

void QuickJSBinder::get_own_property_names(JSContext *ctx, JSValue p_object, Set<StringName> *r_list) {
	ERR_FAIL_COND(!JS_IsObject(p_object));
	JSPropertyEnum *props = NULL;
	uint32_t tab_atom_count;
	JS_GetOwnPropertyNames(ctx, &props, &tab_atom_count, p_object, JS_GPN_STRING_MASK | JS_GPN_SYMBOL_MASK);
	for (uint32_t i = 0; i < tab_atom_count; i++) {
		JSValue key = JS_AtomToValue(ctx, props[i].atom);
		String name = js_string_to_godot_string(ctx, key);
		r_list->insert(name);
		JS_FreeAtom(ctx, props[i].atom);
	}
	js_free_rt(JS_GetRuntime(ctx), props);
}

ECMAScriptGCHandler QuickJSBinder::create_ecma_instance_for_godot_object(const StringName &ecma_class_name, Object *p_object) {

	ERR_FAIL_NULL_V(p_object, ECMAScriptGCHandler());

	ECMAClassInfo *ecma_class_info = ecma_classes.getptr(ecma_class_name);
	ERR_FAIL_NULL_V(ecma_class_info, ECMAScriptGCHandler());

	ECMAScriptGCHandler *bind = BINDING_DATA_FROM_GD(p_object);
	ERR_FAIL_NULL_V(bind, ECMAScriptGCHandler());

	JSValue constructor = JS_MKPTR(JS_TAG_OBJECT, ecma_class_info->ecma_class_function.ecma_object);
	JSValue object = JS_MKPTR(JS_TAG_OBJECT, bind->ecma_object);
	JS_SetPrototype(ctx, object, JS_MKPTR(JS_TAG_OBJECT, ecma_class_info->ecma_prototype.ecma_object));
	JS_CallConstructor2(ctx, constructor, object, 0, NULL);

	return *bind;
}

Variant QuickJSBinder::call_method(const ECMAScriptGCHandler &p_object, const StringName &p_method, const Variant **p_args, int p_argcount, Variant::CallError &r_error) {
	Vector<JSValue> argv;
	argv.resize(p_argcount);
	for (int i = 0; i < p_argcount; ++i) {
		argv.write[i] = variant_to_var(ctx, *p_args[i]);
	}

	JSValue object = GET_JSVALUE(p_object);
	JSAtom atom = get_atom(ctx, p_method);
	JSValue method = JS_GetProperty(ctx, object, atom);
	JS_FreeAtom(ctx, atom);

	if (!JS_IsFunction(ctx, method)) {
		r_error.error = Variant::CallError::CALL_ERROR_INVALID_METHOD;
		JS_FreeValue(ctx, method);
		return Variant();
	}

	JSValue return_val = JS_Call(ctx, method, object, argv.size(), argv.ptrw());

	if (JS_IsException(return_val)) {
		r_error.error = Variant::CallError::CALL_ERROR_INVALID_METHOD;
		JSValue exception = JS_GetException(ctx);
		JSValue message = JS_GetPropertyStr(ctx, exception, "message");
		print_error("Failed to call " + p_method + ": " + js_string_to_godot_string(ctx, message));
		JS_FreeValue(ctx, message);
		JS_FreeValue(ctx, exception);
	}
	Variant ret = var_to_variant(ctx, return_val);
	JS_FreeValue(ctx, return_val);
	JS_FreeValue(ctx, method);

	return ret;
}

bool QuickJSBinder::get_instance_property(const ECMAScriptGCHandler &p_object, const StringName &p_name, Variant &r_ret) {
	bool success = false;
	JSValue obj = GET_JSVALUE(p_object);
	JSAtom atom = get_atom(ctx, p_name);
	JSValue ret = JS_GetProperty(ctx, obj, atom);
	JS_FreeAtom(ctx, atom);
	r_ret = var_to_variant(ctx, ret);
	success = !JS_IsUndefined(ret);
	JS_FreeValue(ctx, ret);
	return success;
}

bool QuickJSBinder::set_instance_property(const ECMAScriptGCHandler &p_object, const StringName &p_name, const Variant &p_value) {
	JSValue obj = GET_JSVALUE(p_object);
	JSAtom atom = get_atom(ctx, p_name);
	bool success = JS_SetProperty(ctx, obj, atom, variant_to_var(ctx, p_value));
	JS_FreeAtom(ctx, atom);
	return success;
}

bool QuickJSBinder::has_method(const ECMAScriptGCHandler &p_object, const StringName &p_name) {
	JSValue obj = GET_JSVALUE(p_object);
	ERR_FAIL_COND_V(!JS_IsObject(obj), false);
	JSAtom atom = get_atom(ctx, p_name);
	JSValue value = JS_GetProperty(ctx, GET_JSVALUE(p_object), atom);
	bool success = JS_IsFunction(ctx, value);
	JS_FreeAtom(ctx, atom);
	JS_FreeValue(ctx, value);
	return success;
}

/**************************** END Script --> C++ ******************************/
