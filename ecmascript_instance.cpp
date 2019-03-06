#include "ecmascript_instance.h"
#include "ecmascript.h"
#include "ecmascript_language.h"

Ref<Script> ECMAScriptInstance::get_script() const {
	return script;
}

void ECMAScriptInstance::get_method_list(List<MethodInfo> *p_list) const {
	ERR_FAIL_COND(script.is_null());
	script->get_script_method_list(p_list);
}

bool ECMAScriptInstance::has_method(const StringName &p_method) const {
	ERR_FAIL_COND_V(script.is_null(), false);
	return script->has_method(p_method);
}

bool ECMAScriptInstance::set(const StringName &p_name, const Variant &p_value) {
	if(!script.is_null()) {
		if (ECMAClassInfo * cls = script->get_ecma_class()) {
			if(ECMAProperyInfo * pi = cls->properties.getptr(p_name)) {
				return ECMAScriptLanguage::get_singleton()->binding->set_instance_property(this->ecma_object, p_name, p_value);
			}
		}
	}
	return false;
}

bool ECMAScriptInstance::get(const StringName &p_name, Variant &r_ret) const {
	if(!script.is_null()) {
		if (ECMAClassInfo * cls = script->get_ecma_class()) {
			if(ECMAProperyInfo * pi = cls->properties.getptr(p_name)) {
				return ECMAScriptLanguage::get_singleton()->binding->get_instance_property(this->ecma_object, p_name, r_ret);
			}
		}
	}
	return false;
}

void ECMAScriptInstance::get_property_list(List<PropertyInfo> *p_properties) const {
	ERR_FAIL_COND(script.is_null());
	return script->get_script_property_list(p_properties);
}

Variant::Type ECMAScriptInstance::get_property_type(const StringName &p_name, bool *r_is_valid) const {
	*r_is_valid = false;
	if(!script.is_null()) {
		if (ECMAClassInfo * cls = script->get_ecma_class()) {
			if(ECMAProperyInfo * pi = cls->properties.getptr(p_name)) {
				*r_is_valid = true;
				return pi->type;
			}
		}
	}
	return Variant::NIL;
}

Variant ECMAScriptInstance::call(const StringName &p_method, const Variant **p_args, int p_argcount, Variant::CallError &r_error) {

	ERR_FAIL_COND_V(script.is_null() || ecma_object.is_null(), Variant());

	ECMAClassInfo *cls = script->get_ecma_class();
	if (cls == NULL) {
		r_error.error = Variant::CallError::CALL_ERROR_INVALID_METHOD;
		return Variant();
	}

	ECMAMethodInfo *method = cls->methods.getptr(p_method);
	if (method == NULL) {
		r_error.error = Variant::CallError::CALL_ERROR_INVALID_METHOD;
		return Variant();
	}

	return ECMAScriptLanguage::get_singleton()->binding->call_method(ecma_object, *method, p_args, p_argcount, r_error);
}

ScriptLanguage *ECMAScriptInstance::get_language() {
	return ECMAScriptLanguage::get_singleton();
}

ECMAScriptInstance::ECMAScriptInstance() {
	owner = NULL;
	ecma_object.ecma_object = NULL;
}

ECMAScriptInstance::~ECMAScriptInstance() {
}
