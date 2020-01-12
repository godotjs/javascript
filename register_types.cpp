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

		if (!p_path.ends_with(".js") || script_mode == EditorExportPreset::MODE_SCRIPT_TEXT)
			return;

		if (script_mode == EditorExportPreset::MODE_SCRIPT_ENCRYPTED) {
			Vector<uint8_t> file = FileAccess::get_file_as_array(p_path);
			if (file.empty())
				return;

			String script_key = preset->get_script_encryption_key().to_lower();
			String tmp_path = EditorSettings::get_singleton()->get_cache_dir().plus_file("script.jse");
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
			add_file(p_path.get_basename() + ".jse", file, true);

			// Clean up temporary file.
			DirAccess::remove_file_or_error(tmp_path);

		} else {

			Error err;
			String code = FileAccess::get_file_as_string(p_path, &err);
			ERR_FAIL_COND(err != OK);

			Vector<uint8_t> file;
			ERR_FAIL_COND(ECMAScriptLanguage::get_singleton()->get_binder()->compile_to_bytecode(code, file) != OK);
			add_file(p_path.get_basename() + ".jsc", file, true);
		}
	}
};

#endif

Ref<ResourceFormatLoaderECMAScript> resource_loader_ecmascript;
Ref<ResourceFormatSaverECMAScript> resource_saver_ecmascript;

static ECMAScriptLanguage *script_language_es = NULL;

void register_ECMAScript_types() {

	ClassDB::register_class<ECMAScript>();

	resource_loader_ecmascript.instance();
	resource_saver_ecmascript.instance();
	ResourceLoader::add_resource_format_loader(resource_loader_ecmascript, true);
	ResourceSaver::add_resource_format_saver(resource_saver_ecmascript, true);

	script_language_es = memnew(ECMAScriptLanguage);
	script_language_es->set_language_index(ScriptServer::get_language_count());
	ScriptServer::register_language(script_language_es);

#ifdef TOOLS_ENABLED
	EditorNode::add_init_callback(editor_init_callback);
#endif
}

void unregister_ECMAScript_types() {
	ScriptServer::unregister_language(script_language_es);
	memdelete(script_language_es);

	ResourceLoader::remove_resource_format_loader(resource_loader_ecmascript);
	ResourceSaver::remove_resource_format_saver(resource_saver_ecmascript);
	resource_loader_ecmascript.unref();
	resource_saver_ecmascript.unref();
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
