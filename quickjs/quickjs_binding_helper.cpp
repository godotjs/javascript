#include "quickjs_binding_helper.h"
#include "../ecmascript_language.h"

QuickJSBindingHelper *QuickJSBindingHelper::singleton = NULL;
JSClassID QuickJSBindingHelper::OBJECT_CLASS_ID = 0;

#define BINDING_DATA_FROM_GD(p_object) (p_object ? (ECMAScriptObjectBindingData *)(p_object)->get_script_instance_binding(ECMAScriptLanguage::get_singleton()->get_language_index()) : NULL);
#define BINDING_DATA_FROM_JS(p_val) (ECMAScriptObjectBindingData *)JS_GetOpaque(p_val, OBJECT_CLASS_ID);

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

JSValue QuickJSBindingHelper::object_method(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv, int method_id) {

	ECMAScriptObjectBindingData *bind = BINDING_DATA_FROM_JS(this_val);
	ERR_FAIL_NULL_V_MSG(bind, JS_EXCEPTION, "JS try call native method without binding data");
	ERR_FAIL_NULL_V_MSG(bind->godot_object, JS_EXCEPTION, "JS try call native method with freed object");

	Object *obj = bind->get_value();
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

	ERR_FAIL_COND_V_MSG((err.error != Variant::CallError::CALL_OK), (JS_EXCEPTION), "Error call native method from JS");
	return singleton->variant_to_var(ret_val);
}

JSValue QuickJSBindingHelper::godot_to_string(JSContext *ctx, JSValue this_val, int argc, JSValue *argv) {
	String str = singleton->var_to_variant(this_val);
	return JS_NewString(ctx, str.utf8().ptr());
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
			ECMAScriptObjectBindingData *data = BINDING_DATA_FROM_GD(obj);
			ERR_FAIL_NULL_V(data, JS_UNDEFINED);
			ERR_FAIL_NULL_V(data->ecma_object, JS_UNDEFINED);
			JSValue js_obj = JS_MKPTR(JS_TAG_OBJECT, data->ecma_object);
			if (data->is_object()) {
				JS_DupValue(ctx, js_obj);
			}
			return js_obj;
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
			ECMAScriptObjectBindingData *bind = BINDING_DATA_FROM_JS(p_val);
			ERR_FAIL_NULL_V(bind, Variant());
			ERR_FAIL_NULL_V(bind->godot_object, Variant());
			return bind->get_value();
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

/************************* Memory Management ******************************/

void *QuickJSBindingHelper::alloc_object_binding_data(Object *p_object) {
	if (const ClassBindData **bind_ptr = classname_bindings.getptr(p_object->get_class_name())) {
		JSValue obj = JS_NewObjectProtoClass(ctx, (*bind_ptr)->prototype, OBJECT_CLASS_ID);
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
		JS_SetOpaque(obj, data);
		return data;
	}
	return NULL;
}

void QuickJSBindingHelper::free_object_binding_data(void *p_gc_handle) {
	ECMAScriptObjectBindingData *bind = (ECMAScriptObjectBindingData *)p_gc_handle;
	if (bind->is_object()) {
		JSValue js_obj = JS_MKPTR(JS_TAG_OBJECT, bind->ecma_object);
		JS_FreeValue(ctx, js_obj);
	}
	memdelete(bind);
}

void QuickJSBindingHelper::godot_refcount_incremented(Reference *p_object) {
	ECMAScriptObjectBindingData *bind = BINDING_DATA_FROM_GD(p_object);
	JSValue js_obj = JS_MKPTR(JS_TAG_OBJECT, bind->ecma_object);
	JS_DupValue(ctx, js_obj); // JS ref_count ++
}

bool QuickJSBindingHelper::godot_refcount_decremented(Reference *p_object) {
	return true;
}

JSValue QuickJSBindingHelper::object_constructor(JSContext *ctx, JSValueConst new_target, int argc, JSValueConst *argv, int class_id) {
	const ClassBindData &cls = singleton->class_bindings.get(class_id);
	Object *gd_obj = cls.gdclass->creation_func();
	ECMAScriptObjectBindingData *bind = BINDING_DATA_FROM_GD(gd_obj);
	bind->flags |= ECMAScriptObjectBindingData::FLAG_FROM_SCRIPT;

	JSValue js_obj = JS_MKPTR(JS_TAG_OBJECT, bind->ecma_object);
	if (bind->is_reference()) {
		bind->godot_reference->ptr()->unreference();
	} else if (bind->is_object()) {
		JS_DupValue(ctx, js_obj);
	}

	return js_obj;
}

void QuickJSBindingHelper::object_finalizer(JSRuntime *rt, JSValue val) {

	ECMAScriptObjectBindingData *bind = BINDING_DATA_FROM_JS(val);
	if (bind) {
		if (bind->is_reference()) {
			memdelete(bind->godot_reference);
		}
		bind->clear();
	}
}

JSValue QuickJSBindingHelper::object_free(JSContext *ctx, JSValue this_val, int argc, JSValue *argv) {
	ECMAScriptObjectBindingData *bind = BINDING_DATA_FROM_JS(this_val);
	ERR_FAIL_NULL_V_MSG(bind, JS_EXCEPTION, "Free object without binding data");
	ERR_FAIL_NULL_V_MSG(bind->godot_object, JS_EXCEPTION, "The object already be freed");
	ERR_FAIL_COND_V_MSG((bind->is_reference()), (JS_EXCEPTION), "Call free to reference is not allowed");
	memdelete(bind->godot_object);
	JS_SetOpaque(this_val, NULL);
	return JS_UNDEFINED;
}

/************************* END Memory Management ******************************/
