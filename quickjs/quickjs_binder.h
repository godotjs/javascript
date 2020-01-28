#ifndef QUICKJS_BINDING_HELPER_H
#define QUICKJS_BINDING_HELPER_H

#include "../ecmascript_binder.h"
#include "core/os/memory.h"
#include "quickjs_builtin_binder.h"
#include <quickjs.h>
#define JS_HIDDEN_SYMBOL(x) ("\xFF" x)
#define BINDING_DATA_FROM_JS(ctx, p_val) (ECMAScriptGCHandler *)JS_GetOpaque((p_val), QuickJSBinder::get_origin_class_id((ctx)))
#define GET_JSVALUE(p_gc_handler) JS_MKPTR(JS_TAG_OBJECT, (p_gc_handler).ecma_object)
#define NO_MODULE_EXPORT_SUPPORT 0
#define MODULE_HAS_REFCOUNT 0 // module seems don't follow the refrence count rule in quickjs
#define MAX_ARGUMENT_COUNT 50

class QuickJSWorker;

class QuickJSBinder : public ECMAScriptBinder {

	friend class QuickJSBuiltinBinder;
	friend class QuickJSWorker;
	QuickJSBuiltinBinder builtin_binder;

protected:
	static uint32_t global_context_id;
	static uint32_t global_transfer_id;
	JSRuntime *runtime;
	JSContext *ctx;
	JSMallocFunctions godot_allocator;
	uint32_t context_id;

public:
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

	struct ModuleCache {
		JSModuleDef *module;
		String code_md5;
		bool evaluated;
	};

	enum {
		PROP_DEF_DEFAULT = JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE,
	};

	enum {
		JS_ATOM_NULL,
#if !defined(EMSCRIPTEN)
#define CONFIG_ATOMICS
#endif
#define DEF(name, str) JS_ATOM_##name,
#include "quickjs-atom.h"
#undef DEF
#ifdef CONFIG_ATOMICS
#undef CONFIG_ATOMICS
#endif
		JS_ATOM_END,
	};

protected:
	JSAtom js_key_godot_classid;
	JSAtom js_key_godot_tooled;
	JSAtom js_key_godot_icon_path;
	JSAtom js_key_godot_exports;
	JSAtom js_key_godot_signals;

	JSValue global_object;
	JSValue godot_object;
	JSValue empty_function;
	JSValue js_operators;
	JSValue js_operators_create;
	Vector<JSValue> godot_singletons;

	_FORCE_INLINE_ static void *js_binder_malloc(JSMallocState *s, size_t size) { return memalloc(size); }
	_FORCE_INLINE_ static void js_binder_free(JSMallocState *s, void *ptr) {
		if (ptr) memfree(ptr);
	}
	_FORCE_INLINE_ static void *js_binder_realloc(JSMallocState *s, void *ptr, size_t size) { return memrealloc(ptr, size); }

	static JSModuleDef *js_module_loader(JSContext *ctx, const char *module_name, void *opaque);
	ModuleCache *js_compile_module(JSContext *ctx, const String &p_code, const String &p_filename, ECMAscriptScriptError *r_error);
	static int resource_module_initializer(JSContext *ctx, JSModuleDef *m);

	struct ClassBindData {
		JSClassID class_id;
		CharString class_name;
		JSValue prototype;
		JSValue constructor;
		JSClassDef jsclass;
		const ClassDB::ClassInfo *gdclass;
		const ClassBindData *base_class;
	};
	ClassBindData godot_origin_class;
	const ClassBindData *godot_object_class;
	HashMap<JSClassID, ClassBindData> class_bindings;
	HashMap<StringName, const ClassBindData *> classname_bindings;
	HashMap<String, ModuleCache> module_cache;
	ClassBindData worker_class_data;
	List<ECMAScriptGCHandler *> workers;
	Vector<MethodBind *> godot_methods;
	int internal_godot_method_id;
	const ECMAScriptGCHandler *lastest_allocated_object = NULL;
	Variant *method_call_arguments;
	const Variant **method_call_argument_ptrs;

#if NO_MODULE_EXPORT_SUPPORT
	String parsing_script_file;
#endif

	JSClassID register_class(const ClassDB::ClassInfo *p_cls);
	void add_godot_origin();
	void add_godot_classes();
	void add_godot_globals();
	void add_global_console();
	void add_global_properties();
	void add_global_worker();

	static JSValue object_constructor(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv, int class_id);
	static void object_finalizer(ECMAScriptGCHandler *p_bind);
	static void origin_finalizer(JSRuntime *rt, JSValue val);

	static JSValue object_free(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv);
	static JSValue object_method(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv, int class_id);
	static JSValue godot_to_string(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv);
	static JSValue godot_get_type(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv);
	static JSValue godot_load(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv);
	static void add_debug_binding_info(JSContext *ctx, JSValueConst p_obj, const ECMAScriptGCHandler *p_bind);

	const ECMAClassInfo *register_ecma_class(const JSValueConst &p_constructor, const String &p_path);
	void free_ecmas_class(const ECMAClassInfo &p_class);
	static JSValue godot_register_class(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv);
	static JSValue godot_register_signal(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv);
	static JSValue godot_register_property(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv);

	enum {
		SCRIPT_META_TOOLED,
		SCRIPT_META_ICON,
	};
	static JSValue godot_set_script_meta(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv, int magic);

	static JSValue console_log_function(JSContext *ctx, JSValue this_val, int argc, JSValue *argv, int magic);
	static JSValue global_request_animation_frame(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv);
	static JSValue global_cancel_animation_frame(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv);

	static JSValue worker_constructor(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv);
	static void worker_finializer(JSRuntime *rt, JSValue val);
	static JSValue worker_post_message(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv);
	static JSValue worker_terminate(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv);
	static JSValue worker_abandon_value(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv);
	static JSValue worker_adopt_value(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv);

	_FORCE_INLINE_ static JSValue js_empty_func(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) { return JS_UNDEFINED; }
	_FORCE_INLINE_ static JSValue js_empty_consturctor(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) { return JS_NewObject(ctx); }
	static JSValue godot_builtin_function(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv, int magic);

	static int get_js_array_length(JSContext *ctx, JSValue p_val);
	static void get_own_property_names(JSContext *ctx, JSValue p_object, Set<String> *r_list);

	static JSAtom get_atom(JSContext *ctx, const StringName &p_key);
	static HashMap<uint32_t, ECMAScriptGCHandler *> transfer_deopot;
	static Map<String, const char *> class_remap;

public:
	static Error define_operators(JSContext *ctx, JSValue p_prototype, JSValue *p_operators, int p_size);

public:
	static JSValue variant_to_var(JSContext *ctx, const Variant p_var);
	static Variant var_to_variant(JSContext *ctx, JSValue p_val);
	static bool validate_type(JSContext *ctx, Variant::Type p_type, JSValueConst &p_val);
	static void dump_exception(JSContext *ctx, const JSValueConst &p_exception, ECMAscriptScriptError *r_error);
	virtual String error_to_string(const ECMAscriptScriptError &p_error);

	_FORCE_INLINE_ static real_t js_to_number(JSContext *ctx, const JSValueConst &p_val) {
		double_t v = 0;
		JS_ToFloat64(ctx, &v, p_val);
		return real_t(v);
	}
	_FORCE_INLINE_ static String js_to_string(JSContext *ctx, const JSValueConst &p_val) {
		String ret;
		size_t len = 0;
		const char *utf8 = JS_ToCStringLen(ctx, &len, p_val);
		ret.parse_utf8(utf8, len);
		JS_FreeCString(ctx, utf8);
		return ret;
	}
	_FORCE_INLINE_ static bool js_to_bool(JSContext *ctx, const JSValueConst &p_val) {
		return JS_ToBool(ctx, p_val);
	}
	_FORCE_INLINE_ static int32_t js_to_int(JSContext *ctx, const JSValueConst &p_val) {
		int32_t i;
		JS_ToInt32(ctx, &i, p_val);
		return i;
	}
	_FORCE_INLINE_ static uint32_t js_to_uint(JSContext *ctx, const JSValueConst &p_val) {
		uint32_t u;
		JS_ToUint32(ctx, &u, p_val);
		return u;
	}
	_FORCE_INLINE_ static JSValue to_js_number(JSContext *ctx, real_t p_val) {
		return JS_NewFloat64(ctx, double(p_val));
	}
	_FORCE_INLINE_ static JSValue to_js_string(JSContext *ctx, const String &text) {
		CharString utf8 = text.utf8();
		return JS_NewStringLen(ctx, utf8.ptr(), utf8.length());
	}
	_FORCE_INLINE_ static JSValue to_js_bool(JSContext *ctx, bool p_val) {
		return JS_NewBool(ctx, p_val);
	}

public:
	QuickJSBinder();
	virtual ~QuickJSBinder();

	_FORCE_INLINE_ static QuickJSBinder *get_context_binder(JSContext *ctx) {
		return static_cast<QuickJSBinder *>(JS_GetContextOpaque(ctx));
	}

	virtual ECMAScriptBinder *get_context_binder(void *p_context) {
		return QuickJSBinder::get_context_binder((JSContext*)p_context);
	}

	_FORCE_INLINE_ static QuickJSBinder *get_runtime_binder(JSRuntime *rt) {
		return static_cast<QuickJSBinder *>(JS_GetMollocState(rt)->opaque);
	}

	_FORCE_INLINE_ static ECMAScriptGCHandler *new_gc_handler(JSContext* ctx) {
		ECMAScriptGCHandler *h = memnew(ECMAScriptGCHandler);
		h->context = ctx;
		return h;
	}

	_FORCE_INLINE_ QuickJSBuiltinBinder &get_builtin_binder() { return builtin_binder; }

	_FORCE_INLINE_ JSClassID get_origin_class_id() { return godot_origin_class.class_id; }
	_FORCE_INLINE_ const ClassBindData get_origin_class() const { return godot_origin_class; }
	_FORCE_INLINE_ static JSClassID get_origin_class_id(JSContext *ctx) { return get_context_binder(ctx)->godot_origin_class.class_id; }

	virtual void initialize();
	virtual void uninitialize();
	virtual void language_finalize();
	virtual void frame();

	virtual void *alloc_object_binding_data(Object *p_object);
	virtual void free_object_binding_data(void *p_gc_handle);
	static Error bind_gc_object(JSContext *ctx, ECMAScriptGCHandler *data, Object *p_object);

	virtual void godot_refcount_incremented(Reference *p_object);
	virtual bool godot_refcount_decremented(Reference *p_object);

	virtual Error eval_string(const String &p_source, const String &p_path);
	virtual Error safe_eval_text(const String &p_source, const String &p_path, String &r_error);

	virtual Error compile_to_bytecode(const String &p_code, Vector<uint8_t> &r_bytecode);
	virtual Error load_bytecode(const Vector<uint8_t> &p_bytecode, ECMAScriptGCHandler *r_module);

	virtual const ECMAClassInfo *parse_ecma_class(const String &p_code, const String &p_path, ECMAscriptScriptError *r_error);
	virtual const ECMAClassInfo *parse_ecma_class(const Vector<uint8_t> &p_bytecode, const String &p_path, ECMAscriptScriptError *r_error);
	const ECMAClassInfo *parse_ecma_class_from_module(ModuleCache *p_module, const String &p_path, ECMAscriptScriptError *r_error);

	virtual ECMAScriptGCHandler create_ecma_instance_for_godot_object(const ECMAClassInfo *p_class, Object *p_object);
	virtual Variant call_method(const ECMAScriptGCHandler &p_object, const StringName &p_method, const Variant **p_args, int p_argcount, Variant::CallError &r_error);
	virtual bool get_instance_property(const ECMAScriptGCHandler &p_object, const StringName &p_name, Variant &r_ret);
	virtual bool set_instance_property(const ECMAScriptGCHandler &p_object, const StringName &p_name, const Variant &p_value);
	virtual bool has_method(const ECMAScriptGCHandler &p_object, const StringName &p_name);
	virtual bool has_signal(const ECMAClassInfo *p_class, const StringName &p_signal);
	virtual bool validate(const String &p_code, const String &p_path, ECMAscriptScriptError *r_error);
};

#endif // QUICKJS_BINDING_HELPER_H
