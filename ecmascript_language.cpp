#include "ecmascript_language.h"
#include "core/class_db.h"
#include "core/os/file_access.h"

ECMAScriptLanguage *ECMAScriptLanguage::singleton = NULL;

void ECMAScriptLanguage::init() {
	ERR_FAIL_NULL(binding);

	binding->initialize();
}

void ECMAScriptLanguage::finish() {
	binding->uninitialize();
}

Error ECMAScriptLanguage::execute_file(const String &p_path) {
	ERR_FAIL_NULL_V(binding, ERR_BUG);
	Error err;
	String code = FileAccess::get_file_as_string(p_path, &err);
	if (err == OK) {
		err = binding->eval_string(code, p_path);
	}
	return err;
}

void ECMAScriptLanguage::get_reserved_words(List<String> *p_words) const {

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
		"Uint8ClampedArray"
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

void ECMAScriptLanguage::get_comment_delimiters(List<String> *p_delimiters) const {
	p_delimiters->push_back("//"); // single-line comment
	p_delimiters->push_back("/* */"); // delimited comment
}

void ECMAScriptLanguage::get_string_delimiters(List<String> *p_delimiters) const {
	p_delimiters->push_back("' '");
	p_delimiters->push_back("\" \"");
	p_delimiters->push_back("` `");
}

void ECMAScriptLanguage::get_recognized_extensions(List<String> *p_extensions) const {
	p_extensions->push_back("js");
}

void *ECMAScriptLanguage::alloc_instance_binding_data(Object *p_object) {
	return binding->alloc_object_binding_data(p_object);
}

void ECMAScriptLanguage::free_instance_binding_data(void *p_data) {
	binding->free_object_binding_data(p_data);
}

void ECMAScriptLanguage::refcount_incremented_instance_binding(Object *p_object) {
	binding->godot_refcount_incremented(static_cast<Reference *>(p_object));
}

bool ECMAScriptLanguage::refcount_decremented_instance_binding(Object *p_object) {
	return binding->godot_refcount_decremented(static_cast<Reference *>(p_object));
}

ECMAScriptLanguage::ECMAScriptLanguage() {

	ERR_FAIL_COND(singleton);
	singleton = this;
	binding = memnew(QuickJSBinder);
}

ECMAScriptLanguage::~ECMAScriptLanguage() {
	memdelete(binding);
}
