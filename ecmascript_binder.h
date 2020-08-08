#ifndef ECMASCRIPT_BINDING_HELPER_H
#define ECMASCRIPT_BINDING_HELPER_H

#include "core/os/thread.h"
#include "ecmascript_gc_handler.h"

typedef ECMAScriptGCHandler ECMAMethodInfo;

struct ECMAProperyInfo : public PropertyInfo {
	Variant default_value;
};

struct ECMAscriptScriptError {
	int line;
	int column;
	String message;
	String file;
	Vector<String> stack;
};

struct BasicECMAClassInfo {
	bool tool;
	StringName class_name;
	String icon_path;
	const ClassDB::ClassInfo *native_class;
	HashMap<StringName, MethodInfo> signals;
	HashMap<StringName, MethodInfo> methods;
	HashMap<StringName, ECMAProperyInfo> properties;
};

struct ECMAClassInfo : public BasicECMAClassInfo {
	ECMAScriptGCHandler constructor;
	ECMAScriptGCHandler prototype;
};

struct GlobalNumberConstant {
	StringName name;
	double_t value;
};

struct ECMAScriptStackInfo {
	int line;
	String file;
	String function;
};

class ECMAScriptBinder {
protected:
	// Path ==> ECMA Class
	HashMap<String, ECMAClassInfo> ecma_classes;
	HashMap<int64_t, ECMAScriptGCHandler> frame_callbacks;
	static String BINDING_SCRIPT_CONTENT;

public:
	enum EvalType {
		EVAL_TYPE_MODULE,
		EVAL_TYPE_GLOBAL,
	};

	ECMAScriptBinder() {}
	virtual ~ECMAScriptBinder(){};

	virtual ECMAScriptBinder *get_context_binder(void *p_context) = 0;
	virtual Thread::ID get_thread_id() const = 0;

	virtual void clear_classes() { ecma_classes.clear(); }

	virtual void initialize() = 0;
	virtual void uninitialize() = 0;
	virtual void language_finalize() = 0;
	virtual void frame() = 0;

	virtual void *alloc_object_binding_data(Object *p_object) = 0;
	virtual void free_object_binding_data(void *p_gc_handle) = 0;

	virtual void godot_refcount_incremented(Reference *p_object) = 0;
	virtual bool godot_refcount_decremented(Reference *p_object) = 0;

	virtual Error eval_string(const String &p_source, EvalType type, const String &p_path, ECMAScriptGCHandler &r_ret) = 0;
	virtual Error safe_eval_text(const String &p_source, EvalType type, const String &p_path, String &r_error, ECMAScriptGCHandler &r_ret) = 0;
	virtual String error_to_string(const ECMAscriptScriptError &p_error) = 0;
	virtual Error get_stacks(List<ECMAScriptStackInfo> &r_stacks) = 0;
	virtual String get_backtrace_message(const List<ECMAScriptStackInfo> &stacks) = 0;

	virtual Error compile_to_bytecode(const String &p_code, const String &p_file, Vector<uint8_t> &r_bytecode) = 0;
	virtual Error load_bytecode(const Vector<uint8_t> &p_bytecode, const String &p_file, ECMAScriptGCHandler *r_module) = 0;
	virtual const ECMAClassInfo *parse_ecma_class(const String &p_code, const String &p_path, bool ignore_cacehe, ECMAscriptScriptError *r_error) = 0;
	virtual const ECMAClassInfo *parse_ecma_class(const Vector<uint8_t> &p_bytecode, const String &p_path, bool ignore_cacehe, ECMAscriptScriptError *r_error) = 0;

	virtual ECMAScriptGCHandler create_ecma_instance_for_godot_object(const ECMAClassInfo *p_class, Object *p_object) = 0;
	virtual Variant call_method(const ECMAScriptGCHandler &p_object, const StringName &p_method, const Variant **p_args, int p_argcount, Variant::CallError &r_error) = 0;
	virtual bool get_instance_property(const ECMAScriptGCHandler &p_object, const StringName &p_name, Variant &r_ret) = 0;
	virtual bool set_instance_property(const ECMAScriptGCHandler &p_object, const StringName &p_name, const Variant &p_value) = 0;
	virtual bool has_method(const ECMAScriptGCHandler &p_object, const StringName &p_name) = 0;
	virtual bool has_signal(const ECMAClassInfo *p_class, const StringName &p_signal) = 0;
	virtual bool validate(const String &p_code, const String &p_path, ECMAscriptScriptError *r_error) = 0;
#ifdef TOOLS_ENABLED
	virtual const Dictionary &get_modified_api() const = 0;
#endif
};

#endif
