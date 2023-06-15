#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/core_bind.h"
#include "core/core_constants.h"
#include "core/io/file_access.h"
#include "core/io/json.h"
#include "core/math/expression.h"
#include "core/os/os.h"

#ifdef TOOLS_ENABLED
#include "editor/editor_settings.h"
#endif

#include "javascript.h"
#include "javascript_instance.h"
#include "javascript_language.h"
#include "quickjs_binder.h"
#include "quickjs_callable.h"
#include "quickjs_worker.h"


#include <cstring>

SafeNumeric<uint32_t> QuickJSBinder::global_context_id;
SafeNumeric<uint64_t> QuickJSBinder::global_transfer_id;

HashMap<uint64_t, Variant> QuickJSBinder::transfer_deopot;
HashMap<String, const char *> QuickJSBinder::class_remap;
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
		if (ptr)
			memdelete_arr(ptr);
		if (arguments)
			memdelete_arr(arguments);
	}
};

_FORCE_INLINE_ static JavaScriptGCHandler *BINDING_DATA_FROM_GD(Object *p_object) {
	ERR_FAIL_COND_V(p_object == NULL, NULL);
	JavaScriptLanguage *lang = JavaScriptLanguage::get_singleton();
	JavaScriptGCHandler *bind = (JavaScriptGCHandler *)(p_object)->get_instance_binding(lang, lang->get_instance_binding_callbacks());
	return bind;
}

_FORCE_INLINE_ static JavaScriptGCHandler *BINDING_DATA_FROM_GD(JSContext *ctx, Object *p_object) {
	ERR_FAIL_COND_V(p_object == NULL || ctx == NULL, NULL);
	JavaScriptGCHandler *bind = BINDING_DATA_FROM_GD(p_object);
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
	PackedStringArray args;
	args.resize(argc);

	QuickJSBinder *binder = get_context_binder(ctx);
	JSValue object_to_json = JS_GetPropertyStr(ctx, binder->console_object, PROP_NAME_CONSOLE_LOG_OBJECT_TO_JSON);
	bool to_json = js_to_bool(ctx, object_to_json);
	JS_FreeValue(ctx, object_to_json);

	for (int i = 0; i < argc; ++i) {
		int32_t tag = JS_VALUE_GET_TAG(argv[i]);
		if (to_json && tag == JS_TAG_OBJECT && NULL == BINDING_DATA_FROM_JS(ctx, argv[i])) {
			JSValue str = JS_JSONStringify(ctx, argv[i], JS_UNDEFINED, JS_UNDEFINED);
			args.write[i] = js_to_string(ctx, str);
			JS_FreeValue(ctx, str);
		} else {
			size_t size;
			const char *utf8 = JS_ToCStringLen(ctx, &size, argv[i]);
			args.write[i].parse_utf8(utf8, size);
			JS_FreeCString(ctx, utf8);
		}
	}

	JavaScriptStackInfo stack_top;
	List<JavaScriptStackInfo> stacks;
	String message = "";
	for (int i = 0; i < argc; ++i) {
		message += args[i];
		if (i < argc - 1) {
			message += " ";
		}
	}

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
	// globalThis.requestAnimationFrame
	JSValue js_func_requestAnimationFrame = JS_NewCFunction(ctx, global_request_animation_frame, "requestAnimationFrame", 1);
	JS_DefinePropertyValueStr(ctx, global_object, "requestAnimationFrame", js_func_requestAnimationFrame, PROP_DEF_DEFAULT);
	// globalThis.cancelAnimationFrame
	JSValue js_func_cancelAnimationFrame = JS_NewCFunction(ctx, global_cancel_animation_frame, "cancelAnimationFrame", 1);
	JS_DefinePropertyValueStr(ctx, global_object, "cancelAnimationFrame", js_func_cancelAnimationFrame, PROP_DEF_DEFAULT);
}

JSValue QuickJSBinder::object_method(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv, int method_id) {
	JavaScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, this_val);
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

	Callable::CallError call_err;
	Variant ret_val = mb->call(obj, args.ptr, argc, call_err);
	JSValue ret = variant_to_var(ctx, ret_val);
#ifdef DEBUG_METHODS_ENABLED
	String err_message;
	switch (call_err.error) {
		case Callable::CallError::CALL_ERROR_INVALID_ARGUMENT:
			err_message = vformat("Argument of type '%s' is not assignable to parameter #%d of type '%s'", Variant::get_type_name(args.arguments[call_err.argument].get_type()), call_err.argument, Variant::get_type_name((Variant::Type)call_err.expected));
			break;
		case Callable::CallError::CALL_ERROR_INVALID_METHOD:
			err_message = vformat("Invalid method '%s' for type '%s'", mb->get_name(), obj->get_class_name());
			break;
		case Callable::CallError::CALL_ERROR_TOO_FEW_ARGUMENTS:
			err_message = vformat("Too few arguments for method '%s'", mb->get_name());
			break;
		case Callable::CallError::CALL_ERROR_TOO_MANY_ARGUMENTS:
			err_message = vformat("Too many arguments for method '%s'", mb->get_name());
			break;
		case Callable::CallError::CALL_ERROR_INSTANCE_IS_NULL:
			err_message = vformat("Attempt to call '%s' on a null instance.", mb->get_name());
			break;
		default:
			break;
	}
	if (call_err.error != Callable::CallError::CALL_OK) {
		List<JavaScriptStackInfo> stacks;
		String stack_message;
		if (binder->get_stacks(stacks) == OK) {
			stack_message = binder->get_backtrace_message(stacks);
		}
		ERR_PRINT(obj->get_class() + "." + mb->get_name() + ENDL + err_message + ENDL + stack_message);
		JS_FreeValue(ctx, ret);
		ret = JS_ThrowTypeError(ctx, "%s", err_message.utf8().get_data());
	}
#endif

	return ret;
}

JSValue QuickJSBinder::object_indexed_property(JSContext *ctx, JSValue this_val, int argc, JSValue *argv, int property_id) {
	JavaScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, this_val);
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

	Callable::CallError call_err;
	Variant ret_val = mb->call(obj, args.ptr, argc + 1, call_err);
	return variant_to_var(ctx, ret_val);
}

JSValue QuickJSBinder::variant_to_var(JSContext *ctx, const Variant p_var) {
	switch (p_var.get_type()) {
		case Variant::BOOL:
			return ((bool)p_var) ? JS_TRUE : JS_FALSE;
		case Variant::INT:
			return JS_NewInt64(ctx, int64_t(p_var));
		case Variant::FLOAT:
			return JS_NewFloat64(ctx, (double)(p_var));
		case Variant::NODE_PATH:
		case Variant::STRING:
			return to_js_string(ctx, p_var);
		case Variant::OBJECT: {
			Object *obj = p_var;
			if (obj == NULL)
				return JS_NULL;
			JavaScriptGCHandler *data = BINDING_DATA_FROM_GD(ctx, obj);
			ERR_FAIL_NULL_V(data, JS_UNDEFINED);
			ERR_FAIL_NULL_V(data->javascript_object, JS_UNDEFINED);
			ERR_FAIL_COND_V(data->context != ctx, (JS_UNDEFINED));
			JSValue js_obj = JS_MKPTR(JS_TAG_OBJECT, data->javascript_object);
			JS_DupValue(ctx, js_obj);

			return js_obj;
		}
		case Variant::ARRAY: {
			Array arr = p_var;
			JSValue js_arr = JS_NewArray(ctx);
			for (int i = 0; i < arr.size(); i++) {
				JS_SetPropertyUint32(ctx, js_arr, i, variant_to_var(ctx, arr[int(i)]));
			}
			return js_arr;
		}
		case Variant::DICTIONARY: {
			Dictionary dict = static_cast<Dictionary>(p_var);
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
				for (int i = 0; i < length; i++) {
					JSValue val = JS_GetPropertyUint32(ctx, p_val, i);
					arr[int(i)] = var_to_variant(ctx, val);
					JS_FreeValue(ctx, val);
				}
				return arr;
			} else if (JavaScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, p_val)) { // Binding object
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
	Callable::CallError err;
	String err_msg;
#if 0 // TODO: builtin functions
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

#endif

	return variant_to_var(ctx, ret);
}

bool QuickJSBinder::validate_type(JSContext *ctx, Variant::Type p_type, JSValueConst &p_val) {
	switch (p_type) {
		case Variant::NIL:
			return JS_IsNull(p_val) || JS_IsUndefined(p_val);
		case Variant::BOOL:
		case Variant::INT:
		case Variant::FLOAT:
			return JS_IsNumber(p_val) || JS_IsBool(p_val) || JS_IsNull(p_val) || JS_IsUndefined(p_val);
		case Variant::STRING:
			return JS_IsString(p_val);
		case Variant::DICTIONARY:
			return JS_IsObject(p_val);
		case Variant::ARRAY:
			return JS_IsArray(ctx, p_val);
		default: {
			JavaScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, p_val);
			return bind != NULL && bind->type == p_type;
		}
	}
}

void QuickJSBinder::dump_exception(JSContext *ctx, const JSValue &p_exception, JavaScriptError *r_error) {
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

String QuickJSBinder::error_to_string(const JavaScriptError &p_error) {
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

Error QuickJSBinder::get_stacks(List<JavaScriptStackInfo> &r_stacks) {
	JSValue error_constructor = JS_GetProperty(ctx, global_object, JS_ATOM_Error);
	JSValue error = JS_CallConstructor(ctx, error_constructor, 0, NULL);
	JSValue stack = JS_GetProperty(ctx, error, JS_ATOM_stack);
	String stack_text = js_to_string(ctx, stack);
	Vector<String> raw_stacks = stack_text.split("\n", false);
	for (int i = 1; i < raw_stacks.size(); i++) {
		String line_text = raw_stacks[i].strip_edges();
		if (line_text.is_empty())
			continue;
		JavaScriptStackInfo s;
		int colon = line_text.rfind(":");
		int bracket = line_text.rfind("(");
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

String QuickJSBinder::get_backtrace_message(const List<JavaScriptStackInfo> &stacks) {
	String message;
	for (const List<JavaScriptStackInfo>::Element *E = stacks.front(); E; E = E->next()) {
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
	HashSet<String> keys;
	get_own_property_names(ctx, p_val, &keys);
	stack.push_back(JS_VALUE_GET_PTR(p_val));
	for (const String &key : keys) {
		JSValue v = JS_GetPropertyStr(ctx, p_val, key.utf8().get_data());
		Variant val;
		if (JS_IsObject(v)) {
			void *ptr = JS_VALUE_GET_PTR(v);
			if (stack.find(ptr)) {
				union {
					const void *p;
					uint64_t i;
				} u;
				u.p = ptr;
				ERR_PRINT(vformat("Property '%s' circular reference to 0x%X", key, u.i));
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
		dict[key] = val;
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
	Ref<Resource> res = ResourceLoader::load(path);
	return variant_to_var(ctx, res);
}

JSValue QuickJSBinder::godot_instance_from_id(JSContext *ctx, JSValue this_val, int argc, JSValue *argv) {
#ifdef DEBUG_METHODS_ENABLED
	ERR_FAIL_COND_V(argc < 1 || !JS_IsNumber(argv[0]), JS_ThrowTypeError(ctx, "number expected for %s.%s", GODOT_OBJECT_NAME, "instance_from_id"));
#endif
	ObjectID id(js_to_uint64(ctx, argv[0]));
	Object *obj = ObjectDB::get_instance(id);
	return variant_to_var(ctx, obj);
}

JSValue QuickJSBinder::godot_object_method_connect(JSContext *ctx, JSValue this_val, int argc, JSValue *argv) {
#ifdef DEBUG_METHODS_ENABLED
	ERR_FAIL_COND_V(argc < 2 || !JS_IsString(argv[0]) || !JS_IsFunction(ctx, argv[1]), JS_ThrowTypeError(ctx, "string and function expected for %s.%s", "Object", "connect"));
#endif
	JavaScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, this_val);
	QuickJSBinder *binder = QuickJSBinder::get_context_binder(ctx);
	Object *obj = bind->get_godot_object();
	StringName signal = binder->js_to_string(ctx, argv[0]);
	Callable callable = memnew(QuickJSCallable(ctx, argv[1]));
	obj->connect(signal, callable);
	return JS_UNDEFINED;
}

void QuickJSBinder::add_debug_binding_info(JSContext *ctx, JSValueConst p_obj, const JavaScriptGCHandler *p_bind) {
	if (!p_bind)
		return;
	JSValue classname = JS_UNDEFINED;
	if (p_bind->type != Variant::OBJECT) {
		classname = to_js_string(ctx, Variant::get_type_name(p_bind->type));
	} else {
		if (p_bind->is_object()) {
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

String QuickJSBinder::resolve_module_file(const String &file) {
	if (const String *ptr = resolve_path_cache.getptr(file)) {
		return *ptr;
	}
	String path = file;
	if (FileAccess::exists(path))
		return path;
	// add extensions to try
	String extension = file.get_extension();
	List<String> extensions;
	JavaScriptLanguage::get_singleton()->get_recognized_extensions(&extensions);
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
	return "";
}

JSModuleDef *QuickJSBinder::js_module_loader(JSContext *ctx, const char *module_name, void *opaque) {
	JSModuleDef *m = NULL;
	Error err;

	String resolving_file;
	resolving_file.parse_utf8(module_name);

	String file = resolve_module_file(resolving_file);
	ERR_FAIL_COND_V_MSG(file.is_empty(), NULL, "Failed to resolve module: '" + resolving_file + "'.");
	resolve_path_cache.insert(resolving_file, file);

	QuickJSBinder *binder = QuickJSBinder::get_context_binder(ctx);
	if (ModuleCache *ptr = binder->module_cache.getptr(file)) {
		m = ptr->module;
	}

	if (!m) {
		List<String> extensions;
		JavaScriptLanguage::get_singleton()->get_recognized_extensions(&extensions);
		if (extensions.find(file.get_extension()) != NULL) {
			Ref<JavaScriptModule> em = ResourceFormatLoaderJavaScriptModule::load_static(file, "", &err);
			if (err != OK || !em.is_valid()) {
				JS_ThrowReferenceError(ctx, "Could not load module '%s'", file.utf8().get_data());
				return NULL;
			}
			JavaScriptError es_err;

			const Vector<uint8_t> &bytecode = em->get_bytecode();
			if (bytecode.size()) {
				JavaScriptGCHandler js;
				if (binder->load_bytecode(bytecode, file, &js) == OK) {
					m = static_cast<JSModuleDef *>(js.javascript_object);
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
			Ref<Resource> res = ResourceLoader::load(file);
			if (res.is_null()) {
				JS_ThrowReferenceError(ctx, "Could not load module '%s'", file.utf8().get_data());
				return NULL;
			}
			JSValue val = variant_to_var(ctx, res);
			m = js_make_module(ctx, file, val);
			JS_FreeValue(ctx, val);

			ModuleCache module;
			Variant hash_var = res;
			module.hash = hash_var.hash();
			module.module = m;
			module.res = res;
			module.res->reference(); // Avoid auto release as module don't release automaticly
			module.res_value = val;
			module.flags = MODULE_FLAG_RESOURCE;
			module.module = m;
			binder->module_cache.insert(file, module);
		}
	}

	return m;
}

JSModuleDef *QuickJSBinder::js_make_module(JSContext *ctx, const String &p_id, const JSValue &p_value) {
	JSModuleDef *m = JS_NewCModule(ctx, p_id.utf8().get_data(), resource_module_initializer);
	JS_AddModuleExport(ctx, m, "default");
	JSValue func = JS_MKPTR(JS_TAG_MODULE, m);
	JS_DupValue(ctx, func);
	JS_EvalFunction(ctx, func);
	JS_SetModuleExport(ctx, m, "default", JS_DupValue(ctx, p_value));
	return m;
}

QuickJSBinder::ModuleCache QuickJSBinder::js_compile_module(JSContext *ctx, const String &p_code, const String &p_filename, JavaScriptError *r_error) {
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

QuickJSBinder::ModuleCache *QuickJSBinder::js_compile_and_cache_module(JSContext *ctx, const String &p_code, const String &p_filename, JavaScriptError *r_error) {
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
		binder->module_cache.insert(p_filename, mc);
	}
	return binder->module_cache.getptr(p_filename);
}

QuickJSBinder::ModuleCache *QuickJSBinder::js_compile_and_cache_module(JSContext *ctx, const Vector<uint8_t> &p_bytecode, const String &p_filename, JavaScriptError *r_error) {
	QuickJSBinder *binder = get_context_binder(ctx);
	JavaScriptGCHandler module;
	if (OK == binder->load_bytecode(p_bytecode, p_filename, &module)) {
		return module_cache.getptr(p_filename);
	}
	return NULL;
}

Error QuickJSBinder::js_evalute_module(JSContext *ctx, QuickJSBinder::ModuleCache *p_module, JavaScriptError *r_error) {
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

JSClassID QuickJSBinder::register_class(const ClassDB::ClassInfo *p_cls) {
	ClassBindData data;
	data.class_id = 0;
	data.base_class = NULL;

	if (class_remap.has(p_cls->name)) {
		data.class_name = class_remap[p_cls->name];
		data.jsclass.class_name = class_remap[p_cls->name];
		if (strcmp(data.jsclass.class_name, "") == 0) {
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
	HashMap<StringName, JSValue> methods;
	{
		godot_methods.resize(internal_godot_method_id + p_cls->method_map.size());
		for (const KeyValue<StringName, MethodBind *> &pair : p_cls->method_map) {
			if (p_cls->name == "Object") {
				const char *connect = "connect";
				if (pair.key == connect) {
					JSValue func = JS_NewCFunction(ctx, godot_object_method_connect, connect, 3);
					JS_DefinePropertyValueStr(ctx, data.prototype, connect, func, PROP_DEF_DEFAULT);
					continue;
				}
			}

			MethodBind *mb = pair.value;
			godot_methods.set(internal_godot_method_id, mb);
			CharString name = String(pair.key).ascii();
			JSValue method = JS_NewCFunctionMagic(ctx, &QuickJSBinder::object_method, name.get_data(), mb->get_argument_count(), JS_CFUNC_generic_magic, internal_godot_method_id);
			JS_DefinePropertyValueStr(ctx, data.prototype, name.get_data(), method, PROP_DEF_DEFAULT);
			methods.insert(pair.key, method);
			++internal_godot_method_id;
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
		for (const KeyValue<StringName, ClassDB::PropertySetGet> &i : p_cls->property_setget) {
			const StringName &prop_name = i.key;
			const ClassDB::PropertySetGet &prop = i.value;

			JSValue setter = JS_UNDEFINED;
			JSValue getter = JS_UNDEFINED;

			if (prop.index >= 0) {
				int size = godot_object_indexed_properties.size();
				if (size <= internal_godot_indexed_property_id) {
					godot_object_indexed_properties.resize(size + 128);
				}
				godot_object_indexed_properties.write[internal_godot_indexed_property_id] = &prop;
				CharString name = String(prop_name).ascii();
				getter = JS_NewCFunctionMagic(ctx, &QuickJSBinder::object_indexed_property, name.get_data(), 0, JS_CFUNC_generic_magic, internal_godot_indexed_property_id);
				setter = JS_NewCFunctionMagic(ctx, &QuickJSBinder::object_indexed_property, name.get_data(), 1, JS_CFUNC_generic_magic, internal_godot_indexed_property_id);
				++internal_godot_indexed_property_id;
			} else {
				if (HashMap<StringName, JSValue>::ConstIterator it = methods.find(prop.setter)) {
					setter = it->value;
					JS_DupValue(ctx, setter);
				} else if (MethodBind *mb = prop._setptr) {
					if (godot_methods.size() >= internal_godot_method_id) {
						godot_methods.resize(godot_methods.size() + 1);
					}
					godot_methods.write[internal_godot_method_id] = mb;
					String setter_name = prop.setter;
					setter = JS_NewCFunctionMagic(ctx, &QuickJSBinder::object_method, setter_name.ascii().get_data(), mb->get_argument_count(), JS_CFUNC_generic_magic, internal_godot_method_id);
					++internal_godot_method_id;
				}

				if (HashMap<StringName, JSValue>::ConstIterator it = methods.find(prop.getter)) {
					getter = it->value;
					JS_DupValue(ctx, getter);
				} else if (MethodBind *mb = prop._getptr) {
					if (godot_methods.size() >= internal_godot_method_id) {
						godot_methods.resize(godot_methods.size() + 1);
					}
					godot_methods.write[internal_godot_method_id] = mb;
					String getter_name = prop.getter;
					getter = JS_NewCFunctionMagic(ctx, &QuickJSBinder::object_method, getter_name.ascii().get_data(), mb->get_argument_count(), JS_CFUNC_generic_magic, internal_godot_method_id);
					++internal_godot_method_id;
				}
			}

			JSAtom atom = get_atom(ctx, prop_name);
			JS_DefinePropertyGetSet(ctx, data.prototype, atom, getter, setter, PROP_DEF_DEFAULT);
			JS_FreeAtom(ctx, atom);
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
	for (const KeyValue<StringName, int64_t> &pair : p_cls->constant_map) {
		JSAtom atom = get_atom(ctx, pair.key);
		JS_DefinePropertyValue(ctx, data.constructor, atom, JS_NewInt64(ctx, pair.value), PROP_DEF_DEFAULT);
		JS_FreeAtom(ctx, atom);
	}

	// enumeration
	for (const KeyValue<StringName, ClassDB::ClassInfo::EnumInfo> &pair : p_cls->enum_map) {
		JSValue enum_obj = JS_NewObject(ctx);
		JSAtom atom = get_atom(ctx, pair.key);

		const List<StringName> &const_keys = pair.value.constants;
		for (const List<StringName>::Element *E = const_keys.front(); E; E = E->next()) {
			int value = p_cls->constant_map.get(E->get());
			JSAtom atom_key = get_atom(ctx, E->get());
			JS_DefinePropertyValue(ctx, enum_obj, atom_key, JS_NewInt32(ctx, value), PROP_DEF_DEFAULT);
			JS_FreeAtom(ctx, atom_key);
		}

		JS_DefinePropertyValue(ctx, data.constructor, atom, enum_obj, PROP_DEF_DEFAULT);
		JS_FreeAtom(ctx, atom);
	}

	// signals
	for (const KeyValue<StringName, MethodInfo> &pair : p_cls->signal_map) {
		JSAtom atom = get_atom(ctx, pair.key);
		JS_DefinePropertyValue(ctx, data.constructor, atom, to_js_string(ctx, pair.key), PROP_DEF_DEFAULT);
		JS_FreeAtom(ctx, atom);
	}

	class_bindings.insert(data.class_id, data);
	classname_bindings.insert(p_cls->name, class_bindings.getptr(data.class_id));

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
	HashMap<const ClassDB::ClassInfo *, JSClassID> gdclass_jsmap;
	// register classes
	for (const KeyValue<StringName, ClassDB::ClassInfo> &pair : ClassDB::classes) {
		const ClassDB::ClassInfo &cls = pair.value;
		if (JSClassID id = register_class(&cls)) {
			gdclass_jsmap.insert(&cls, id);
		}
	}

	// Setup inherits chain
	for (const KeyValue<const ClassDB::ClassInfo *, JSClassID> &pair : gdclass_jsmap) {
		const ClassDB::ClassInfo *gdcls = pair.key;
		ClassBindData &bind = class_bindings.get(pair.value);
		if (gdcls->inherits_ptr) {
			if (const HashMap<const ClassDB::ClassInfo *, JSClassID>::ConstIterator &base = gdclass_jsmap.find(gdcls->inherits_ptr)) {
				bind.base_class = class_bindings.getptr(base->value);
			}
		}
	}

	// Setup the prototype chain
	for (const KeyValue<JSClassID, ClassBindData> &pair : class_bindings) {
		const ClassBindData &data = pair.value;
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
	}

	godot_object_class = *classname_bindings.getptr("Object");
	godot_reference_class = *classname_bindings.getptr("RefCounted");
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
		JavaScriptGCHandler *data = new_gc_handler(ctx);
		data->javascript_object = JS_VALUE_GET_PTR(obj);
		data->godot_object = s.ptr;
		data->type = Variant::OBJECT;
		data->flags = JavaScriptGCHandler::FLAG_OBJECT;
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

		ClassDB::ClassInfo *gdcls = ClassDB::classes.getptr(s.name);
		gdcls = gdcls == NULL ? ClassDB::classes.getptr(s.ptr->get_class_name()) : gdcls;
		if (gdcls) {
			// constants
			for (KeyValue<StringName, int64_t> &pair : gdcls->constant_map) {
				JSAtom const_name = get_atom(ctx, pair.key);
				JS_DefinePropertyValue(ctx, obj, const_name, JS_NewInt64(ctx, pair.value), QuickJSBinder::PROP_DEF_DEFAULT);
				JS_FreeAtom(ctx, const_name);
			}
			// enumrations
			for (const KeyValue<StringName, ClassDB::ClassInfo::EnumInfo> &pair : gdcls->enum_map) {
				const List<StringName> &consts = pair.value.constants;
				JSValue enum_obj = JS_NewObject(ctx);
				for (const List<StringName>::Element *E2 = consts.front(); E2; E2 = E2->next()) {
					JSAtom const_name = get_atom(ctx, E2->get());
					JS_DefinePropertyValue(ctx, enum_obj, const_name, variant_to_var(ctx, gdcls->constant_map.get(E2->get())), QuickJSBinder::PROP_DEF_DEFAULT);
					JS_FreeAtom(ctx, const_name);
				}
				JSAtom enum_name = get_atom(ctx, pair.key);
				JS_DefinePropertyValue(ctx, obj, enum_name, enum_obj, QuickJSBinder::PROP_DEF_DEFAULT);
				JS_FreeAtom(ctx, enum_name);
			}
		}
	}

	// global constants
	HashMap<StringName, HashMap<StringName, int64_t>> global_constants;
	for (int i = 0; i < CoreConstants::get_global_constant_count(); ++i) {
		StringName enum_name = CoreConstants::get_global_constant_enum(i);
		const char *const_name = CoreConstants::get_global_constant_name(i);
		const int value = CoreConstants::get_global_constant_value(i);

		JSAtom js_const_name = JS_NewAtom(ctx, const_name);
		JS_DefinePropertyValue(ctx, godot_object, js_const_name, JS_MKVAL(JS_TAG_INT, value), QuickJSBinder::PROP_DEF_DEFAULT);
		JS_FreeAtom(ctx, js_const_name);

		if (HashMap<StringName, int64_t> *consts = global_constants.getptr(enum_name)) {
			consts->insert(const_name, value);
		} else {
			HashMap<StringName, int64_t> enum_;
			enum_.insert(const_name, value);
			global_constants.insert(enum_name, enum_);
		}
	}
	// global numbers
	GlobalNumberConstant consts[] = {
		{ "PI", Math_PI },
		{ "TAU", Math_TAU },
		{ "NAN", NAN },
		{ "INF", INFINITY },
		{ "E", Math_E },
		{ "LN2", Math_LN2 },
		{ "SQRT2", Math_SQRT2 },
		{ "SQRT12", Math_SQRT12 },
	};
	for (uint32_t i = 0; i < sizeof(consts) / sizeof(GlobalNumberConstant); i++) {
		const GlobalNumberConstant &c = consts[i];
		JSAtom js_const_name = get_atom(ctx, c.name);
		JS_DefinePropertyValue(ctx, godot_object, js_const_name, JS_NewFloat64(ctx, c.value), QuickJSBinder::PROP_DEF_DEFAULT);
		JS_FreeAtom(ctx, js_const_name);
	}

	// global enums
	for (const KeyValue<StringName, HashMap<StringName, int64_t>> &pair : global_constants) {
		String enum_name_str = pair.key;
		if (enum_name_str.is_empty())
			continue;
		enum_name_str = enum_name_str.replace(".", "");

		JSAtom atom_enum_name = get_atom(ctx, enum_name_str);
		JSValue enum_object = JS_NewObject(ctx);
		JS_DefinePropertyValue(ctx, godot_object, atom_enum_name, enum_object, QuickJSBinder::PROP_DEF_DEFAULT);
		JS_FreeAtom(ctx, atom_enum_name);

		for (const KeyValue<StringName, int64_t> &c : pair.value) {
			JSAtom js_const_name = get_atom(ctx, c.key);
			JS_DefinePropertyValue(ctx, enum_object, js_const_name, JS_NewInt64(ctx, c.value), QuickJSBinder::PROP_DEF_DEFAULT);
			JS_FreeAtom(ctx, js_const_name);
		}
	}

#if 0
	// builtin functions
	for (int i = 0; i < Expression::FUNC_MAX; ++i) {
		Expression::BuiltinFunc func = (Expression::BuiltinFunc)i;
		String name = Expression::get_func_name(func);
		JSValue js_func = JS_NewCFunctionMagic(ctx, godot_builtin_function, name.ascii().get_data(), 0, JS_CFUNC_generic_magic, i);
		JSAtom atom = get_atom(ctx, name);
		JS_DefinePropertyValue(ctx, godot_object, atom, js_func, QuickJSBinder::PROP_DEF_DEFAULT);
		JS_FreeAtom(ctx, atom);
	}
#endif

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

void QuickJSBinder::initialize() {
	thread_id = Thread::get_caller_id();
	{
		GLOBAL_LOCK_FUNCTION
		JavaScriptLanguage::get_singleton()->thread_binder_map.insert(thread_id, this);
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
	js_key_godot_classname = JS_NewAtom(ctx, JS_HIDDEN_SYMBOL("js_class"));
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
	JavaScriptGCHandler eval_ret;
	if (OK == safe_eval_text(JavaScriptBinder::BINDING_SCRIPT_CONTENT, JavaScriptBinder::EVAL_TYPE_GLOBAL, "<internal: binding_script.js>", script_binding_error, eval_ret)) {
#ifdef TOOLS_ENABLED
		if (eval_ret.javascript_object) {
			JSValue ret = JS_MKPTR(JS_TAG_OBJECT, eval_ret.javascript_object);
			modified_api = var_to_variant(ctx, ret);
			JS_FreeValue(ctx, ret);
		}
#endif
	} else {
		CRASH_NOW_MSG("Execute script binding failed:" ENDL + script_binding_error);
	}

#ifdef QUICKJS_WITH_DEBUGGER
	debugger.instantiate();
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
					ERR_PRINT(vformat("Failed to connect to JavaScript debugger at %s", address));
				}
			} else {
				ERR_PRINT("Invalid debugger address");
			}
		} else if (List<String>::Element *E2 = args.find("--js-debugger-listen")) {
			if (E2->next() && E2->next()->get().find(":") != -1) {
				String address = E2->next()->get();
				Error err = debugger->listen(ctx, address);
				if (err == OK) {
					print_line(vformat("JavaScript debugger started at %s", address));
				} else {
					ERR_PRINT(vformat("Failed to start JavaScript debugger at %s", address));
				}
			} else {
				ERR_PRINT("Invalid debugger address");
			}
		} else if (default_server_enabled) {
			String address = vformat("0.0.0.0:%d", default_port);
			Error err = debugger->listen(ctx, address);
			if (err == OK) {
				print_line(vformat("JavaScript debugger started at %s", address));
			} else {
				ERR_PRINT(vformat("Failed to start JavaScript debugger at %s", address));
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
		JavaScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, godot_singletons[i]);
		if (bind) {
			memdelete(bind);
			JS_SetOpaque(godot_singletons[i], NULL);
		}
		JS_FreeValue(ctx, godot_singletons[i]);
	}

	// Free registered js classes
	for (const KeyValue<String, JavaScriptClassInfo> &pair : javascript_classes) {
		const JavaScriptClassInfo &js_class = pair.value;
		free_javascript_class(js_class);
	}
	javascript_classes.clear();

	// Free frame callbacks
	for (const KeyValue<int64_t, JavaScriptGCHandler> &pair : frame_callbacks) {
		const JavaScriptGCHandler &func = pair.value;
		JSValueConst js_func = JS_MKPTR(JS_TAG_OBJECT, func.javascript_object);
		JS_FreeValue(ctx, js_func);
	}
	frame_callbacks.clear();

	// modules
	List<Ref<Resource>> module_resources;
	for (KeyValue<String, ModuleCache> &pair : module_cache) {
		ModuleCache &m = pair.value;
#if MODULE_HAS_REFCOUNT
		if (m.module) {
			JSValue val = JS_MKPTR(JS_TAG_MODULE, m.module);
			JS_FreeValue(ctx, val);
		}
#endif
		if (m.res.is_valid()) {
			JS_FreeValue(ctx, m.res_value);
			module_resources.push_back(m.res);
			m.res = Ref<Resource>();
		}
	}
	module_cache.clear();

	// commonjs modules
	for (KeyValue<String, CommonJSModule> &pair : commonjs_module_cache) {
		CommonJSModule &m = pair.value;
		JS_FreeValue(ctx, m.exports);
		m.res = Ref<Resource>();
	}
	commonjs_module_cache.clear();

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

	for (List<Ref<Resource>>::Element *E = module_resources.front(); E; E = E->next()) {
		E->get()->unreference(); // Avoid imported resource leaking
	}

#ifdef QUICKJS_WITH_DEBUGGER
	debugger = Ref<QuickJSDebugger>();
#endif

	ctx = NULL;
	runtime = NULL;

	{
		GLOBAL_LOCK_FUNCTION
		JavaScriptLanguage::get_singleton()->thread_binder_map.erase(thread_id);
	}
}

void QuickJSBinder::language_finalize() {
	GLOBAL_LOCK_FUNCTION
	transfer_deopot.clear();
}

void QuickJSBinder::frame() {
	JSContext *ctx1;
	for (;;) {
		int err = JS_ExecutePendingJob(JS_GetRuntime(ctx), &ctx1);
		if (err <= 0) {
			if (err < 0) {
				JavaScriptError script_err;
				JSValue e = JS_GetException(ctx1);
				dump_exception(ctx1, e, &script_err);
				ERR_PRINT(error_to_string(script_err));
				JS_FreeValue(ctx1, e);
			}
			break;
		}
	}

	for (List<JavaScriptGCHandler *>::Element *E = workers.front(); E; E = E->next()) {
		JavaScriptGCHandler *bind = E->get();
		QuickJSWorker *worker = static_cast<QuickJSWorker *>(bind->native_ptr);
		JSValue object = JS_MKPTR(JS_TAG_OBJECT, bind->javascript_object);
		worker->frame_of_host(this, object);
	}

	for (const KeyValue<int64_t, JavaScriptGCHandler> &pair : frame_callbacks) {
		if (canceled_frame_callbacks.has(pair.key))
			continue;
		const JavaScriptGCHandler &func = pair.value;
		JSValueConst js_func = JS_MKPTR(JS_TAG_OBJECT, func.javascript_object);
		double timestamp = OS::get_singleton()->get_ticks_usec() / 1000.0;
		JSValue argv[] = { JS_NewFloat64(ctx, timestamp) };
		JSValue ret = JS_Call(ctx, js_func, global_object, 1, argv);
		JS_FreeValue(ctx, argv[0]);
		if (JS_IsException(ret)) {
			JSValue e = JS_GetException(ctx);
			JavaScriptError err;
			dump_exception(ctx, e, &err);
			ERR_PRINT("Error in requestAnimationFrame:" ENDL + error_to_string(err));
			JS_FreeValue(ctx, e);
		}
	}
	for (const KeyValue<int64_t, JavaScriptGCHandler> &pair : frame_callbacks) {
		const JavaScriptGCHandler &func = pair.value;
		JSValueConst js_func = JS_MKPTR(JS_TAG_OBJECT, func.javascript_object);
		JS_FreeValue(ctx, js_func);
	}
	frame_callbacks.clear();
	canceled_frame_callbacks.clear();

#ifdef QUICKJS_WITH_DEBUGGER
	debugger->poll();
#endif
}

Error QuickJSBinder::eval_string(const String &p_source, EvalType type, const String &p_path, JavaScriptGCHandler &r_ret) {
	String error;
	Error err = safe_eval_text(p_source, type, p_path, error, r_ret);
	if (err != OK && !error.is_empty()) {
		ERR_PRINT(error);
	}
	return err;
}

Error QuickJSBinder::safe_eval_text(const String &p_source, EvalType type, const String &p_path, String &r_error, JavaScriptGCHandler &r_ret) {
	ERR_FAIL_COND_V(p_source.is_empty(), FAILED);
	CharString utf8_str = p_source.utf8();
	const char *filename = p_path.utf8().get_data();
	const char *code = utf8_str.get_data();
	int flags = JS_EVAL_FLAG_STRICT;
	if (type == JavaScriptBinder::EVAL_TYPE_MODULE) {
		flags |= JS_EVAL_TYPE_MODULE;
	} else {
		flags |= JS_EVAL_TYPE_GLOBAL;
	}
	JSValue ret = JS_Eval(ctx, code, utf8_str.length(), filename, flags);
	r_ret.context = ctx;
	r_ret.javascript_object = JS_VALUE_GET_PTR(ret);
	if (JS_IsException(ret)) {
		JSValue e = JS_GetException(ctx);
		JavaScriptError err;
		dump_exception(ctx, e, &err);
		r_error = error_to_string(err);
		JS_Throw(ctx, e);
		return ERR_PARSE_ERROR;
	}
	return OK;
}
Error QuickJSBinder::compile_to_bytecode(const String &p_code, const String &p_file, Vector<uint8_t> &r_bytecode) {
	JavaScriptError script_err;
	ModuleCache mc = js_compile_module(ctx, p_code, p_file, &script_err);
	if (mc.module) {
		JSValue module = JS_MKPTR(JS_TAG_MODULE, mc.module);
		size_t size;
		if (uint8_t *buf = JS_WriteObject(ctx, &size, module, JS_WRITE_OBJ_BYTECODE | JS_WRITE_OBJ_REFERENCE | JS_WRITE_OBJ_SAB)) {
			r_bytecode.resize(size);
			memcpy(r_bytecode.ptrw(), buf, size);
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

Error QuickJSBinder::load_bytecode(const Vector<uint8_t> &p_bytecode, const String &p_file, JavaScriptGCHandler *r_module) {
	Variant bytes = p_bytecode;
	if (ModuleCache *ptr = module_cache.getptr(p_file)) {
		if (bytes.hash() == ptr->hash) {
			r_module->javascript_object = ptr->module;
			return OK;
		}
	}

	JSValue value = JS_ReadObject(ctx, p_bytecode.ptr(), p_bytecode.size(), JS_READ_OBJ_BYTECODE | JS_READ_OBJ_REFERENCE | JS_READ_OBJ_SAB | JS_READ_OBJ_ROM_DATA);
	ERR_FAIL_COND_V(JS_VALUE_GET_TAG(value) != JS_TAG_MODULE, ERR_PARSE_ERROR);
	void *ptr = JS_VALUE_GET_PTR(value);
	r_module->javascript_object = ptr;

	ModuleCache mc;
	mc.flags = MODULE_FLAG_SCRIPT;
	mc.hash = bytes.hash();
	mc.module = static_cast<JSModuleDef *>(ptr);
	module_cache.insert(p_file, mc);

	if (JS_ResolveModule(ctx, value) < 0) {
		JSValue e = JS_GetException(ctx);
		JavaScriptError err;
		dump_exception(ctx, e, &err);
		JS_Throw(ctx, e);
		ERR_FAIL_V_MSG(ERR_PARSE_ERROR, error_to_string(err));
	}
	return OK;
}

/************************* Memory Management ******************************/

JavaScriptGCHandler *QuickJSBinder::alloc_object_binding_data(Object *p_object) {
	JavaScriptGCHandler *data = new_gc_handler(nullptr);
	bind_gc_object(ctx, data, p_object);
	return data;
}

void QuickJSBinder::free_object_binding_data(JavaScriptGCHandler *p_gc_handle) {
	JavaScriptGCHandler *bind = (JavaScriptGCHandler *)p_gc_handle;
	if (!bind->is_ref_counted()) {
		JSValue js_obj = JS_MKPTR(JS_TAG_OBJECT, bind->javascript_object);
		JS_SetOpaque(js_obj, NULL);
		JS_FreeValue((JSContext *)bind->context, js_obj);
	}
	memdelete(bind);
}

Error QuickJSBinder::bind_gc_object(JSContext *ctx, JavaScriptGCHandler *data, Object *p_object) {
	QuickJSBinder *binder = get_context_binder(ctx);
	const ClassBindData **bind_ptr = binder->classname_bindings.getptr(p_object->get_class_name());
	if (!bind_ptr)
		bind_ptr = binder->classname_bindings.getptr(p_object->get_parent_class_static());
	if (!bind_ptr) {
		bind_ptr = Object::cast_to<RefCounted>(p_object) == NULL ? &binder->godot_object_class : &binder->godot_reference_class;
#ifdef DEBUG_ENABLED
		WARN_PRINT("Class " + p_object->get_class_name() + " is not registed to ClassDB");
#endif
	}
	if (bind_ptr) {
		JSValue obj = JS_NewObjectProtoClass(ctx, (*bind_ptr)->prototype, binder->get_origin_class_id());
		data->javascript_object = JS_VALUE_GET_PTR(obj);
		data->context = ctx;
		data->godot_object = p_object;
		data->type = Variant::OBJECT;
		data->flags = JavaScriptGCHandler::FLAG_OBJECT;
		if (p_object->is_ref_counted()) {
			if (static_cast<RefCounted *>(p_object)->init_ref()) {
				data->flags |= JavaScriptGCHandler::FLAG_REF_COUNTED;
			}
		}
		JS_SetOpaque(obj, data);
#ifdef DUMP_LEAKS
		add_debug_binding_info(ctx, obj, data);
		JS_DefinePropertyValueStr(ctx, obj, "__id__", to_js_number(ctx, static_cast<uint64_t>(p_object->get_instance_id())), PROP_DEF_DEFAULT);
#endif
		return OK;
	}
	return FAILED;
}

void QuickJSBinder::godot_refcount_incremented(JavaScriptGCHandler *bind) {
	if (bind->is_valid_javascript_object()) {
		JSValue js_obj = JS_MKPTR(JS_TAG_OBJECT, bind->javascript_object);
		JS_DupValue((JSContext *)bind->context, js_obj);
	}
}

bool QuickJSBinder::godot_refcount_decremented(JavaScriptGCHandler *bind) {
	if (bind->is_valid_javascript_object()) {
		JSValue js_obj = JS_MKPTR(JS_TAG_OBJECT, bind->javascript_object);
		JS_FreeValue((JSContext *)bind->context, js_obj);
		return bind->is_finalized();
	} else {
		return true;
	}
}

JSValue QuickJSBinder::object_constructor(JSContext *ctx, JSValueConst new_target, int argc, JSValueConst *argv, int class_id) {
	QuickJSBinder *binder = QuickJSBinder::get_context_binder(ctx);
	const ClassBindData &cls = binder->class_bindings.get(class_id);
	JavaScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, new_target);
	JSValue js_obj;
	if (bind) {
		js_obj = new_target;
	} else {
		Object *gd_obj = cls.gdclass->creation_func();
		JavaScriptLanguage *lang = JavaScriptLanguage::get_singleton();
		bind = new_gc_handler(ctx);
		bind_gc_object(ctx, bind, gd_obj);
		gd_obj->set_instance_binding(lang, bind, lang->get_instance_binding_callbacks());
		js_obj = JS_MKPTR(JS_TAG_OBJECT, bind->javascript_object);

		if (JS_IsFunction(ctx, new_target)) {
			JSValue prototype = JS_GetProperty(ctx, new_target, QuickJSBinder::JS_ATOM_prototype);
			JS_SetPrototype(ctx, js_obj, prototype);
			JS_FreeValue(ctx, prototype);
		}

#if 0
		// Make script and script instance for the object
		JavaScriptInstance *si = memnew(JavaScriptInstance);
		si->javascript_object = *bind;
		si->binder = binder;
		si->owner = gd_obj;
		JSValue js_class_name = JS_GetProperty(ctx, js_obj, binder->js_key_godot_classname);
		if (JS_IsString(js_class_name)) {
			if (JavaScriptClassInfo *js_class = binder->javascript_classes.getptr(js_to_string(ctx, js_class_name))) {
				si->script.instantiate();
				si->script->javascript_class = js_class;
				si->script->instances.insert(gd_obj);
				si->script->javascript_class = js_class;
				si->javascript_class = js_class;
				initialize_properties(ctx, js_class, js_obj);
			}
		}
		JS_FreeValue(ctx, js_class_name);
		gd_obj->set_script_instance(si);
#endif
		if (!bind->is_ref_counted()) { // Object need to be freed manually
			JS_DupValue(ctx, js_obj);
		}
	}
	return js_obj;
}

void QuickJSBinder::initialize_properties(JSContext *ctx, const JavaScriptClassInfo *p_class, JSValue p_object) {
	QuickJSBinder *binder = get_context_binder(ctx);
	for (const KeyValue<StringName, JavaScriptProperyInfo> &pair : p_class->properties) {
		JSAtom pname = get_atom(ctx, pair.key);
		int ret = JS_SetProperty(ctx, p_object, pname, variant_to_var(ctx, pair.value.default_value));
		if (ret < 0) {
			JSValue e = JS_GetException(ctx);
			JavaScriptError error;
			dump_exception(ctx, e, &error);
			JS_FreeValue(ctx, e);
			ERR_PRINT(vformat("Cannot initialize property '%s' of class '%s'\n%s", pair.key, p_class->class_name, binder->error_to_string(error)));
		}
		JS_FreeAtom(ctx, pname);
	}
}

void QuickJSBinder::object_finalizer(JavaScriptGCHandler *p_bind) {
	p_bind->flags ^= JavaScriptGCHandler::FLAG_OBJECT;
	if (p_bind->godot_object->is_ref_counted()) {
		RefCounted *ref = static_cast<RefCounted *>(p_bind->godot_object);
		if (ref->unreference()) {
			memdelete(ref);
		}
	}
}

void QuickJSBinder::origin_finalizer(JSRuntime *rt, JSValue val) {
	QuickJSBinder *binder = get_runtime_binder(rt);
	JavaScriptGCHandler *bind = static_cast<JavaScriptGCHandler *>(JS_GetOpaque(val, binder->godot_origin_class.class_id));
	if (bind) {
		bind->flags |= JavaScriptGCHandler::FLAG_FINALIZED;
		if (bind->type == Variant::OBJECT) {
			object_finalizer(bind);
		} else {
			binder->builtin_binder.builtin_finalizer(bind);
		}
		JS_SetOpaque(val, NULL);
	}
}

JSValue QuickJSBinder::object_free(JSContext *ctx, JSValue this_val, int argc, JSValue *argv) {
	JavaScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, this_val);
	ERR_FAIL_NULL_V(bind, JS_ThrowReferenceError(ctx, "The object already be freed"));
	ERR_FAIL_NULL_V(bind->godot_object, JS_ThrowReferenceError(ctx, "The object already be freed"));
	ERR_FAIL_COND_V(bind->godot_object->is_ref_counted(), JS_ThrowReferenceError(ctx, "Call free to RefCounted object is not allowed"));

	memdelete(bind->godot_object);
	JS_SetOpaque(this_val, NULL);
	return JS_UNDEFINED;
}

/************************* END Memory Management ******************************/

/********************************* Script --> C++ ****************************/
JSValue QuickJSBinder::godot_register_class(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
	ERR_FAIL_COND_V(argc < 1 || !JS_IsFunction(ctx, argv[0]), JS_ThrowTypeError(ctx, "Godot class expected"));
	QuickJSBinder *binder = get_context_binder(ctx);
	String class_name;
#if NO_MODULE_EXPORT_SUPPORT
	class_name = binder->parsing_script_file;
#endif
	if (class_name.is_empty()) {
		if (argc > 1 && JS_IsString(argv[1])) {
			class_name = js_to_string(ctx, argv[1]);
		} else {
			JSValue name = JS_GetProperty(ctx, *argv, JS_ATOM_name);
			class_name = js_to_string(ctx, name);
			JS_FreeValue(ctx, name);
		}
	}
	binder->register_javascript_class(argv[0], class_name);
	return JS_UNDEFINED;
}

const JavaScriptClassInfo *QuickJSBinder::register_javascript_class(const JSValue &p_constructor, const String &p_path) {
	QuickJSBinder *binder = get_context_binder(ctx);
	JSValue prototype = JS_UNDEFINED;
	JSValue classid = JS_UNDEFINED;
	JSValue tooled = JS_UNDEFINED;
	JSValue icon = JS_UNDEFINED;
	JSClassID id = 0;

	if (!JS_IsFunction(ctx, p_constructor)) {
		JS_ThrowTypeError(ctx, "Godot class expected: %s", p_path.utf8().get_data());
		goto fail;
	}

	prototype = JS_GetProperty(ctx, p_constructor, QuickJSBinder::JS_ATOM_prototype);
	classid = JS_GetProperty(ctx, prototype, js_key_godot_classid);
	tooled = JS_GetProperty(ctx, p_constructor, js_key_godot_tooled);
	icon = JS_GetProperty(ctx, p_constructor, js_key_godot_icon_path);

	if (JS_IsUndefined(classid)) {
		JS_ThrowTypeError(ctx, "Godot class expected: %s", p_path.utf8().get_data());
		goto fail;
	}

	id = JS_VALUE_GET_INT(classid);
	if (const ClassBindData *bind = binder->class_bindings.getptr(id)) {
		// Add reference to the class function
		JS_DupValue(ctx, p_constructor);

		JSValue name = JS_GetProperty(ctx, p_constructor, JS_ATOM_name);
		StringName class_name = js_to_string(ctx, name);
		JS_FreeValue(ctx, name);

		JavaScriptClassInfo js_class;
		js_class.icon_path = "";
		js_class.tool = false;
		js_class.native_class = bind->gdclass;
		js_class.class_name = class_name;
		js_class.prototype.context = ctx;
		js_class.prototype.javascript_object = JS_VALUE_GET_PTR(prototype);
		js_class.constructor.context = ctx;
		js_class.constructor.javascript_object = JS_VALUE_GET_PTR(p_constructor);
		js_class.tool = JS_ToBool(ctx, tooled);
		if (JS_IsString(icon)) {
			js_class.icon_path = js_to_string(ctx, icon);
		}

		// signals
		JSValue signals = JS_GetProperty(ctx, prototype, js_key_godot_signals);
		if (JS_IsObject(signals)) {
			HashSet<String> keys;
			get_own_property_names(ctx, signals, &keys);
			for (const String &key : keys) {
				MethodInfo mi;
				mi.name = key;
				js_class.signals.insert(mi.name, mi);
			}
		}
		JS_FreeValue(ctx, signals);

		// properties
		JSValue props = JS_GetProperty(ctx, prototype, js_key_godot_exports);
		if (JS_IsObject(props)) {
			HashSet<String> keys;
			get_own_property_names(ctx, props, &keys);
			for (const String &key : keys) {
				JavaScriptProperyInfo ei;
				ei.name = key;
				ei.type = Variant::NIL;
				ei.hint = PropertyHint::PROPERTY_HINT_NONE;
				ei.usage = PropertyUsageFlags::PROPERTY_USAGE_DEFAULT;

				JSAtom pname = get_atom(ctx, key);
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
				js_class.properties.insert(key, ei);
				JS_FreeValue(ctx, js_prop);
			}
		}
		JS_FreeValue(ctx, props);

		// methods
		HashSet<String> keys;
		get_own_property_names(ctx, prototype, &keys);
		for (const String &E : keys) {
			JSAtom key = get_atom(ctx, E);
			JSValue value = JS_GetProperty(ctx, prototype, key);
			if (JS_IsFunction(ctx, value)) {
				MethodInfo mi;
				mi.name = E;
				js_class.methods.insert(E, mi);
			}
			JS_FreeValue(ctx, value);
			JS_FreeAtom(ctx, key);
		}

		// cache the class
		if (const JavaScriptClassInfo *ptr = binder->javascript_classes.getptr(p_path)) {
			binder->free_javascript_class(*ptr);
		}
		binder->javascript_classes.insert(p_path, js_class);
		JS_DefinePropertyValue(ctx, prototype, js_key_godot_classname, to_js_string(ctx, p_path), PROP_DEF_DEFAULT);
	}
fail:
	JS_FreeValue(ctx, classid);
	JS_FreeValue(ctx, prototype);
	JS_FreeValue(ctx, icon);
	JS_FreeValue(ctx, tooled);
	return binder->javascript_classes.getptr(p_path);
}

void QuickJSBinder::free_javascript_class(const JavaScriptClassInfo &p_class) {
	JSValue class_func = JS_MKPTR(JS_TAG_OBJECT, p_class.constructor.javascript_object);
	JS_FreeValue(ctx, class_func);
}

JSValue QuickJSBinder::godot_register_signal(JSContext *ctx, JSValue this_val, int argc, JSValue *argv) {
	ERR_FAIL_COND_V(argc < 2, JS_ThrowTypeError(ctx, "Two or more arguments expected"));
	ERR_FAIL_COND_V(!JS_IsObject(argv[0]), JS_ThrowTypeError(ctx, "protorype of Godot function expected for agurment 0"));
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
	ERR_FAIL_COND_V(!JS_IsObject(argv[0]), JS_ThrowTypeError(ctx, "protorype of Godot class function expected for agurment 0"));
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
		JavaScriptError error;
		JSValue e = JS_GetException(ctx);
		dump_exception(ctx, e, &error);
		JS_FreeValue(ctx, e);
		ERR_PRINT(binder->error_to_string(error));
		return FAILED;
	}
	JS_DefinePropertyValue(ctx, p_prototype, JS_ATOM_Symbol_operatorSet, operators, PROP_DEF_DEFAULT);
	return OK;
}

JSValue QuickJSBinder::godot_set_script_meta(JSContext *ctx, JSValue this_val, int argc, JSValue *argv, int magic) {
	ERR_FAIL_COND_V(argc < 2, JS_ThrowTypeError(ctx, "Two or more arguments expected"));
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
	static SafeNumeric<int64_t> id;
	JSValue js_func = JS_DupValue(ctx, argv[0]);
	JavaScriptGCHandler func;
	func.javascript_object = JS_VALUE_GET_PTR(js_func);
	QuickJSBinder *binder = get_context_binder(ctx);
	int64_t value = id.increment();
	binder->frame_callbacks.insert(value, func);
	return JS_NewInt64(ctx, value);
}

JSValue QuickJSBinder::global_cancel_animation_frame(JSContext *ctx, JSValue this_val, int argc, JSValue *argv) {
	ERR_FAIL_COND_V(argc < 1 || !JS_IsNumber(argv[0]), JS_ThrowTypeError(ctx, "Request ID expected for argument #0"));
	QuickJSBinder *binder = get_context_binder(ctx);
	binder->canceled_frame_callbacks.insert(js_to_int64(ctx, argv[0]));
	return JS_UNDEFINED;
}

int QuickJSBinder::get_js_array_length(JSContext *ctx, JSValue p_val) {
	if (!JS_IsArray(ctx, p_val))
		return -1;
	JSValue ret = JS_GetProperty(ctx, p_val, JS_ATOM_length);
	int32_t length = -1;
	if (JS_ToInt32(ctx, &length, ret))
		return -1;
	return length;
}

void QuickJSBinder::get_own_property_names(JSContext *ctx, JSValue p_object, HashSet<String> *r_list) {
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

JavaScriptGCHandler QuickJSBinder::create_js_instance_for_godot_object(const JavaScriptClassInfo *p_class, Object *p_object) {
	ERR_FAIL_NULL_V(p_object, JavaScriptGCHandler());
	ERR_FAIL_NULL_V(p_class, JavaScriptGCHandler());

	JavaScriptGCHandler *bind = BINDING_DATA_FROM_GD(ctx, p_object);
	ERR_FAIL_NULL_V(bind, JavaScriptGCHandler());

	JSValue constructor = JS_MKPTR(JS_TAG_OBJECT, p_class->constructor.javascript_object);
	JSValue object = JS_MKPTR(JS_TAG_OBJECT, bind->javascript_object);
	JS_CallConstructor2(ctx, constructor, object, 0, NULL);
	if (JS_SetPrototype(ctx, object, JS_MKPTR(JS_TAG_OBJECT, p_class->prototype.javascript_object)) < 0) {
		JSValue e = JS_GetException(ctx);
		JavaScriptError error;
		dump_exception(ctx, e, &error);
		JS_FreeValue(ctx, e);
		bind->javascript_object = NULL;
		ERR_FAIL_V_MSG(*bind, vformat("Cannot create instance from Godot class '%s'\n%s", p_class->class_name, error_to_string(error)));
	}
	initialize_properties(ctx, p_class, object);
	return *bind;
}

Variant QuickJSBinder::call_method(const JavaScriptGCHandler &p_object, const StringName &p_method, const Variant **p_args, int p_argcount, Callable::CallError &r_error) {
	JSValue object = GET_JSVALUE(p_object);
	JSAtom atom = get_atom(ctx, p_method);
	JSValue method = JS_GetProperty(ctx, object, atom);
	JS_FreeAtom(ctx, atom);
	JavaScriptGCHandler func;
	func.javascript_object = JS_VALUE_GET_PTR(method);
	Variant ret = call(func, p_object, p_args, p_argcount, r_error);
	JS_FreeValue(ctx, method);
	return ret;
}

Variant QuickJSBinder::call(const JavaScriptGCHandler &p_fuction, const JavaScriptGCHandler &p_target, const Variant **p_args, int p_argcount, Callable::CallError &r_error) {
	JSValue return_val = JS_UNDEFINED;
	JSValue *argv = NULL;
	JSValue method = JS_MKPTR(JS_TAG_OBJECT, p_fuction.javascript_object);
	JSValue object = p_target.is_valid_javascript_object() ? JS_MKPTR(JS_TAG_OBJECT, p_target.javascript_object) : JS_UNDEFINED;

	if (!JS_IsFunction(ctx, method) || JS_IsPureCFunction(ctx, method)) {
		r_error.error = Callable::CallError::CALL_ERROR_INVALID_METHOD;
		goto finish;
	}

	argv = memnew_arr(JSValue, p_argcount);
	for (int i = 0; i < p_argcount; ++i) {
		argv[i] = variant_to_var(ctx, *p_args[i]);
	}
	return_val = JS_Call(ctx, method, object, p_argcount, argv);

	if (JS_IsException(return_val)) {
		r_error.error = Callable::CallError::CALL_ERROR_INVALID_METHOD;
		JSValue exception = JS_GetException(ctx);
		JavaScriptError err;
		dump_exception(ctx, exception, &err);
		ERR_PRINT(error_to_string(err));
		JS_Throw(ctx, exception);
	} else {
		r_error.error = Callable::CallError::CALL_OK;
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
	return ret;
}

bool QuickJSBinder::get_instance_property(const JavaScriptGCHandler &p_object, const StringName &p_name, Variant &r_ret) {
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

bool QuickJSBinder::set_instance_property(const JavaScriptGCHandler &p_object, const StringName &p_name, const Variant &p_value) {
	JSValue obj = GET_JSVALUE(p_object);
	JSAtom atom = get_atom(ctx, p_name);
	bool success = JS_SetProperty(ctx, obj, atom, variant_to_var(ctx, p_value));
	JS_FreeAtom(ctx, atom);
	return success;
}

bool QuickJSBinder::has_method(const JavaScriptGCHandler &p_object, const StringName &p_name) {
	JSValue obj = GET_JSVALUE(p_object);
	ERR_FAIL_COND_V(!JS_IsObject(obj), false);
	JSAtom atom = get_atom(ctx, p_name);
	JSValue value = JS_GetProperty(ctx, GET_JSVALUE(p_object), atom);
	bool success = JS_IsFunction(ctx, value);
	JS_FreeAtom(ctx, atom);
	JS_FreeValue(ctx, value);
	return success;
}

const JavaScriptClassInfo *QuickJSBinder::parse_javascript_class(const String &p_code, const String &p_path, bool ignore_cacehe, JavaScriptError *r_error) {
	if (!ignore_cacehe) {
		if (const JavaScriptClassInfo *cls = javascript_classes.getptr(p_path)) {
			return cls;
		}
	}
	JavaScriptError err;
	ModuleCache *mc = js_compile_and_cache_module(ctx, p_code, p_path, r_error);
	return parse_javascript_class_from_module(mc, p_path, r_error);
}

const JavaScriptClassInfo *QuickJSBinder::parse_javascript_class(const Vector<uint8_t> &p_bytecode, const String &p_path, bool ignore_cacehe, JavaScriptError *r_error) {
	if (!ignore_cacehe) {
		if (const JavaScriptClassInfo *cls = javascript_classes.getptr(p_path)) {
			return cls;
		}
	}
	ModuleCache *mc = js_compile_and_cache_module(ctx, p_bytecode, p_path, r_error);
	ERR_FAIL_NULL_V(mc, NULL);
	return parse_javascript_class_from_module(mc, p_path, r_error);
}

const JavaScriptClassInfo *QuickJSBinder::parse_javascript_class_from_module(ModuleCache *p_module, const String &p_path, JavaScriptError *r_error) {
	ERR_FAIL_COND_V(p_module == NULL || p_module->module == NULL, NULL);

	const JavaScriptClassInfo *js_class = NULL;
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
		String err = "Failed parse Godot class from script " + p_path + ENDL "\t" + "Default export entry must be a godot class!";
		ERR_PRINT(err);
		JS_ThrowTypeError(ctx, "%s", err.utf8().get_data());
		goto fail;
	}
	js_class = register_javascript_class(default_entry, p_path);
fail:
	JS_FreeValue(ctx, default_entry);
	return js_class;
}

bool QuickJSBinder::has_signal(const JavaScriptClassInfo *p_class, const StringName &p_signal) {
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

	JavaScriptGCHandler *data = host->new_gc_handler(ctx);
	data->native_ptr = worker;
	data->javascript_object = JS_VALUE_GET_PTR(obj);
	JS_SetOpaque(obj, data);
	host->workers.push_back(data);

	return obj;
}

void QuickJSBinder::worker_finializer(JSRuntime *rt, JSValue val) {
	QuickJSBinder *host = QuickJSBinder::get_runtime_binder(rt);
	if (JavaScriptGCHandler *bind = static_cast<JavaScriptGCHandler *>(JS_GetOpaque(val, host->worker_class_data.class_id))) {
		QuickJSWorker *worker = static_cast<QuickJSWorker *>(bind->native_ptr);
		worker->stop();
		if (List<JavaScriptGCHandler *>::Element *E = host->workers.find(bind)) {
			host->workers.erase(E);
		}
		memdelete(worker);
		memdelete(bind);
	}
}

JSValue QuickJSBinder::worker_post_message(JSContext *ctx, JSValue this_val, int argc, JSValue *argv) {
	ERR_FAIL_COND_V(argc < 1, JS_ThrowTypeError(ctx, "message value expected of argument #0"));
	QuickJSBinder *host = QuickJSBinder::get_context_binder(ctx);
	if (JavaScriptGCHandler *bind = static_cast<JavaScriptGCHandler *>(JS_GetOpaque(this_val, host->worker_class_data.class_id))) {
		QuickJSWorker *worker = static_cast<QuickJSWorker *>(bind->native_ptr);
		worker->post_message_from_host(var_to_variant(ctx, argv[0]));
	}
	return JS_UNDEFINED;
}

JSValue QuickJSBinder::worker_terminate(JSContext *ctx, JSValue this_val, int argc, JSValue *argv) {
	QuickJSBinder *host = QuickJSBinder::get_context_binder(ctx);
	if (JavaScriptGCHandler *bind = static_cast<JavaScriptGCHandler *>(JS_GetOpaque(this_val, host->worker_class_data.class_id))) {
		QuickJSWorker *worker = static_cast<QuickJSWorker *>(bind->native_ptr);
		if (List<JavaScriptGCHandler *>::Element *E = host->workers.find(bind)) {
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
		JavaScriptGCHandler *data = BINDING_DATA_FROM_JS(ctx, value);
		if (data) {
			JS_SetOpaque(value, NULL);
			if (data->is_ref_counted()) {
				static_cast<RefCounted *>(data->godot_object)->unreference();
			} else if (data->is_object()) {
				JS_FreeValue(ctx, value);
			}
			data->godot_object = NULL;
			data->javascript_object = NULL;
			data->context = NULL;
			data->flags |= JavaScriptGCHandler::FLAG_TRANSFERABLE;
		} else {
			valid = false;
		}
	}

	uint64_t id = 0;
	if (valid) {
		id = QuickJSBinder::global_transfer_id.increment();
		GLOBAL_LOCK_FUNCTION
		transfer_deopot.insert(id, gd_value);
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
	ERR_FAIL_NULL_V(static_cast<Object *>(value), JS_UNDEFINED);
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

bool QuickJSBinder::validate(const String &p_code, const String &p_path, JavaScriptError *r_error) {
	ModuleCache *module = js_compile_and_cache_module(ctx, p_code, p_path, r_error);
	return module != NULL;
}
