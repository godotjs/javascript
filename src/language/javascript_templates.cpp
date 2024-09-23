/* Instantiates template functions for godot editor */

#include "javascript_language.h"
#include "templates/javascript_template_manager.h"

Ref<Script> JavaScriptLanguage::make_template(const String &p_template, const String &p_class_name, const String &p_base_class_name) const {
	Ref<JavaScript> javaScript;
	javaScript.instantiate();
	const String src = p_template.replace("BASE", p_base_class_name).replace("CLASS", p_class_name);
	javaScript->set_source_code(src);
	return javaScript;
}

Vector<ScriptLanguage::ScriptTemplate> JavaScriptLanguage::get_built_in_templates(const StringName p_object) {
	Vector<ScriptTemplate> templates;
#ifdef TOOLS_ENABLED
		const Vector<ScriptTemplate> all_templates = JavaScriptLanguageManager::get_templates();

	for (int i = 0; i < all_templates.size(); i++) {
		if (all_templates[i].inherit == p_object) {
			templates.append(all_templates[i]);
		}
	}
#endif

	return templates;
}

bool JavaScriptLanguage::is_using_templates() { return true; }
