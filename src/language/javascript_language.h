
#ifndef JAVASCRIPT_LANGUAGE_H
#define JAVASCRIPT_LANGUAGE_H

#include "core/object/script_language.h"

#include "../../javascript.h"
#include "../../javascript_binder.h"

class CallableMiddleman : public Object {
	GDCLASS(CallableMiddleman, Object);
};

class JavaScriptBinder;
class JavaScriptLanguage : public ScriptLanguage {
	friend class JavaScriptBinder;
	friend class JavaScript;
	friend class JavaScriptInstance;
	friend class DuktapeBindingHelper;
	friend class QuickJSBinder;

private:
	static JavaScriptLanguage *singleton;
	JavaScriptBinder *main_binder;
	HashMap<Thread::ID, JavaScriptBinder *> thread_binder_map;
	GDExtensionInstanceBindingCallbacks instance_binding_callbacks;

	CallableMiddleman *callable_middleman;
#ifdef TOOLS_ENABLED
	HashSet<Ref<JavaScript>> scripts;
#endif

public:
	/* LANGUAGE FUNCTIONS */
	virtual String get_name() const override;
	virtual String get_type() const override;
	virtual String get_extension() const override;
	virtual bool has_named_classes() const override;
	virtual bool supports_builtin_mode() const override;
	virtual void get_recognized_extensions(List<String> *p_extensions) const override;
	virtual void get_reserved_words(List<String> *p_words) const override;
	virtual bool is_control_flow_keyword(String p_keywords) const override;
	virtual void get_comment_delimiters(List<String> *p_delimiters) const override;
	virtual void get_string_delimiters(List<String> *p_delimiters) const override;

	virtual void init() override;
	virtual void finish() override;
	virtual void frame() override;
	virtual Script *create_script() const override;
	virtual bool validate(const String &p_script, const String &p_path = "", List<String> *r_functions = nullptr, List<ScriptError> *r_errors = nullptr, List<Warning> *r_warnings = nullptr, HashSet<int> *r_safe_lines = nullptr) const override;
	virtual void reload_all_scripts() override;
	virtual void reload_tool_script(const Ref<Script> &p_script, bool p_soft_reload) override;

	/**
	 * Add a global before autoload
	 * @param p_variable Name of the global constant
	 * @param p_value Value of the global constant
	 */
	virtual void add_global_constant(const StringName &p_variable, const Variant &p_value) override;

	/* TEXT EDITOR FUNCTIONS */
	virtual int find_function(const String &p_function, const String &p_code) const override;
	virtual String make_function(const String &p_class, const String &p_name, const PackedStringArray &p_args) const override;
	virtual void auto_indent_code(String &p_code, int p_from_line, int p_to_line) const override;

	/* SCRIPT GLOBAL CLASS FUNCTIONS */
	/*TODO*/ virtual bool handles_global_class_type(const String &p_type) const override { return false; }
	/*TODO*/ virtual String get_global_class_name(const String &p_path, String *r_base_type = NULL, String *r_icon_path = NULL) const override { return String(); }

	/* DEBUGGER FUNCTIONS */
	virtual String debug_get_error() const override;
	virtual int debug_get_stack_level_count() const override;
	virtual int debug_get_stack_level_line(int p_level) const override;
	virtual String debug_get_stack_level_function(int p_level) const override;
	virtual String debug_get_stack_level_source(int p_level) const override;
	virtual void debug_get_stack_level_locals(int p_level, List<String> *p_locals, List<Variant> *p_values, int p_max_subitems, int p_max_depth) override;
	virtual void debug_get_stack_level_members(int p_level, List<String> *p_members, List<Variant> *p_values, int p_max_subitems, int p_max_depth) override;
	virtual void debug_get_globals(List<String> *p_locals, List<Variant> *p_values, int p_max_subitems, int p_max_depth) override;
	virtual String debug_parse_stack_level_expression(int p_level, const String &p_expression, int p_max_subitems, int p_max_depth) override;
	virtual Vector<StackInfo> debug_get_current_stack_info() override;

	/* MULTITHREAD FUNCTIONS */
	virtual void thread_enter() override;
	virtual void thread_exit() override;

	/* TEMPLATE Functions */
	virtual Ref<Script> make_template(const String &p_template, const String &p_class_name, const String &p_base_class_name) const override;
	virtual Vector<ScriptTemplate> get_built_in_templates(StringName p_object) override;
	virtual bool is_using_templates() override;

	/* DOCTOOL FUNCTIONS */
	virtual void get_public_functions(List<MethodInfo> *p_functions) const override;
	virtual void get_public_constants(List<Pair<String, Variant>> *p_constants) const override;
	virtual void get_public_annotations(List<MethodInfo> *p_annotations) const override;

	/* Profiling Functions */
	virtual void profiling_start() override;
	virtual void profiling_stop() override;

	virtual int profiling_get_accumulated_data(ProfilingInfo *p_info_arr, int p_info_max) override;
	virtual int profiling_get_frame_data(ProfilingInfo *p_info_arr, int p_info_max) override;

	/* ----------- Custom Functions ----------- */
	_FORCE_INLINE_ static JavaScriptLanguage *get_singleton() { return singleton; }
	_FORCE_INLINE_ static JavaScriptBinder *get_main_binder() { return singleton->main_binder; }
	_FORCE_INLINE_ static JavaScriptBinder *get_thread_binder(Thread::ID p_id) {
		if (JavaScriptBinder **ptr = singleton->thread_binder_map.getptr(p_id)) {
			return *ptr;
		}
		return nullptr;
	}
	_FORCE_INLINE_ CallableMiddleman *get_callable_middleman() const { return callable_middleman; }
	const GDExtensionInstanceBindingCallbacks *get_instance_binding_callbacks() const { return &instance_binding_callbacks; }

	/**
	 * Helper function to reload a single script
	 * @param p_script Pointer to the JS script
	 * @param p_soft_reload Pointer to boolean if script should be soft reloaded
	 */
	static void reload_script(const Ref<Script> &p_script, bool p_soft_reload);

	/**
	 * Executes a js file with JavaScriptBinder - currently used via init() for testing the editor in cicd
	 * @param code Code as string which should be executed
	 * @return
	 */
	virtual Error execute_file(const String &code);

	/* Instance Binding Callbacks*/
	/**
	 * Allocates space for a new object created in binders
	 * @param p_token Not required for js
	 * @param p_instance Pointer to the js object
	 * @return Pointer to the bound obj
	 */
	static void *create_callback(void *p_token, void *p_instance);

	/**
	 * Frees space for an object created by binders
	 * @param p_token Not required for js
	 * @param p_instance Not required for js
	 * @param p_binding Pointer returned by create_callback
	 */
	static void free_callback(void *p_token, void *p_instance, void *p_binding);

	/**
	 *
	 * @param p_token Not required for js
	 * @param p_binding Pointer returned by create_callback
	 * @param p_reference A boolean to increase/decrease the ref_count for godot
	 * @return Successful increase or decrease
	 */
	static GDExtensionBool reference_callback(void *p_token, void *p_binding, GDExtensionBool p_reference);

#ifdef TOOLS_ENABLED
	_FORCE_INLINE_ HashSet<Ref<JavaScript>> &get_scripts() { return scripts; }
#endif

	JavaScriptLanguage();
	virtual ~JavaScriptLanguage();
};

#endif // JAVASCRIPT_LANGUAGE_H
