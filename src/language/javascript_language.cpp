/* This is the language server implementation. It handles how/when to use JS Scripts. */

#include "core/io/file_access.h"
#include "core/object/class_db.h"

#include "../../thirdparty/quickjs/quickjs_binder.h"
#include "javascript_language.h"

JavaScriptLanguage *JavaScriptLanguage::singleton = nullptr;

JavaScriptLanguage::JavaScriptLanguage() {
	ERR_FAIL_COND(singleton);
	singleton = this;
	main_binder = memnew(QuickJSBinder);
	callable_middleman = memnew(CallableMiddleman);
	instance_binding_callbacks.create_callback = create_callback;
	instance_binding_callbacks.free_callback = free_callback;
	instance_binding_callbacks.reference_callback = reference_callback;
}

JavaScriptLanguage::~JavaScriptLanguage() {
	memdelete(main_binder);
	memdelete(callable_middleman);
}

void JavaScriptLanguage::init() {
	ERR_FAIL_NULL(main_binder);
	main_binder->initialize();
}

void JavaScriptLanguage::finish() {
	ERR_FAIL_NULL(main_binder);
	main_binder->uninitialize();
	main_binder->language_finalize();
}

Error JavaScriptLanguage::execute_file(const String &code) {
	ERR_FAIL_NULL_V(main_binder, ERR_BUG);
	JavaScriptGCHandler eval_ret;
	const Error err = main_binder->eval_string(code, JavaScriptBinder::EVAL_TYPE_GLOBAL, "test.js", eval_ret);
	return err;
}

bool JavaScriptLanguage::validate(const String &p_script, const String &p_path, List<String> *r_functions, List<ScriptError> *r_errors, List<Warning> *r_warnings, HashSet<int> *r_safe_lines) const {
	JavaScriptError script_error;
	const bool ret = main_binder->validate(p_script, p_path, &script_error);
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

void JavaScriptLanguage::reload_tool_script(const Ref<Script> &p_script, bool p_soft_reload) { reload_script(p_script, p_soft_reload); }

void JavaScriptLanguage::reload_script(const Ref<Script> &p_script, bool p_soft_reload) {
	const Ref<JavaScript> s = p_script;
	if (s.is_valid()) {
		Error err = OK;
		const Ref<JavaScriptModule> module = ResourceFormatLoaderJavaScriptModule::load_static(s->get_script_path(), "", &err);

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

void JavaScriptLanguage::frame() {
	main_binder->frame();
}

// TODO
void JavaScriptLanguage::add_global_constant(const StringName &p_variable, const Variant &p_value) {}


