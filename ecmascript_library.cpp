#include "ecmascript_library.h"
#include "core/os/file_access.h"
#include "ecmascript_language.h"
#include "scene/resources/text_file.h"

RES ECMAScriptLibraryResourceLoader::load(const String &p_path, const String &p_original_path, Error *r_error) {

	Ref<ECMAScriptLibrary> file;
	file.instance();

	Error err = file->load_text(p_path);
	if (r_error) {
		*r_error = err;
	}
	ERR_FAIL_COND_V(err != OK, NULL);

	err = ECMAScriptLanguage::get_singleton()->eval_text(file->get_text());
	if (r_error) {
		*r_error = err;
	}
	ERR_FAIL_COND_V(err != OK, NULL);

	return file;
}

void ECMAScriptLibraryResourceLoader::get_recognized_extensions(List<String> *p_extensions) const {
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
