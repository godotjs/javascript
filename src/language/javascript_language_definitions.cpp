/* Instantiates definition functions for javascript language */

#include "javascript_language.h"

String JavaScriptLanguage::get_name() const { return EXT_NAME; }
String JavaScriptLanguage::get_type() const { return EXT_NAME; }
String JavaScriptLanguage::get_extension() const { return EXT_JSCLASS; }
bool JavaScriptLanguage::has_named_classes() const { return true; }
bool JavaScriptLanguage::supports_builtin_mode() const { return false; }

void JavaScriptLanguage::get_recognized_extensions(List<String> *p_extensions) const {
	p_extensions->push_back(EXT_JSMODULE);
	p_extensions->push_back(EXT_JSCLASS);
	p_extensions->push_back(EXT_JSON);
}

bool JavaScriptLanguage::is_control_flow_keyword(String p_keyword) const {
	return p_keyword == "if" ||
			p_keyword == "else" ||
			p_keyword == "return" ||
			p_keyword == "do" ||
			p_keyword == "while" ||
			p_keyword == "for" ||
			p_keyword == "break" ||
			p_keyword == "continue" ||
			p_keyword == "switch" ||
			p_keyword == "case" ||
			p_keyword == "throw" ||
			p_keyword == "try" ||
			p_keyword == "catch" ||
			p_keyword == "finally";
}

void JavaScriptLanguage::get_comment_delimiters(List<String> *p_delimiters) const {
	p_delimiters->push_back("//"); // single-line comment
	p_delimiters->push_back("/* */"); // delimited comment
}

void JavaScriptLanguage::get_string_delimiters(List<String> *p_delimiters) const {
	p_delimiters->push_back("' '");
	p_delimiters->push_back("\" \"");
	p_delimiters->push_back("` `");
}

void JavaScriptLanguage::get_reserved_words(List<String> *p_words) const {
	static const char *_reserved_words[] = {
		"null",
		"false",
		"true",
		"if",
		"else",
		"return",
		"var",
		"this",
		"delete",
		"void",
		"typeof",
		"new",
		"in",
		"instanceof",
		"do",
		"while",
		"for",
		"break",
		"continue",
		"switch",
		"case",
		"default",
		"throw",
		"try",
		"catch",
		"finally",
		"function",
		"debugger",
		"with",
		"class",
		"const",
		"enum",
		"export",
		"default",
		"extends",
		"import",
		"super",
		"implements",
		"interface",
		"let",
		"package",
		"private",
		"protected",
		"public",
		"static",
		"yield",
		"await",
		"prototype",
		"constructor",
		"get",
		"set",
		"of",
		"__proto__",
		"undefined",
		"number",
		"boolean",
		"string",
		"object",
		"symbol",
		"arguments",
		"join",
		"global",
		"as",
		"from",
		"default",
		"*",
		"then",
		"resolve",
		"reject",
		"promise",
		"proxy",
		"revoke",
		"async",
		"globalThis",
		"Object",
		"Array",
		"Error",
		"Number",
		"String",
		"Boolean",
		"Symbol",
		"Arguments",
		"Math",
		"JSON",
		"Date",
		"Function",
		"GeneratorFunction",
		"ForInIterator",
		"RegExp",
		"ArrayBuffer",
		"SharedArrayBuffer",
		"Uint8ClampedArray",
		"Int8Array",
		"Uint8Array",
		"Int16Array",
		"Uint16Array",
		"Int32Array",
		"Uint32Array",
		"BigInt64Array",
		"BigUint64Array",
		"Float32Array",
		"Float64Array",
		"DataView",
		"Map",
		"Set",
		"WeakMap",
		"WeakSet",
		"Generator",
		"Proxy",
		"Promise",
		0
	};

	const char **w = _reserved_words;

	while (*w) {
		p_words->push_back(*w);
		w++;
	}
}
