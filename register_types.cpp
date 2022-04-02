/*************************************************************************/
/*  register_types.cpp                                                   */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2019 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2019 Godot Engine contributors (cf. AUTHORS.md)    */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/

#include "register_types.h"
#include "ecmascript.h"
#include "ecmascript_language.h"

#ifdef TOOLS_ENABLED
#include "core/io/file_access_encrypted.h"
#include "editor/editor_export.h"
#include "editor/editor_node.h"
#include "tools/editor_tools.h"
void editor_init_callback();

class EditorExportECMAScript : public EditorExportPlugin {

	GDCLASS(EditorExportECMAScript, EditorExportPlugin);

public:
	virtual void _export_file(const String &p_path, const String &p_type, const Set<String> &p_features) {
		int script_mode = EditorExportPreset::MODE_SCRIPT_COMPILED;
		const Ref<EditorExportPreset> &preset = get_export_preset();

		if (preset.is_valid()) {
			script_mode = preset->get_script_export_mode();
		}

		if (script_mode == EditorExportPreset::MODE_SCRIPT_TEXT)
			return;

		String extension = p_path.get_extension();
		if (extension != EXT_JSCLASS && extension != EXT_JSMODULE)
			return;

		if (script_mode == EditorExportPreset::MODE_SCRIPT_ENCRYPTED) {
			Vector<uint8_t> file = FileAccess::get_file_as_array(p_path);
			if (file.empty())
				return;

			String script_key = preset->get_script_encryption_key().to_lower();
			String tmp_path = EditorSettings::get_singleton()->get_cache_dir().plus_file("script." + extension + "e");
			FileAccess *fa = FileAccess::open(tmp_path, FileAccess::WRITE);

			Vector<uint8_t> key;
			key.resize(32);
			for (int i = 0; i < 32; i++) {
				int v = 0;
				if (i * 2 < script_key.length()) {
					CharType ct = script_key[i * 2];
					if (ct >= '0' && ct <= '9')
						ct = ct - '0';
					else if (ct >= 'a' && ct <= 'f')
						ct = 10 + ct - 'a';
					v |= ct << 4;
				}

				if (i * 2 + 1 < script_key.length()) {
					CharType ct = script_key[i * 2 + 1];
					if (ct >= '0' && ct <= '9')
						ct = ct - '0';
					else if (ct >= 'a' && ct <= 'f')
						ct = 10 + ct - 'a';
					v |= ct;
				}
				key.write[i] = v;
			}
			FileAccessEncrypted *fae = memnew(FileAccessEncrypted);
			Error err = fae->open_and_parse(fa, key, FileAccessEncrypted::MODE_WRITE_AES256);

			if (err == OK) {
				fae->store_buffer(file.ptr(), file.size());
			}

			memdelete(fae);

			file = FileAccess::get_file_as_array(tmp_path);
			add_file(p_path.get_basename() + "." + extension + "e", file, true);

			// Clean up temporary file.
			DirAccess::remove_file_or_error(tmp_path);

		}
//		} else {
//#if 0 // Disable compile to bytecode as it is not battle tested on all platform
//			Error err;
//			String code = FileAccess::get_file_as_string(p_path, &err);
//			ERR_FAIL_COND(err != OK);
//
//			Vector<uint8_t> file;
//			ERR_FAIL_COND(ECMAScriptLanguage::get_singleton()->get_main_binder()->compile_to_bytecode(code, p_path, file) != OK);
//			add_file(p_path.get_basename() + "." + extension + "b", file, true);
//#endif
//		}
	}
};

#endif

Ref<ResourceFormatLoaderECMAScript> resource_loader_ecmascript;
Ref<ResourceFormatSaverECMAScript> resource_saver_ecmascript;
Ref<ResourceFormatLoaderECMAScriptModule> resource_loader_ecmascript_module;
Ref<ResourceFormatSaverECMAScriptModule> resource_saver_ecmascript_module;
static ECMAScriptLanguage *script_language_js = NULL;

void register_ECMAScript_types() {

	ClassDB::register_class<ECMAScript>();
	ClassDB::register_class<ECMAScriptModule>();

	resource_loader_ecmascript.instance();
	resource_saver_ecmascript.instance();
	ResourceLoader::add_resource_format_loader(resource_loader_ecmascript, true);
	ResourceSaver::add_resource_format_saver(resource_saver_ecmascript, true);

	resource_loader_ecmascript_module.instance();
	resource_saver_ecmascript_module.instance();
	ResourceLoader::add_resource_format_loader(resource_loader_ecmascript_module, true);
	ResourceSaver::add_resource_format_saver(resource_saver_ecmascript_module, true);

	script_language_js = memnew(ECMAScriptLanguage);
	script_language_js->set_language_index(ScriptServer::get_language_count());
	ScriptServer::register_language(script_language_js);

#ifdef TOOLS_ENABLED
	EditorNode::add_init_callback(editor_init_callback);
#endif
}

void unregister_ECMAScript_types() {
	ScriptServer::unregister_language(script_language_js);
	memdelete(script_language_js);

	ResourceLoader::remove_resource_format_loader(resource_loader_ecmascript);
	ResourceSaver::remove_resource_format_saver(resource_saver_ecmascript);
	resource_loader_ecmascript.unref();
	resource_saver_ecmascript.unref();

	ResourceLoader::remove_resource_format_loader(resource_loader_ecmascript_module);
	ResourceSaver::remove_resource_format_saver(resource_saver_ecmascript_module);
	resource_loader_ecmascript_module.unref();
	resource_saver_ecmascript_module.unref();
}

#ifdef TOOLS_ENABLED
void editor_init_callback() {
	ECMAScriptPlugin *plugin = memnew(ECMAScriptPlugin(EditorNode::get_singleton()));
	EditorNode::get_singleton()->add_editor_plugin(plugin);

	Ref<EditorExportECMAScript> js_export;
	js_export.instance();
	EditorExport::get_singleton()->add_export_plugin(js_export);
}
#endif
