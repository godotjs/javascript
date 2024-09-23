#ifndef JAVASCRIPT_TEMPLATE_MANAGER_H
#define JAVASCRIPT_TEMPLATE_MANAGER_H

#include "../javascript_language.h"

class JavaScriptLanguageManager {

public:
	static ScriptLanguage::ScriptTemplate DEFAULT_TEMPLATE;
	static ScriptLanguage::ScriptTemplate EMPTY_TEMPLATE;

	static Vector<ScriptLanguage::ScriptTemplate> get_templates();
};

#endif //JAVASCRIPT_TEMPLATE_MANAGER_H
