#ifndef ECMA_CLASS_BROWSER_H
#define ECMA_CLASS_BROWSER_H
#include "editor/editor_node.h"

class ECMAScriptLibrary;

class ECMAClassBrower : public VBoxContainer {
	GDCLASS(ECMAClassBrower, VBoxContainer);
	Tree *class_tree;
	LineEdit *filter_input;

protected:
	DirAccessRef res_dir;
	static void _bind_methods();
	void _on_filter_changed(const String &p_text);
	Variant get_drag_data_fw(const Point2 &p_point, Control *p_from);

public:
	void update_tree();

	ECMAClassBrower();
};

class EditorInspectorPluginECMALib : public EditorInspectorPlugin {
	GDCLASS(EditorInspectorPluginECMALib, EditorInspectorPlugin);

protected:
	static void _bind_methods();
	ECMAScriptLibrary *editing_lib;
	void on_reload_editing_lib();
public:
	virtual bool can_handle(Object *p_object);
	virtual void parse_begin(Object *p_object);
	EditorInspectorPluginECMALib();
};

class ECMAScriptPlugin : public EditorPlugin {

	GDCLASS(ECMAScriptPlugin, EditorPlugin);

	ToolButton *bottom_button;
	ECMAClassBrower *ecma_class_browser;
	Ref<EditorInspectorPluginECMALib> eslib_inspector_plugin;

protected:
	static void _bind_methods();
	void _on_bottom_panel_toggled(bool pressed);

public:
	virtual String get_name() const { return "ECMAClassBrowser"; }

	ECMAScriptPlugin(EditorNode *p_node);
};

#endif // ECMA_CLASS_BROWSER_H
