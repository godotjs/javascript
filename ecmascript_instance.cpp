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
	ERR_FAIL_COND_V( script.is_null(), false);
	return script->has_method(p_method);
}

Variant ECMAScriptInstance::call(const StringName &p_method, const Variant **p_args, int p_argcount, Variant::CallError &r_error) {

	ERR_FAIL_COND_V(script.is_null() || ecma_object.is_null(), NULL);

	ECMAClassInfo * cls = script->get_ecma_class();
	ERR_FAIL_NULL_V(cls, NULL);
	ECMAMethodInfo * method = cls->methods.getptr(p_method);
	ERR_FAIL_NULL_V(method, NULL);

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
