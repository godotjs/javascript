#ifndef QUICKJS_BINDING_HELPER_H
#define QUICKJS_BINDING_HELPER_H

#include "../ecmascript_binding_helper.h"
#include "core/os/memory.h"
#include <quickjs.h>

class QuickJSBindingHelper : public ECMAScriptBindingHelper {

	enum {
		PROP_DEF_DEFAULT = JS_PROP_ENUMERABLE,
	};

	static JSClassID OBJECT_CLASS_ID;

	static QuickJSBindingHelper *singleton;
	JSValue global_object;
	JSValue godot_object;
	JSValue empty_function;
	JSAtom js_key_godot_classid;
	JSAtom js_key_constructor;
	JSAtom js_key_prototype;
	JSAtom js_key_name;

	JSRuntime *runtime;
	JSContext *ctx;
	JSMallocFunctions godot_allocator;

	_FORCE_INLINE_ static void *js_malloc(JSMallocState *s, size_t size) { return memalloc(size); }
	_FORCE_INLINE_ static void js_free(JSMallocState *s, void *ptr) {
		if (ptr) memfree(ptr);
	}
	_FORCE_INLINE_ static void *js_realloc(JSMallocState *s, void *ptr, size_t size) { return memrealloc(ptr, size); }

	struct ClassBindData {

		JSClassID class_id;
		CharString class_name;

		JSValue prototype;
		JSValue constructor;
		JSClassDef jsclass;

		const ClassDB::ClassInfo *gdclass;
		const ClassBindData *base_class;
	};

	HashMap<JSClassID, ClassBindData> class_bindings;
	HashMap<StringName, const ClassBindData *> classname_bindings;

	Vector<MethodBind *> godot_methods;
	int internal_godot_method_id;

	struct PtrHasher {
		static _FORCE_INLINE_ uint32_t hash(const void *p_ptr) {
			union {
				const void *p;
				uint64_t i;
			} u;
			u.p = p_ptr;
			return HashMapHasherDefault::hash(u.i);
		}
	};

	JSClassID register_class(const ClassDB::ClassInfo *p_cls);
	void add_godot_classes();
	void add_godot_globals();
	void add_global_console();

	static JSValue object_constructor(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv, int class_id);
	static void object_finalizer(JSRuntime *rt, JSValue val);

	static JSValue object_method(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv, int class_id);
	static JSValue godot_to_string(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv);
	static JSValue object_free(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv);

	JSValue variant_to_var(const Variant p_var);
	Variant var_to_variant(JSValue p_val);

	JSAtom get_atom(const StringName &p_key) const;
	JSValue godot_string_to_jsvalue(const String &text) const;
	JSAtom stringname_to_atom(const StringName &text) const;
	String js_string_to_godot_string(JSValue p_val) const;

	static JSValue godot_register_emca_class(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv);

	_FORCE_INLINE_ static JSValue js_empty_func(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) { return JS_UNDEFINED; }

public:
	QuickJSBindingHelper();
	_FORCE_INLINE_ static QuickJSBindingHelper *get_singleton() { return singleton; }

	virtual void initialize();
	virtual void uninitialize();

	virtual void *alloc_object_binding_data(Object *p_object);
	virtual void free_object_binding_data(void *p_gc_handle);

	virtual void godot_refcount_incremented(Reference *p_object);
	virtual bool godot_refcount_decremented(Reference *p_object);

	virtual Error eval_string(const String &p_source);
	virtual Error safe_eval_text(const String &p_source, String &r_error);

	virtual ECMAScriptGCHandler create_ecma_instance_for_godot_object(const StringName &ecma_class_name, Object *p_object);
	virtual Variant call_method(const ECMAScriptGCHandler &p_object, const StringName &p_method, const Variant **p_args, int p_argcount, Variant::CallError &r_error);
	virtual bool get_instance_property(const ECMAScriptGCHandler &p_object, const StringName &p_name, Variant &r_ret);
	virtual bool set_instance_property(const ECMAScriptGCHandler &p_object, const StringName &p_name, const Variant &p_value);
};

#endif // QUICKJS_BINDING_HELPER_H
