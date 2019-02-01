#include "ecmascript.h"
#include "ecmascript_language.h"

bool ECMAScript::can_instance() const {
	return is_valid();
}

ScriptLanguage *ECMAScript::get_language() const {
	return ECMAScriptLanguage::get_singleton();
}

ECMAScript::ECMAScript() {
	ecma_constructor.ecma_object = NULL;
}

ECMAScript::~ECMAScript() {
}
