#ifndef DUKTAPE_BINDING_HELPER_H
#define DUKTAPE_BINDING_HELPER_H

#include "core/hash_map.h"
#include "core/object.h"
#include "core/string_db.h"
#include "core/variant.h"
#include <duktape/duktape.h>

#define NO_RET_VAL 0
#define HAS_RET_VAL 1

typedef void DuktapeHeapObject;

class DuktapeBindingHelper {
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

public:
	HashMap<ObjectID, DuktapeHeapObject *> heap_objects;
	HashMap<StringName, DuktapeHeapObject *> class_prototypes;
	HashMap<const MethodBind *, DuktapeHeapObject *, MethodPtrHash> method_bindings;

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
	static duk_ret_t duk_godot_object_method(duk_context *ctx);
	static duk_ret_t godot_object_free(duk_context *ctx);

	static duk_ret_t godot_print_function(duk_context *ctx);

	static void duk_push_godot_variant(duk_context *ctx, const Variant &var);
	static void duk_push_godot_object(duk_context *ctx, Object *obj);
	static void duk_push_godot_string(duk_context *ctx, const String &str);
	static void duk_push_godot_string_name(duk_context *ctx, const StringName &str);

	static void duk_put_prop_godot_string(duk_context *ctx, duk_idx_t idx, const String &str);
	static void duk_put_prop_godot_string_name(duk_context *ctx, duk_idx_t idx, const StringName &str);

	static Variant duk_get_godot_variant(duk_context *ctx, duk_idx_t idx);
	static String duk_get_godot_string(duk_context *ctx, duk_idx_t idx, bool convert_type = false);
	static Object *duk_get_godot_object(duk_context *ctx, duk_idx_t idx);

	void rigister_class(duk_context *ctx, const ClassDB::ClassInfo *cls);

	_FORCE_INLINE_ static DuktapeBindingHelper *get_singleton();
	_FORCE_INLINE_ duk_context *get_context() { return this->ctx; }

	void initialize();
	void uninitialize();

private:
	void register_class_members(duk_context *ctx, const ClassDB::ClassInfo *cls);
	void duk_push_godot_method(duk_context *ctx, const MethodBind *mb);
};

#endif
