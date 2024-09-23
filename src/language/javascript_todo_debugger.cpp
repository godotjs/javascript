/* Instantiates debugger functions for godot editor */

#include "javascript_language.h"

String JavaScriptLanguage::debug_get_error() const { return ""; }
int JavaScriptLanguage::debug_get_stack_level_count() const { return 1; }
int JavaScriptLanguage::debug_get_stack_level_line(int p_level) const { return 1; }
String JavaScriptLanguage::debug_get_stack_level_function(int p_level) const { return ""; }
String JavaScriptLanguage::debug_get_stack_level_source(int p_level) const { return ""; }
void JavaScriptLanguage::debug_get_stack_level_locals(int p_level, List<String> *p_locals, List<Variant> *p_values, int p_max_subitems, int p_max_depth) {}
void JavaScriptLanguage::debug_get_stack_level_members(int p_level, List<String> *p_members, List<Variant> *p_values, int p_max_subitems, int p_max_depth) {}
void JavaScriptLanguage::debug_get_globals(List<String> *p_locals, List<Variant> *p_values, int p_max_subitems, int p_max_depth) {}
String JavaScriptLanguage::debug_parse_stack_level_expression(int p_level, const String &p_expression, int p_max_subitems, int p_max_depth) { return ""; }
Vector<ScriptLanguage::StackInfo> JavaScriptLanguage::debug_get_current_stack_info() { return Vector<StackInfo>(); }
