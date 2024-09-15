
#include "register_types.h"

#include "javascript.h"
#include "javascript_language.h"

#ifdef TOOLS_ENABLED
#include "core/io/file_access_encrypted.h"
#include "editor/editor_node.h"
#include "editor/editor_tools.h"
#include "editor/export/editor_export.h"
void editor_init_callback();

class EditorExportJavaScript : public EditorExportPlugin {
	GDCLASS(EditorExportJavaScript, EditorExportPlugin);

public:
	virtual void _export_file(const String &p_path, const String &p_type, const HashSet<String> &p_features) override {
		String extension = p_path.get_extension();
		if (extension != EXT_JSCLASS && extension != EXT_JSMODULE) {
			return;
		}
	}
	virtual String _get_name() const override { return "JavaScript"; }
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
