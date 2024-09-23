/* Instantiates debugger functions for godot editor */

#include "javascript_language.h"

int JavaScriptLanguage::find_function(const String &p_function, const String &p_code) const  { return -1; }
String JavaScriptLanguage::make_function(const String &p_class, const String &p_name, const PackedStringArray &p_args) const { return ""; }
void JavaScriptLanguage::auto_indent_code(String &p_code, int p_from_line, int p_to_line) const  {}
