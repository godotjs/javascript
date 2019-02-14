#ifndef DUKTAPE_BINDING_HELPER_H
#define DUKTAPE_BINDING_HELPER_H

#include "../ecmascript_binding_helper.h"
#include "core/hash_map.h"
#include "core/object.h"
#include "core/reference.h"
#include "core/string_db.h"
#include "core/variant.h"
#include "src/duktape.h"

#define DUK_NO_RET_VAL 0
#define DUK_HAS_RET_VAL 1

typedef void DuktapeHeapObject;
class ECMAScriptLanguage;

class DuktapeBindingHelper : public ECMAScriptBindingHelper {

	friend class ECMAScriptLanguage;

	duk_context *ctx;

	struct MethodPtrHash {
		static _FORCE_INLINE_ uint32_t hash(const MethodBind *p_mb) {
			union {
				const MethodBind *p;
				unsigned long i;
			} u;
			u.p = p_mb;
			return HashMapHasherDefault::hash((uint64_t)u.i);
		}
	};

private:
	// memery managerment functions
	_FORCE_INLINE_ static void *alloc_function(void *udata, duk_size_t size) { return memalloc(size); }
	_FORCE_INLINE_ static void *realloc_function(void *udata, void *ptr, duk_size_t size) { return memrealloc(ptr, size); }
	_FORCE_INLINE_ static void free_function(void *udata, void *ptr) {
		if (ptr) memfree(ptr);
	}

	// handle duktape fatal errors
	static void fatal_function(void *udata, const char *msg);

	static duk_ret_t duk_godot_object_constructor(duk_context *ctx);
	static duk_ret_t duk_godot_object_finalizer(duk_context *ctx);
	static duk_ret_t godot_object_free(duk_context *ctx);
	_FORCE_INLINE_ static duk_ret_t godot_object_virtual_method(duk_context *ctx) {
		return DUK_NO_RET_VAL;
	}
	static duk_ret_t duk_godot_object_method(duk_context *ctx);
	static duk_ret_t godot_object_to_string(duk_context *ctx);

	static duk_ret_t godot_print_function(duk_context *ctx);

	static void duk_push_godot_variant(duk_context *ctx, const Variant &var);
	static void duk_push_godot_object(duk_context *ctx, Object *obj, bool from_constructor = false);
	static void duk_push_godot_string(duk_context *ctx, const String &str);
	static void duk_push_godot_string_name(duk_context *ctx, const StringName &str);

	static void duk_put_prop_godot_string(duk_context *ctx, duk_idx_t idx, const String &str);
	static void duk_put_prop_godot_string_name(duk_context *ctx, duk_idx_t idx, const StringName &str);

	static Variant duk_get_godot_variant(duk_context *ctx, duk_idx_t idx);
	static String duk_get_godot_string(duk_context *ctx, duk_idx_t idx, bool convert_type = false);
	static Object *duk_get_godot_object(duk_context *ctx, duk_idx_t idx);

	void rigister_class(duk_context *ctx, const ClassDB::ClassInfo *cls);

private:
	HashMap<StringName, DuktapeHeapObject *> native_class_prototypes;
	HashMap<const MethodBind *, DuktapeHeapObject *, MethodPtrHash> method_bindings;

	HashMap<ObjectID, DuktapeHeapObject *> weakref_pool;

	DuktapeHeapObject *strongref_pool_ptr;
	HashMap<ObjectID, DuktapeHeapObject *> strongref_pool;

	// for register godot classes
	void register_class_members(duk_context *ctx, const ClassDB::ClassInfo *cls);
	void duk_push_godot_method(duk_context *ctx, const MethodBind *mb);

	// weak references
	DuktapeHeapObject *get_weak_ref(Object *obj);
	void set_weak_ref(Object *obj, DuktapeHeapObject *ptr);

	// strong references
	void duk_push_strong_ref_container(duk_context *ctx);
	void set_strong_ref(Object *obj, DuktapeHeapObject *ptr);
	DuktapeHeapObject *get_strong_ref(Object *obj);

	/**
	 * godot.register_class = function(ecma_class, name, icon) {
	 *		ecma_class.name = name;
	 *		ecma_class.icon = icon;
	 *		return ecma_class;
	 * }
	 */
	static duk_ret_t register_ecma_class(duk_context *ctx);

#if 0
	/**
	 * godot.GDCLASS = functions(name, icon) {
	 *		return godot.register_class(ecma_class, name, icon);
	 * }
	 */
	static duk_ret_t decorator_register_ecma_class(duk_context *ctx);
#endif

	// cached Duktape/C functions
	DuktapeHeapObject *duk_ptr_godot_object_finalizer;
	DuktapeHeapObject *duk_ptr_godot_object_free;
	DuktapeHeapObject *duk_ptr_godot_object_to_string;
	DuktapeHeapObject *duk_ptr_godot_object_virtual_method;
	DuktapeHeapObject *duk_ptr_register_ecma_class;

public:
	_FORCE_INLINE_ duk_context *get_context() { return this->ctx; }
	static DuktapeBindingHelper *get_singleton();
	static ECMAScriptLanguage *get_language();

	virtual void initialize();
	virtual void uninitialize();

	virtual void *alloc_object_binding_data(Object *p_object);
	virtual void free_object_binding_data(void *p_gc_handle);

	virtual void godot_refcount_incremented(Reference *p_object);
	virtual bool godot_refcount_decremented(Reference *p_object);

	virtual Error eval_string(const String &p_source);

	virtual ECMAScriptGCHandler create_ecma_instance_for_godot_object(const StringName& ecma_class_name, Object *p_object);
	virtual Variant call_method(const ECMAScriptGCHandler& p_object, const ECMAMethodInfo& p_method, const Variant **p_args, int p_argcount, Variant::CallError &r_error);
};

#endif
