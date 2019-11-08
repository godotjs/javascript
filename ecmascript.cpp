#include "ecmascript.h"
#include "core/engine.h"
#include "ecmascript_instance.h"
#include "ecmascript_language.h"
#include "scene/resources/resource_format_text.h"

ScriptLanguage *ECMAScript::get_language() const {
	return ECMAScriptLanguage::get_singleton();
}

ECMAScript::ECMAScript() {
}

ECMAScript::~ECMAScript() {
}

bool ECMAScript::can_instance() const {

#ifdef TOOLS_ENABLED
	return is_valid() && (is_tool() || ScriptServer::is_scripting_enabled());
#else
	return is_valid();
#endif
}

StringName ECMAScript::get_instance_base_type() const {
	ECMAClassInfo *cls = get_ecma_class();
	ERR_FAIL_NULL_V(cls, StringName());
	return cls->native_class->name;
}

ScriptInstance *ECMAScript::instance_create(Object *p_this) {

	ECMAClassInfo *cls = get_ecma_class();
	ERR_FAIL_NULL_V(cls, NULL);

	if (!ClassDB::is_parent_class(p_this->get_class_name(), cls->native_class->name)) {
		ERR_EXPLAIN("Script inherits from native type '" + String(cls->native_class->name) + "', so it can't be instanced in object of type: '" + p_this->get_class() + "'");
		ERR_FAIL_V(NULL);
	}

	Variant::CallError unchecked_error;
	ECMAScriptGCHandler ecma_instance = ECMAScriptLanguage::get_singleton()->binding->create_ecma_instance_for_godot_object(class_name, p_this);
	ERR_FAIL_NULL_V(ecma_instance.ecma_object, NULL);

	ECMAScriptInstance *instance = memnew(ECMAScriptInstance);
	instance->script = Ref<ECMAScript>(this);
	instance->owner = p_this;
	instance->owner->set_script_instance(instance);
	instance->ecma_object = ecma_instance;

	return instance;
}

PlaceHolderScriptInstance *ECMAScript::placeholder_instance_create(Object *p_this) {
#ifdef TOOLS_ENABLED
	PlaceHolderScriptInstance *si = memnew(PlaceHolderScriptInstance(ECMAScriptLanguage::get_singleton(), Ref<Script>(this), p_this));
	placeholders.insert(si);
	update_exports();
	return si;
#else
	return NULL;
#endif
}

bool ECMAScript::is_placeholder_fallback_enabled() const {
#ifdef TOOLS_ENABLED
	return Engine::get_singleton()->is_editor_hint() && false;
#else
	return false;
#endif
}

Error ECMAScript::reload(bool p_keep_state) {
	ECMAClassInfo *cls = get_ecma_class();
	ERR_FAIL_COND_V(cls == NULL || library.is_null(), ERR_INVALID_DATA);

	library->reload_from_file();

	return OK;
}

bool ECMAScript::instance_has(const Object *p_this) const {
	return instances.has(const_cast<Object *>(p_this));
}

ECMAClassInfo *ECMAScript::get_ecma_class() const {
	return ECMAScriptLanguage::get_singleton()->binding->ecma_classes.getptr(class_name);
}

#ifdef TOOLS_ENABLED
void ECMAScript::_placeholder_erased(PlaceHolderScriptInstance *p_placeholder) {
	placeholders.erase(p_placeholder);
}
#endif

bool ECMAScript::has_method(const StringName &p_method) const {

	ECMAClassInfo *cls = get_ecma_class();
	ERR_FAIL_NULL_V(cls, false);

	return cls->methods.has(p_method);
}

MethodInfo ECMAScript::get_method_info(const StringName &p_method) const {

	MethodInfo mi;
	ECMAClassInfo *cls = get_ecma_class();
	ERR_FAIL_NULL_V(cls, mi);

	if (ECMAScriptGCHandler *method = cls->methods.getptr(p_method)) {
		mi.name = p_method;
	}

	return mi;
}

bool ECMAScript::is_tool() const {
	ECMAClassInfo *cls = get_ecma_class();
	ERR_FAIL_NULL_V(cls, false);
	return cls->tool;
}

void ECMAScript::get_script_method_list(List<MethodInfo> *p_list) const {

	ECMAClassInfo *cls = get_ecma_class();
	ERR_FAIL_NULL(cls);

	const StringName *key = cls->methods.next(NULL);
	while (key) {

		MethodInfo mi;
		mi.name = String(*key);
		p_list->push_back(mi);

		key = cls->methods.next(key);
	}
}

void ECMAScript::get_script_property_list(List<PropertyInfo> *p_list) const {
	ECMAClassInfo *cls = get_ecma_class();
	ERR_FAIL_NULL(cls);
	for (const StringName *name = cls->properties.next(NULL); name; name = cls->properties.next(name)) {
		const ECMAProperyInfo &prop = cls->properties.get(*name);
		PropertyInfo pi;
		pi.name = *name;
		pi.type = prop.type;
		p_list->push_back(pi);
	}
}

bool ECMAScript::get_property_default_value(const StringName &p_property, Variant &r_value) const {
	ECMAClassInfo *cls = get_ecma_class();
	ERR_FAIL_NULL_V(cls, false);

	if (ECMAProperyInfo *pi = cls->properties.getptr(p_property)) {
		r_value = pi->default_value;
		return true;
	}

	return false;
}

void ECMAScript::update_exports() {

#ifdef TOOLS_ENABLED

	ECMAClassInfo *cls = get_ecma_class();
	ERR_FAIL_NULL(cls);

	List<PropertyInfo> props;
	Map<StringName, Variant> values;
	for (const StringName *name = cls->properties.next(NULL); name; name = cls->properties.next(name)) {
		const ECMAProperyInfo epi = cls->properties.get(*name);
		PropertyInfo pi;
		pi.name = *name;
		pi.type = epi.type;
		props.push_back(pi);
		values[*name] = epi.default_value;
	}

	for (Set<PlaceHolderScriptInstance *>::Element *E = placeholders.front(); E; E = E->next()) {
		E->get()->update(props, values);
	}
#endif
}

bool ECMAScript::has_script_signal(const StringName &p_signal) const {
	ECMAClassInfo *cls = get_ecma_class();
	ERR_FAIL_NULL_V(cls, false);
	return cls->signals.has(p_signal);
}

void ECMAScript::get_script_signal_list(List<MethodInfo> *r_signals) const {
	ECMAClassInfo *cls = get_ecma_class();
	ERR_FAIL_NULL(cls);
	for (const StringName *name = cls->signals.next(NULL); name; name = cls->signals.next(name)) {
		r_signals->push_back(cls->signals.get(*name));
	}
}

bool ECMAScript::is_valid() const {
	return get_ecma_class() != NULL;
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

	Ref<ECMAScript> script = ResourceFormatLoaderText::singleton->load(p_path, p_original_path, r_error);
	if (!script.is_null()) {
		if (Ref<ECMAScript> *script_ptr = ECMAScriptLanguage::get_singleton()->get_class_script_ptr(script->get_class_name())) {
			(*script_ptr)->set_library(script->get_library());
			script->set_name((*script_ptr)->get_name());
			script->set_path((*script_ptr)->get_path());
			script->set_subindex((*script_ptr)->get_subindex());
			script = *script_ptr;
		}
	}
	return script;
}

void ResourceFormatLoaderECMAScript::get_recognized_extensions(List<String> *p_extensions) const {
	p_extensions->push_back("es");
}

void ResourceFormatLoaderECMAScript::get_recognized_extensions_for_type(const String &p_type, List<String> *p_extensions) const {
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
