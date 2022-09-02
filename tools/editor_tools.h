#ifndef JAVASCRIPT_PLUGIN_H
#define JAVASCRIPT_PLUGIN_H
#include "editor/editor_file_dialog.h"
#include "editor/editor_node.h"

class JavaScriptPlugin : public EditorPlugin {
	GDCLASS(JavaScriptPlugin, EditorPlugin);

	enum MenuItem {
		ITEM_GEN_DECLARE_FILE,
		ITEM_GEN_TYPESCRIPT_PROJECT,
		ITEM_GEN_ENUM_BINDING_SCRIPT,
	};

	EditorFileDialog *declaration_file_dialog;
	EditorFileDialog *enumberation_file_dialog;
	const Dictionary *modified_api;

protected:
	static String BUILTIN_DECLARATION_TEXT;
	static String TSCONFIG_CONTENT;
	static String TS_DECORATORS_CONTENT;
	static String PACKAGE_JSON_CONTENT;

	static void _bind_methods();

	void _notification(int p_what);
	void _on_menu_item_pressed(int item);
	void _export_typescript_declare_file(const String &p_path);
	void _export_enumeration_binding_file(const String &p_path);
	void _generate_typescript_project();

public:
	virtual String get_name() const override { return "JavaScriptPlugin"; }
	JavaScriptPlugin(EditorNode *p_node);
};

#endif
