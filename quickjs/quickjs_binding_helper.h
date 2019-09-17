#ifndef QUICKJS_BINDING_HELPER_H
#define QUICKJS_BINDING_HELPER_H

#include "../ecmascript_binding_helper.h"
#include <quickjs.h>
#include "core/os/memory.h"

class QuickJSBindingHelper : public ECMAScriptBindingHelper {

	enum {
		PROP_DEF_DEFAULT = 0,
	};

	static QuickJSBindingHelper* singleton;
	JSValue global_object;
	JSValue godot_object;

	JSRuntime *runtime;
	JSContext *ctx;
	JSMallocFunctions godot_allocator;

	_FORCE_INLINE_ static void* js_malloc(JSMallocState *s, size_t size) { return memalloc(size); }
	_FORCE_INLINE_ static void js_free(JSMallocState *s, void *ptr) { if (ptr) memfree(ptr); }
	_FORCE_INLINE_ static void* js_realloc(JSMallocState *s, void *ptr, size_t size) { return memrealloc(ptr, size); }

	struct ClassBindData {

		JSClassID id;
		CharString class_name;

		JSValue prototype;
		JSValue constructor;
		JSClassDef jsclass;

		const ClassDB::ClassInfo *gdclass;
		const ClassBindData *base_class;
	};

	HashMap<JSClassID, ClassBindData> class_bindings;
	HashMap<StringName, const ClassBindData*> classname_bindings;

	Vector<MethodBind*> godot_methods;
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
    HashMap<void*, ECMAScriptObjectBindingData*, PtrHasher> object_map;

	JSClassID register_class(const ClassDB::ClassInfo *p_cls);
	void add_godot_classes();
	void add_global_console();

	static JSValue object_constructor(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv, int class_id);
    static void object_finalizer(JSRuntime *rt, JSValue val);

	static JSValue object_method(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv, int class_id);
	static JSValue godot_to_string(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv);



	JSValue variant_to_var(const Variant p_var);
    Variant var_to_variant(JSValue p_val);

    JSValue get_js_object(Object* p_object);

public:
	QuickJSBindingHelper();
	_FORCE_INLINE_ static QuickJSBindingHelper* get_singleton() { return singleton; }

	virtual void initialize();
	virtual void uninitialize();

	virtual void *alloc_object_binding_data(Object *p_object) ;
	virtual void free_object_binding_data(void *p_gc_handle);

	virtual void godot_refcount_incremented(Reference *p_object);
	virtual bool godot_refcount_decremented(Reference *p_object);

	virtual Error eval_string(const String &p_source);
	virtual Error safe_eval_text(const String &p_source, String &r_error);

	virtual ECMAScriptGCHandler create_ecma_instance_for_godot_object(const StringName &ecma_class_name, Object *p_object);
	virtual Variant call_method(const ECMAScriptGCHandler &p_object, const ECMAMethodInfo &p_method, const Variant **p_args, int p_argcount, Variant::CallError &r_error);
	virtual bool get_instance_property(const ECMAScriptGCHandler &p_object, const StringName &p_name, Variant &r_ret);
	virtual bool set_instance_property(const ECMAScriptGCHandler &p_object, const StringName &p_name, const Variant &p_value);
};

#endif // QUICKJS_BINDING_HELPER_H
