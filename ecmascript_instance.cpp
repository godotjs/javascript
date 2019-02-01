#include "ecmascript_instance.h"
#include "ecmascript_language.h"

ScriptLanguage *ECMAScriptInstance::get_language() {
	return ECMAScriptLanguage::get_singleton();
}

Ref<Script> ECMAScriptInstance::get_script() const {
	return script;
}

ECMAScriptInstance::ECMAScriptInstance() {
	ecma_object.ecma_object = NULL;
}

ECMAScriptInstance::~ECMAScriptInstance() {
}
