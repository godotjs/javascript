#include "ecmascript_instance.h"
#include "ecmascript.h"
#include "ecmascript_language.h"

Ref<Script> ECMAScriptInstance::get_script() const {
	return script;
}

void ECMAScriptInstance::get_method_list(List<MethodInfo> *p_list) const {
	if (!ecma_class) return;
	for (const KeyValue<StringName, MethodInfo> &i: ecma_class->methods) {
		p_list->push_back(i.value);
	}
}

bool ECMAScriptInstance::has_method(const StringName &p_method) const {
	if (!binder || !ecma_object.ecma_object) return false;
	return binder->has_method(ecma_object, p_method);
}

bool ECMAScriptInstance::set(const StringName &p_name, const Variant &p_value) {
	if (!binder || !ecma_object.ecma_object) return false;
	return binder->set_instance_property(ecma_object, p_name, p_value);
}

bool ECMAScriptInstance::get(const StringName &p_name, Variant &r_ret) const {
	if (!binder || !ecma_object.ecma_object) return false;
	return binder->get_instance_property(this->ecma_object, p_name, r_ret);
}

void ECMAScriptInstance::get_property_list(List<PropertyInfo> *p_properties) const {
	if (!ecma_class) return;
	for (const KeyValue<StringName, ECMAProperyInfo> &i: ecma_class->properties) {
		p_properties->push_back(i.value);
	}
}

Variant::Type ECMAScriptInstance::get_property_type(const StringName &p_name, bool *r_is_valid) const {
	*r_is_valid = false;
	if (ecma_class) {
		if (const ECMAProperyInfo *pi = ecma_class->properties.getptr(p_name)) {
			*r_is_valid = true;
			return pi->type;
		}
	}
	return Variant::NIL;
}

Variant ECMAScriptInstance::call(const StringName &p_method, const Variant **p_args, int p_argcount, Callable::CallError &r_error) {
	if (binder == NULL || ecma_object.ecma_object == NULL) {
		r_error.error = Callable::CallError::CALL_ERROR_INSTANCE_IS_NULL;
		ERR_FAIL_V(Variant());
	}
	return binder->call_method(ecma_object, p_method, p_args, p_argcount, r_error);
}

ScriptLanguage *ECMAScriptInstance::get_language() {
	return ECMAScriptLanguage::get_singleton();
}

ECMAScriptInstance::ECMAScriptInstance() {
	owner = NULL;
	binder = NULL;
	ecma_class = NULL;
}

ECMAScriptInstance::~ECMAScriptInstance() {
	if (script.is_valid() && owner) {
		script->instances.erase(owner);
	}
}
