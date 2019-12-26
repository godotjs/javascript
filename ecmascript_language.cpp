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

Ref<Script> ECMAScriptLanguage::get_template(const String &p_class_name, const String &p_base_class_name) const {

	String script_template = "export default class %CLASS% extends " GODOT_OBJECT_NAME ".%BASE% {\n"
							 "    \n"
							 "    // Declare member variables here. Examples:\n"
							 "    a = 2;\n"
							 "    b = \"text\";\n"
							 "    \n"
							 "    constructor() {\n"
							 "        super();\n"
							 "    }\n"
							 "    \n"
							 "    // Called when the node enters the scene tree for the first time.\n"
							 "    _ready() {\n"
							 "        \n"
							 "    }\n"
							 "    \n"
							 "    // Called every frame. 'delta' is the elapsed time since the previous frame.\n"
							 "    _process(delta) {\n"
							 "        \n"
							 "    }\n"
							 "}\n";
	script_template = script_template.replace("%BASE%", p_base_class_name).replace("%CLASS%", p_class_name);

	Ref<ECMAScript> script;
	script.instance();
	script->set_source_code(script_template);
	script->set_name(p_class_name);
	script->set_script_path(p_class_name);
	return script;
}

void ECMAScriptLanguage::make_template(const String &p_class_name, const String &p_base_class_name, Ref<Script> &p_script) {
	String src = p_script->get_source_code();
	src = src.replace("%BASE%", p_base_class_name).replace("%CLASS%", p_class_name);
	p_script->set_source_code(src);
}

bool ECMAScriptLanguage::validate(const String &p_script, int &r_line_error, int &r_col_error, String &r_test_error, const String &p_path, List<String> *r_functions, List<ScriptLanguage::Warning> *r_warnings, Set<int> *r_safe_lines) const {
	ECMAscriptScriptError script_error;
	bool ret = binding->validate(p_script, p_path, &script_error);
	if (!ret) {
		r_test_error = binding->error_to_string(script_error);
		r_line_error = script_error.line;
		r_col_error = script_error.column;
	}
	return ret;
}

Script *ECMAScriptLanguage::create_script() const {
	return memnew(ECMAScript);
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

void ECMAScriptLanguage::frame() {
	binding->frame();
}

ECMAScriptLanguage::ECMAScriptLanguage() {

	ERR_FAIL_COND(singleton);
	singleton = this;
	binding = memnew(QuickJSBinder);
}

ECMAScriptLanguage::~ECMAScriptLanguage() {
	memdelete(binding);
}
