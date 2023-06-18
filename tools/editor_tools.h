/**************************************************************************/
/*  editor_tools.h                                                        */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#ifndef EDITOR_TOOLS_H
#define EDITOR_TOOLS_H

#include "editor/editor_file_dialog.h"
#include "editor/editor_node.h"
#include "editor/editor_plugin.h"

class DocTools;
class EditorFileDialog;
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

#endif // EDITOR_TOOLS_H
