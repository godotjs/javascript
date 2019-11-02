#include "quickjs_binding_helper.h"
#include "../ecmascript_language.h"

QuickJSBindingHelper *QuickJSBindingHelper::singleton = NULL;
JSClassID QuickJSBindingHelper::OBJECT_CLASS_ID = 0;

#define GET_BINDING_DATA(p_object) (p_object ? (ECMAScriptObjectBindingData *)(p_object)->get_script_instance_binding(ECMAScriptLanguage::get_singleton()->get_language_index()) : NULL);

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

void QuickJSBindingHelper::add_global_console() {
	JSValue console = JS_NewObject(ctx);
	JSValue log = JS_NewCFunction(ctx, console_log_function, "log", 0);
	JS_DefinePropertyValueStr(ctx, global_object, "console", console, PROP_DEF_DEFAULT);
	JS_DefinePropertyValueStr(ctx, console, "log", log, PROP_DEF_DEFAULT);
}

JSValue QuickJSBindingHelper::object_constructor(JSContext *ctx, JSValueConst new_target, int argc, JSValueConst *argv, int class_id) {
	const ClassBindData &cls = singleton->class_bindings.get(class_id);
	Object *gd_obj = cls.gdclass->creation_func();
	ECMAScriptObjectBindingData *data = GET_BINDING_DATA(gd_obj);
	if (Reference *ref = Object::cast_to<Reference>(gd_obj)) {
		ref->unreference();
	}
	return JS_MKPTR(JS_TAG_OBJECT, data->ecma_object);
}

void QuickJSBindingHelper::object_finalizer(JSRuntime *rt, JSValue val) {
	void *js_ptr = JS_VALUE_GET_PTR(val);
	if (ECMAScriptObjectBindingData **ptr = singleton->object_map.getptr(js_ptr)) {
		ECMAScriptObjectBindingData *bind = *ptr;
		if (bind->flags & ECMAScriptObjectBindingData::FLAG_REFERENCE) {
			memdelete(bind->godot_reference);
		}
		singleton->object_map.erase(js_ptr);
	}
}

JSValue QuickJSBindingHelper::object_method(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv, int method_id) {
	if (ECMAScriptObjectBindingData **ptr = singleton->object_map.getptr(JS_VALUE_GET_PTR(this_val))) {
		Object *obj = (*ptr)->get_value();
		MethodBind *mb = singleton->godot_methods[method_id];

		if (!mb->is_vararg()) {
			argc = MIN(argc, mb->get_argument_count());
		}

		Variant::CallError err;

		const Variant **args = memnew_arr(const Variant *, argc);
		Vector<Variant> vargs;
		vargs.resize(argc);
		for (int i = 0; i < argc; ++i) {
			vargs.write[i] = singleton->var_to_variant(*(argv + i));
			args[i] = (vargs.ptr() + i);
		}

		Variant ret_val = mb->call(obj, args, argc, err);
		if (args != NULL) {
			memdelete_arr(args);
		}

		if (err.error == Variant::CallError::CALL_OK) {
			return singleton->variant_to_var(ret_val);
		} else {
			return JS_EXCEPTION;
		}
	}
	return JS_UNDEFINED;
}

JSValue QuickJSBindingHelper::godot_to_string(JSContext *ctx, JSValue this_val, int argc, JSValue *argv) {
	String str = singleton->var_to_variant(this_val);
	return JS_NewString(ctx, str.utf8().ptr());
}

JSValue QuickJSBindingHelper::object_free(JSContext *ctx, JSValue this_val, int argc, JSValue *argv) {
	void *js_ptr = JS_VALUE_GET_PTR(this_val);
	if (ECMAScriptObjectBindingData **ptr = singleton->object_map.getptr(js_ptr)) {
		ECMAScriptObjectBindingData *data = *ptr;
		if (data && data->godot_object) {
			if (!(data->flags & ECMAScriptObjectBindingData::FLAG_REFERENCE) && data->flags & ECMAScriptObjectBindingData::FLAG_OBJECT) {
				memdelete(data->godot_object);
				singleton->object_map.erase(js_ptr);
				data->godot_object = NULL;
				return JS_UNDEFINED;
			}
		}
	}
	return JS_EXCEPTION;
}

JSValue QuickJSBindingHelper::variant_to_var(const Variant p_var) {
	switch (p_var.get_type()) {
		case Variant::BOOL:
			return ((bool)p_var) ? JS_TRUE : JS_FALSE;
		case Variant::INT:
			return JS_NewInt32(ctx, int32_t(p_var));
		case Variant::REAL:
			return JS_NewFloat64(ctx, (double)(p_var));
		case Variant::STRING: {
			String str = p_var;
			CharString utf8 = str.utf8();
			return JS_NewString(ctx, utf8.ptr());
		}
		case Variant::OBJECT: {
			Object *obj = p_var;
			ECMAScriptObjectBindingData *data = GET_BINDING_DATA(obj);
			JSValue val = JS_MKPTR(JS_TAG_OBJECT, data->ecma_object);
			return JS_DupValue(ctx, val);
		}
		case Variant::NIL:
			return JS_NULL;
		default:
			return JS_UNDEFINED;
	}
}

Variant QuickJSBindingHelper::var_to_variant(JSValue p_val) {

	switch (JS_VALUE_GET_TAG(p_val)) {
		case JS_TAG_INT:
			return Variant(JS_VALUE_GET_INT(p_val));
		case JS_TAG_BOOL:
			return Variant(bool(JS_VALUE_GET_INT(p_val)));
		case JS_TAG_FLOAT64:
			return Variant(real_t(JS_VALUE_GET_FLOAT64(p_val)));
		case JS_TAG_STRING: {
			String ret;
			size_t len = 0;
			const char *utf8 = JS_ToCStringLen(ctx, &len, p_val);
			ret.parse_utf8(utf8, len);
			JS_FreeCString(ctx, utf8);
			return ret;
		}
		case JS_TAG_OBJECT: {
			if (ECMAScriptObjectBindingData **ptr = object_map.getptr(JS_VALUE_GET_PTR(p_val))) {
				ECMAScriptObjectBindingData *data = *ptr;
				return data->get_value();
			}
			return Variant();
		}
		default:
			return Variant();
	}
}

JSClassID QuickJSBindingHelper::register_class(const ClassDB::ClassInfo *p_cls) {

	ClassBindData data;
	data.class_id = 0;
	data.base_class = NULL;
	data.class_name = String(p_cls->name).utf8();
	data.jsclass.class_name = data.class_name.ptr();
	data.jsclass.finalizer = object_finalizer;
	data.jsclass.exotic = NULL;
	data.jsclass.gc_mark = NULL;
	data.jsclass.call = NULL;
	data.gdclass = p_cls;

	data.prototype = JS_NewObject(ctx);
	// methods
	{
		godot_methods.resize(internal_godot_method_id + p_cls->method_map.size());
		const StringName *key = p_cls->method_map.next(NULL);
		while (key) {
			MethodBind *mb = p_cls->method_map.get(*key);
			godot_methods.write[internal_godot_method_id] = mb;

			CharString name = String(*key).utf8();
			JSValue method = JS_NewCFunctionMagic(ctx, &QuickJSBindingHelper::object_method, name.ptr(), mb->get_argument_count() - mb->get_default_argument_count(), JS_CFUNC_generic_magic, internal_godot_method_id);
			JS_DefinePropertyValueStr(ctx, data.prototype, name.ptr(), method, PROP_DEF_DEFAULT);

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

	JS_NewClassID(&data.class_id);
	JS_NewClass(JS_GetRuntime(ctx), data.class_id, &data.jsclass);
	JS_SetClassProto(ctx, data.class_id, data.prototype);

	data.constructor = JS_NewCFunctionMagic(ctx, object_constructor, data.jsclass.class_name, data.class_name.size(), JS_CFUNC_constructor_magic, (int)data.class_id);
	JS_SetConstructor(ctx, data.constructor, data.prototype);

	if (p_cls->name == "Object") {
		OBJECT_CLASS_ID = data.class_id;
	}

	class_bindings.set(data.class_id, data);
	classname_bindings.set(p_cls->name, class_bindings.getptr(data.class_id));

	return data.class_id;
}

void QuickJSBindingHelper::add_godot_classes() {

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
		}

		id = class_bindings.next(id);
	}
}

QuickJSBindingHelper::QuickJSBindingHelper() {

	singleton = this;
	internal_godot_method_id = 0;

	godot_allocator.js_malloc = QuickJSBindingHelper::js_malloc;
	godot_allocator.js_free = QuickJSBindingHelper::js_free;
	godot_allocator.js_realloc = QuickJSBindingHelper::js_realloc;
	godot_allocator.js_malloc_usable_size = NULL;
}

void QuickJSBindingHelper::initialize() {
	runtime = JS_NewRuntime2(&godot_allocator, NULL);
	ctx = JS_NewContext(runtime);

	// global.godot
	global_object = JS_GetGlobalObject(ctx);
	godot_object = JS_NewObject(ctx);
	JS_DefinePropertyValueStr(ctx, global_object, "godot", godot_object, PROP_DEF_DEFAULT);

	add_global_console();
	add_godot_classes();
}

void QuickJSBindingHelper::uninitialize() {
	JS_FreeValue(ctx, global_object);
	JS_FreeContext(ctx);
	JS_FreeRuntime(runtime);
	ctx = NULL;
	runtime = NULL;
}

void *QuickJSBindingHelper::alloc_object_binding_data(Object *p_object) {
	if (const ClassBindData **bind_ptr = classname_bindings.getptr(p_object->get_class_name())) {
		JSValue obj = JS_NewObjectProtoClass(ctx, (*bind_ptr)->prototype, OBJECT_CLASS_ID);
		JS_SetOpaque(obj, p_object);
		ECMAScriptObjectBindingData *data = memnew(ECMAScriptObjectBindingData);
		data->ecma_object = JS_VALUE_GET_PTR(obj);
		data->godot_object = p_object;
		data->flags = ECMAScriptObjectBindingData::FLAG_OBJECT;
		if (Reference *ref = Object::cast_to<Reference>(p_object)) {
			data->flags |= ECMAScriptObjectBindingData::FLAG_REFERENCE;
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
		object_map.set(data->ecma_object, data);
		return data;
	}
	return NULL;
}

void QuickJSBindingHelper::free_object_binding_data(void *p_gc_handle) {
}

void QuickJSBindingHelper::godot_refcount_incremented(Reference *p_object) {
}

bool QuickJSBindingHelper::godot_refcount_decremented(Reference *p_object) {
	return true;
}

Error QuickJSBindingHelper::eval_string(const String &p_source) {
	String error;
	return safe_eval_text(p_source, error);
}

Error QuickJSBindingHelper::safe_eval_text(const String &p_source, String &r_error) {
	CharString utf8_str = p_source.utf8();
	JSValue ret = JS_Eval(ctx, utf8_str.ptr(), utf8_str.size(), "", JS_EVAL_TYPE_GLOBAL | JS_EVAL_FLAG_STRICT);
	if (JS_IsException(ret)) {
		JSValue e = JS_GetException(ctx);
		const char *message = JS_ToCString(ctx, e);
		r_error = message;
		JS_FreeCString(ctx, message);
	}
	return OK;
}

ECMAScriptGCHandler QuickJSBindingHelper::create_ecma_instance_for_godot_object(const StringName &ecma_class_name, Object *p_object) {
	return { NULL };
}

Variant QuickJSBindingHelper::call_method(const ECMAScriptGCHandler &p_object, const ECMAMethodInfo &p_method, const Variant **p_args, int p_argcount, Variant::CallError &r_error) {
	return Variant();
}

bool QuickJSBindingHelper::get_instance_property(const ECMAScriptGCHandler &p_object, const StringName &p_name, Variant &r_ret) {
	return false;
}

bool QuickJSBindingHelper::set_instance_property(const ECMAScriptGCHandler &p_object, const StringName &p_name, const Variant &p_value) {
	return false;
}
