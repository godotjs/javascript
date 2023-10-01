#ifndef ECMASCRIPT_INSTANCE_H
#define ECMASCRIPT_INSTANCE_H

#include "ecmascript.h"
#include "ecmascript_binder.h"
#include <core/script_language.h>

class ECMAScriptInstance : public ScriptInstance {

	friend class ECMAScript;
	friend class QuickJSBinder;

	Object *owner;
	Ref<ECMAScript> script;
	ECMAScriptGCHandler ecma_object;
	ECMAScriptBinder *binder;
	const ECMAClassInfo *ecma_class;

public:
	virtual bool set(const StringName &p_name, const Variant &p_value);
	virtual bool get(const StringName &p_name, Variant &r_ret) const;
	virtual void get_property_list(List<PropertyInfo> *p_properties) const;
	virtual Variant::Type get_property_type(const StringName &p_name, bool *r_is_valid = NULL) const;

	virtual Object *get_owner() { return owner; }
	virtual Ref<Script> get_script() const;

	virtual void get_method_list(List<MethodInfo> *p_list) const;
	virtual bool has_method(const StringName &p_method) const;

	virtual Variant call(const StringName &p_method, const Variant **p_args, int p_argcount, Variant::CallError &r_error);

	/* TODO */ virtual void notification(int p_notification) {}

	//this is used by script languages that keep a reference counter of their own
	//you can make make Ref<> not die when it reaches zero, so deleting the reference
	//depends entirely from the script

	/* TODO */ virtual void refcount_incremented() {}
	/* TODO */ virtual bool refcount_decremented() { return true; } //return true if it can die

	/* TODO */ virtual bool is_placeholder() const { return false; }

	/* TODO */ virtual void property_set_fallback(const StringName &p_name, const Variant &p_value, bool *r_valid) {}
	/* TODO */ virtual Variant property_get_fallback(const StringName &p_name, bool *r_valid) { return Variant(); }

	/* TODO */ virtual MultiplayerAPI::RPCMode get_rpc_mode(const StringName &p_method) const { return MultiplayerAPI::RPC_MODE_DISABLED; }
	/* TODO */ virtual MultiplayerAPI::RPCMode get_rset_mode(const StringName &p_variable) const { return MultiplayerAPI::RPC_MODE_DISABLED; }

	virtual ScriptLanguage *get_language();

	ECMAScriptInstance();
	~ECMAScriptInstance();
};

#endif // ECMASCRIPT_INSTANCE_H
