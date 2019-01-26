#ifndef ECMASCRIPT_H
#define ECMASCRIPT_H

#include "core/script_language.h"
#include "duktape/duktape.h"

class DuktapeBindingHelper;

class ECMAScriptLanguage : public ScriptLanguage {

	friend class DuktapeBindingHelper;

private:
	static ECMAScriptLanguage *singleton;
	DuktapeBindingHelper *binding;

public:
	_FORCE_INLINE_ static ECMAScriptLanguage *get_singleton() { return singleton; }
	_FORCE_INLINE_ virtual String get_name() const { return "ECMAScript"; }

	/* LANGUAGE FUNCTIONS */

	_FORCE_INLINE_ virtual String get_type() const { return "ECMAScript"; }
	_FORCE_INLINE_ virtual String get_extension() const { return "js"; }
	_FORCE_INLINE_ virtual bool has_named_classes() const { return true; }
	_FORCE_INLINE_ virtual bool supports_builtin_mode() const { return false; }
	_FORCE_INLINE_ virtual bool can_inherit_from_file() { return false; }
	_FORCE_INLINE_ virtual bool is_using_templates() { return true; }
	_FORCE_INLINE_ virtual bool overrides_external_editor() { return false; }

	virtual void init();
	virtual void finish();

	/* TODO */ virtual Error execute_file(const String &p_path);

	virtual void get_reserved_words(List<String> *p_words) const;
	virtual void get_comment_delimiters(List<String> *p_delimiters) const;
	virtual void get_string_delimiters(List<String> *p_delimiters) const;

	/* TODO */ virtual Ref<Script> get_template(const String &p_class_name, const String &p_base_class_name) const { return NULL; }
	/* TODO */ virtual void make_template(const String &p_class_name, const String &p_base_class_name, Ref<Script> &p_script) {}
	/* TODO */ virtual bool validate(const String &p_script, int &r_line_error, int &r_col_error, String &r_test_error, const String &p_path = "", List<String> *r_functions = NULL, List<Warning> *r_warnings = NULL, Set<int> *r_safe_lines = NULL) const { return true; }
	/* TODO */ virtual String validate_path(const String &p_path) const { return ""; }
	/* TODO */ virtual Script *create_script() const { return NULL; }

	/* TODO */ virtual int find_function(const String &p_function, const String &p_code) const { return -1; }
	/* TODO */ virtual String make_function(const String &p_class, const String &p_name, const PoolStringArray &p_args) const { return ""; }
	/* TODO */ virtual Error open_in_external_editor(const Ref<Script> &p_script, int p_line, int p_col) { return ERR_UNAVAILABLE; }

	/* TODO */ virtual Error complete_code(const String &p_code, const String &p_base_path, Object *p_owner, List<String> *r_options, bool &r_force, String &r_call_hint) { return ERR_UNAVAILABLE; }
	/* TODO */ virtual Error lookup_code(const String &p_code, const String &p_symbol, const String &p_base_path, Object *p_owner, LookupResult &r_result) { return ERR_UNAVAILABLE; }

	/* TODO */ virtual void auto_indent_code(String &p_code, int p_from_line, int p_to_line) const {}
	/* TODO */ virtual void add_global_constant(const StringName &p_variable, const Variant &p_value) {}
	/* TODO */ virtual void add_named_global_constant(const StringName &p_name, const Variant &p_value) {}
	/* TODO */ virtual void remove_named_global_constant(const StringName &p_name) {}

	/* MULTITHREAD FUNCTIONS */

	//some VMs need to be notified of thread creation/exiting to allocate a stack
	/* TODO */ virtual void thread_enter() {}
	/* TODO */ virtual void thread_exit() {}

	/* DEBUGGER FUNCTIONS */

	/* DEBUGGER FUNCTIONS */
	/* TODO */ virtual String debug_get_error() const { return ""; }
	/* TODO */ virtual int debug_get_stack_level_count() const { return 1; }
	/* TODO */ virtual int debug_get_stack_level_line(int p_level) const { return 1; }
	/* TODO */ virtual String debug_get_stack_level_function(int p_level) const { return ""; }
	/* TODO */ virtual String debug_get_stack_level_source(int p_level) const { return ""; }
	/* TODO */ virtual void debug_get_stack_level_locals(int p_level, List<String> *p_locals, List<Variant> *p_values, int p_max_subitems, int p_max_depth) {}
	/* TODO */ virtual void debug_get_stack_level_members(int p_level, List<String> *p_members, List<Variant> *p_values, int p_max_subitems, int p_max_depth) {}
	/* TODO */ virtual void debug_get_globals(List<String> *p_locals, List<Variant> *p_values, int p_max_subitems, int p_max_depth) {}
	/* TODO */ virtual String debug_parse_stack_level_expression(int p_level, const String &p_expression, int p_max_subitems, int p_max_depth) { return ""; }
	/* TODO */ virtual Vector<StackInfo> debug_get_current_stack_info() { return Vector<StackInfo>(); }

	/* TODO */ virtual void reload_all_scripts(){};
	/* TODO */ virtual void reload_tool_script(const Ref<Script> &p_script, bool p_soft_reload){};

	/* LOADER FUNCTIONS */
	virtual void get_recognized_extensions(List<String> *p_extensions) const;
	/* TODO */ virtual void get_public_functions(List<MethodInfo> *p_functions) const {}
	/* TODO */ virtual void get_public_constants(List<Pair<String, Variant> > *p_constants) const {}

	/* TODO */ virtual void profiling_start() {}
	/* TODO */ virtual void profiling_stop() {}

	/* TODO */ virtual int profiling_get_accumulated_data(ProfilingInfo *p_info_arr, int p_info_max) { return -1; }
	/* TODO */ virtual int profiling_get_frame_data(ProfilingInfo *p_info_arr, int p_info_max) { return -1; }

	/* TODO */ virtual void *alloc_instance_binding_data(Object *p_object) { return NULL; } //optional, not used by all languages
	/* TODO */ virtual void free_instance_binding_data(void *p_data) {} //optional, not used by all languages
	/* TODO */ virtual void refcount_incremented_instance_binding(Object *p_object) {} //optional, not used by all languages
	/* TODO */ virtual bool refcount_decremented_instance_binding(Object *p_object) { return true; } //return true if it can die //optional, not used by all languages

	/* TODO */ virtual void frame(){};

	/* TODO */ virtual bool handles_global_class_type(const String &p_type) const { return false; }
	/* TODO */ virtual String get_global_class_name(const String &p_path, String *r_base_type = NULL, String *r_icon_path = NULL) const { return String(); }

	ECMAScriptLanguage();
	virtual ~ECMAScriptLanguage();
};

typedef void ECMAScriptHeapObject;
typedef Object *(*GodotObjectCreator)();

#define NO_RET_VAL 0
#define HAS_RET_VAL 1

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
	HashMap<ObjectID, ECMAScriptHeapObject *> heap_objects;
	HashMap<StringName, ECMAScriptHeapObject *> class_prototypes;
	HashMap<const MethodBind *, ECMAScriptHeapObject *, MethodPtrHash> method_bindings;

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

	_FORCE_INLINE_ static DuktapeBindingHelper *get_singleton() { return ECMAScriptLanguage::get_singleton()->binding; }
	_FORCE_INLINE_ duk_context *get_context() { return this->ctx; }

	void initialize();
	void uninitialize();

private:
	void register_class_members(duk_context *ctx, const ClassDB::ClassInfo *cls);
	void duk_push_godot_method(duk_context *ctx, const MethodBind *mb);
};

#endif
