/* Instantiates debugger functions for godot editor */

#include "javascript_language.h"

int JavaScriptLanguage::find_function(const String &p_function, const String &p_code) const { return -1; }
String JavaScriptLanguage::make_function(const String &p_class, const String &p_name, const PackedStringArray &p_args) const { return ""; }
void JavaScriptLanguage::auto_indent_code(String &p_code, int p_from_line, int p_to_line) const {}

bool JavaScriptLanguage::supports_documentation() const { return false; }
bool JavaScriptLanguage::can_inherit_from_file() const { return false; }
Error JavaScriptLanguage::open_in_external_editor(const Ref<Script> &p_script, int p_line, int p_col) { return ERR_UNAVAILABLE; }
bool JavaScriptLanguage::overrides_external_editor() { return false; }
Error JavaScriptLanguage::complete_code(const String &p_code, const String &p_path, Object *p_owner, List<ScriptLanguage::CodeCompletionOption> *r_options, bool &r_force, String &r_call_hint) { return ERR_UNAVAILABLE; }
Error JavaScriptLanguage::lookup_code(const String &p_code, const String &p_symbol, const String &p_path, Object *p_owner, ScriptLanguage::LookupResult &r_result) { return ERR_UNAVAILABLE; }
