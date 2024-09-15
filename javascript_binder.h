#ifndef JAVASCRIPT_BINDER_H
#define JAVASCRIPT_BINDER_H

#include "core/os/thread.h"
#include "javascript_gc_handler.h"

typedef JavaScriptGCHandler JSMethodInfo;

struct JavaScriptProperyInfo : public PropertyInfo {
	Variant default_value;
};

struct JavaScriptError {
	int line;
	int column;
	String message;
	String file;
	Vector<String> stack;
};

struct BasicJavaScriptClassInfo {
	bool tool;
	StringName class_name;
	String icon_path;
	const ClassDB::ClassInfo *native_class;
	HashMap<StringName, MethodInfo> signals;
	HashMap<StringName, MethodInfo> methods;
	HashMap<StringName, JavaScriptProperyInfo> properties;
};

struct JavaScriptClassInfo : public BasicJavaScriptClassInfo {
	JavaScriptGCHandler constructor;
	JavaScriptGCHandler prototype;
};

struct GlobalNumberConstant {
	StringName name;
	double_t value;
};

struct JavaScriptStackInfo {
	int line;
	String file;
	String function;
};

class JavaScriptBinder {
protected:
	// Path ==> JavaScript Class
	HashMap<String, JavaScriptClassInfo> javascript_classes;
	HashMap<int64_t, JavaScriptGCHandler> frame_callbacks;
	HashSet<int64_t> canceled_frame_callbacks;
	static String BINDING_SCRIPT_CONTENT;

public:
	enum EvalType {
		EVAL_TYPE_MODULE,
		EVAL_TYPE_GLOBAL,
	};

	JavaScriptBinder() {}
	virtual ~JavaScriptBinder(){};

	virtual JavaScriptBinder *get_context_binder(void *p_context) = 0;
	virtual Thread::ID get_thread_id() const = 0;

	virtual void clear_classes() { javascript_classes.clear(); }

	virtual void initialize() = 0;
	virtual void uninitialize() = 0;
	virtual void language_finalize() = 0;
	virtual void frame() = 0;

	virtual JavaScriptGCHandler *alloc_object_binding_data(Object *p_object) = 0;
	virtual void free_object_binding_data(JavaScriptGCHandler *p_gc_handle) = 0;
	virtual void godot_refcount_incremented(JavaScriptGCHandler *p_gc_handle) = 0;
	virtual bool godot_refcount_decremented(JavaScriptGCHandler *p_gc_handle) = 0;

	virtual Error eval_string(const String &p_source, EvalType type, const String &p_path, JavaScriptGCHandler &r_ret) = 0;
	virtual Error safe_eval_text(const String &p_source, EvalType type, const String &p_path, String &r_error, JavaScriptGCHandler &r_ret) = 0;
	virtual String error_to_string(const JavaScriptError &p_error) = 0;
	virtual Error get_stacks(List<JavaScriptStackInfo> &r_stacks) = 0;
	virtual String get_backtrace_message(const List<JavaScriptStackInfo> &stacks) = 0;

	virtual Error compile_to_bytecode(const String &p_code, const String &p_file, Vector<uint8_t> &r_bytecode) = 0;
	virtual Error load_bytecode(const Vector<uint8_t> &p_bytecode, const String &p_file, JavaScriptGCHandler *r_module) = 0;
	virtual const JavaScriptClassInfo *parse_javascript_class(const String &p_code, const String &p_path, bool ignore_cacehe, JavaScriptError *r_error) = 0;
	virtual const JavaScriptClassInfo *parse_javascript_class(const Vector<uint8_t> &p_bytecode, const String &p_path, bool ignore_cacehe, JavaScriptError *r_error) = 0;

	virtual JavaScriptGCHandler create_js_instance_for_godot_object(const JavaScriptClassInfo *p_class, Object *p_object) = 0;
	virtual bool get_instance_property(const JavaScriptGCHandler &p_object, const StringName &p_name, Variant &r_ret) = 0;
	virtual bool set_instance_property(const JavaScriptGCHandler &p_object, const StringName &p_name, const Variant &p_value) = 0;
	virtual bool has_method(const JavaScriptGCHandler &p_object, const StringName &p_name) = 0;
	virtual bool has_signal(const JavaScriptClassInfo *p_class, const StringName &p_signal) = 0;
	virtual bool validate(const String &p_code, const String &p_path, JavaScriptError *r_error) = 0;

	virtual Variant call_method(const JavaScriptGCHandler &p_object, const StringName &p_method, const Variant **p_args, int p_argcount, Callable::CallError &r_error) = 0;
	virtual Variant call(const JavaScriptGCHandler &p_fuction, const JavaScriptGCHandler &p_target, const Variant **p_args, int p_argcount, Callable::CallError &r_error) = 0;

#ifdef TOOLS_ENABLED
	virtual const Dictionary &get_modified_api() const = 0;
#endif
};

#endif // JAVASCRIPT_BINDER_H
