#ifndef ECMASCRIPT_LANGUAGE_H
#define ECMASCRIPT_LANGUAGE_H

#include "core/script_language.h"
#include "ecmascript.h"
#include "quickjs/quickjs_binder.h"

/*********************** ECMAScriptLanguage ***********************/
class ECMAScriptBinder;
class ECMAScriptLanguage : public ScriptLanguage {

	friend class ECMAScriptBinder;
	friend class ECMAScript;
	friend class ECMAScriptInstance;
	friend class DuktapeBindingHelper;
	friend class QuickJSBinder;

private:
	static ECMAScriptLanguage *singleton;
	ECMAScriptBinder *main_binder;
	int language_index;
	HashMap<Thread::ID, ECMAScriptBinder *> thread_binder_map;
#ifdef TOOLS_ENABLED
	Set<Ref<ECMAScript> > scripts;
#endif

public:
	_FORCE_INLINE_ static ECMAScriptLanguage *get_singleton() { return singleton; }
	_FORCE_INLINE_ static ECMAScriptBinder *get_main_binder() { return singleton->main_binder; }
	_FORCE_INLINE_ static ECMAScriptBinder *get_thread_binder(Thread::ID p_id) {
		if (ECMAScriptBinder **ptr = singleton->thread_binder_map.getptr(p_id)) {
			return *ptr;
		}
		return NULL;
	}

	_FORCE_INLINE_ virtual String get_name() const { return "JavaScript"; }
	_FORCE_INLINE_ int get_language_index() const { return language_index; }
	_FORCE_INLINE_ void set_language_index(int value) { language_index = value; }

#ifdef TOOLS_ENABLED
	_FORCE_INLINE_ Set<Ref<ECMAScript> > &get_scripts() { return scripts; }
#endif
	/* LANGUAGE FUNCTIONS */

	_FORCE_INLINE_ virtual String get_type() const { return "JavaScript"; }
	_FORCE_INLINE_ virtual String get_extension() const { return EXT_JSCLASS; }
	_FORCE_INLINE_ virtual bool has_named_classes() const { return true; }
	_FORCE_INLINE_ virtual bool supports_builtin_mode() const { return false; }
	_FORCE_INLINE_ virtual bool can_inherit_from_file() { return false; }
	_FORCE_INLINE_ virtual bool is_using_templates() { return true; }
	_FORCE_INLINE_ virtual bool overrides_external_editor() { return false; }

	virtual void init();
	virtual void finish();

	virtual Error execute_file(const String &p_path);

	virtual void get_reserved_words(List<String> *p_words) const;
	virtual bool is_control_flow_keyword(String p_keywords) const;
	virtual void get_comment_delimiters(List<String> *p_delimiters) const;
	virtual void get_string_delimiters(List<String> *p_delimiters) const;

	virtual Ref<Script> get_template(const String &p_class_name, const String &p_base_class_name) const;
	virtual void make_template(const String &p_class_name, const String &p_base_class_name, Ref<Script> &p_script);

	virtual bool validate(const String &p_script, int &r_line_error, int &r_col_error, String &r_test_error, const String &p_path = "", List<String> *r_functions = NULL, List<Warning> *r_warnings = NULL, Set<int> *r_safe_lines = NULL) const;
	virtual String validate_path(const String &p_path) const { return ""; }
	virtual Script *create_script() const;

	/* TODO */ virtual int find_function(const String &p_function, const String &p_code) const { return -1; }
	/* TODO */ virtual String make_function(const String &p_class, const String &p_name, const PoolStringArray &p_args) const { return ""; }
	/* TODO */ virtual Error open_in_external_editor(const Ref<Script> &p_script, int p_line, int p_col) { return ERR_UNAVAILABLE; }

	/* TODO */ virtual Error complete_code(const String &p_code, const String &p_path, Object *p_owner, List<ScriptCodeCompletionOption> *r_options, bool &r_force, String &r_call_hint) { return ERR_UNAVAILABLE; }
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

	void reload_script(const Ref<Script> &p_script, bool p_soft_reload);
	virtual void reload_all_scripts();
	virtual void reload_tool_script(const Ref<Script> &p_script, bool p_soft_reload) { reload_script(p_script, p_soft_reload); }

	/* LOADER FUNCTIONS */
	virtual void get_recognized_extensions(List<String> *p_extensions) const;
	/* TODO */ virtual void get_public_functions(List<MethodInfo> *p_functions) const {}
	/* TODO */ virtual void get_public_constants(List<Pair<String, Variant> > *p_constants) const {}

	/* TODO */ virtual void profiling_start() {}
	/* TODO */ virtual void profiling_stop() {}

	/* TODO */ virtual int profiling_get_accumulated_data(ProfilingInfo *p_info_arr, int p_info_max) { return -1; }
	/* TODO */ virtual int profiling_get_frame_data(ProfilingInfo *p_info_arr, int p_info_max) { return -1; }

	virtual void *alloc_instance_binding_data(Object *p_object); //optional, not used by all languages
	virtual void free_instance_binding_data(void *p_data); //optional, not used by all languages
	virtual void refcount_incremented_instance_binding(Object *p_object); //optional, not used by all languages
	virtual bool refcount_decremented_instance_binding(Object *p_object); //return true if it can die //optional, not used by all languages

	virtual void frame();

	/* TODO */ virtual bool handles_global_class_type(const String &p_type) const { return false; }
	/* TODO */ virtual String get_global_class_name(const String &p_path, String *r_base_type = NULL, String *r_icon_path = NULL) const { return String(); }

	static String globalize_relative_path(const String &p_relative, const String &p_base_dir);

	ECMAScriptLanguage();
	virtual ~ECMAScriptLanguage();
};

#endif // ECMASCRIPT_LANGUAGE_H
