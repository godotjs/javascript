/**************************************************************************/
/*  javascript_language.cpp                                               */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

/* This is the language server implementation. It handles how/when to use JS Scripts. */

#include "javascript_language.h"
#include "core/io/file_access.h"
#include "core/object/class_db.h"

#include "javascript_binder.h"
#include "thirdparty/quickjs/quickjs_binder.h"

JavaScriptLanguage *JavaScriptLanguage::singleton = nullptr;

namespace JavaScriptInstanceBindingCallbacks {

static void *create_callback(void *p_token, void *p_instance) {
	if (JavaScriptBinder *binder = JavaScriptLanguage::get_singleton()->get_thread_binder(Thread::get_caller_id())) {
		return binder->alloc_object_binding_data(static_cast<Object *>(p_instance));
	}
	return nullptr;
}

static void free_callback(void *p_token, void *p_instance, void *p_binding) {
	if (JavaScriptBinder *binder = JavaScriptLanguage::get_singleton()->get_thread_binder(Thread::get_caller_id())) {
		return binder->free_object_binding_data(static_cast<JavaScriptGCHandler *>(p_binding));
	}
}

static GDExtensionBool reference_callback(void *p_token, void *p_binding, GDExtensionBool p_reference) {
	if (JavaScriptBinder *binder = JavaScriptLanguage::get_singleton()->get_thread_binder(Thread::get_caller_id())) {
		if (p_reference) {
			binder->godot_refcount_incremented(static_cast<JavaScriptGCHandler *>(p_binding));
			return false;
		} else {
			return binder->godot_refcount_decremented(static_cast<JavaScriptGCHandler *>(p_binding));
		}
	}
	return true;
}

} // namespace JavaScriptInstanceBindingCallbacks

JavaScriptLanguage::JavaScriptLanguage() {
	ERR_FAIL_COND(singleton);
	singleton = this;
	main_binder = memnew(QuickJSBinder);
	callable_middleman = memnew(CallableMiddleman);
	instance_binding_callbacks.create_callback = JavaScriptInstanceBindingCallbacks::create_callback;
	instance_binding_callbacks.free_callback = JavaScriptInstanceBindingCallbacks::free_callback;
	instance_binding_callbacks.reference_callback = JavaScriptInstanceBindingCallbacks::reference_callback;
}

JavaScriptLanguage::~JavaScriptLanguage() {
	memdelete(main_binder);
	memdelete(callable_middleman);
}

void JavaScriptLanguage::init() {
	ERR_FAIL_NULL(main_binder);
	main_binder->initialize();
	execute_file("modules/javascript/tests/UnitTest.js");
}

void JavaScriptLanguage::finish() {
	ERR_FAIL_NULL(main_binder);
	main_binder->uninitialize();
	main_binder->language_finalize();
}

Error JavaScriptLanguage::execute_file(const String &p_path) {
	ERR_FAIL_NULL_V(main_binder, ERR_BUG);
	Error err;
	String code = FileAccess::get_file_as_string(p_path, &err);
	if (err == OK) {
		JavaScriptGCHandler eval_ret;
		err = main_binder->eval_string(code, JavaScriptBinder::EVAL_TYPE_GLOBAL, p_path, eval_ret);
	}
	return err;
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

Ref<Script> JavaScriptLanguage::make_template(const String &p_template, const String &p_class_name, const String &p_base_class_name) const {
	Ref<JavaScript> javaScript;
	javaScript.instantiate();
	String src = p_template.replace("%BASE%", p_base_class_name).replace("%CLASS%", p_class_name);
	javaScript->set_source_code(src);
	return javaScript;
}

Vector<ScriptLanguage::ScriptTemplate> JavaScriptLanguage::get_built_in_templates(StringName p_object) {
	Vector<ScriptLanguage::ScriptTemplate> templates;
#ifdef TOOLS_ENABLED
	constexpr int len = 2;
	static const struct ScriptLanguage::ScriptTemplate TEMPLATES[len] = {
		{ "Node",
				"Default",
				"Base template for Node with default Godot cycle methods",
				"export default class %CLASS% extends " GODOT_OBJECT_NAME ".%BASE% {\n"
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
				"}\n" },
		{ "Object",
				"Empty",
				"Empty template suitable for all Objects",
				"export default class %CLASS% extends " GODOT_OBJECT_NAME ".%BASE% {\n"
				"    \n"
				"    // Declare member variables here. Examples:\n"
				"    a = 2;\n"
				"    b = \"text\";\n"
				"    \n"
				"    constructor() {\n"
				"        super();\n"
				"    }\n"
				"    \n"
				"}\n" },
	};

	for (int i = 0; i < len; i++) {
		if (TEMPLATES[i].inherit == p_object) {
			templates.append(TEMPLATES[i]);
		}
	}
#endif

	return templates;
}

bool JavaScriptLanguage::validate(const String &p_script, const String &p_path, List<String> *r_functions, List<ScriptError> *r_errors, List<Warning> *r_warnings, HashSet<int> *r_safe_lines) const {
	JavaScriptError script_error;
	bool ret = main_binder->validate(p_script, p_path, &script_error);
	if (!ret) {
		ScriptError se;
		se.line = script_error.line;
		se.column = script_error.column;
		se.message = script_error.message;
		r_errors->push_back(se);
	}
	return ret;
}

Script *JavaScriptLanguage::create_script() const {
	return memnew(JavaScript);
}

void JavaScriptLanguage::reload_all_scripts() {
#ifdef TOOLS_ENABLED
	for (const Ref<JavaScript> &s : scripts) {
		reload_script(s, true);
	}
#endif
}

void JavaScriptLanguage::reload_script(const Ref<Script> &p_script, bool p_soft_reload) {
	Ref<JavaScript> s = p_script;
	if (s.is_valid()) {
		Error err = OK;
		Ref<JavaScriptModule> module = ResourceFormatLoaderJavaScriptModule::load_static(s->get_script_path(), "", &err);

		if (err != ERR_FILE_NOT_FOUND) {
			// We don't need to reload a script if it isn't existing

			ERR_FAIL_COND_MSG(err != OK, ("Cannot reload script file '" + s->get_script_path() + "'."));
			s->set_source_code(module->get_source_code());
			err = s->reload(p_soft_reload);
			ERR_FAIL_COND_MSG(err != OK, "Parse source code from file '" + s->get_script_path() + "' failed.");
		} else {
			// If we are in the editor we need to erase the script from the language server to avoid reload on focus (editor_tools.cpp[_notification()])
#ifdef TOOLS_ENABLED
			singleton->get_scripts().erase(s);
#endif
		}
	}
}

void JavaScriptLanguage::get_recognized_extensions(List<String> *p_extensions) const {
	p_extensions->push_back(EXT_JSMODULE);
	p_extensions->push_back(EXT_JSCLASS);
	p_extensions->push_back(EXT_JSON);
}

void JavaScriptLanguage::frame() {
	main_binder->frame();
}

String JavaScriptLanguage::globalize_relative_path(const String &p_relative, const String &p_base_dir) {
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
		if (!base_dir.ends_with("/"))
			base_dir += "/";
		file = base_dir + file_path;
	}
	return file;
}
