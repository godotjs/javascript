#ifndef ECMASCRIPT_H
#define ECMASCRIPT_H

#include <core/script_language.h>
#include "ecmascript_binding_helper.h"

class ECMAScriptInstance;

class ECMAScript : public Script {
	GDCLASS(ECMAScript, Script);

	ECMAScriptGCHandler ecma_constructor;

	bool tool;
	bool valid;
	bool builtin;
	Set<Object *> instances;
	String source;
	StringName name;

	/* TODO */ ECMAScriptInstance *_create_instance(const Variant **p_args, int p_argcount, Object *p_owner, bool p_isref, Variant::CallError &r_error) { return NULL; }
	/* TODO */ Variant _new(const Variant **p_args, int p_argcount, Variant::CallError &r_error) { return Variant(); }

protected:
	/* TODO */ virtual bool editor_can_reload_from_file() { return false; } // this is handled by editor better
	/* TODO */ void _notification(int p_what) {}
	/* TODO */ static void _bind_methods() {}

	friend class PlaceHolderScriptInstance;
	/* TODO */ virtual void _placeholder_erased(PlaceHolderScriptInstance *p_placeholder) {}

public:
	virtual bool can_instance() const;

	/* TODO */ virtual Ref<Script> get_base_script() const { return NULL; } //for script inheritance

	/* TODO */ virtual StringName get_instance_base_type() const { return StringName(); } // this may not work in all scripts, will return empty if so
	/* TODO */ virtual ScriptInstance *instance_create(Object *p_this) { return NULL; }
	/* TODO */ virtual PlaceHolderScriptInstance *placeholder_instance_create(Object *p_this) { return NULL; }
	/* TODO */ virtual bool instance_has(const Object *p_this) const { return false; }

	/* TODO */ virtual bool has_source_code() const { return false; }
	/* TODO */ virtual String get_source_code() const { return ""; }
	/* TODO */ virtual void set_source_code(const String &p_code) {}
	/* TODO */ virtual Error reload(bool p_keep_state = false) { return ERR_UNAVAILABLE; }

	/* TODO */ virtual bool has_method(const StringName &p_method) const { return false; }
	/* TODO */ virtual MethodInfo get_method_info(const StringName &p_method) const { return MethodInfo(); }

	/* TODO */ virtual bool is_tool() const { return false; }
	/* TODO */ virtual bool is_valid() const { return false; }

	virtual ScriptLanguage *get_language() const;

	/* TODO */ virtual bool has_script_signal(const StringName &p_signal) const { return false; }
	/* TODO */ virtual void get_script_signal_list(List<MethodInfo> *r_signals) const {};

	/* TODO */ virtual bool get_property_default_value(const StringName &p_property, Variant &r_value) const { return false; }

	/* TODO */ virtual void update_exports() {} //editor tool
	/* TODO */ virtual void get_script_method_list(List<MethodInfo> *p_list) const {};
	/* TODO */ virtual void get_script_property_list(List<PropertyInfo> *p_list) const {};

	/* TODO */ virtual int get_member_line(const StringName &p_member) const { return -1; }

	/* TODO */ virtual void get_constants(Map<StringName, Variant> *p_constants) {}
	/* TODO */ virtual void get_members(Set<StringName> *p_constants) {}

	/* TODO */ virtual bool is_placeholder_fallback_enabled() const { return false; }

	ECMAScript();
	~ECMAScript();
};

#endif // ECMASCRIPT_H
