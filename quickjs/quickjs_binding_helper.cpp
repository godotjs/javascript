#include "quickjs_binding_helper.h"

QuickJSBindingHelper* QuickJSBindingHelper::singleton = NULL;

static JSValue console_log_function(JSContext *ctx, JSValue this_val, int argc, JSValue *argv) {
	PoolStringArray args;
	args.resize(argc);
	for (int i = 0; i < argc; ++i) {
		args.write()[i] = JS_ToCString(ctx, *(argv + i));
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

JSValue QuickJSBindingHelper::object_constructor(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv, int class_id) {
	const ClassBindData& cls = singleton->class_bindings.get(class_id);
	Object *gd_obj = cls.gdclass->creation_func();
	JSValue js_obj = JS_NewObjectClass(ctx, class_id);
	singleton->object_map.set(JS_VALUE_GET_PTR(js_obj), gd_obj);
	return js_obj;
}

JSValue QuickJSBindingHelper::object_method(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv, int method_id) {
	if (Object **ptr_obj = singleton->object_map.getptr(JS_VALUE_GET_PTR(this_val))) {
		Object *obj = *ptr_obj;
		const MethodBind *mb = singleton->godot_methods[method_id];
		print_line("call method: " + itos(obj->get_instance_id()) + ":" + mb->get_name());
	}
	return JS_UNDEFINED;
}

void QuickJSBindingHelper::object_finalizer(JSRuntime *rt, JSValue val) {
	if (Object **obj_ptr = singleton->object_map.getptr(JS_VALUE_GET_PTR(val))) {
		Object *godot_obj = *obj_ptr;
		memdelete(godot_obj);
		singleton->object_map.erase(JS_VALUE_GET_PTR(val));
	}
}

JSClassID QuickJSBindingHelper::register_class(const ClassDB::ClassInfo *p_cls) {

	ClassBindData data;
	data.id = 0;
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
		while(key) {
			const MethodBind *mb = p_cls->method_map.get(*key);
			godot_methods.write[internal_godot_method_id] = mb;

			CharString name = String(*key).utf8();
			JSValue method = JS_NewCFunctionMagic(ctx, &QuickJSBindingHelper::object_method, name.ptr(), mb->get_argument_count() - mb->get_default_argument_count(), JS_CFUNC_generic_magic, internal_godot_method_id);
			JS_DefinePropertyValueStr(ctx, data.prototype, name.ptr(), method, PROP_DEF_DEFAULT);

			++internal_godot_method_id;
			key = p_cls->method_map.next(key);
		}
	}

	JS_NewClassID(&data.id);
	JS_NewClass(JS_GetRuntime(ctx), data.id, &data.jsclass);
	JS_SetClassProto(ctx, data.id, data.prototype);

	data.constructor = JS_NewCFunctionMagic(ctx, object_constructor, data.jsclass.class_name, data.class_name.size(), JS_CFUNC_constructor_magic, (int)data.id);
	JS_DefinePropertyValueStr(ctx, data.constructor, PROTOTYPE_LITERAL, data.prototype, PROP_DEF_DEFAULT);


	class_bindings.set(data.id, data);

	return data.id;
}

void QuickJSBindingHelper::add_godot_classes() {

	Map<const ClassDB::ClassInfo*, JSClassID> gdclass_jsmap;
	// register classes
	const StringName *key = ClassDB::classes.next(NULL);
	while (key) {
		const ClassDB::ClassInfo *cls = ClassDB::classes.getptr(*key);
		JSClassID id = register_class(cls);
		gdclass_jsmap.insert(cls, id);
		key = ClassDB::classes.next(key);
	}

	// Setup inherits chain
	for(Map<const ClassDB::ClassInfo*, JSClassID>::Element * E = gdclass_jsmap.front(); E; E = E->next()) {
		const ClassDB::ClassInfo *gdcls = E->key();
		ClassBindData &bind = class_bindings.get(E->get());
		if (gdcls->inherits_ptr) {
			if (Map<const ClassDB::ClassInfo*, JSClassID>::Element * base = gdclass_jsmap.find(gdcls->inherits_ptr)) {
				bind.base_class = class_bindings.getptr(base->get());
			}
		}
	}

	// Setup the prototype chain
	const JSClassID *id = class_bindings.next(NULL);
	while (id) {

		const ClassBindData& data = class_bindings.get(*id);
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
	return NULL;
}

void QuickJSBindingHelper::free_object_binding_data(void *p_gc_handle) {

}

void QuickJSBindingHelper::godot_refcount_incremented(Reference *p_object) {

}

bool QuickJSBindingHelper::godot_refcount_decremented(Reference *p_object) {
	return false;
}

Error QuickJSBindingHelper::eval_string(const String &p_source) {
	String error;
	return safe_eval_text(p_source, error);
}

Error QuickJSBindingHelper::safe_eval_text(const String &p_source, String &r_error) {
	CharString utf8_str = p_source.utf8();
	JSValue ret = JS_Eval(ctx, utf8_str.ptr(), utf8_str.size(), "", JS_EVAL_TYPE_GLOBAL | JS_EVAL_FLAG_STRICT);
	if(JS_IsException(ret)) {
		JSValue e = JS_GetException(ctx);
		r_error = JS_ToCString(ctx, e);
	}
	return OK;
}

ECMAScriptGCHandler QuickJSBindingHelper::create_ecma_instance_for_godot_object(const StringName &ecma_class_name, Object *p_object) {
	JSValue obj = JS_NewObject(ctx);
	return { obj.u.ptr };
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
