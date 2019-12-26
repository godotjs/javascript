#include "quickjs_binder.h"
#include "../ecmascript_instance.h"
#include "../ecmascript_language.h"
#include "core/engine.h"
#include "core/global_constants.h"
#include "core/io/json.h"
#include "core/math/expression.h"
#include "core/os/file_access.h"
#include "core/os/os.h"
#include "quickjs_binder.h"

uint16_t QuickJSBinder::global_context_id = 0;
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

	Variant *args = memnew_arr(Variant, argc);
	const Variant **argsptr = memnew_arr(const Variant *, argc);
	for (int i = 0; i < argc; ++i) {
		args[i] = var_to_variant(ctx, argv[i]);
		argsptr[i] = &(args[i]);
	}

	Variant::CallError call_err;
	Variant ret_val = mb->call(obj, argsptr, argc, call_err);
	if (args != NULL) {
		memdelete_arr(args);
		memdelete_arr(argsptr);
	}
#ifdef DEBUG_METHODS_ENABLED
	CharString err_message;
	switch (call_err.error) {
		case Variant::CallError::CALL_ERROR_INVALID_ARGUMENT: {
			String err = "Invalid type for argument #" + itos(call_err.argument) + ", expected '" + Variant::get_type_name(call_err.expected) + "'.";
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
#endif
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
		case Variant::NODE_PATH:
		case Variant::STRING:
			return to_js_string(ctx, p_var);
		case Variant::OBJECT: {
			Object *obj = p_var;
			ECMAScriptGCHandler *data = BINDING_DATA_FROM_GD(obj);
			ERR_FAIL_NULL_V(data, JS_UNDEFINED);
			ERR_FAIL_NULL_V(data->ecma_object, JS_UNDEFINED);
			QuickJSBinder *binder = get_context_binder(ctx);
			JSValue js_obj = JS_MKPTR(JS_TAG_OBJECT, data->ecma_object);
			if (binder->lastest_allocated_object == data) {
				if (data->is_object()) {
					JS_DupValue(ctx, js_obj);
				}
				binder->lastest_allocated_object = NULL;
			} else {
				JS_DupValue(ctx, js_obj);
			}

			return js_obj;
		}
		case Variant::ARRAY: {
			Array arr = p_var;
			JSValue js_arr = JS_NewArray(ctx);
			for (uint32_t i = 0; i < arr.size(); i++) {
				JS_SetPropertyUint32(ctx, js_arr, i, variant_to_var(ctx, arr[int(i)]));
			}
			return js_arr;
		}
		case Variant::DICTIONARY: {
			Dictionary dict = p_var;
			JSValue obj = JS_NewObject(ctx);
			Array keys = dict.keys();
			for (int i = 0; i < keys.size(); i++) {
				const Variant &key = keys[i];
				const Variant &value = dict[key];
				String key_str = keys[i];
				JS_SetPropertyStr(ctx, obj, key_str.utf8().ptr(), variant_to_var(ctx, value));
			}
			return obj;
		}
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
			return js_to_string(ctx, p_val);
		}
		case JS_TAG_OBJECT: {
			int length = get_js_array_length(ctx, p_val);
			if (length != -1) { // Array
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
			} else { // Plain Object as Dictionary
				Dictionary dict;
				Set<String> keys;
				get_own_property_names(ctx, p_val, &keys);
				for (Set<String>::Element *E = keys.front(); E; E = E->next()) {
					JSValue v = JS_GetPropertyStr(ctx, p_val, E->get().utf8().ptr());
					dict[E->get()] = var_to_variant(ctx, v);
					JS_FreeValue(ctx, v);
				}
				return dict;
			}
		}
		default:
			return Variant();
	}
}

JSValue QuickJSBinder::godot_builtin_function(JSContext *ctx, JSValue this_val, int argc, JSValue *argv, int magic) {

	Variant ret;
	Variant::CallError err;
	String err_msg;

	Expression::BuiltinFunc func = (Expression::BuiltinFunc)magic;
	int arg_required = Expression::get_func_argument_count(func);
	if (argc < arg_required) {
		String func_name = Expression::get_func_name(func);
		return JS_ThrowTypeError(ctx, "%d arguments expected for builtin funtion %s", arg_required, func_name.ascii().ptr());
	}

	Variant *args = memnew_arr(Variant, argc);
	const Variant **argsptr = memnew_arr(const Variant *, argc);
	for (int i = 0; i < argc; ++i) {
		args[i] = var_to_variant(ctx, argv[i]);
		argsptr[i] = &(args[i]);
	}

	Expression::exec_func(func, argsptr, &ret, err, err_msg);
	if (args != NULL) {
		memdelete_arr(args);
		memdelete_arr(argsptr);
	}

	if (err.error != Variant::CallError::CALL_OK) {
		String func_name = Expression::get_func_name(func);
		return JS_ThrowTypeError(ctx, "Call builtin function error %s: %s", func_name.ascii().ptr(), err_msg.utf8().ptr());
	}

	return variant_to_var(ctx, ret);
}

bool QuickJSBinder::validate_type(JSContext *ctx, Variant::Type p_type, JSValueConst &p_val) {
	switch (p_type) {
		case Variant::NIL:
			return JS_IsNull(p_val) || JS_IsUndefined(p_val);
		case Variant::BOOL:
		case Variant::INT:
		case Variant::REAL:
			return JS_IsNumber(p_val) || JS_IsInteger(p_val) || JS_IsBool(p_val) || JS_IsNull(p_val) || JS_IsUndefined(p_val);
		case Variant::STRING:
			return JS_IsString(p_val);
		case Variant::DICTIONARY:
			return JS_IsObject(p_val);
		case Variant::ARRAY:
			return JS_IsArray(ctx, p_val);
		default: {
			ECMAScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, p_val);
			return bind != NULL && bind->type == bind->type;
		}
	}
}

void QuickJSBinder::dump_exception(JSContext *ctx, const JSValue &p_exception, ECMAscriptScriptError *r_error) {
	JSValue err_file = JS_GetProperty(ctx, p_exception, JS_ATOM_fileName);
	JSValue err_line = JS_GetProperty(ctx, p_exception, JS_ATOM_lineNumber);
	JSValue err_msg = JS_GetProperty(ctx, p_exception, JS_ATOM_message);
	JSValue err_stack = JS_GetProperty(ctx, p_exception, JS_ATOM_stack);

	JS_ToInt32(ctx, &r_error->line, err_line);
	r_error->message = js_to_string(ctx, err_msg);
	r_error->file = js_to_string(ctx, err_file);
	r_error->stack.push_back(js_to_string(ctx, err_stack));
	r_error->column = 0;

	JS_FreeValue(ctx, err_file);
	JS_FreeValue(ctx, err_line);
	JS_FreeValue(ctx, err_msg);
	JS_FreeValue(ctx, err_stack);
}

String QuickJSBinder::error_to_string(const ECMAscriptScriptError &p_error) {
	String message = "JavaScript Error";
	if (p_error.stack.size()) {
		message += p_error.stack[0];
	}
	message += p_error.message;
	for (int i = 1; i < p_error.stack.size(); i++) {
		message += p_error.stack[i];
	}
	return message;
}

JSAtom QuickJSBinder::get_atom(JSContext *ctx, const StringName &p_key) {
	String name = p_key;
	CharString name_str = name.ascii();
	JSAtom atom = JS_NewAtom(ctx, name_str.ptr());
	return atom;
}

JSValue QuickJSBinder::godot_to_string(JSContext *ctx, JSValue this_val, int argc, JSValue *argv) {
	String str = var_to_variant(ctx, this_val);
	CharString ascii = str.ascii();
	return JS_NewStringLen(ctx, ascii.ptr(), ascii.length());
}

JSValue QuickJSBinder::godot_get_type(JSContext *ctx, JSValue this_val, int argc, JSValue *argv) {
#ifdef DEBUG_METHODS_ENABLED
	ERR_FAIL_COND_V(argc < 1, JS_ThrowTypeError(ctx, "parameter expected for %s.%s", GODOT_OBJECT_NAME, "get_type"));
#endif
	Variant value;
	if (argc) {
		value = var_to_variant(ctx, argv[0]);
	}
	return JS_NewInt32(ctx, value.get_type());
}

JSValue QuickJSBinder::godot_load(JSContext *ctx, JSValue this_val, int argc, JSValue *argv) {
#ifdef DEBUG_METHODS_ENABLED
	ERR_FAIL_COND_V(argc < 1 || !JS_IsString(argv[0]), JS_ThrowTypeError(ctx, "string expected for %s.%s", GODOT_OBJECT_NAME, "load"));
#endif
	String path = js_to_string(ctx, argv[0]);
	RES res = ResourceLoader::load(path);
	return variant_to_var(ctx, res);
}

void QuickJSBinder::add_debug_binding_info(JSContext *ctx, JSValueConst p_obj, const ECMAScriptGCHandler *p_bind) {
	if (!p_bind) return;
	JSValue classname = JS_UNDEFINED;
	if (p_bind->type != Variant::OBJECT) {
		classname = to_js_string(ctx, Variant::get_type_name(p_bind->type));
	} else {
		if (p_bind->is_reference()) {
			classname = to_js_string(ctx, (*p_bind->godot_reference)->get_class_name());
		} else if (p_bind->is_object()) {
			classname = to_js_string(ctx, p_bind->godot_object->get_class_name());
		}
	}

	Array arr;
	String ptr_str = "0x%X";
	bool err;
	union {
		const void *p;
		uint64_t i;
	} u;
	u.p = p_bind->godot_object;
	arr.push_back(int(u.i));
	ptr_str = ptr_str.sprintf(arr, &err);
	JSValue ptrvalue = to_js_string(ctx, ptr_str);

	JS_DefinePropertyValueStr(ctx, p_obj, "__class__", classname, PROP_DEF_DEFAULT);
	JS_DefinePropertyValueStr(ctx, p_obj, "__ptr__", ptrvalue, PROP_DEF_DEFAULT);
}

JSModuleDef *QuickJSBinder::js_module_loader(JSContext *ctx, const char *module_name, void *opaque) {
	JSModuleDef *m = NULL;
	Error err;
	String file;
	file.parse_utf8(module_name);
	String extension = file.get_extension();
	if (extension.empty()) {
		extension = ECMAScriptLanguage::get_singleton()->get_extension();
		file += "." + extension;
	}

	QuickJSBinder *binder = QuickJSBinder::get_context_binder(ctx);
	if (ModuleCache *ptr = binder->module_cache.getptr(file)) {
		m = ptr->module;
	}

	if (!m) {
		if (extension == ECMAScriptLanguage::get_singleton()->get_extension()) {
			String code = FileAccess::get_file_as_string(file, &err);
			if (err != OK || !code.size()) {
				JS_ThrowReferenceError(ctx, "Could not load module '%s'", file.utf8().ptr());
				return NULL;
			}
			ECMAscriptScriptError err;
			if (ModuleCache *module = binder->js_compile_module(ctx, code, file, &err)) {
				m = module->module;
			}
		} else { // Try load as Resource
			RES res = ResourceLoader::load(file);
			if (res.is_null()) {
				JS_ThrowReferenceError(ctx, "Could not load module '%s'", file.utf8().ptr());
				return NULL;
			}
			// hack the quick module to make the resource value as default entry
			m = JS_NewCModule(ctx, file.utf8().ptr(), resource_module_initializer);
			JS_AddModuleExport(ctx, m, "default");
			JSValue func = JS_MKPTR(JS_TAG_MODULE, m);
			JS_DupValue(ctx, func);
			JS_EvalFunction(ctx, func);
			JS_SetModuleExport(ctx, m, "default", variant_to_var(ctx, res));
		}
	}

	return m;
}

QuickJSBinder::ModuleCache *QuickJSBinder::js_compile_module(JSContext *ctx, const String &p_code, const String &p_filename, ECMAscriptScriptError *r_error) {

	QuickJSBinder *binder = QuickJSBinder::get_context_binder(ctx);
	CharString code = p_code.utf8();
	CharString filename = p_filename.utf8();
	const char *cfilename = filename.ptr();
	if (!cfilename) cfilename = ""; // avoid crash with empty file name here

	String md5 = p_code.md5_text();
	ModuleCache *last_module = binder->module_cache.getptr(p_filename);
	if (last_module) {
		if (last_module->code_md5 == md5) {
#if MODULE_HAS_REFCOUNT
			if (last_module->module) {
				JSValue val = JS_MKPTR(JS_TAG_MODULE, last_module->module);
				JS_DupValue(ctx, val);
			}
#endif
			return last_module;
		} else {
#if MODULE_HAS_REFCOUNT
			JSModuleDef *m = last_module->module;
			if (m) {
				JSValue val = JS_MKPTR(JS_TAG_MODULE, m);
				JS_FreeValue(ctx, val);
			}
#endif
		}
	}

	JSValue func = JS_Eval(ctx, code.ptr(), code.length(), cfilename, JS_EVAL_TYPE_MODULE | JS_EVAL_FLAG_COMPILE_ONLY);
	if (!JS_IsException(func)) {
		ModuleCache module;
		module.code_md5 = md5;
		module.module = NULL;
		module.evaluated = false;
		module.module = static_cast<JSModuleDef *>(JS_VALUE_GET_PTR(func));
		binder->module_cache.set(p_filename, module);
	} else {
		JSValue e = JS_GetException(ctx);
		dump_exception(ctx, e, r_error);
		JS_Throw(ctx, e);
		return NULL;
	}
	return binder->module_cache.getptr(p_filename);
}

int QuickJSBinder::resource_module_initializer(JSContext *ctx, JSModuleDef *m) {
	return JS_SetModuleExport(ctx, m, "default", JS_UNDEFINED);
}

JSClassID QuickJSBinder::register_class(const ClassDB::ClassInfo *p_cls) {

	ClassBindData data;
	data.class_id = 0;
	data.base_class = NULL;
	data.class_name = String(p_cls->name).ascii();
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

			CharString name = String(*key).ascii();
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
			JS_DefinePropertyValue(ctx, data.constructor, atom, to_js_string(ctx, *key), PROP_DEF_DEFAULT);
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

		int flags = PROP_DEF_DEFAULT;
		// Allows redefine as to global object
		if (Engine::get_singleton()->has_singleton(data.gdclass->name)) {
			flags |= JS_PROP_CONFIGURABLE;
		}
		JS_DefinePropertyValueStr(ctx, godot_object, data.jsclass.class_name, data.constructor, flags);
		if (data.base_class) {
			JS_SetPrototype(ctx, data.prototype, data.base_class->prototype);
		} else {
			JS_SetPrototype(ctx, data.prototype, godot_origin_class.prototype);
		}

		id = class_bindings.next(id);
	}
	godot_object_class = *classname_bindings.getptr("Object");
}

void QuickJSBinder::add_godot_globals() {
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
		ECMAScriptGCHandler *data = new_gc_handler();
		data->ecma_object = JS_VALUE_GET_PTR(obj);
		data->godot_object = s.ptr;
		data->type = Variant::OBJECT;
		data->flags = ECMAScriptGCHandler::FLAG_OBJECT;
		JS_SetOpaque(obj, data);

		JSAtom singleton_name = get_atom(ctx, s.name);
		JSValue last_obj = JS_GetProperty(ctx, godot_object, singleton_name);
		if (!JS_IsUndefined(last_obj)) {
			JS_SetPrototype(ctx, obj, cls->prototype);
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
	// global numbers
	GlobalNumberConstant consts[] = {
		{ "PI", Math_PI },
		{ "TAU", Math_TAU },
		{ "NAN", Math_NAN },
		{ "INF", Math_INF },
		{ "E", Math_E },
		{ "LN2", Math_LN2 },
		{ "SQRT2", Math_SQRT2 },
		{ "SQRT12", Math_SQRT12 },
	};
	for (int i = 0; i < sizeof(consts) / sizeof(GlobalNumberConstant); i++) {
		const GlobalNumberConstant &c = consts[i];
		JSAtom js_const_name = get_atom(ctx, c.name);
		JS_DefinePropertyValue(ctx, godot_object, js_const_name, JS_NewFloat64(ctx, c.value), QuickJSBinder::PROP_DEF_DEFAULT);
		JS_FreeAtom(ctx, js_const_name);
	}

	// global enums
	for (const StringName *enum_name = global_constants.next(NULL); enum_name; enum_name = global_constants.next(enum_name)) {
		String enum_name_str = *enum_name;
		if (enum_name_str.empty()) continue;
		enum_name_str = enum_name_str.replace(".", "");

		JSAtom atom_enum_name = get_atom(ctx, enum_name_str);
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
		JSValue js_func = JS_NewCFunctionMagic(ctx, godot_builtin_function, name.ascii().ptr(), 0, JS_CFUNC_generic_magic, i);
		JSAtom atom = get_atom(ctx, name);
		JS_DefinePropertyValue(ctx, godot_object, atom, js_func, QuickJSBinder::PROP_DEF_DEFAULT);
		JS_FreeAtom(ctx, atom);
	}

	// godot.register_class
	JSValue js_func_register_class = JS_NewCFunction(ctx, godot_register_class, "register_class", 2);
	JS_DefinePropertyValueStr(ctx, godot_object, "register_class", js_func_register_class, PROP_DEF_DEFAULT);
	// godot.register_signal
	JSValue js_func_register_signal = JS_NewCFunction(ctx, godot_register_signal, "register_signal", 2);
	JS_DefinePropertyValueStr(ctx, godot_object, "register_signal", js_func_register_signal, PROP_DEF_DEFAULT);
	// godot.register_property
	JSValue js_func_register_property = JS_NewCFunction(ctx, godot_register_property, "register_property", 3);
	JS_DefinePropertyValueStr(ctx, godot_object, "register_property", js_func_register_property, PROP_DEF_DEFAULT);
	// godot.set_script_tooled
	JSValue js_func_set_tooled = JS_NewCFunctionMagic(ctx, godot_set_script_meta, "set_script_tooled", 2, JS_CFUNC_generic_magic, QuickJSBinder::SCRIPT_META_TOOLED);
	JS_DefinePropertyValueStr(ctx, godot_object, "set_script_tooled", js_func_set_tooled, PROP_DEF_DEFAULT);
	// godot.set_script_icon
	JSValue js_set_script_icon = JS_NewCFunctionMagic(ctx, godot_set_script_meta, "set_script_icon", 2, JS_CFUNC_generic_magic, QuickJSBinder::SCRIPT_META_ICON);
	JS_DefinePropertyValueStr(ctx, godot_object, "set_script_icon", js_set_script_icon, PROP_DEF_DEFAULT);
	// godot.get_type
	JSValue js_get_type = JS_NewCFunction(ctx, godot_get_type, "get_type", 1);
	JS_DefinePropertyValueStr(ctx, godot_object, "get_type", js_get_type, PROP_DEF_DEFAULT);
	// godot.load
	JSValue js_func_load = JS_NewCFunction(ctx, godot_load, "load", 1);
	JS_DefinePropertyValueStr(ctx, godot_object, "load", js_func_load, PROP_DEF_DEFAULT);
	// godot.requestAnimationFrame
	JSValue js_func_requestAnimationFrame = JS_NewCFunction(ctx, godot_request_animation_frame, "requestAnimationFrame", 1);
	JS_DefinePropertyValueStr(ctx, godot_object, "requestAnimationFrame", js_func_requestAnimationFrame, PROP_DEF_DEFAULT);
	// godot.cancelAnimationFrame
	JSValue js_func_cancelAnimationFrame = JS_NewCFunction(ctx, godot_cancel_animation_frame, "cancelAnimationFrame", 1);
	JS_DefinePropertyValueStr(ctx, godot_object, "cancelAnimationFrame", js_func_cancelAnimationFrame, PROP_DEF_DEFAULT);

	{
		// godot.DEBUG_ENABLED
#ifdef DEBUG_ENABLED
		JS_DefinePropertyValueStr(ctx, godot_object, "DEBUG_ENABLED", JS_TRUE, JS_PROP_ENUMERABLE);
#endif
		// godot.TOOLS_ENABLED
#ifdef TOOLS_ENABLED
		JS_DefinePropertyValueStr(ctx, godot_object, "TOOLS_ENABLED", JS_TRUE, JS_PROP_ENUMERABLE);
#endif
#ifdef DEBUG_METHODS_ENABLED
		JS_DefinePropertyValueStr(ctx, godot_object, "DEBUG_METHODS_ENABLED", JS_TRUE, JS_PROP_ENUMERABLE);
#endif
	}
}

QuickJSBinder::QuickJSBinder() {
	context_id = global_context_id++;
	internal_godot_method_id = 0;
	godot_allocator.js_malloc = QuickJSBinder::js_malloc;
	godot_allocator.js_free = QuickJSBinder::js_free;
	godot_allocator.js_realloc = QuickJSBinder::js_realloc;
	godot_allocator.js_malloc_usable_size = NULL;
	godot_object_class = NULL;
}

void QuickJSBinder::initialize() {
	// create runtime and context for the binder
	runtime = JS_NewRuntime2(&godot_allocator, this);
	ctx = JS_NewContext(runtime);
	JS_SetModuleLoaderFunc(runtime, /*js_module_resolve*/ NULL, js_module_loader, this);
	JS_SetContextOpaque(ctx, this);
	runtime_context_map.set(runtime, ctx);

	empty_function = JS_NewCFunction(ctx, js_empty_func, "virtual_fuction", 0);
	// global = globalThis
	global_object = JS_GetGlobalObject(ctx);
	// global.console
	add_global_console();
	// global.godot
	godot_object = JS_NewObject(ctx);
	js_key_godot_classid = JS_NewAtom(ctx, JS_HIDDEN_SYMBOL("cls"));
	js_key_godot_exports = JS_NewAtom(ctx, JS_HIDDEN_SYMBOL("exports"));
	js_key_godot_signals = JS_NewAtom(ctx, JS_HIDDEN_SYMBOL("signals"));
	js_key_godot_tooled = JS_NewAtom(ctx, JS_HIDDEN_SYMBOL("tool"));
	js_key_godot_icon_path = JS_NewAtom(ctx, JS_HIDDEN_SYMBOL("icon"));
	JS_DefinePropertyValueStr(ctx, global_object, GODOT_OBJECT_NAME, godot_object, PROP_DEF_DEFAULT);
	// godot.GodotOrigin
	add_godot_origin();
	// godot.Vector2 godot.Color ...
	builtin_binder.initialize(ctx, this);
	// godot.Object godot.Node godot.Theme ...
	add_godot_classes();
	// godot.print godot.sin ...
	add_godot_globals();
	// binding script
	eval_string(BINDING_SCRIPT_CONTENT, "");
}

void QuickJSBinder::uninitialize() {
	godot_object_class = NULL;
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
	const String *key = ecma_classes.next(NULL);
	while (key) {
		const ECMAClassInfo &ecma_class = ecma_classes.get(*key);
		free_ecmas_class(ecma_class);
		key = ecma_classes.next(key);
	}
	ecma_classes.clear();

	// Free frame callbacks
	const int64_t *id = frame_callbacks.next(NULL);
	while (id) {
		const ECMAScriptGCHandler &func = frame_callbacks.get(*id);
		JSValueConst js_func = JS_MKPTR(JS_TAG_OBJECT, func.ecma_object);
		JS_FreeValue(ctx, js_func);
		id = frame_callbacks.next(id);
	}
	frame_callbacks.clear();

	// modules
#if MODULE_HAS_REFCOUNT
	const String *file = module_cache.next(NULL);
	while (file) {
		const ModuleCache &m = module_cache.get(*file);
		if (m.module) {
			JSValue val = JS_MKPTR(JS_TAG_MODULE, m.module);
			JS_FreeValue(ctx, val);
		}
		file = module_cache.next(file);
	}
#endif
	module_cache.clear();

	JS_FreeAtom(ctx, js_key_godot_classid);
	JS_FreeAtom(ctx, js_key_godot_tooled);
	JS_FreeAtom(ctx, js_key_godot_icon_path);
	JS_FreeAtom(ctx, js_key_godot_exports);
	JS_FreeAtom(ctx, js_key_godot_signals);
	JS_FreeValue(ctx, empty_function);
	JS_FreeValue(ctx, global_object);
	JS_FreeContext(ctx);
	JS_FreeRuntime(runtime);
	runtime_context_map.erase(runtime);
	ctx = NULL;
	runtime = NULL;
}

void QuickJSBinder::frame() {
	JSContext *ctx1;
	int err;
	for (;;) {
		err = JS_ExecutePendingJob(JS_GetRuntime(ctx), &ctx1);
		if (err <= 0) {
			if (err < 0) {
				ECMAscriptScriptError script_err;
				JSValue e = JS_GetException(ctx1);
				dump_exception(ctx1, e, &script_err);
				ERR_PRINTS(error_to_string(script_err));
				JS_FreeValue(ctx1, e);
			}
			break;
		}
	}

	const int64_t *id = frame_callbacks.next(NULL);
	while (id) {
		const ECMAScriptGCHandler &func = frame_callbacks.get(*id);
		JSValueConst js_func = JS_MKPTR(JS_TAG_OBJECT, func.ecma_object);
		JSValue argvs = { JS_NewInt64(ctx, (int64_t)OS::get_singleton()->get_system_time_msecs()) };
		JS_Call(ctx, js_func, godot_object, 1, &argvs);
		id = frame_callbacks.next(id);
	}
}

Error QuickJSBinder::eval_string(const String &p_source, const String &p_path) {
	String error;
	Error err = safe_eval_text(p_source, p_path, error);
	if (!error.empty()) {
		ERR_PRINTS(error);
	}
	return err;
}

Error QuickJSBinder::safe_eval_text(const String &p_source, const String &p_path, String &r_error) {
	ERR_FAIL_COND_V(p_source.empty(), FAILED);

	CharString utf8_str = p_source.utf8();
	const char *filename = p_path.utf8().ptr();
	if (!filename) filename = "";
	JSValue ret = JS_Eval(ctx, utf8_str.ptr(), utf8_str.length(), filename, JS_EVAL_TYPE_MODULE | JS_EVAL_FLAG_STRICT);
	if (JS_IsException(ret)) {
		JSValue e = JS_GetException(ctx);
		ECMAscriptScriptError err;
		dump_exception(ctx, e, &err);
		r_error = error_to_string(err);
		ERR_PRINTS(r_error);
		JS_Throw(ctx, e);
		return ERR_PARSE_ERROR;
	}
	return OK;
}

/************************* Memory Management ******************************/

void *QuickJSBinder::alloc_object_binding_data(Object *p_object) {
	if (const ClassBindData **bind_ptr = classname_bindings.getptr(p_object->get_class_name())) {
		JSValue obj = JS_NewObjectProtoClass(ctx, (*bind_ptr)->prototype, get_origin_class_id());
		ECMAScriptGCHandler *data = new_gc_handler();
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
#ifdef DUMP_LEAKS
		add_debug_binding_info(ctx, obj, data);
		JS_DefinePropertyValueStr(ctx, obj, "__id__", to_js_number(ctx, p_object->get_instance_id()), PROP_DEF_DEFAULT);
#endif

		lastest_allocated_object = data;
		return data;
	}
	return NULL;
}

void QuickJSBinder::free_object_binding_data(void *p_gc_handle) {
	ECMAScriptGCHandler *bind = (ECMAScriptGCHandler *)p_gc_handle;
	if (bind->is_object()) {
		JSValue js_obj = JS_MKPTR(JS_TAG_OBJECT, bind->ecma_object);
		JS_SetOpaque(js_obj, NULL);
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
		if (JS_IsFunction(ctx, new_target)) {
			JSValue prototype = JS_GetProperty(ctx, new_target, QuickJSBinder::JS_ATOM_prototype);
			JS_SetPrototype(ctx, js_obj, prototype);
			JS_FreeValue(ctx, prototype);
		}

		ECMAScriptInstance *si = memnew(ECMAScriptInstance);
		si->ecma_object = *bind;
		gd_obj->set_script_instance(si);

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

/************************* END Memory Management ******************************/

/********************************* Script --> C++ ****************************/
JSValue QuickJSBinder::godot_register_class(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
	ERR_FAIL_COND_V(argc < 1 || !JS_IsFunction(ctx, argv[0]), JS_ThrowTypeError(ctx, "ECMAClass class function expected"));
	QuickJSBinder *binder = get_context_binder(ctx);
	String class_name;
#if NO_MODULE_EXPORT_SUPPORT
	class_name = binder->parsing_script_file;
#endif
	if (class_name.empty()) {
		if (argc > 1 && JS_IsString(argv[1])) {
			class_name = js_to_string(ctx, argv[1]);
		} else {
			JSValue name = JS_GetProperty(ctx, *argv, JS_ATOM_name);
			class_name = js_to_string(ctx, name);
			JS_FreeValue(ctx, name);
		}
	}
	binder->register_ecma_class(argv[0], class_name);
	return JS_UNDEFINED;
}

const ECMAClassInfo *QuickJSBinder::register_ecma_class(const JSValue &p_constructor, const String &p_path) {

	QuickJSBinder *binder = get_context_binder(ctx);
	JSValue prototype = JS_UNDEFINED;
	JSValue classid = JS_UNDEFINED;
	JSValue tooled = JS_UNDEFINED;
	JSValue icon = JS_UNDEFINED;
	JSClassID id = 0;

	if (!JS_IsFunction(ctx, p_constructor)) {
		JS_ThrowTypeError(ctx, "ECMAClass class expected: %s", p_path.utf8().ptr());
		goto fail;
	}

	prototype = JS_GetProperty(ctx, p_constructor, QuickJSBinder::JS_ATOM_prototype);
	classid = JS_GetProperty(ctx, prototype, js_key_godot_classid);
	tooled = JS_GetProperty(ctx, p_constructor, js_key_godot_tooled);
	icon = JS_GetProperty(ctx, p_constructor, js_key_godot_icon_path);

	if (JS_IsUndefined(classid)) {
		JS_ThrowTypeError(ctx, "ECMAClass class expected: %s", p_path.utf8().ptr());
		goto fail;
	}

	id = JS_VALUE_GET_INT(classid);
	if (const ClassBindData *bind = binder->class_bindings.getptr(id)) {
		// Add reference to the class function
		JS_DupValue(ctx, p_constructor);

		JSValue name = JS_GetProperty(ctx, p_constructor, JS_ATOM_name);
		StringName class_name = js_to_string(ctx, name);
		JS_FreeValue(ctx, name);

		ECMAClassInfo ecma_class;
		ecma_class.icon_path = "";
		ecma_class.tool = false;
		ecma_class.native_class = bind->gdclass;
		ecma_class.class_name = class_name;
		ecma_class.prototype.ecma_object = JS_VALUE_GET_PTR(prototype);
		ecma_class.constructor.ecma_object = JS_VALUE_GET_PTR(p_constructor);
		ecma_class.tool = JS_ToBool(ctx, tooled);
		if (JS_IsString(icon)) {
			ecma_class.icon_path = js_to_string(ctx, icon);
		}

		// signals
		JSValue signals = JS_GetProperty(ctx, prototype, js_key_godot_signals);
		if (JS_IsObject(signals)) {
			Set<String> keys;
			get_own_property_names(ctx, signals, &keys);
			for (Set<String>::Element *E = keys.front(); E; E = E->next()) {
				MethodInfo mi;
				mi.name = E->get();
				ecma_class.signals.set(mi.name, mi);
			}
		}
		JS_FreeValue(ctx, signals);

		// properties
		JSValue props = JS_GetProperty(ctx, prototype, js_key_godot_exports);
		if (JS_IsObject(props)) {
			Set<String> keys;
			get_own_property_names(ctx, props, &keys);
			for (Set<String>::Element *E = keys.front(); E; E = E->next()) {

				JSAtom pname = get_atom(ctx, E->get());
				JSValue val = JS_GetProperty(ctx, props, pname);

				JS_DupValue(ctx, val);
				JS_SetProperty(ctx, prototype, pname, val);

				ECMAProperyInfo ei;
				ei.default_value = QuickJSBinder::var_to_variant(ctx, val);
				ei.type = ei.default_value.get_type();
				ecma_class.properties.set(E->get(), ei);

				JS_FreeValue(ctx, val);
				JS_FreeAtom(ctx, pname);
			}
		}
		JS_FreeValue(ctx, props);

		// cache the class
		if (const ECMAClassInfo *ptr = binder->ecma_classes.getptr(p_path)) {
			binder->free_ecmas_class(*ptr);
		}
		binder->ecma_classes.set(p_path, ecma_class);
	}
fail:
	JS_FreeValue(ctx, classid);
	JS_FreeValue(ctx, prototype);
	JS_FreeValue(ctx, icon);
	JS_FreeValue(ctx, tooled);
	return binder->ecma_classes.getptr(p_path);
}

void QuickJSBinder::free_ecmas_class(const ECMAClassInfo &p_class) {
	JSValue class_func = JS_MKPTR(JS_TAG_OBJECT, p_class.constructor.ecma_object);
	JS_FreeValue(ctx, class_func);
}

JSValue QuickJSBinder::godot_register_signal(JSContext *ctx, JSValue this_val, int argc, JSValue *argv) {
	ERR_FAIL_COND_V(argc < 2, JS_ThrowTypeError(ctx, "Two or more arguments expected"));
	ERR_FAIL_COND_V(!JS_IsObject(argv[0]), JS_ThrowTypeError(ctx, "protorype of ECMAClass function expected for agurment 0"));
	ERR_FAIL_COND_V(!JS_IsString(argv[1]), JS_ThrowTypeError(ctx, "string expected for agurment 1"));
	QuickJSBinder *binder = get_context_binder(ctx);

	JSValue prototype = JS_UNDEFINED;
	if (JS_IsFunction(ctx, argv[0])) {
		prototype = JS_GetProperty(ctx, argv[0], QuickJSBinder::JS_ATOM_prototype);
	} else {
		prototype = JS_DupValue(ctx, argv[0]);
	}

	JSValue object = JS_GetProperty(ctx, prototype, binder->js_key_godot_signals);
	if (!JS_IsObject(object)) {
		object = JS_NewObject(ctx);
	}
	const char *signal = JS_ToCString(ctx, argv[1]);
	JS_DupValue(ctx, argv[1]);
	JS_SetPropertyStr(ctx, object, signal, argv[1]);
	JS_FreeCString(ctx, signal);

	JS_SetProperty(ctx, prototype, binder->js_key_godot_signals, object);
	JS_FreeValue(ctx, prototype);
	return JS_UNDEFINED;
}

JSValue QuickJSBinder::godot_register_property(JSContext *ctx, JSValue this_val, int argc, JSValue *argv) {
	ERR_FAIL_COND_V(argc < 3, JS_ThrowTypeError(ctx, "Three or more arguments expected"));
	ERR_FAIL_COND_V(!JS_IsObject(argv[0]), JS_ThrowTypeError(ctx, "protorype of ECMAClass function expected for agurment 0"));
	ERR_FAIL_COND_V(!JS_IsString(argv[1]), JS_ThrowTypeError(ctx, "string expected for agurment 1"));
	ERR_FAIL_COND_V(JS_IsUndefined(argv[2]) || JS_IsNull(argv[2]), JS_ThrowTypeError(ctx, "Truthy value expected for agurment 2"));

	QuickJSBinder *binder = get_context_binder(ctx);
	JSValue prototype = JS_UNDEFINED;
	if (JS_IsFunction(ctx, argv[0])) {
		prototype = JS_GetProperty(ctx, argv[0], QuickJSBinder::JS_ATOM_prototype);
	} else {
		prototype = JS_DupValue(ctx, argv[0]);
	}

	JSValue object = JS_GetProperty(ctx, prototype, binder->js_key_godot_exports);
	if (!JS_IsObject(object)) {
		object = JS_NewObject(ctx);
	}
	const char *name = JS_ToCString(ctx, argv[1]);
	JS_DupValue(ctx, argv[2]);
	JS_SetPropertyStr(ctx, object, name, argv[2]);
	JS_FreeCString(ctx, name);

	JS_SetProperty(ctx, prototype, binder->js_key_godot_exports, object);
	JS_FreeValue(ctx, prototype);

	return JS_UNDEFINED;
}

JSValue QuickJSBinder::godot_set_script_meta(JSContext *ctx, JSValue this_val, int argc, JSValue *argv, int magic) {
	ERR_FAIL_COND_V(argc < 2, JS_ThrowTypeError(ctx, "Two or more arguments expected"))
	ERR_FAIL_COND_V(!JS_IsFunction(ctx, argv[0]), JS_ThrowTypeError(ctx, "godot class expected for argument #0"));
	JSValue constructor = argv[0];
	QuickJSBinder *binder = get_context_binder(ctx);
	switch (magic) {
		case QuickJSBinder::SCRIPT_META_TOOLED:
			JS_SetProperty(ctx, constructor, binder->js_key_godot_tooled, JS_DupValue(ctx, argv[1]));
			break;
		case QuickJSBinder::SCRIPT_META_ICON:
			JS_SetProperty(ctx, constructor, binder->js_key_godot_icon_path, JS_DupValue(ctx, argv[1]));
			break;
	}
	return JS_UNDEFINED;
}

JSValue QuickJSBinder::godot_request_animation_frame(JSContext *ctx, JSValue this_val, int argc, JSValue *argv) {
	ERR_FAIL_COND_V(argc < 1 || !JS_IsFunction(ctx, argv[0]), JS_ThrowTypeError(ctx, "Function expected for argument #0"));
	static int64_t id = 0;
	JSValue js_func = JS_DupValue(ctx, argv[0]);
	ECMAScriptGCHandler func;
	func.ecma_object = JS_VALUE_GET_PTR(js_func);
	QuickJSBinder *binder = get_context_binder(ctx);
	binder->frame_callbacks.set(++id, func);
	return JS_NewInt64(ctx, id);
}

JSValue QuickJSBinder::godot_cancel_animation_frame(JSContext *ctx, JSValue this_val, int argc, JSValue *argv) {
	ERR_FAIL_COND_V(argc < 1 || !JS_IsInteger(argv[0]), JS_ThrowTypeError(ctx, "Request ID expected for argument #0"));
	int32_t id = js_to_int(ctx, argv[0]);
	QuickJSBinder *binder = get_context_binder(ctx);
	if (ECMAScriptGCHandler *callback = binder->frame_callbacks.getptr(id)) {
		JSValue func = JS_MKPTR(JS_TAG_OBJECT, callback->ecma_object);
		JS_FreeValue(ctx, func);
		binder->frame_callbacks.erase(id);
	}
	return JS_UNDEFINED;
}

int QuickJSBinder::get_js_array_length(JSContext *ctx, JSValue p_val) {
	if (!JS_IsArray(ctx, p_val)) return -1;
	JSValue ret = JS_GetProperty(ctx, p_val, JS_ATOM_length);
	int32_t length = -1;
	if (JS_ToInt32(ctx, &length, ret)) return -1;
	return length;
}

void QuickJSBinder::get_own_property_names(JSContext *ctx, JSValue p_object, Set<String> *r_list) {
	ERR_FAIL_COND(!JS_IsObject(p_object));
	JSPropertyEnum *props = NULL;
	uint32_t tab_atom_count;
	JS_GetOwnPropertyNames(ctx, &props, &tab_atom_count, p_object, JS_GPN_STRING_MASK | JS_GPN_SYMBOL_MASK);
	for (uint32_t i = 0; i < tab_atom_count; i++) {
		JSValue key = JS_AtomToValue(ctx, props[i].atom);
		String name = js_to_string(ctx, key);
		r_list->insert(name);
		JS_FreeAtom(ctx, props[i].atom);
		JS_FreeValue(ctx, key);
	}
	js_free_rt(JS_GetRuntime(ctx), props);
}

ECMAScriptGCHandler QuickJSBinder::create_ecma_instance_for_godot_object(const ECMAClassInfo *p_class, Object *p_object) {

	ERR_FAIL_NULL_V(p_object, ECMAScriptGCHandler());
	ERR_FAIL_NULL_V(p_class, ECMAScriptGCHandler());

	ECMAScriptGCHandler *bind = BINDING_DATA_FROM_GD(p_object);
	ERR_FAIL_NULL_V(bind, ECMAScriptGCHandler());

	JSValue constructor = JS_MKPTR(JS_TAG_OBJECT, p_class->constructor.ecma_object);
	JSValue object = JS_MKPTR(JS_TAG_OBJECT, bind->ecma_object);
	JS_CallConstructor2(ctx, constructor, object, 0, NULL);
	JS_SetPrototype(ctx, object, JS_MKPTR(JS_TAG_OBJECT, p_class->prototype.ecma_object));

	return *bind;
}

Variant QuickJSBinder::call_method(const ECMAScriptGCHandler &p_object, const StringName &p_method, const Variant **p_args, int p_argcount, Variant::CallError &r_error) {

	JSValue object = GET_JSVALUE(p_object);
	JSAtom atom = get_atom(ctx, p_method);
	JSValue method = JS_GetProperty(ctx, object, atom);
	JS_FreeAtom(ctx, atom);

	JSValue return_val = JS_UNDEFINED;
	JSValue *argv = NULL;

	if (!JS_IsFunction(ctx, method) || JS_IsPureCFunction(ctx, method)) {
		r_error.error = Variant::CallError::CALL_ERROR_INVALID_METHOD;
		goto finish;
	}

	argv = memnew_arr(JSValue, p_argcount);
	for (int i = 0; i < p_argcount; ++i) {
		argv[i] = variant_to_var(ctx, *p_args[i]);
	}
	return_val = JS_Call(ctx, method, object, p_argcount, argv);

	if (JS_IsException(return_val)) {
		r_error.error = Variant::CallError::CALL_ERROR_INVALID_ARGUMENT;
		JSValue exception = JS_GetException(ctx);
		ECMAscriptScriptError err;
		dump_exception(ctx, exception, &err);
		ERR_PRINTS(error_to_string(err));
		JS_Throw(ctx, exception);
	} else {
		r_error.error = Variant::CallError::CALL_OK;
	}
finish:
	Variant ret = var_to_variant(ctx, return_val);
	if (argv) {
		for (int i = 0; i < p_argcount; i++) {
			JS_FreeValue(ctx, argv[i]);
		}
		memdelete_arr(argv);
	}
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

const ECMAClassInfo *QuickJSBinder::parse_ecma_class(const String &p_code, const String &p_path, ECMAscriptScriptError *r_error) {
	const ECMAClassInfo *ecma_class = NULL;
#if NO_MODULE_EXPORT_SUPPORT // call godot.register_class(ClassFunction) to register script class
	GLOBAL_LOCK_FUNCTION
	parsing_script_file = p_path;
	r_error = eval_string(p_code, p_path);
	parsing_script_file = "";
	ecma_class = ecma_classes.getptr(p_path);
#else // export default = ClassFunction to register class
	JSValue default_entry = JS_UNDEFINED;
	JSValue ret = JS_UNDEFINED;
	JSValue module = JS_UNDEFINED;

	ECMAscriptScriptError err;
	ModuleCache *mc = js_compile_module(ctx, p_code, p_path, r_error);
	if (mc == NULL || mc->module == NULL) {
		JS_ThrowTypeError(ctx, "Compile error %s", p_path.utf8().ptr());
		goto fail;
	}

	module = JS_MKPTR(JS_TAG_MODULE, mc->module);
	if (JS_IsException(module)) {
		JSValue e = JS_GetException(ctx);
		dump_exception(ctx, e, r_error);
		JS_Throw(ctx, e);
		goto fail;
	}

	if (!mc->evaluated) {
		ret = JS_EvalFunction(ctx, module);
		mc->evaluated = true;
		if (JS_IsException(ret)) {
			JSValue e = JS_GetException(ctx);
			dump_exception(ctx, e, r_error);
			JS_Throw(ctx, e);
			goto fail;
		}
	}

	for (int i = 0; i < JS_GetModuleExportEntriesCount(mc->module); i++) {
		JSAtom name = JS_GetModuleExportEntryName(ctx, mc->module, i);
		if (name == JS_ATOM_default) {
			default_entry = JS_GetModuleExportEntry(ctx, mc->module, i);
			JS_FreeAtom(ctx, name);
			break;
		}
		JS_FreeAtom(ctx, name);
	}
	if (!JS_IsFunction(ctx, default_entry)) {
		String err = "Failed parse ECMAClass from script " + p_path + "\n\t" + "Default export entry must be a godot class!";
		ERR_PRINTS(err);
		JS_ThrowTypeError(ctx, err.utf8().ptr());
		goto fail;
	}
	ecma_class = register_ecma_class(default_entry, p_path);
fail:
	JS_FreeValue(ctx, default_entry);
	JS_FreeValue(ctx, ret);
#endif
	return ecma_class;
}

bool QuickJSBinder::has_signal(const ECMAClassInfo *p_class, const StringName &p_signal) {
	ERR_FAIL_NULL_V(p_class, false);
	bool found = false;
	JSValue object = JS_GetProperty(ctx, GET_JSVALUE(p_class->prototype), js_key_godot_signals);
	if (!JS_IsObject(object)) {
		JS_FreeValue(ctx, object);
		return false;
	}
	JSAtom atom = get_atom(ctx, p_signal);
	JSValue signal = JS_GetProperty(ctx, object, atom);
	found = !JS_IsUndefined(signal);
	JS_FreeAtom(ctx, atom);
	JS_FreeValue(ctx, signal);
	JS_FreeValue(ctx, object);
	return found;
}
/**************************** END Script --> C++ ******************************/

bool QuickJSBinder::validate(const String &p_code, const String &p_path, ECMAscriptScriptError *r_error) {
	ModuleCache *module = js_compile_module(ctx, p_code, p_path, r_error);
	return module != NULL;
}
