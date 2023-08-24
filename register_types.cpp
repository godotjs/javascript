/**************************************************************************/
/*  register_types.cpp                                                    */
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

#include "register_types.h"

#include "javascript.h"
#include "javascript_language.h"

#ifdef TOOLS_ENABLED
#include "core/io/file_access_encrypted.h"
#include "editor/editor_node.h"
#include "editor/export/editor_export.h"
#include "tools/editor_tools.h"
void editor_init_callback();

class EditorExportJavaScript : public EditorExportPlugin {
	GDCLASS(EditorExportJavaScript, EditorExportPlugin);

public:
	virtual void _export_file(const String &p_path, const String &p_type, const HashSet<String> &p_features) override {
		String script_key;
		const Ref<EditorExportPreset> &preset = get_export_preset();
		String extension = p_path.get_extension();
		if (extension != EXT_JSCLASS && extension != EXT_JSMODULE) {
			return;
		}
	}
	virtual String get_name() const override { return "JavaScript"; }
};

#endif

Ref<ResourceFormatLoaderJavaScript> resource_loader_javascript;
Ref<ResourceFormatSaverJavaScript> resource_saver_javascript;
Ref<ResourceFormatLoaderJavaScriptModule> resource_loader_javascript_module;
Ref<ResourceFormatSaverJavaScriptModule> resource_saver_javascript_module;
static JavaScriptLanguage *script_language_js = nullptr;

void initialize_javascript_module(ModuleInitializationLevel p_level) {
	if (p_level != ModuleInitializationLevel::MODULE_INITIALIZATION_LEVEL_CORE)
		return;

	ClassDB::register_class<JavaScript>();
	ClassDB::register_class<JavaScriptModule>();

	resource_loader_javascript.instantiate();
	resource_saver_javascript.instantiate();
	ResourceLoader::add_resource_format_loader(resource_loader_javascript, true);
	ResourceSaver::add_resource_format_saver(resource_saver_javascript, true);

	resource_loader_javascript_module.instantiate();
	resource_saver_javascript_module.instantiate();
	ResourceLoader::add_resource_format_loader(resource_loader_javascript_module, true);
	ResourceSaver::add_resource_format_saver(resource_saver_javascript_module, true);

	script_language_js = memnew(JavaScriptLanguage);
	ScriptServer::register_language(script_language_js);

#ifdef TOOLS_ENABLED
	EditorNode::add_init_callback(editor_init_callback);
#endif
}

void uninitialize_javascript_module(ModuleInitializationLevel p_level) {
	if (p_level != ModuleInitializationLevel::MODULE_INITIALIZATION_LEVEL_CORE)
		return;

	ScriptServer::unregister_language(script_language_js);
	memdelete(script_language_js);

	ResourceLoader::remove_resource_format_loader(resource_loader_javascript);
	ResourceSaver::remove_resource_format_saver(resource_saver_javascript);
	resource_loader_javascript.unref();
	resource_saver_javascript.unref();

	ResourceLoader::remove_resource_format_loader(resource_loader_javascript_module);
	ResourceSaver::remove_resource_format_saver(resource_saver_javascript_module);
	resource_loader_javascript_module.unref();
	resource_saver_javascript_module.unref();
}

#ifdef TOOLS_ENABLED
void editor_init_callback() {
	JavaScriptPlugin *plugin = memnew(JavaScriptPlugin(EditorNode::get_singleton()));
	EditorNode::get_singleton()->add_editor_plugin(plugin);

	Ref<EditorExportJavaScript> js_export;
	js_export.instantiate();
	EditorExport::get_singleton()->add_export_plugin(js_export);
}
#endif
