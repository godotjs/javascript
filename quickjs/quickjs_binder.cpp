#include "quickjs_binder.h"
#include "../ecmascript.h"
#include "../ecmascript_instance.h"
#include "../ecmascript_language.h"
#include "core/bind/core_bind.h"
#include "core/engine.h"
#include "core/global_constants.h"
#include "core/io/json.h"
#include "core/math/expression.h"
#include "core/os/file_access.h"
#include "core/os/os.h"
#include "core/project_settings.h"
#include "quickjs_binder.h"
#include "quickjs_worker.h"
#ifdef TOOLS_ENABLED
#include "editor/editor_settings.h"
#endif

uint32_t QuickJSBinder::global_context_id = 0;
uint64_t QuickJSBinder::global_transfer_id = 0;
HashMap<uint64_t, Variant> QuickJSBinder::transfer_deopot;
Map<String, const char *> QuickJSBinder::class_remap;
List<String> compiling_modules;

struct GodotMethodArguments {
	Variant *arguments;
	const Variant **ptr;
	GodotMethodArguments(int argc) {
		if (argc) {
			arguments = memnew_arr(Variant, argc);
			ptr = memnew_arr(const Variant *, argc);
			for (int i = 0; i < argc; i++) {
				ptr[i] = &arguments[i];
			}
		} else {
			arguments = NULL;
			ptr = NULL;
		}
	}
	~GodotMethodArguments() {
		if (ptr) memdelete_arr(ptr);
		if (arguments) memdelete_arr(arguments);
	}
};

_FORCE_INLINE_ static ECMAScriptGCHandler *BINDING_DATA_FROM_GD(Object *p_object) {
	ERR_FAIL_COND_V(p_object == NULL, NULL);
	ECMAScriptGCHandler *bind = (ECMAScriptGCHandler *)(p_object)->get_script_instance_binding(ECMAScriptLanguage::get_singleton()->get_language_index());
	return bind;
}

_FORCE_INLINE_ static ECMAScriptGCHandler *BINDING_DATA_FROM_GD(JSContext *ctx, Object *p_object) {
	ERR_FAIL_COND_V(p_object == NULL || ctx == NULL, NULL);
	ECMAScriptGCHandler *bind = BINDING_DATA_FROM_GD(p_object);
	if (bind) {
		if (bind->context && bind->context != ctx) {
			ERR_FAIL_V_MSG(NULL, "The object is not belong to this context");
		}
		if (!bind->context) {
			QuickJSBinder::bind_gc_object(ctx, bind, p_object);
		}
	}
	return bind;
}

JSValue QuickJSBinder::console_functions(JSContext *ctx, JSValue this_val, int argc, JSValue *argv, int magic) {
	PoolStringArray args;
	args.resize(argc);

	QuickJSBinder *binder = get_context_binder(ctx);
	JSValue object_to_json = JS_GetPropertyStr(ctx, binder->console_object, PROP_NAME_CONSOLE_LOG_OBJECT_TO_JSON);
	bool to_json = js_to_bool(ctx, object_to_json);
	JS_FreeValue(ctx, object_to_json);

	for (int i = 0; i < argc; ++i) {
		int32_t tag = JS_VALUE_GET_TAG(argv[i]);
		if (to_json && tag == JS_TAG_OBJECT && NULL == BINDING_DATA_FROM_JS(ctx, argv[i])) {
			JSValue str = JS_JSONStringify(ctx, argv[i], JS_UNDEFINED, JS_UNDEFINED);
			args.write()[i] = js_to_string(ctx, str);
			JS_FreeValue(ctx, str);
		} else {
			size_t size;
			const char *utf8 = JS_ToCStringLen(ctx, &size, argv[i]);
			args.write()[i].parse_utf8(utf8, size);
			JS_FreeCString(ctx, utf8);
		}
	}

	ECMAScriptStackInfo stack_top;
	List<ECMAScriptStackInfo> stacks;
	String message = args.join(" ");
	if (magic == CONSOLE_ERROR || magic == CONSOLE_TRACE || magic == CONSOLE_WARN) {
		if (binder->get_stacks(stacks) == OK) {
			stack_top = stacks.front()->get();
			message += ENDL;
			message += binder->get_backtrace_message(stacks);
		}
	}

	switch (magic) {
		case CONSOLE_ERROR:
			_err_print_error(stack_top.function.utf8().get_data(), stack_top.file.utf8().get_data(), stack_top.line, message, ErrorHandlerType::ERR_HANDLER_ERROR);
			break;
		case CONSOLE_WARN:
			_err_print_error(stack_top.function.utf8().get_data(), stack_top.file.utf8().get_data(), stack_top.line, message, ErrorHandlerType::ERR_HANDLER_WARNING);
			break;
		case CONSOLE_LOG:
		case CONSOLE_TRACE:
		default:
			print_line(message);
			break;
	}
	return JS_UNDEFINED;
}

void QuickJSBinder::add_global_console() {
	JSValue console = JS_NewObject(ctx);
	JSValue log = JS_NewCFunctionMagic(ctx, console_functions, "log", 0, JS_CFUNC_generic_magic, CONSOLE_LOG);
	JSValue warn = JS_NewCFunctionMagic(ctx, console_functions, "warn", 0, JS_CFUNC_generic_magic, CONSOLE_WARN);
	JSValue err = JS_NewCFunctionMagic(ctx, console_functions, "error", 0, JS_CFUNC_generic_magic, CONSOLE_ERROR);
	JSValue trace = JS_NewCFunctionMagic(ctx, console_functions, "trace", 0, JS_CFUNC_generic_magic, CONSOLE_TRACE);
	JS_DefinePropertyValueStr(ctx, global_object, "console", console, PROP_DEF_DEFAULT);
	JS_DefinePropertyValueStr(ctx, console, "log", log, PROP_DEF_DEFAULT);
	JS_DefinePropertyValueStr(ctx, console, "warn", warn, PROP_DEF_DEFAULT);
	JS_DefinePropertyValueStr(ctx, console, "error", err, PROP_DEF_DEFAULT);
	JS_DefinePropertyValueStr(ctx, console, "trace", trace, PROP_DEF_DEFAULT);
	console_object = JS_DupValue(ctx, console);
}

void QuickJSBinder::add_global_properties() {
	// globalThis.require
	JSValue js_func_require = JS_NewCFunction(ctx, require_function, "require", 1);
	JS_DefinePropertyValueStr(ctx, global_object, "require", js_func_require, PROP_DEF_DEFAULT);

	// globalThis.requestAnimationFrame
	JSValue js_func_requestAnimationFrame = JS_NewCFunction(ctx, global_request_animation_frame, "requestAnimationFrame", 1);
	JS_DefinePropertyValueStr(ctx, global_object, "requestAnimationFrame", js_func_requestAnimationFrame, PROP_DEF_DEFAULT);
	// globalThis.cancelAnimationFrame
	JSValue js_func_cancelAnimationFrame = JS_NewCFunction(ctx, global_cancel_animation_frame, "cancelAnimationFrame", 1);
	JS_DefinePropertyValueStr(ctx, global_object, "cancelAnimationFrame", js_func_cancelAnimationFrame, PROP_DEF_DEFAULT);
}

JSValue QuickJSBinder::object_method(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv, int method_id) {

	ECMAScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, this_val);
	ERR_FAIL_NULL_V(bind, JS_ThrowReferenceError(ctx, "Call native method without native binding data"));
	ERR_FAIL_NULL_V(bind->godot_object, JS_ThrowReferenceError(ctx, "Call native method without native object caller"));

	QuickJSBinder *binder = QuickJSBinder::get_context_binder(ctx);
	Object *obj = bind->get_godot_object();
	MethodBind *mb = binder->godot_methods[method_id];

	if (!mb->is_vararg()) {
		argc = MIN(argc, mb->get_argument_count());
	}

	GodotMethodArguments args(argc);
	for (int i = 0; i < argc; ++i) {
		args.arguments[i] = var_to_variant(ctx, argv[i]);
	}

	Variant::CallError call_err;
	Variant ret_val = mb->call(obj, args.ptr, argc, call_err);
	JSValue ret = variant_to_var(ctx, ret_val);
#ifdef DEBUG_METHODS_ENABLED
	String err_message;
	switch (call_err.error) {
		case Variant::CallError::CALL_ERROR_INVALID_ARGUMENT:
			err_message = vformat("Argument of type '%s' is not assignable to parameter #%d of type '%s'", Variant::get_type_name(args.arguments[call_err.argument].get_type()), call_err.argument, Variant::get_type_name(call_err.expected));
			break;
		case Variant::CallError::CALL_ERROR_INVALID_METHOD:
			err_message = vformat("Invalid method '%s' for type '%s'", mb->get_name(), obj->get_class_name());
			break;
		case Variant::CallError::CALL_ERROR_TOO_FEW_ARGUMENTS:
			err_message = vformat("Too few arguments for method '%s'", mb->get_name());
			break;
		case Variant::CallError::CALL_ERROR_TOO_MANY_ARGUMENTS:
			err_message = vformat("Too many arguments for method '%s'", mb->get_name());
			break;
		case Variant::CallError::CALL_ERROR_INSTANCE_IS_NULL:
			err_message = vformat("Attempt to call '%s' on a null instance.", mb->get_name());
			break;
		default:
			break;
	}
	if (call_err.error != Variant::CallError::CALL_OK) {
		List<ECMAScriptStackInfo> stacks;
		String stack_message;
		if (binder->get_stacks(stacks) == OK) {
			stack_message = binder->get_backtrace_message(stacks);
		}
		ERR_PRINTS(obj->get_class() + "." + mb->get_name() + ENDL + err_message + ENDL + stack_message);
		JS_FreeValue(ctx, ret);
		ret = JS_ThrowTypeError(ctx, err_message.utf8().get_data());
	}
#endif

	return ret;
}

JSValue QuickJSBinder::object_indexed_property(JSContext *ctx, JSValue this_val, int argc, JSValue *argv, int property_id) {
	ECMAScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, this_val);
	ERR_FAIL_NULL_V(bind, JS_ThrowReferenceError(ctx, "Using indexed property without native binding data"));
	ERR_FAIL_NULL_V(bind->godot_object, JS_ThrowReferenceError(ctx, "Using indexed property without native object caller"));

	QuickJSBinder *binder = QuickJSBinder::get_context_binder(ctx);
	bool is_setter = argc > 0;

	Object *obj = bind->get_godot_object();
	const ClassDB::PropertySetGet *prop = binder->godot_object_indexed_properties[property_id];
	MethodBind *mb = is_setter ? prop->_setptr : prop->_getptr;

	GodotMethodArguments args(2);
	args.arguments[0] = prop->index;
	if (is_setter) {
		args.arguments[1] = var_to_variant(ctx, argv[0]);
	}

	Variant::CallError call_err;
	Variant ret_val = mb->call(obj, args.ptr, argc + 1, call_err);
	return variant_to_var(ctx, ret_val);
}

JSValue QuickJSBinder::variant_to_var(JSContext *ctx, const Variant p_var) {
	switch (p_var.get_type()) {
		case Variant::BOOL:
			return ((bool)p_var) ? JS_TRUE : JS_FALSE;
		case Variant::INT:
			return JS_NewInt64(ctx, int64_t(p_var));
		case Variant::REAL:
			return JS_NewFloat64(ctx, (double)(p_var));
		case Variant::NODE_PATH:
		case Variant::STRING:
			return to_js_string(ctx, p_var);
		case Variant::OBJECT: {
			Object *obj = p_var;
			if (obj == NULL) return JS_NULL;
			ECMAScriptGCHandler *data = BINDING_DATA_FROM_GD(ctx, obj);
			ERR_FAIL_NULL_V(data, JS_UNDEFINED);
			ERR_FAIL_NULL_V(data->ecma_object, JS_UNDEFINED);
			ERR_FAIL_COND_V(data->context != ctx, (JS_UNDEFINED));
			QuickJSBinder *binder = get_context_binder(ctx);
			JSValue js_obj = JS_MKPTR(JS_TAG_OBJECT, data->ecma_object);
			if (binder->lastest_allocated_object == data) {
				binder->lastest_allocated_object = NULL;
			}
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
		}
		case Variant::DICTIONARY: {
			Dictionary dict = p_var;
			JSValue obj = JS_NewObject(ctx);
			Array keys = dict.keys();
			for (int i = 0; i < keys.size(); i++) {
				const Variant &key = keys[i];
				const Variant &value = dict[key];
				String key_str = keys[i];
				JS_SetPropertyStr(ctx, obj, key_str.utf8().get_data(), variant_to_var(ctx, value));
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
	int64_t tag = JS_VALUE_GET_TAG(p_val);
	switch (tag) {
		case JS_TAG_INT:
			return Variant(JS_VALUE_GET_INT(p_val));
		case JS_TAG_BOOL:
			return Variant(bool(JS_VALUE_GET_INT(p_val)));
		case JS_TAG_FLOAT64:
			return Variant(real_t(JS_VALUE_GET_FLOAT64(p_val)));
		case JS_TAG_STRING:
			return js_to_string(ctx, p_val);
		case JS_TAG_OBJECT: {
			if (JS_VALUE_GET_PTR(p_val) == NULL) {
				return Variant();
			}
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
			} else if (JS_IsFunction(ctx, p_val)) {
				JSValue name = JS_GetProperty(ctx, p_val, JS_ATOM_name);
				String ret = vformat("function %s() { ... }", js_to_string(ctx, name));
				JS_FreeValue(ctx, name);
				return ret;
			} else { // Plain Object as Dictionary
				List<void *> stack;
				return js_to_dictionary(ctx, p_val, stack);
			}
		} break;
		case JS_TAG_NULL:
		case JS_TAG_UNDEFINED:
			return Variant();
			break;
		default:
#ifdef JS_NAN_BOXING
			if (tag > JS_TAG_FLOAT64 || tag < JS_TAG_FIRST) {
				return Variant(real_t(JS_VALUE_GET_FLOAT64(p_val)));
			}
#endif
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
		return JS_ThrowTypeError(ctx, "%d arguments expected for builtin funtion %s", arg_required, func_name.ascii().get_data());
	}

	QuickJSBinder *binder = get_context_binder(ctx);
	GodotMethodArguments args(argc);
	for (int i = 0; i < argc; ++i) {
		args.arguments[i] = var_to_variant(ctx, argv[i]);
	}
	Expression::exec_func(func, args.ptr, &ret, err, err_msg);
#ifdef DEBUG_METHODS_ENABLED
	if (err.error != Variant::CallError::CALL_OK) {
		String func_name = Expression::get_func_name(func);
		if (err_msg.empty()) {
			switch (err.error) {
				case Variant::CallError::CALL_ERROR_INVALID_ARGUMENT:
					err_msg = vformat("Argument of type '%s' is not assignable to parameter #%d of type '%s'", Variant::get_type_name(args.arguments[err.argument].get_type()), err.argument, Variant::get_type_name(err.expected));
					break;
				case Variant::CallError::CALL_ERROR_INVALID_METHOD:
					err_msg = vformat("Invalid builtin function", func_name);
					break;
				case Variant::CallError::CALL_ERROR_TOO_FEW_ARGUMENTS:
					err_msg = vformat("Too few arguments builtin function", func_name);
					break;
				case Variant::CallError::CALL_ERROR_TOO_MANY_ARGUMENTS:
					err_msg = vformat("Too many arguments for builtin function", func_name);
					break;
				default:
					break;
			}
		}
	}
#endif

	if (err.error != Variant::CallError::CALL_OK) {
		String func_name = Expression::get_func_name(func);
		return JS_ThrowTypeError(ctx, "Call builtin function error %s: %s", func_name.ascii().get_data(), err_msg.utf8().get_data());
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
			return JS_IsNumber(p_val) || JS_IsBool(p_val) || JS_IsNull(p_val) || JS_IsUndefined(p_val);
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

Error QuickJSBinder::get_stacks(List<ECMAScriptStackInfo> &r_stacks) {
	JSValue error_constructor = JS_GetProperty(ctx, global_object, JS_ATOM_Error);
	JSValue error = JS_CallConstructor(ctx, error_constructor, 0, NULL);
	JSValue stack = JS_GetProperty(ctx, error, JS_ATOM_stack);
	String stack_text = js_to_string(ctx, stack);
	Vector<String> raw_stacks = stack_text.split("\n", false);
	for (int i = 1; i < raw_stacks.size(); i++) {
		String line_text = raw_stacks[i].strip_edges();
		if (line_text.empty()) continue;
		ECMAScriptStackInfo s;
		int colon = line_text.find_last(":");
		int bracket = line_text.find_last("(");
		s.line = line_text.substr(colon + 1, line_text.length() - colon - 2).to_int();
		s.file = line_text.substr(bracket + 1, colon - bracket - 1);
		s.function = line_text.substr(3, bracket - 4);
		r_stacks.push_back(s);
	}
	JS_FreeValue(ctx, stack);
	JS_FreeValue(ctx, error);
	JS_FreeValue(ctx, error_constructor);
	return OK;
}

String QuickJSBinder::get_backtrace_message(const List<ECMAScriptStackInfo> &stacks) {
	String message;
	for (const List<ECMAScriptStackInfo>::Element *E = stacks.front(); E; E = E->next()) {
		message += "  ";
		message += vformat("at %s (%s:%d)", E->get().function, E->get().file, E->get().line);
		if (E != stacks.back()) {
			message += ENDL;
		}
	}
	return message;
}

Dictionary QuickJSBinder::js_to_dictionary(JSContext *ctx, const JSValue &p_val, List<void *> &stack) {
	Dictionary dict;
	Set<String> keys;
	get_own_property_names(ctx, p_val, &keys);
	stack.push_back(JS_VALUE_GET_PTR(p_val));
	for (Set<String>::Element *E = keys.front(); E; E = E->next()) {
		JSValue v = JS_GetPropertyStr(ctx, p_val, E->get().utf8().get_data());
		Variant val;
		if (JS_IsObject(v)) {
			void *ptr = JS_VALUE_GET_PTR(v);
			if (stack.find(ptr)) {
				union {
					const void *p;
					uint64_t i;
				} u;
				u.p = ptr;
				ERR_PRINTS(vformat("Property '%s' circular reference to 0x%X", E->get(), u.i));
				JS_FreeValue(ctx, v);
				continue;
			} else {
				stack.push_back(ptr);
				val = var_to_variant(ctx, v);
				stack.pop_back();
			}
		} else {
			val = var_to_variant(ctx, v);
		}
		dict[E->get()] = val;
		JS_FreeValue(ctx, v);
	}
	stack.pop_back();
	return dict;
}

JSAtom QuickJSBinder::get_atom(JSContext *ctx, const StringName &p_key) {
	String name = p_key;
	CharString name_str = name.ascii();
	JSAtom atom = JS_NewAtom(ctx, name_str.get_data());
	return atom;
}

JSValue QuickJSBinder::godot_to_string(JSContext *ctx, JSValue this_val, int argc, JSValue *argv) {
	String str = var_to_variant(ctx, this_val);
	CharString ascii = str.ascii();
	return JS_NewStringLen(ctx, ascii.get_data(), ascii.length());
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

JSValue QuickJSBinder::godot_instance_from_id(JSContext *ctx, JSValue this_val, int argc, JSValue *argv) {
#ifdef DEBUG_METHODS_ENABLED
	ERR_FAIL_COND_V(argc < 1 || !JS_IsNumber(argv[0]), JS_ThrowTypeError(ctx, "number expected for %s.%s", GODOT_OBJECT_NAME, "instance_from_id"));
#endif
	ObjectID id = js_to_uint64(ctx, argv[0]);
	Object *obj = ObjectDB::get_instance(id);
	return variant_to_var(ctx, obj);
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

	u.p = ctx;
	arr[0] = int(u.i);
	ptr_str = "0x%X";
	ptr_str = ptr_str.sprintf(arr, &err);
	JSValue ptrctx = to_js_string(ctx, ptr_str);

	JS_DefinePropertyValueStr(ctx, p_obj, "__class__", classname, PROP_DEF_DEFAULT);
	JS_DefinePropertyValueStr(ctx, p_obj, "__ptr__", ptrvalue, PROP_DEF_DEFAULT);
	JS_DefinePropertyValueStr(ctx, p_obj, "__ctx__", ptrctx, PROP_DEF_DEFAULT);
}

static HashMap<String, String> resolve_path_cache;

static String resolve_module_file(const String &file, bool allow_node_module = false) {
	if (const String *ptr = resolve_path_cache.getptr(file)) {
		return *ptr;
	}
	String path = file;
	if (FileAccess::exists(path)) return path;
	// add extensions to try
	String extension = file.get_extension();
	List<String> extensions;
	ECMAScriptLanguage::get_singleton()->get_recognized_extensions(&extensions);
	if (extensions.find(extension) == NULL) {
		for (List<String>::Element *E = extensions.front(); E; E = E->next()) {
			path = file + "." + E->get();
			if (FileAccess::exists(path)) {
				return path;
			}
		}
	}
	// try index file under the folder
	for (List<String>::Element *E = extensions.front(); E; E = E->next()) {
		path = file + "/index." + E->get();
		if (FileAccess::exists(path)) {
			return path;
		}
	}

	if (allow_node_module && !file.begins_with(".")) {
		String package_file = "node_modules/" + file + "/package.json";
		bool package_found = false;
		if (FileAccess::exists("user://" + package_file)) {
			package_file = "user://" + package_file;
			package_found = true;
		} else if (FileAccess::exists("res://" + package_file)) {
			package_file = "res://" + package_file;
			package_found = true;
		}
		if (package_found) {
			Error err;
			String package_content = FileAccess::get_file_as_string(package_file, &err);
			ERR_FAIL_COND_V_MSG(err != OK, "", "Fail to load module package: " + package_file);
			Variant package_parse_ret;
			String package_parse_err;
			int error_line;
			if (OK != JSON::parse(package_content, package_parse_ret, package_parse_err, error_line)) {
				ERR_FAIL_V_MSG("", "Fail to parse module package:" + package_file + ENDL + package_parse_err + ENDL + "At " + itos(error_line));
			}
			Dictionary dict = package_parse_ret;
			String entry = dict.has("main") ? dict["main"] : "index.js";
			entry = "node_modules/" + file + "/" + entry;
			ERR_FAIL_COND_V_MSG(!FileAccess::exists(entry), "", "Module entry does not exists: " + entry);
			return entry;
		}
	}
	return "";
}

JSModuleDef *QuickJSBinder::js_module_loader(JSContext *ctx, const char *module_name, void *opaque) {
	JSModuleDef *m = NULL;
	Error err;

	String resolving_file;
	resolving_file.parse_utf8(module_name);

	String file = resolve_module_file(resolving_file, false);
	ERR_FAIL_COND_V_MSG(file.empty(), NULL, "Failed to resolve module: '" + resolving_file + "'.");
	resolve_path_cache.set(resolving_file, file);

	QuickJSBinder *binder = QuickJSBinder::get_context_binder(ctx);
	if (ModuleCache *ptr = binder->module_cache.getptr(file)) {
		m = ptr->module;
	}

	if (!m) {
		List<String> extensions;
		ECMAScriptLanguage::get_singleton()->get_recognized_extensions(&extensions);
		if (extensions.find(file.get_extension()) != NULL) {
			Ref<ECMAScriptModule> em = ResourceFormatLoaderECMAScriptModule::load_static(file, "", &err);
			if (err != OK || !em.is_valid()) {
				JS_ThrowReferenceError(ctx, "Could not load module '%s'", file.utf8().get_data());
				return NULL;
			}
			ECMAscriptScriptError es_err;

			const Vector<uint8_t> &bytecode = em->get_bytecode();
			if (bytecode.size()) {
				ECMAScriptGCHandler ecma;
				if (binder->load_bytecode(bytecode, file, &ecma) == OK) {
					m = static_cast<JSModuleDef *>(ecma.ecma_object);
				}
			} else {
				String code = em->get_source_code();
				if (file.ends_with(EXT_JSON)) {
					code = "export default " + code;
				}
				if (ModuleCache *module = binder->js_compile_and_cache_module(ctx, code, file, &es_err)) {
					m = module->module;
				}
			}

		} else { // Try load as Resource
			RES res = ResourceLoader::load(file);
			if (res.is_null()) {
				JS_ThrowReferenceError(ctx, "Could not load module '%s'", file.utf8().get_data());
				return NULL;
			}
			// hack the quick module to make the resource value as default entry
			m = JS_NewCModule(ctx, file.utf8().get_data(), resource_module_initializer);
			JS_AddModuleExport(ctx, m, "default");
			JSValue func = JS_MKPTR(JS_TAG_MODULE, m);
			JS_DupValue(ctx, func);
			JS_EvalFunction(ctx, func);
			JSValue val = variant_to_var(ctx, res);
			JS_SetModuleExport(ctx, m, "default", val);

			ModuleCache module;
			Variant hash_var = res;
			module.hash = hash_var.hash();
			module.module = m;
			module.res = res;
			module.res->reference(); // Avoid auto release as module don't release automaticly
			module.res_value = val;
			module.flags = MODULE_FLAG_RESOURCE;
			module.module = static_cast<JSModuleDef *>(JS_VALUE_GET_PTR(func));
			binder->module_cache.set(file, module);
		}
	}

	return m;
}

QuickJSBinder::ModuleCache QuickJSBinder::js_compile_module(JSContext *ctx, const String &p_code, const String &p_filename, ECMAscriptScriptError *r_error) {

	if (NULL != compiling_modules.find(p_filename)) {
		String chain;
		for (List<String>::Element *E = compiling_modules.front(); E; E = E->next()) {
			chain += E->get();
			if (E->next() != NULL) {
				chain += " <- ";
			}
		}
		ERR_FAIL_V_MSG(ModuleCache(), vformat("Cyclic module import detected:\r\n  %s", chain));
	}

	ModuleCache module;
	module.flags = 0;
	module.module = NULL;
	module.hash = p_code.hash();

	CharString code = p_code.utf8();
	CharString filename = p_filename.utf8();
	const char *cfilename = filename.get_data();
	const char *cfilesource = code.get_data();

	compiling_modules.push_back(p_filename);
	JSValue func = JS_Eval(ctx, cfilesource, code.length(), cfilename, JS_EVAL_TYPE_MODULE | JS_EVAL_FLAG_COMPILE_ONLY);
	compiling_modules.pop_back();

	if (!JS_IsException(func)) {
		module.flags = MODULE_FLAG_SCRIPT;
		module.module = static_cast<JSModuleDef *>(JS_VALUE_GET_PTR(func));
		module.res_value = JS_UNDEFINED;
	} else {
		JSValue e = JS_GetException(ctx);
		dump_exception(ctx, e, r_error);
		JS_Throw(ctx, e);
	}

	return module;
}

QuickJSBinder::ModuleCache *QuickJSBinder::js_compile_and_cache_module(JSContext *ctx, const String &p_code, const String &p_filename, ECMAscriptScriptError *r_error) {

	QuickJSBinder *binder = QuickJSBinder::get_context_binder(ctx);
	ModuleCache *last_module = binder->module_cache.getptr(p_filename);
	if (last_module) {
		if (last_module->hash == p_code.hash()) {
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

	ModuleCache mc = js_compile_module(ctx, p_code, p_filename, r_error);
	mc.hash = p_code.hash();
	if (mc.module) {
		binder->module_cache.set(p_filename, mc);
	}
	return binder->module_cache.getptr(p_filename);
}

QuickJSBinder::ModuleCache *QuickJSBinder::js_compile_and_cache_module(JSContext *ctx, const Vector<uint8_t> &p_bytecode, const String &p_filename, ECMAscriptScriptError *r_error) {
	QuickJSBinder *binder = get_context_binder(ctx);
	ECMAScriptGCHandler module;
	if (OK == binder->load_bytecode(p_bytecode, p_filename, &module)) {
		return module_cache.getptr(p_filename);
	}
	return NULL;
}

Error QuickJSBinder::js_evalute_module(JSContext *ctx, QuickJSBinder::ModuleCache *p_module, ECMAscriptScriptError *r_error) {
	if (p_module->flags & MODULE_FLAG_EVALUATED)
		return OK;

	JSValue module = JS_MKPTR(JS_TAG_MODULE, p_module->module);
	if (JS_IsException(module)) {
		JSValue e = JS_GetException(ctx);
		dump_exception(ctx, e, r_error);
		JS_Throw(ctx, e);
		return ERR_PARSE_ERROR;
	}

	if (!(p_module->flags & MODULE_FLAG_EVALUATED)) {
		JSValue ret = JS_EvalFunction(ctx, module);
		if (JS_IsException(ret)) {
			JSValue e = JS_GetException(ctx);
			dump_exception(ctx, e, r_error);
			JS_Throw(ctx, e);
			return ERR_PARSE_ERROR;
		} else {
			p_module->flags |= MODULE_FLAG_EVALUATED;
		}
		JS_FreeValue(ctx, ret);
	}

	return OK;
}

int QuickJSBinder::resource_module_initializer(JSContext *ctx, JSModuleDef *m) {
	return JS_SetModuleExport(ctx, m, "default", JS_UNDEFINED);
}

JSValue QuickJSBinder::require_function(JSContext *ctx, JSValue this_val, int argc, JSValue *argv) {
	ERR_FAIL_COND_V(argc < 1 || !validate_type(ctx, Variant::STRING, argv[0]), JS_ThrowTypeError(ctx, "A string argument expected for require"));
	String file = js_to_string(ctx, argv[0]);
	if (file.begins_with(".")) {
		JSValue func = JS_GetStackFunction(ctx, 1);
		JSValue filename = JS_GetProperty(ctx, func, JS_ATOM_fileName);
		String caller_path = js_to_string(ctx, filename);
		JS_FreeValue(ctx, filename);
		JS_FreeValue(ctx, func);
		file = ECMAScriptLanguage::globalize_relative_path(file, caller_path.get_base_dir());
	}
	String resolving_file = file;
	file = resolve_module_file(file, true);
	ERR_FAIL_COND_V_MSG(file.empty(), (JS_UNDEFINED), "Failed to resolve module '" + resolving_file + "'.");
	resolve_path_cache.set(resolving_file, file);

	if (NULL != compiling_modules.find(file)) {
		String chain;
		for (List<String>::Element *E = compiling_modules.front(); E; E = E->next()) {
			chain += E->get();
			if (E->next() != NULL) {
				chain += " <- ";
			}
		}
		return JS_ThrowTypeError(ctx, "Cyclic module import detected:\r\n  %s", chain.utf8().get_data());
	}

	JSValue ret = JS_UNDEFINED;
	String md5 = FileAccess::get_md5(file);
	QuickJSBinder *binder = QuickJSBinder::get_context_binder(ctx);
	if (CommonJSModule *ptr = binder->commonjs_module_cache.getptr(md5)) {
		ret = JS_DupValue(ctx, ptr->exports);
	} else {
		CommonJSModule m;
		m.md5 = md5;
		m.exports = JS_UNDEFINED;
		List<String> extensions;
		ECMAScriptLanguage::get_singleton()->get_recognized_extensions(&extensions);
		if (extensions.find(file.get_extension()) != NULL) {
			Error err;
			Ref<ECMAScriptModule> em = ResourceFormatLoaderECMAScriptModule::load_static(file, "", &err);
			ERR_FAIL_COND_V(err != OK || em.is_null(), JS_ThrowTypeError(ctx, "Error to load module file %s", file.utf8().get_data()));
			String text = em->get_source_code();
			if (!text.empty()) {
				if (file.ends_with(EXT_JSON)) {
					CharString utf8code = text.utf8();
					ret = JS_ParseJSON(ctx, utf8code.get_data(), utf8code.length(), file.utf8().get_data());
				} else {
					String code = "(function() {"
								  "  const module = {"
								  "    exports: {}"
								  "  };"
								  "  let exports = module.exports;"
								  "  (function(){ " +
								  text +
								  "    }"
								  "  )();"
								  "  return module.exports;"
								  "})();";
					CharString utf8code = code.utf8();
					compiling_modules.push_back(file);
					ret = JS_Eval(ctx, utf8code.get_data(), utf8code.length(), file.utf8().get_data(), JS_EVAL_TYPE_GLOBAL | JS_EVAL_FLAG_STRICT);
					compiling_modules.pop_back();
				}
			} else {
				// TODO: require module from bytecode
			}
			m.exports = JS_DupValue(ctx, ret);
			m.flags = MODULE_FLAG_SCRIPT;
		} else {
			RES res = ResourceLoader::load(file);
			if (!res.is_null()) {
				ret = variant_to_var(ctx, res);
				m.exports = ret;
				m.flags = MODULE_FLAG_RESOURCE;
				m.res = res;
			} else {
				ret = JS_ThrowReferenceError(ctx, "Cannot load resource from '%s'", file.utf8().get_data());
			}
		}
		binder->commonjs_module_cache.set(md5, m);
	}
	return ret;
}

JSClassID QuickJSBinder::register_class(const ClassDB::ClassInfo *p_cls) {

	ClassBindData data;
	data.class_id = 0;
	data.base_class = NULL;

	if (class_remap.has(p_cls->name)) {
		data.class_name = class_remap[p_cls->name];
		data.jsclass.class_name = class_remap[p_cls->name];
		if (data.jsclass.class_name == "") {
			return 0;
		}
	} else {
		data.class_name = String(p_cls->name).ascii();
		data.jsclass.class_name = data.class_name.get_data();
	}

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
			JSValue method = JS_NewCFunctionMagic(ctx, &QuickJSBinder::object_method, name.get_data(), mb->get_argument_count(), JS_CFUNC_generic_magic, internal_godot_method_id);
			JS_DefinePropertyValueStr(ctx, data.prototype, name.get_data(), method, PROP_DEF_DEFAULT);

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

			JSValue setter = JS_UNDEFINED;
			JSValue getter = JS_UNDEFINED;

			if (prop.index >= 0) {
				int size = godot_object_indexed_properties.size();
				if (size <= internal_godot_indexed_property_id) {
					godot_object_indexed_properties.resize(size + 128);
				}
				godot_object_indexed_properties.write[internal_godot_indexed_property_id] = &prop;
				CharString name = String(*key).ascii();
				getter = JS_NewCFunctionMagic(ctx, &QuickJSBinder::object_indexed_property, name.get_data(), 0, JS_CFUNC_generic_magic, internal_godot_indexed_property_id);
				setter = JS_NewCFunctionMagic(ctx, &QuickJSBinder::object_indexed_property, name.get_data(), 1, JS_CFUNC_generic_magic, internal_godot_indexed_property_id);
				++internal_godot_indexed_property_id;
			} else {
				if (Map<StringName, JSValue>::Element *E = methods.find(prop.setter)) {
					setter = E->get();
					JS_DupValue(ctx, setter);
				}
				if (Map<StringName, JSValue>::Element *E = methods.find(prop.getter)) {
					getter = E->get();
					JS_DupValue(ctx, getter);
				}
			}

			JSAtom atom = get_atom(ctx, prop_name);
			JS_DefinePropertyGetSet(ctx, data.prototype, atom, getter, setter, PROP_DEF_DEFAULT);
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
	godot_origin_class.jsclass.class_name = godot_origin_class.class_name.get_data();
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
		if (JSClassID id = register_class(cls)) {
			gdclass_jsmap.insert(cls, id);
		}
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
	godot_reference_class = *classname_bindings.getptr("Reference");
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
		ECMAScriptGCHandler *data = new_gc_handler(ctx);
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
		JSValue js_func = JS_NewCFunctionMagic(ctx, godot_builtin_function, name.ascii().get_data(), 0, JS_CFUNC_generic_magic, i);
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
	// godot.instance_from_id
	JSValue js_godot_instance_from_id = JS_NewCFunction(ctx, godot_instance_from_id, "instance_from_id", 1);
	JS_DefinePropertyValueStr(ctx, godot_object, "instance_from_id", js_godot_instance_from_id, PROP_DEF_DEFAULT);

	// godot.abandon_value
	JSValue abandon_value_func = JS_NewCFunction(ctx, godot_abandon_value, "abandon_value", 1);
	JS_DefinePropertyValueStr(ctx, godot_object, "abandon_value", abandon_value_func, PROP_DEF_DEFAULT);
	// godot.adopt_value
	JSValue adopt_value_func = JS_NewCFunction(ctx, godot_adopt_value, "adopt_value", 1);
	JS_DefinePropertyValueStr(ctx, godot_object, "adopt_value", adopt_value_func, PROP_DEF_DEFAULT);

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
		// godot.DEBUG_METHODS_ENABLED
		JS_DefinePropertyValueStr(ctx, godot_object, "DEBUG_METHODS_ENABLED", JS_TRUE, JS_PROP_ENUMERABLE);
#endif
	}
}

QuickJSBinder::QuickJSBinder() {
	context_id = global_context_id++;
	internal_godot_method_id = 0;
	internal_godot_indexed_property_id = 0;
	godot_allocator.js_malloc = QuickJSBinder::js_binder_malloc;
	godot_allocator.js_free = QuickJSBinder::js_binder_free;
	godot_allocator.js_realloc = QuickJSBinder::js_binder_realloc;
	godot_allocator.js_malloc_usable_size = NULL;
	godot_object_class = NULL;
	godot_reference_class = NULL;

	if (class_remap.empty()) {
		class_remap.insert(_File::get_class_static(), "File");
		class_remap.insert(_Directory::get_class_static(), "Directory");
		class_remap.insert(_Thread::get_class_static(), "");
		class_remap.insert(_Mutex::get_class_static(), "");
		class_remap.insert(_Semaphore::get_class_static(), "");
	}
}

QuickJSBinder::~QuickJSBinder() {
}

void QuickJSBinder::initialize() {

	thread_id = Thread::get_caller_id();
	{
		GLOBAL_LOCK_FUNCTION
		ECMAScriptLanguage::get_singleton()->thread_binder_map.set(thread_id, this);
	}

	// create runtime and context for the binder
	runtime = JS_NewRuntime2(&godot_allocator, this);
	ctx = JS_NewContext(runtime);
	JS_AddIntrinsicOperators(ctx);

	JS_SetModuleLoaderFunc(runtime, /*js_module_resolve*/ NULL, js_module_loader, this);
	JS_SetContextOpaque(ctx, this);

	empty_function = JS_NewCFunction(ctx, js_empty_func, "virtual_fuction", 0);
	// global = globalThis
	global_object = JS_GetGlobalObject(ctx);
	// globalThis.Operators.create
	js_operators = JS_GetProperty(ctx, global_object, JS_ATOM_Operators);
	js_operators_create = JS_GetPropertyStr(ctx, js_operators, "create");
	// global.godot
	godot_object = JS_NewObject(ctx);
	js_key_godot_classid = JS_NewAtom(ctx, JS_HIDDEN_SYMBOL("native_class"));
	js_key_godot_classname = JS_NewAtom(ctx, JS_HIDDEN_SYMBOL("ecma_class"));
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
	// globalThis.Worker
	add_global_worker();
	// Other global properties
	add_global_properties();
	// globalThis.console
	add_global_console();
	// binding script
	String script_binding_error;

	ECMAScriptGCHandler eval_ret;
	if (OK == safe_eval_text(ECMAScriptBinder::BINDING_SCRIPT_CONTENT, ECMAScriptBinder::EVAL_TYPE_GLOBAL, "<internal: binding_script.js>", script_binding_error, eval_ret)) {
#ifdef TOOLS_ENABLED
		JSValue ret = JS_MKPTR(JS_TAG_OBJECT, eval_ret.ecma_object);
		modified_api = var_to_variant(ctx, ret);
		JS_FreeValue(ctx, ret);
#endif
	} else {
		CRASH_NOW_MSG("Execute script binding failed:" ENDL + script_binding_error);
	}

#ifdef QUICKJS_WITH_DEBUGGER
	debugger.instance();
	bool is_editor_hint = false;
	bool default_server_enabled = GLOBAL_DEF("JavaScript/debugger/enabled", false);
	int default_port = GLOBAL_DEF("JavaScript/debugger/port", 5556);
#ifdef TOOLS_ENABLED
	is_editor_hint = Engine::get_singleton()->is_editor_hint();
#endif
	if (!is_editor_hint) {
		List<String> args = OS::get_singleton()->get_cmdline_args();
		if (List<String>::Element *E = args.find("--js-debugger-connect")) {
			if (E->next() && E->next()->get().find(":") != -1) {
				String address = E->next()->get();
				Error err = debugger->connect(ctx, address);
				if (err != OK) {
					ERR_PRINTS(vformat("Failed to connect to JavaScript debugger at %s", address));
				}
			} else {
				ERR_PRINTS("Invalid debugger address");
			}
		} else if (List<String>::Element *E = args.find("--js-debugger-listen")) {
			if (E->next() && E->next()->get().find(":") != -1) {
				String address = E->next()->get();
				Error err = debugger->listen(ctx, address);
				if (err == OK) {
					print_line(vformat("JavaScript debugger started at %s", address));
				} else {
					ERR_PRINTS(vformat("Failed to start JavaScript debugger at %s", address));
				}
			} else {
				ERR_PRINTS("Invalid debugger address");
			}
		} else if (default_server_enabled) {
			String address = vformat("0.0.0.0:%d", default_port);
			Error err = debugger->listen(ctx, address);
			if (err == OK) {
				print_line(vformat("JavaScript debugger started at %s", address));
			} else {
				ERR_PRINTS(vformat("Failed to start JavaScript debugger at %s", address));
			}
		}
	} else {
#ifdef TOOLS_ENABLED
		// TODO: debug editor plugin support
#endif
	}

#endif
}

void QuickJSBinder::uninitialize() {

	godot_object_class = NULL;
	godot_reference_class = NULL;
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

	List<RES> module_resources;
	{ // modules
		const String *file = module_cache.next(NULL);
		while (file) {
			ModuleCache &m = module_cache.get(*file);
#if MODULE_HAS_REFCOUNT
			if (m.module) {
				JSValue val = JS_MKPTR(JS_TAG_MODULE, m.module);
				JS_FreeValue(ctx, val);
			}
#endif
			if (m.res.is_valid()) {
				JS_FreeValue(ctx, m.res_value);
				module_resources.push_back(m.res);
				m.res = RES();
			}
			file = module_cache.next(file);
		}
		module_cache.clear();
	}

	{ // commonjs modules
		const String *file = commonjs_module_cache.next(NULL);
		while (file) {
			CommonJSModule &m = commonjs_module_cache.get(*file);
			JS_FreeValue(ctx, m.exports);
			m.res = RES();
			file = commonjs_module_cache.next(file);
		}
		commonjs_module_cache.clear();
	}

	JS_FreeAtom(ctx, js_key_godot_classid);
	JS_FreeAtom(ctx, js_key_godot_classname);
	JS_FreeAtom(ctx, js_key_godot_tooled);
	JS_FreeAtom(ctx, js_key_godot_icon_path);
	JS_FreeAtom(ctx, js_key_godot_exports);
	JS_FreeAtom(ctx, js_key_godot_signals);
	JS_FreeValue(ctx, js_operators);
	JS_FreeValue(ctx, js_operators_create);
	JS_FreeValue(ctx, empty_function);
	JS_FreeValue(ctx, console_object);
	JS_FreeValue(ctx, global_object);

	JS_SetContextOpaque(ctx, NULL);
	JS_FreeContext(ctx);
	JS_FreeRuntime(runtime);

	for (List<RES>::Element *E = module_resources.front(); E; E = E->next()) {
		E->get()->unreference(); // Avoid imported resource leaking
	}

#ifdef QUICKJS_WITH_DEBUGGER
	debugger = Ref<QuickJSDebugger>();
#endif

	ctx = NULL;
	runtime = NULL;

	{
		GLOBAL_LOCK_FUNCTION
		ECMAScriptLanguage::get_singleton()->thread_binder_map.erase(thread_id);
	}
}

void QuickJSBinder::language_finalize() {

	GLOBAL_LOCK_FUNCTION
	transfer_deopot.clear();
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

	for (List<ECMAScriptGCHandler *>::Element *E = workers.front(); E; E = E->next()) {
		ECMAScriptGCHandler *bind = E->get();
		QuickJSWorker *worker = static_cast<QuickJSWorker *>(bind->native_ptr);
		JSValue object = JS_MKPTR(JS_TAG_OBJECT, bind->ecma_object);
		worker->frame_of_host(this, object);
	}

	const int64_t *id = frame_callbacks.next(NULL);
	while (id) {
		const ECMAScriptGCHandler &func = frame_callbacks.get(*id);
		JSValueConst js_func = JS_MKPTR(JS_TAG_OBJECT, func.ecma_object);
		double timestamp = OS::get_singleton()->get_ticks_usec() / 1000.0;
		JSValue argv[] = { JS_NewFloat64(ctx, timestamp) };
		JSValue ret = JS_Call(ctx, js_func, global_object, 1, argv);
		JS_FreeValue(ctx, argv[0]);
		if (JS_IsException(ret)) {
			JSValue e = JS_GetException(ctx);
			ECMAscriptScriptError err;
			dump_exception(ctx, e, &err);
			ERR_PRINTS("Error in requestAnimationFrame:" ENDL + error_to_string(err));
			JS_FreeValue(ctx, e);
		}
		id = frame_callbacks.next(id);
	}

#ifdef QUICKJS_WITH_DEBUGGER
	debugger->poll();
#endif
}

Error QuickJSBinder::eval_string(const String &p_source, EvalType type, const String &p_path, ECMAScriptGCHandler &r_ret) {
	String error;
	Error err = safe_eval_text(p_source, type, p_path, error, r_ret);
	if (err != OK && !error.empty()) {
		ERR_PRINTS(error);
	}
	return err;
}

Error QuickJSBinder::safe_eval_text(const String &p_source, EvalType type, const String &p_path, String &r_error, ECMAScriptGCHandler &r_ret) {
	ERR_FAIL_COND_V(p_source.empty(), FAILED);
	CharString utf8_str = p_source.utf8();
	const char *filename = p_path.utf8().get_data();
	const char *code = utf8_str.get_data();
	int flags = JS_EVAL_FLAG_STRICT;
	if (type == ECMAScriptBinder::EVAL_TYPE_MODULE) {
		flags |= JS_EVAL_TYPE_MODULE;
	} else {
		flags |= JS_EVAL_TYPE_GLOBAL;
	}
	JSValue ret = JS_Eval(ctx, code, utf8_str.length(), filename, flags);
	r_ret.context = ctx;
	r_ret.ecma_object = JS_VALUE_GET_PTR(ret);
	if (JS_IsException(ret)) {
		JSValue e = JS_GetException(ctx);
		ECMAscriptScriptError err;
		dump_exception(ctx, e, &err);
		r_error = error_to_string(err);
		JS_Throw(ctx, e);
		return ERR_PARSE_ERROR;
	}
	return OK;
}
Error QuickJSBinder::compile_to_bytecode(const String &p_code, const String &p_file, Vector<uint8_t> &r_bytecode) {
	ECMAscriptScriptError script_err;
	ModuleCache mc = js_compile_module(ctx, p_code, p_file, &script_err);
	if (mc.module) {
		JSValue module = JS_MKPTR(JS_TAG_MODULE, mc.module);
		size_t size;
		if (uint8_t *buf = JS_WriteObject(ctx, &size, module, JS_WRITE_OBJ_BYTECODE | JS_WRITE_OBJ_REFERENCE | JS_WRITE_OBJ_SAB)) {
			r_bytecode.resize(size);
			copymem(r_bytecode.ptrw(), buf, size);
			js_free(ctx, buf);
			JS_FreeValue(ctx, module);
		} else {
			ERR_FAIL_V(ERR_PARSE_ERROR);
		}
	} else {
		ERR_FAIL_V_MSG(ERR_PARSE_ERROR, error_to_string(script_err));
	}
	return OK;
}

Error QuickJSBinder::load_bytecode(const Vector<uint8_t> &p_bytecode, const String &p_file, ECMAScriptGCHandler *r_module) {

	Variant bytes = p_bytecode;
	if (ModuleCache *ptr = module_cache.getptr(p_file)) {
		if (bytes.hash() == ptr->hash) {
			r_module->ecma_object = ptr->module;
			return OK;
		}
	}

	JSValue value = JS_ReadObject(ctx, p_bytecode.ptr(), p_bytecode.size(), JS_READ_OBJ_BYTECODE | JS_READ_OBJ_REFERENCE | JS_READ_OBJ_SAB | JS_READ_OBJ_ROM_DATA);
	ERR_FAIL_COND_V(JS_VALUE_GET_TAG(value) != JS_TAG_MODULE, ERR_PARSE_ERROR);
	void *ptr = JS_VALUE_GET_PTR(value);
	r_module->ecma_object = ptr;

	ModuleCache mc;
	mc.flags = MODULE_FLAG_SCRIPT;
	mc.hash = bytes.hash();
	mc.module = static_cast<JSModuleDef *>(ptr);
	module_cache.set(p_file, mc);

	if (JS_ResolveModule(ctx, value) < 0) {
		JSValue e = JS_GetException(ctx);
		ECMAscriptScriptError err;
		dump_exception(ctx, e, &err);
		JS_Throw(ctx, e);
		ERR_FAIL_V_MSG(ERR_PARSE_ERROR, error_to_string(err));
	}
	return OK;
}

/************************* Memory Management ******************************/

void *QuickJSBinder::alloc_object_binding_data(Object *p_object) {
	ECMAScriptGCHandler *data = new_gc_handler(NULL); // pasa null context as all thread allocate from here
	lastest_allocated_object = data;
	return data;
}

void QuickJSBinder::free_object_binding_data(void *p_gc_handle) {
	ECMAScriptGCHandler *bind = (ECMAScriptGCHandler *)p_gc_handle;
	if (bind->is_object()) {
		JSValue js_obj = JS_MKPTR(JS_TAG_OBJECT, bind->ecma_object);
		JS_SetOpaque(js_obj, NULL);
		JS_FreeValue((JSContext *)bind->context, js_obj);
	}
	memdelete(bind);
}

Error QuickJSBinder::bind_gc_object(JSContext *ctx, ECMAScriptGCHandler *data, Object *p_object) {
	QuickJSBinder *binder = get_context_binder(ctx);
	const ClassBindData **bind_ptr = binder->classname_bindings.getptr(p_object->get_class_name());
	if (!bind_ptr)
		bind_ptr = binder->classname_bindings.getptr(p_object->get_parent_class_static());
	if (!bind_ptr) {
		bind_ptr = Object::cast_to<Reference>(p_object) == NULL ? &binder->godot_object_class : &binder->godot_reference_class;
#ifdef DEBUG_ENABLED
		WARN_PRINTS("Class " + p_object->get_class_name() + " is not registed to ClassDB");
#endif
	}
	if (bind_ptr) {
		JSValue obj = JS_NewObjectProtoClass(ctx, (*bind_ptr)->prototype, binder->get_origin_class_id());
		data->ecma_object = JS_VALUE_GET_PTR(obj);
		data->context = ctx;
		data->godot_object = p_object;
		data->type = Variant::OBJECT;
		data->flags = ECMAScriptGCHandler::FLAG_OBJECT;
		if (Reference *ref = Object::cast_to<Reference>(p_object)) {
			ref->reference();
			data->flags |= ECMAScriptGCHandler::FLAG_REFERENCE;
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
		return OK;
	}
	return FAILED;
}

void QuickJSBinder::godot_refcount_incremented(Reference *p_object) {
	ECMAScriptGCHandler *bind = BINDING_DATA_FROM_GD(p_object);
	if (bind->is_valid_ecma_object() && bind->is_reference()) {
		JSValue js_obj = JS_MKPTR(JS_TAG_OBJECT, bind->ecma_object);
		JS_DupValue((JSContext *)bind->context, js_obj); // JS ref_count ++
	}
}

bool QuickJSBinder::godot_refcount_decremented(Reference *p_object) {
	ECMAScriptGCHandler *bind = BINDING_DATA_FROM_GD(p_object);
	if (bind->is_valid_ecma_object() && bind->is_reference()) {
		JSValue js_obj = JS_MKPTR(JS_TAG_OBJECT, bind->ecma_object);
		JS_FreeValue((JSContext *)bind->context, js_obj); // JS ref_count --
		return bind->is_finalized();
	} else {
		return true;
	}
}

JSValue QuickJSBinder::object_constructor(JSContext *ctx, JSValueConst new_target, int argc, JSValueConst *argv, int class_id) {
	QuickJSBinder *binder = QuickJSBinder::get_context_binder(ctx);
	const ClassBindData &cls = binder->class_bindings.get(class_id);
	ECMAScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, new_target);
	JSValue js_obj;
	if (bind) {
		js_obj = new_target;
	} else {
		Object *gd_obj = cls.gdclass->creation_func();
		bind = BINDING_DATA_FROM_GD(ctx, gd_obj);
		js_obj = JS_MKPTR(JS_TAG_OBJECT, bind->ecma_object);

		if (JS_IsFunction(ctx, new_target)) {
			JSValue prototype = JS_GetProperty(ctx, new_target, QuickJSBinder::JS_ATOM_prototype);
			JS_SetPrototype(ctx, js_obj, prototype);
			JS_FreeValue(ctx, prototype);
		}

		// Make script and script instance for the object
		ECMAScriptInstance *si = memnew(ECMAScriptInstance);
		si->ecma_object = *bind;
		si->binder = binder;
		si->owner = gd_obj;
		JSValue es_class_name = JS_GetProperty(ctx, js_obj, binder->js_key_godot_classname);
		if (JS_IsString(es_class_name)) {
			if (ECMAClassInfo *es_class = binder->ecma_classes.getptr(js_to_string(ctx, es_class_name))) {
				si->script.instance();
				si->script->ecma_class = es_class;
				si->script->instances.insert(gd_obj);
				si->script->ecma_class = es_class;
				si->ecma_class = es_class;
				initialize_properties(ctx, es_class, js_obj);
			}
		}
		JS_FreeValue(ctx, es_class_name);
		gd_obj->set_script_instance(si);

		if (bind->is_reference()) { // restore the ref count added in bind_gc_object
			bind->flags ^= ECMAScriptGCHandler::FLAG_REFERENCE; // Diable the flag to avoid affects to js ref count by unreference call
			bind->godot_reference->ptr()->unreference();
			bind->flags |= ECMAScriptGCHandler::FLAG_REFERENCE;
		} else if (bind->is_object()) {
			JS_DupValue(ctx, js_obj);
		}
	}

	return js_obj;
}

void QuickJSBinder::initialize_properties(JSContext *ctx, const ECMAClassInfo *p_class, JSValue p_object) {
	const StringName *prop_name = p_class->properties.next(NULL);
	QuickJSBinder *binder = get_context_binder(ctx);
	while (prop_name) {
		JSAtom pname = get_atom(ctx, *prop_name);
		int ret = JS_SetProperty(ctx, p_object, pname, variant_to_var(ctx, p_class->properties.getptr(*prop_name)->default_value));
		if (ret < 0) {
			JSValue e = JS_GetException(ctx);
			ECMAscriptScriptError error;
			dump_exception(ctx, e, &error);
			JS_FreeValue(ctx, e);
			ERR_PRINTS(vformat("Cannot initialize property '%s' of class '%s'\n%s", *prop_name, p_class->class_name, binder->error_to_string(error)));
		}
		JS_FreeAtom(ctx, pname);
		prop_name = p_class->properties.next(prop_name);
	}
}

void QuickJSBinder::object_finalizer(ECMAScriptGCHandler *p_bind) {
	p_bind->flags ^= ECMAScriptGCHandler::FLAG_OBJECT;
	if (p_bind->is_reference()) {
		p_bind->flags ^= ECMAScriptGCHandler::FLAG_REFERENCE;
		memdelete(p_bind->godot_reference);
	}
}

void QuickJSBinder::origin_finalizer(JSRuntime *rt, JSValue val) {
	QuickJSBinder *binder = get_runtime_binder(rt);
	ECMAScriptGCHandler *bind = static_cast<ECMAScriptGCHandler *>(JS_GetOpaque(val, binder->godot_origin_class.class_id));
	if (bind) {
		bind->flags |= ECMAScriptGCHandler::FLAG_SCRIPT_FINALIZED;
		if (bind->type == Variant::OBJECT) {
			object_finalizer(bind);
		} else {
			binder->builtin_binder.builtin_finalizer(bind);
		}
		JS_SetOpaque(val, NULL);
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
		JS_ThrowTypeError(ctx, "ECMAClass class expected: %s", p_path.utf8().get_data());
		goto fail;
	}

	prototype = JS_GetProperty(ctx, p_constructor, QuickJSBinder::JS_ATOM_prototype);
	classid = JS_GetProperty(ctx, prototype, js_key_godot_classid);
	tooled = JS_GetProperty(ctx, p_constructor, js_key_godot_tooled);
	icon = JS_GetProperty(ctx, p_constructor, js_key_godot_icon_path);

	if (JS_IsUndefined(classid)) {
		JS_ThrowTypeError(ctx, "ECMAClass class expected: %s", p_path.utf8().get_data());
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
		ecma_class.prototype.context = ctx;
		ecma_class.prototype.ecma_object = JS_VALUE_GET_PTR(prototype);
		ecma_class.constructor.context = ctx;
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

				ECMAProperyInfo ei;
				ei.name = E->get();
				ei.type = Variant::NIL;
				ei.hint = PropertyHint::PROPERTY_HINT_NONE;
				ei.usage = PropertyUsageFlags::PROPERTY_USAGE_DEFAULT;

				JSAtom pname = get_atom(ctx, E->get());
				JSValue js_prop = JS_GetProperty(ctx, props, pname);
				Variant prop = var_to_variant(ctx, js_prop);

				int found_entry = 0;
				if (prop.get_type() == Variant::DICTIONARY) {
					Dictionary prop_info = prop;
					if (const Variant *ptr = prop_info.getptr("type")) {
						Variant::Type type = Variant::Type(int(*ptr));
						if (type > Variant::NIL && type < Variant::VARIANT_MAX) {
							ei.type = type;
							found_entry++;
						}
					}
					if (const Variant *ptr = prop_info.getptr("hint")) {
						PropertyHint hint = PropertyHint(int(*ptr));
						if (hint > PropertyHint::PROPERTY_HINT_NONE && hint < PropertyHint::PROPERTY_HINT_MAX) {
							ei.hint = hint;
							found_entry++;
						}
					}
					if (const Variant *ptr = prop_info.getptr("usage")) {
						ei.usage = *ptr;
						found_entry++;
					}
					if (const Variant *ptr = prop_info.getptr("hint_string")) {
						ei.hint_string = *ptr;
						found_entry++;
					}
					if (const Variant *ptr = prop_info.getptr("class_name")) {
						ei.class_name = *ptr;
						found_entry++;
					}
					if (const Variant *ptr = prop_info.getptr("default")) {
						ei.default_value = *ptr;
						found_entry++;
					}
				}

				if (found_entry <= 0) {
					ei.default_value = prop;
				}
				if (ei.type == Variant::NIL) {
					ei.type = ei.default_value.get_type();
				}
				JS_FreeAtom(ctx, pname);
				ecma_class.properties.set(E->get(), ei);
				JS_FreeValue(ctx, js_prop);
			}
		}
		JS_FreeValue(ctx, props);

		// methods
		Set<String> keys;
		get_own_property_names(ctx, prototype, &keys);
		for (Set<String>::Element *E = keys.front(); E; E = E->next()) {
			StringName method_name = E->get();
			JSAtom key = get_atom(ctx, method_name);
			JSValue value = JS_GetProperty(ctx, prototype, key);
			if (JS_IsFunction(ctx, value)) {
				MethodInfo mi;
				mi.name = E->get();
				ecma_class.methods.set(method_name, mi);
			}
			JS_FreeValue(ctx, value);
			JS_FreeAtom(ctx, key);
		}

		// cache the class
		if (const ECMAClassInfo *ptr = binder->ecma_classes.getptr(p_path)) {
			binder->free_ecmas_class(*ptr);
		}
		binder->ecma_classes.set(p_path, ecma_class);
		JS_DefinePropertyValue(ctx, prototype, js_key_godot_classname, to_js_string(ctx, p_path), PROP_DEF_DEFAULT);
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

Error QuickJSBinder::define_operators(JSContext *ctx, JSValue p_prototype, JSValue *p_operators, int p_size) {
	QuickJSBinder *binder = get_context_binder(ctx);
	JSValue operators = JS_Call(ctx, binder->js_operators_create, binder->js_operators, p_size, p_operators);
	if (JS_IsException(operators)) {
		ECMAscriptScriptError error;
		JSValue e = JS_GetException(ctx);
		dump_exception(ctx, e, &error);
		JS_FreeValue(ctx, e);
		ERR_PRINTS(binder->error_to_string(error));
		return FAILED;
	}
	JS_DefinePropertyValue(ctx, p_prototype, JS_ATOM_Symbol_operatorSet, operators, PROP_DEF_DEFAULT);
	return OK;
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

JSValue QuickJSBinder::global_request_animation_frame(JSContext *ctx, JSValue this_val, int argc, JSValue *argv) {
	ERR_FAIL_COND_V(argc < 1 || !JS_IsFunction(ctx, argv[0]), JS_ThrowTypeError(ctx, "Function expected for argument #0"));
	static int64_t id = 0;
	JSValue js_func = JS_DupValue(ctx, argv[0]);
	ECMAScriptGCHandler func;
	func.ecma_object = JS_VALUE_GET_PTR(js_func);
	QuickJSBinder *binder = get_context_binder(ctx);
	binder->frame_callbacks.set(++id, func);
	return JS_NewInt64(ctx, id);
}

JSValue QuickJSBinder::global_cancel_animation_frame(JSContext *ctx, JSValue this_val, int argc, JSValue *argv) {
	ERR_FAIL_COND_V(argc < 1 || !JS_IsNumber(argv[0]), JS_ThrowTypeError(ctx, "Request ID expected for argument #0"));
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

	ECMAScriptGCHandler *bind = BINDING_DATA_FROM_GD(ctx, p_object);
	ERR_FAIL_NULL_V(bind, ECMAScriptGCHandler());

	JSValue constructor = JS_MKPTR(JS_TAG_OBJECT, p_class->constructor.ecma_object);
	JSValue object = JS_MKPTR(JS_TAG_OBJECT, bind->ecma_object);
	JS_CallConstructor2(ctx, constructor, object, 0, NULL);
	if (JS_SetPrototype(ctx, object, JS_MKPTR(JS_TAG_OBJECT, p_class->prototype.ecma_object)) < 0) {
		JSValue e = JS_GetException(ctx);
		ECMAscriptScriptError error;
		dump_exception(ctx, e, &error);
		JS_FreeValue(ctx, e);
		bind->ecma_object = NULL;
		ERR_FAIL_V_MSG(*bind, vformat("Cannot create instance from ECMAScript class '%s'\n%s", p_class->class_name, error_to_string(error)));
	}
	initialize_properties(ctx, p_class, object);
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
		r_error.error = Variant::CallError::CALL_ERROR_INVALID_METHOD;
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

const ECMAClassInfo *QuickJSBinder::parse_ecma_class(const String &p_code, const String &p_path, bool ignore_cacehe, ECMAscriptScriptError *r_error) {
	if (!ignore_cacehe) {
		if (const ECMAClassInfo *cls = ecma_classes.getptr(p_path)) {
			return cls;
		}
	}
	ECMAscriptScriptError err;
	ModuleCache *mc = js_compile_and_cache_module(ctx, p_code, p_path, r_error);
	return parse_ecma_class_from_module(mc, p_path, r_error);
}

const ECMAClassInfo *QuickJSBinder::parse_ecma_class(const Vector<uint8_t> &p_bytecode, const String &p_path, bool ignore_cacehe, ECMAscriptScriptError *r_error) {
	if (!ignore_cacehe) {
		if (const ECMAClassInfo *cls = ecma_classes.getptr(p_path)) {
			return cls;
		}
	}
	ModuleCache *mc = js_compile_and_cache_module(ctx, p_bytecode, p_path, r_error);
	ERR_FAIL_NULL_V(mc, NULL);
	return parse_ecma_class_from_module(mc, p_path, r_error);
}

const ECMAClassInfo *QuickJSBinder::parse_ecma_class_from_module(ModuleCache *p_module, const String &p_path, ECMAscriptScriptError *r_error) {
	ERR_FAIL_COND_V(p_module == NULL || p_module->module == NULL, NULL);

	const ECMAClassInfo *ecma_class = NULL;
	JSValue default_entry = JS_UNDEFINED;
	if (OK != js_evalute_module(ctx, p_module, r_error)) {
		goto fail;
	}

	for (int i = 0; i < JS_GetModuleExportEntriesCount(p_module->module); i++) {
		JSAtom name = JS_GetModuleExportEntryName(ctx, p_module->module, i);
		if (name == JS_ATOM_default) {
			default_entry = JS_GetModuleExportEntry(ctx, p_module->module, i);
			JS_FreeAtom(ctx, name);
			break;
		}
		JS_FreeAtom(ctx, name);
	}
	if (!JS_IsFunction(ctx, default_entry)) {
		String err = "Failed parse ECMAClass from script " + p_path + ENDL "\t" + "Default export entry must be a godot class!";
		ERR_PRINTS(err);
		JS_ThrowTypeError(ctx, err.utf8().get_data());
		goto fail;
	}
	ecma_class = register_ecma_class(default_entry, p_path);
fail:
	JS_FreeValue(ctx, default_entry);
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

/********************************** Worker *************************************/
JSValue QuickJSBinder::worker_constructor(JSContext *ctx, JSValue this_val, int argc, JSValue *argv) {
	ERR_FAIL_COND_V(argc < 1 || !JS_IsString(argv[0]), JS_ThrowTypeError(ctx, "script path expected for argument #0"));
	QuickJSBinder *host = QuickJSBinder::get_context_binder(ctx);

	QuickJSWorker *worker = memnew(QuickJSWorker(host));
	worker->start(js_to_string(ctx, argv[0]));
	JSValue obj = JS_NewObjectProtoClass(ctx, host->worker_class_data.prototype, host->worker_class_data.class_id);

	ECMAScriptGCHandler *data = host->new_gc_handler(ctx);
	data->native_ptr = worker;
	data->ecma_object = JS_VALUE_GET_PTR(obj);
	JS_SetOpaque(obj, data);
	host->workers.push_back(data);

	return obj;
}

void QuickJSBinder::worker_finializer(JSRuntime *rt, JSValue val) {
	QuickJSBinder *host = QuickJSBinder::get_runtime_binder(rt);
	if (ECMAScriptGCHandler *bind = static_cast<ECMAScriptGCHandler *>(JS_GetOpaque(val, host->worker_class_data.class_id))) {
		QuickJSWorker *worker = static_cast<QuickJSWorker *>(bind->native_ptr);
		worker->stop();
		if (List<ECMAScriptGCHandler *>::Element *E = host->workers.find(bind)) {
			host->workers.erase(E);
		}
		memdelete(worker);
		memdelete(bind);
	}
}

JSValue QuickJSBinder::worker_post_message(JSContext *ctx, JSValue this_val, int argc, JSValue *argv) {
	ERR_FAIL_COND_V(argc < 1, JS_ThrowTypeError(ctx, "message value expected of argument #0"));
	QuickJSBinder *host = QuickJSBinder::get_context_binder(ctx);
	if (ECMAScriptGCHandler *bind = static_cast<ECMAScriptGCHandler *>(JS_GetOpaque(this_val, host->worker_class_data.class_id))) {
		QuickJSWorker *worker = static_cast<QuickJSWorker *>(bind->native_ptr);
		worker->post_message_from_host(var_to_variant(ctx, argv[0]));
	}
	return JS_UNDEFINED;
}

JSValue QuickJSBinder::worker_terminate(JSContext *ctx, JSValue this_val, int argc, JSValue *argv) {
	QuickJSBinder *host = QuickJSBinder::get_context_binder(ctx);
	if (ECMAScriptGCHandler *bind = static_cast<ECMAScriptGCHandler *>(JS_GetOpaque(this_val, host->worker_class_data.class_id))) {
		QuickJSWorker *worker = static_cast<QuickJSWorker *>(bind->native_ptr);
		if (List<ECMAScriptGCHandler *>::Element *E = host->workers.find(bind)) {
			host->workers.erase(E);
		}
		worker->stop();
	}
	return JS_UNDEFINED;
}

JSValue QuickJSBinder::godot_abandon_value(JSContext *ctx, JSValue this_val, int argc, JSValue *argv) {
	ERR_FAIL_COND_V(argc != 1, JS_ThrowTypeError(ctx, "one argument expected"));
	JSValue &value = argv[0];
	bool valid = true;
	Variant gd_value = var_to_variant(ctx, value);
	if (gd_value.get_type() == Variant::OBJECT) {
		ECMAScriptGCHandler *data = BINDING_DATA_FROM_JS(ctx, value);
		if (data) {
			JS_SetOpaque(value, NULL);
			if (data->type == Variant::OBJECT) {
				JS_FreeValue(ctx, value);
				if (data->is_reference()) {
					memdelete(data->godot_reference);
				}
			}
			data->godot_object = NULL;
			data->ecma_object = NULL;
			data->context = NULL;
			data->flags |= ECMAScriptGCHandler::FLAG_CONTEXT_TRANSFERABLE;
		} else {
			valid = false;
		}
	}

	uint64_t id = 0;
	if (valid) {
		id = atomic_increment(&global_transfer_id);
		GLOBAL_LOCK_FUNCTION
		transfer_deopot.set(id, gd_value);
	}
	return JS_NewInt64(ctx, id);
}

JSValue QuickJSBinder::godot_adopt_value(JSContext *ctx, JSValue this_val, int argc, JSValue *argv) {
	ERR_FAIL_COND_V(argc != 1 || !JS_IsNumber(argv[0]), JS_ThrowTypeError(ctx, "value id expected"));
	int64_t id = js_to_int64(ctx, argv[0]);
	ERR_FAIL_COND_V(id == 0, JS_ThrowTypeError(ctx, "id must greater than 0"));

	Variant value;
	{
		GLOBAL_LOCK_FUNCTION
		if (Variant *ptr = transfer_deopot.getptr(id)) {
			value = *ptr;
			transfer_deopot.erase(id);
		}
	}
	ERR_FAIL_NULL_V(value, JS_UNDEFINED);
	Object *obj = value;
	return variant_to_var(ctx, value);
}

void QuickJSBinder::add_global_worker() {
	worker_class_data.gdclass = NULL;
	worker_class_data.class_id = 0;
	worker_class_data.base_class = NULL;
	worker_class_data.class_name = "Worker";
	worker_class_data.jsclass.class_name = "Worker";
	worker_class_data.jsclass.finalizer = worker_finializer;
	worker_class_data.jsclass.exotic = NULL;
	worker_class_data.jsclass.gc_mark = NULL;
	worker_class_data.jsclass.call = NULL;
	worker_class_data.gdclass = NULL;
	worker_class_data.prototype = JS_NewObject(ctx);
	worker_class_data.constructor = JS_NewCFunction2(ctx, worker_constructor, worker_class_data.jsclass.class_name, worker_class_data.class_name.size(), JS_CFUNC_constructor, 0);

	// Worker.prototype.onmessage
	JS_SetPropertyStr(ctx, worker_class_data.prototype, "onmessage", JS_NULL);
	// Worker.prototype.postMessage
	JSValue post_message_func = JS_NewCFunction(ctx, worker_post_message, "postMessage", 1);
	JS_DefinePropertyValueStr(ctx, worker_class_data.prototype, "postMessage", post_message_func, PROP_DEF_DEFAULT);
	// Worker.prototype.terminate
	JSValue terminate_func = JS_NewCFunction(ctx, worker_terminate, "terminate", 1);
	JS_DefinePropertyValueStr(ctx, worker_class_data.prototype, "terminate", terminate_func, PROP_DEF_DEFAULT);

	JS_NewClassID(&worker_class_data.class_id);
	JS_NewClass(JS_GetRuntime(ctx), worker_class_data.class_id, &worker_class_data.jsclass);
	JS_SetClassProto(ctx, worker_class_data.class_id, worker_class_data.prototype);
	JS_SetConstructor(ctx, worker_class_data.constructor, worker_class_data.prototype);
	JS_DefinePropertyValueStr(ctx, global_object, "Worker", worker_class_data.constructor, PROP_DEF_DEFAULT);
}

/********************************* END Worker **********************************/

bool QuickJSBinder::validate(const String &p_code, const String &p_path, ECMAscriptScriptError *r_error) {
	ModuleCache *module = js_compile_and_cache_module(ctx, p_code, p_path, r_error);
	return module != NULL;
}
