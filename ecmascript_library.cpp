#include "ecmascript_library.h"
#include "core/os/file_access.h"
#include "ecmascript_language.h"
#include "scene/resources/text_file.h"
#include "core/engine.h"

Ref<ECMAScriptLibrary> ECMAScriptLibraryResourceLoader::loading_lib;
Set<ObjectID> ECMAScriptLibraryResourceLoader::ecma_libs;

RES ECMAScriptLibraryResourceLoader::load(const String &p_path, const String &p_original_path, Error *r_error) {

	Ref<ECMAScriptLibrary> file;
	file.instance();
	file->set_path(p_path);

	Error err = file->load_text(p_path);
	if (r_error) {
		*r_error = err;
	}
	ERR_FAIL_COND_V(err != OK, RES());

	err = file->eval_text();

	if (r_error) {
		*r_error = err;
	}
	ERR_FAIL_COND_V(err != OK, RES());
	ecma_libs.insert(file->get_instance_id());
	return file;
}

void ECMAScriptLibraryResourceLoader::get_recognized_extensions(List<String> *p_extensions) const {
	p_extensions->push_front("js");
}

void ECMAScriptLibraryResourceLoader::get_recognized_extensions_for_type(const String &p_type, List<String> *p_extensions) const {
	p_extensions->push_front("js");
}

bool ECMAScriptLibraryResourceLoader::handles_type(const String &p_type) const {
	return p_type == "ECMAScriptLibrary";
}

String ECMAScriptLibraryResourceLoader::get_resource_type(const String &p_path) const {
	String el = p_path.get_extension().to_lower();
	if (el == "js")
		return "ECMAScriptLibrary";
	return "";
}

void ECMAScriptLibraryResourceLoader::reload_cached_libs() {

	ECMAScriptLanguage::get_singleton()->clear_script_classes();
	ECMAScriptLanguage::get_singleton()->get_binder()->clear_classes();

	for (Set<ObjectID>::Element * E = ecma_libs.front(); E; E = E->next()) {
		ECMAScriptLibrary *lib = Object::cast_to<ECMAScriptLibrary>(ObjectDB::get_instance(E->get()));
		if (lib) {
			lib->reload_from_file();
		}
	}
}

Error ECMAScriptLibraryResourceSaver::save(const String &p_path, const RES &p_resource, uint32_t p_flags) {

	const ECMAScriptLibrary *lib = Object::cast_to<ECMAScriptLibrary>(*p_resource);
	ERR_FAIL_NULL_V(lib, ERR_INVALID_DATA);

	FileAccessRef file = FileAccess::open(p_path, FileAccess::WRITE);
	ERR_FAIL_COND_V(!file->is_open(), ERR_FILE_CANT_OPEN);

	file->store_string(lib->get_text());
	file->close();

	return OK;
}

bool ECMAScriptLibraryResourceSaver::recognize(const RES &p_resource) const {
	return Object::cast_to<ECMAScriptLibrary>(*p_resource) != NULL;
}

void ECMAScriptLibraryResourceSaver::get_recognized_extensions(const RES &p_resource, List<String> *p_extensions) const {
	if (Object::cast_to<ECMAScriptLibrary>(*p_resource)) {
		p_extensions->push_front("js");
	}
}

void ECMAScriptLibrary::reload_from_file() {
	TextFile::reload_from_file();
	eval_text();
}

Error ECMAScriptLibrary::eval_text() {
	Error err = ERR_UNAVAILABLE;
	ECMAScriptLibraryResourceLoader::loading_lib = RES(this);
	if (Engine::get_singleton()->is_editor_hint()) {
		String err_msg;
		err = ECMAScriptLanguage::get_singleton()->safe_eval_text(get_text(), err_msg);
		if (OK != err) {
			ERR_EXPLAIN(err_msg);
		}
	} else {
		err = ECMAScriptLanguage::get_singleton()->eval_text(get_text());
	}
	ECMAScriptLibraryResourceLoader::loading_lib = RES();
	return err;
}
