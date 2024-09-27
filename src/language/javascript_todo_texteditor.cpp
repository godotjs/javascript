/* Instantiates debugger functions for godot editor */

#include "javascript_language.h"

#include <core/config/project_settings.h>
#include <core/io/json.h>

#ifdef TOOLS_ENABLED
#include <editor/editor_settings.h>
#endif

int JavaScriptLanguage::find_function(const String &p_function, const String &p_code) const { return -1; }
String JavaScriptLanguage::make_function(const String &p_class, const String &p_name, const PackedStringArray &p_args) const { return ""; }
void JavaScriptLanguage::auto_indent_code(String &p_code, int p_from_line, int p_to_line) const {}

bool JavaScriptLanguage::supports_documentation() const { return false; }
bool JavaScriptLanguage::can_inherit_from_file() const { return false; }
Error JavaScriptLanguage::complete_code(const String &p_code, const String &p_path, Object *p_owner, List<ScriptLanguage::CodeCompletionOption> *r_options, bool &r_force, String &r_call_hint) { return ERR_UNAVAILABLE; }
Error JavaScriptLanguage::lookup_code(const String &p_code, const String &p_symbol, const String &p_path, Object *p_owner, ScriptLanguage::LookupResult &r_result) { return ERR_UNAVAILABLE; }

#ifdef TOOLS_ENABLED
Error JavaScriptLanguage::open_in_external_editor(const Ref<Script> &p_script, int p_line, int p_col) {
	const Ref<JavaScript> s = p_script;
	const String origin_script_path = s->get_script_path();
	if (origin_script_path.ends_with(EXT_JSCLASS)) {
		const String path = EditorSettings::get_singleton()->get("text_editor/external/exec_path");
		String flags = EditorSettings::get_singleton()->get("text_editor/external/exec_flags");

		List<String> args;
		bool has_file_flag = false;

		String resolved_path = origin_script_path;
		const String source_code = s->get_source_code();
		if (source_code.begins_with(EXT_GENERATE)) {
			const Vector<String> found_path = source_code.split("\n", false, 0);
			if (found_path.size() > 0) {
				resolved_path = found_path[0].replacen(EXT_GENERATE, "");
			}
		}

		const String script_path = ProjectSettings::get_singleton()->globalize_path(resolved_path);
		if (flags.size()) {
			const String project_path = ProjectSettings::get_singleton()->get_resource_path();
			flags = flags.replacen("{line}", itos(p_line > 0 ? p_line : 0));
			flags = flags.replacen("{col}", itos(p_col));
			flags = flags.strip_edges().replace("\\\\", "\\");
			int from = 0;
			int num_chars = 0;
			bool inside_quotes = false;
			for (int i = 0; i < flags.size(); i++) {
				if (flags[i] == '"' && (!i || flags[i - 1] != '\\')) {
					if (!inside_quotes) {
						from++;
					}
					inside_quotes = !inside_quotes;
				} else if (flags[i] == '\0' || (!inside_quotes && flags[i] == ' ')) {
					String arg = flags.substr(from, num_chars);
					if (arg.find("{file}") != -1) {
						has_file_flag = true;
					}
					// do path replacement here, else there will be issues with spaces and quotes
					arg = arg.replacen("{project}", project_path);
					arg = arg.replacen("{file}", script_path);
					args.push_back(arg);
					from = i + 1;
					num_chars = 0;
				} else {
					num_chars++;
				}
			}
		}
		// Default to passing script path if no {file} flag is specified.
		if (!has_file_flag) {
			args.push_back(script_path);
		}
		const Error err = OS::get_singleton()->execute(path, args, false);
		if (err != OK) {
			WARN_PRINT("Couldn't open external text editor, using internal");
		}

		return err;
	}

	return OK;
}

bool JavaScriptLanguage::overrides_external_editor() {
	return EditorSettings::get_singleton()->get("text_editor/external/use_external_editor");
}
#else
Error JavaScriptLanguage::open_in_external_editor(const Ref<Script> &p_script, int p_line, int p_col) { return ERR_UNAVAILABLE; }
bool JavaScriptLanguage::overrides_external_editor() { return false; }
#endif