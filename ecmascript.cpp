#include "ecmascript.h"
#include "ecmascript_language.h"
#include "scene/resources/scene_format_text.h"

bool ECMAScript::can_instance() const {
	return is_valid();
}

ScriptLanguage *ECMAScript::get_language() const {
	return ECMAScriptLanguage::get_singleton();
}

ECMAScript::ECMAScript() {
	ecma_constructor.ecma_object = NULL;
}

ECMAScript::~ECMAScript() {
}

void ECMAScript::_bind_methods() {

	ClassDB::bind_method(D_METHOD("set_class_name", "class_name"), &ECMAScript::set_class_name);
	ClassDB::bind_method(D_METHOD("get_class_name"), &ECMAScript::get_class_name);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "class_name"), "set_class_name", "get_class_name");

	ClassDB::bind_method(D_METHOD("set_library", "library"), &ECMAScript ::set_library);
	ClassDB::bind_method(D_METHOD("get_library"), &ECMAScript ::get_library);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "library"), "set_library", "get_library");
}

RES ResourceFormatLoaderECMAScript::load(const String &p_path, const String &p_original_path, Error *r_error) {
	return ResourceFormatLoaderText::singleton->load(p_path, p_original_path, r_error);
}

void ResourceFormatLoaderECMAScript::get_recognized_extensions(List<String> *p_extensions) const {
	p_extensions->push_back("es");
}

bool ResourceFormatLoaderECMAScript::handles_type(const String &p_type) const {
	return p_type == "ECMAScript";
}

String ResourceFormatLoaderECMAScript::get_resource_type(const String &p_path) const {
	String el = p_path.get_extension().to_lower();
	if (el == "es")
		return "ECMAScript";
	return "";
}

Error ResourceFormatSaverECMAScript::save(const String &p_path, const RES &p_resource, uint32_t p_flags) {

	return ResourceFormatSaverText::singleton->save(p_path, p_resource, p_flags);
}

void ResourceFormatSaverECMAScript::get_recognized_extensions(const RES &p_resource, List<String> *p_extensions) const {

	if (Object::cast_to<ECMAScript>(*p_resource)) {
		p_extensions->push_back("es");
	}
}

bool ResourceFormatSaverECMAScript::recognize(const RES &p_resource) const {
	return Object::cast_to<ECMAScript>(*p_resource) != NULL;
}
