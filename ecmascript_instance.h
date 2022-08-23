#ifndef ECMASCRIPT_INSTANCE_H
#define ECMASCRIPT_INSTANCE_H

#include "ecmascript.h"
#include "ecmascript_binder.h"
#include "core/object/script_language.h"
#include "core/variant/callable.h"

class ECMAScriptInstance : public ScriptInstance {

	friend class ECMAScript;
	friend class QuickJSBinder;

	Object *owner;
	Ref<ECMAScript> script;
	ECMAScriptGCHandler ecma_object;
	ECMAScriptBinder *binder;
	const ECMAClassInfo *ecma_class;

public:
	virtual bool set(const StringName &p_name, const Variant &p_value) override;
	virtual bool get(const StringName &p_name, Variant &r_ret) const override;
	virtual void get_property_list(List<PropertyInfo> *p_properties) const override;
	virtual Variant::Type get_property_type(const StringName &p_name, bool *r_is_valid = nullptr) const override;

	virtual void get_method_list(List<MethodInfo> *p_list) const override;
	virtual bool has_method(const StringName &p_method) const override;

	virtual Variant callp(const StringName &p_method, const Variant **p_args, int p_argcount, Callable::CallError &r_error) override;

	virtual Object *get_owner() override { return owner; }
	virtual Ref<Script> get_script() const override;
	virtual ScriptLanguage *get_language() override;

	virtual void notification(int p_notification) override{};

	ECMAScriptInstance();
	~ECMAScriptInstance();
};

#endif // ECMASCRIPT_INSTANCE_H
