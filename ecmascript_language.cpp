#include "ecmascript_language.h"
#include "core/class_db.h"
#include "core/os/file_access.h"
ECMAScriptLanguage *ECMAScriptLanguage::singleton = NULL;

void ECMAScriptLanguage::init() {
	ERR_FAIL_NULL(main_binder);
	main_binder->initialize();
}

void ECMAScriptLanguage::finish() {
	ERR_FAIL_NULL(main_binder);
	main_binder->uninitialize();
	main_binder->language_finalize();
}

Error ECMAScriptLanguage::execute_file(const String &p_path) {
	ERR_FAIL_NULL_V(main_binder, ERR_BUG);
	Error err;
	String code = FileAccess::get_file_as_string(p_path, &err);
	if (err == OK) {
		ECMAScriptGCHandler eval_ret;
		err = main_binder->eval_string(code, ECMAScriptBinder::EVAL_TYPE_GLOBAL, p_path, eval_ret);
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

bool ECMAScriptLanguage::is_control_flow_keyword(String p_keyword) const {
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
		   p_keyword == "default" ||
		   p_keyword == "throw" ||
		   p_keyword == "try" ||
		   p_keyword == "catch" ||
		   p_keyword == "finally";
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
	bool ret = main_binder->validate(p_script, p_path, &script_error);
	if (!ret) {
		r_test_error = main_binder->error_to_string(script_error);
		r_line_error = script_error.line;
		r_col_error = script_error.column;
	}
	return ret;
}

Script *ECMAScriptLanguage::create_script() const {
	return memnew(ECMAScript);
}

void ECMAScriptLanguage::reload_all_scripts() {
#ifdef TOOLS_ENABLED
	for (Set<Ref<ECMAScript> >::Element *E = scripts.front(); E; E = E->next()) {
		reload_script(E->get(), true);
	}
#endif
}

void ECMAScriptLanguage::reload_script(const Ref<Script> &p_script, bool p_soft_reload) {
	Ref<ECMAScript> s = p_script;
	if (s.is_valid()) {
		Error err = OK;
		Ref<ECMAScriptModule> module = ResourceFormatLoaderECMAScriptModule::load_static(s->get_script_path(), "", &err);
		ERR_FAIL_COND_MSG(err != OK, ("Cannot load script file '" + s->get_script_path() + "'."));
		s->set_source_code(module->get_source_code());
		err = s->reload(p_soft_reload);
		ERR_FAIL_COND_MSG(err != OK, "Parse source code from file '" + s->get_script_path() + "' failed.");
	}
}

void ECMAScriptLanguage::get_recognized_extensions(List<String> *p_extensions) const {
	p_extensions->push_back(EXT_JSMODULE);
	p_extensions->push_back(EXT_JSCLASS);
	p_extensions->push_back(EXT_JSON);
	p_extensions->push_back(EXT_JSMODULE_ENCRYPTED);
	p_extensions->push_back(EXT_JSMODULE_BYTECODE);
	p_extensions->push_back(EXT_JSCLASS_ENCRYPTED);
	p_extensions->push_back(EXT_JSCLASS_BYTECODE);
}

void *ECMAScriptLanguage::alloc_instance_binding_data(Object *p_object) {
	if (ECMAScriptBinder *binder = get_thread_binder(Thread::get_caller_id())) {
		return binder->alloc_object_binding_data(p_object);
	}
	return NULL;
}

void ECMAScriptLanguage::free_instance_binding_data(void *p_data) {
	if (ECMAScriptBinder *binder = get_thread_binder(Thread::get_caller_id())) {
		return binder->free_object_binding_data(p_data);
	}
}

void ECMAScriptLanguage::refcount_incremented_instance_binding(Object *p_object) {
	if (ECMAScriptBinder *binder = get_thread_binder(Thread::get_caller_id())) {
		binder->godot_refcount_incremented(static_cast<Reference *>(p_object));
	}
}

bool ECMAScriptLanguage::refcount_decremented_instance_binding(Object *p_object) {
	if (ECMAScriptBinder *binder = get_thread_binder(Thread::get_caller_id())) {
		return binder->godot_refcount_decremented(static_cast<Reference *>(p_object));
	}
	return true;
}

void ECMAScriptLanguage::frame() {
	main_binder->frame();
}

String ECMAScriptLanguage::globalize_relative_path(const String &p_relative, const String &p_base_dir) {
	String file = p_relative;
	if (file.begins_with(".")) {
		String base_dir = p_base_dir;
		while (base_dir.ends_with(".")) {
			if (base_dir.ends_with("..")) {
				base_dir = base_dir.get_base_dir().get_base_dir();
			} else {
				base_dir = base_dir.get_base_dir();
			}
		}
		String file_path = file;
		while (file_path.begins_with(".")) {
			if (file_path.begins_with("../")) {
				base_dir = base_dir.get_base_dir();
				file_path = file_path.substr(3);
			} else if (file_path.begins_with("./")) {
				file_path = file_path.substr(2);
			} else {
				file_path = file_path.get_basename();
				break;
			}
		}
		if (!base_dir.ends_with("/")) base_dir += "/";
		file = base_dir + file_path;
	}
	return file;
}

ECMAScriptLanguage::ECMAScriptLanguage() {

	ERR_FAIL_COND(singleton);
	singleton = this;
	main_binder = memnew(QuickJSBinder);
}

ECMAScriptLanguage::~ECMAScriptLanguage() {
	memdelete(main_binder);
}
