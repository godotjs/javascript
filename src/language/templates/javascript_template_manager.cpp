#include "javascript_template_manager.h"

Vector<ScriptLanguage::ScriptTemplate> JavaScriptLanguageManager::get_templates() {
	Vector<ScriptLanguage::ScriptTemplate> templates;
	templates.append(JavaScriptLanguageManager::DEFAULT_TEMPLATE);
	templates.append(JavaScriptLanguageManager::EMPTY_TEMPLATE);
	return templates;
};