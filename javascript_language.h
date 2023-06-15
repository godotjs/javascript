/**************************************************************************/
/*  javascript_language.h                                                 */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#ifndef JAVASCRIPT_LANGUAGE_H
#define JAVASCRIPT_LANGUAGE_H

#include "core/object/script_language.h"

#include "javascript.h"
#include "javascript_binder.h"

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
	_FORCE_INLINE_ static JavaScriptLanguage *get_singleton() { return singleton; }
	_FORCE_INLINE_ static JavaScriptBinder *get_main_binder() { return singleton->main_binder; }
	_FORCE_INLINE_ static JavaScriptBinder *get_thread_binder(Thread::ID p_id) {
		if (JavaScriptBinder **ptr = singleton->thread_binder_map.getptr(p_id)) {
			return *ptr;
		}
		return nullptr;
	}

	_FORCE_INLINE_ CallableMiddleman *get_callable_middleman() const {
		return callable_middleman;
	}

	_FORCE_INLINE_ virtual String get_name() const override { return "JavaScript"; }
	const GDExtensionInstanceBindingCallbacks *get_instance_binding_callbacks() const { return &instance_binding_callbacks; }
#ifdef TOOLS_ENABLED
	_FORCE_INLINE_ HashSet<Ref<JavaScript>> &get_scripts() { return scripts; }
#endif
	/* LANGUAGE FUNCTIONS */

	_FORCE_INLINE_ virtual String get_type() const override { return "JavaScript"; }
	_FORCE_INLINE_ virtual String get_extension() const override { return EXT_JSCLASS; }

	virtual void init() override;
	virtual void finish() override;
	virtual Error execute_file(const String &p_path);

	virtual void get_reserved_words(List<String> *p_words) const override;
	virtual bool is_control_flow_keyword(String p_keywords) const override;
	virtual void get_comment_delimiters(List<String> *p_delimiters) const override;
	virtual void get_string_delimiters(List<String> *p_delimiters) const override;

	virtual Ref<Script> make_template(const String &p_template, const String &p_class_name, const String &p_base_class_name) const override;
	virtual Vector<ScriptTemplate> get_built_in_templates(StringName p_object) override;
	virtual bool is_using_templates() override { return true; }

	virtual Script *create_script() const override;
	_FORCE_INLINE_ virtual bool has_named_classes() const override { return true; }
	_FORCE_INLINE_ virtual bool supports_builtin_mode() const override { return false; }

	virtual bool validate(const String &p_script, const String &p_path = "", List<String> *r_functions = nullptr, List<ScriptError> *r_errors = nullptr, List<Warning> *r_warnings = nullptr, HashSet<int> *r_safe_lines = nullptr) const override;

	virtual int find_function(const String &p_function, const String &p_code) const override { return -1; }
	virtual String make_function(const String &p_class, const String &p_name, const PackedStringArray &p_args) const override { return ""; }

	virtual void auto_indent_code(String &p_code, int p_from_line, int p_to_line) const override {}
	virtual void add_global_constant(const StringName &p_variable, const Variant &p_value) override {}

	virtual void thread_enter() override {}
	virtual void thread_exit() override {}

	/* DEBUGGER FUNCTIONS */

	/* DEBUGGER FUNCTIONS */
	virtual String debug_get_error() const override { return ""; }
	virtual int debug_get_stack_level_count() const override { return 1; }
	virtual int debug_get_stack_level_line(int p_level) const override { return 1; }
	virtual String debug_get_stack_level_function(int p_level) const override { return ""; }
	virtual String debug_get_stack_level_source(int p_level) const override { return ""; }
	virtual void debug_get_stack_level_locals(int p_level, List<String> *p_locals, List<Variant> *p_values, int p_max_subitems, int p_max_depth) override {}
	virtual void debug_get_stack_level_members(int p_level, List<String> *p_members, List<Variant> *p_values, int p_max_subitems, int p_max_depth) override {}
	virtual void debug_get_globals(List<String> *p_locals, List<Variant> *p_values, int p_max_subitems, int p_max_depth) override {}
	virtual String debug_parse_stack_level_expression(int p_level, const String &p_expression, int p_max_subitems, int p_max_depth) override { return ""; }
	virtual Vector<StackInfo> debug_get_current_stack_info() override { return Vector<StackInfo>(); }

	void reload_script(const Ref<Script> &p_script, bool p_soft_reload);

	virtual void reload_all_scripts() override;
	virtual void reload_tool_script(const Ref<Script> &p_script, bool p_soft_reload) override { reload_script(p_script, p_soft_reload); }

	/* LOADER FUNCTIONS */
	virtual void get_recognized_extensions(List<String> *p_extensions) const override;
	virtual void get_public_functions(List<MethodInfo> *p_functions) const override {}
	virtual void get_public_constants(List<Pair<String, Variant>> *p_constants) const override {}
	virtual void get_public_annotations(List<MethodInfo> *p_annotations) const override{};

	virtual void profiling_start() override {}
	virtual void profiling_stop() override {}

	virtual int profiling_get_accumulated_data(ProfilingInfo *p_info_arr, int p_info_max) override { return -1; }
	virtual int profiling_get_frame_data(ProfilingInfo *p_info_arr, int p_info_max) override { return -1; }

	virtual void frame() override;

	virtual bool handles_global_class_type(const String &p_type) const override { return false; }
	virtual String get_global_class_name(const String &p_path, String *r_base_type = NULL, String *r_icon_path = NULL) const override { return String(); }

	static String globalize_relative_path(const String &p_relative, const String &p_base_dir);

	JavaScriptLanguage();
	virtual ~JavaScriptLanguage();
};

#endif // JAVASCRIPT_LANGUAGE_H
