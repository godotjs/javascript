#include "ecmascript.h"
#include "core/config/engine.h"
#include "core/io/file_access_encrypted.h"
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

bool ECMAScript::can_instantiate() const {
#ifdef TOOLS_ENABLED
	return is_valid() && (is_tool() || ScriptServer::is_scripting_enabled());
#else
	return is_valid();
#endif
}

StringName ECMAScript::get_instance_base_type() const {
	static StringName empty;
	ERR_FAIL_NULL_V(ecma_class, empty);
	ERR_FAIL_NULL_V(ecma_class->native_class, empty);
	return ecma_class->native_class->name;
}

ScriptInstance *ECMAScript::instance_create(Object *p_this) {

	ECMAScriptBinder *binder = ECMAScriptLanguage::get_thread_binder(Thread::get_caller_id());
	ERR_FAIL_NULL_V_MSG(binder, NULL, "Cannot create instance from this thread");
	const ECMAClassInfo *cls = NULL;
	ECMAscriptScriptError ecma_err;
	if (!bytecode.is_empty()) {
		cls = binder->parse_ecma_class(bytecode, script_path, false, &ecma_err);
	} else {
		cls = binder->parse_ecma_class(code, script_path, false, &ecma_err);
	}
	ERR_FAIL_NULL_V_MSG(cls, NULL, vformat("Cannot parse class from %s", get_script_path()));

	if (!ClassDB::is_parent_class(p_this->get_class_name(), cls->native_class->name)) {
		ERR_FAIL_V_MSG(NULL, vformat("Script inherits from native type '%s', so it can't be instanced in object of type: '%s'", cls->native_class->name, p_this->get_class()));
	}

	ECMAScriptGCHandler ecma_instance = binder->create_ecma_instance_for_godot_object(cls, p_this);
	ERR_FAIL_NULL_V(ecma_instance.ecma_object, NULL);

	ECMAScriptInstance *instance = memnew(ECMAScriptInstance);
	instance->script = Ref<ECMAScript>(this);
	instance->owner = p_this;
	instance->binder = binder;
	instance->ecma_object = ecma_instance;
	instance->ecma_class = cls;
	instance->owner->set_script_instance(instance);
	instances.insert(p_this);
	return instance;
}

PlaceHolderScriptInstance *ECMAScript::placeholder_instance_create(Object *p_this) {
#ifdef TOOLS_ENABLED
	PlaceHolderScriptInstance *si = memnew(PlaceHolderScriptInstance(ECMAScriptLanguage::get_singleton(), Ref<Script>(this), p_this));
	instances.insert(p_this);
	placeholders.insert(si);
	update_exports();
	return si;
#else
	return NULL;
#endif
}

Error ECMAScript::reload(bool p_keep_state) {
	ecma_class = NULL;
	Error err = OK;
	ECMAScriptBinder *binder = ECMAScriptLanguage::get_thread_binder(Thread::get_caller_id());
	ERR_FAIL_COND_V_MSG(binder == NULL, ERR_INVALID_DATA, "Cannot load script in this thread");
	ECMAscriptScriptError ecma_err;
	if (!bytecode.is_empty()) {
		ecma_class = binder->parse_ecma_class(bytecode, script_path, true, &ecma_err);
	} else {
		ecma_class = binder->parse_ecma_class(code, script_path, true, &ecma_err);
	}

	if (!ecma_class) {
		err = ERR_PARSE_ERROR;
		ERR_PRINT(binder->error_to_string(ecma_err));
	} else {
#ifdef TOOLS_ENABLED
		set_last_modified_time(FileAccess::get_modified_time(script_path));
		p_keep_state = true;

		for (Object *owner : instances) {
			HashMap<StringName, Variant> values;
			if (p_keep_state) {
				for (const KeyValue<StringName, ECMAProperyInfo> &pair : ecma_class->properties) {
					values.insert(pair.key, owner->get(pair.key));
				}
			}

			ScriptInstance *si = owner->get_script_instance();
			if (si->is_placeholder()) {
				PlaceHolderScriptInstance *psi = static_cast<PlaceHolderScriptInstance *>(si);
				if (ecma_class->tool) {
					_placeholder_erased(psi);
				}
			} else if (!ecma_class->tool) { // from tooled to !tooled
				PlaceHolderScriptInstance *psi = placeholder_instance_create(owner);
				owner->set_script_instance(psi);
				instances.insert(owner);
			}
			if (ecma_class->tool) { // re-create as an instance
				instance_create(owner);
			}

			if (p_keep_state) {
				for (const KeyValue<StringName, Variant> &pair : values) {
					if (const ECMAProperyInfo *epi = ecma_class->properties.getptr(pair.key)) {
						const Variant &backup = pair.value;
						owner->set(pair.key, backup.get_type() == epi->type ? backup : epi->default_value);
					}
				}
			}
		}
#endif
	}
	return err;
}

bool ECMAScript::instance_has(const Object *p_this) const {
	return instances.has(const_cast<Object *>(p_this));
}

#ifdef TOOLS_ENABLED
void ECMAScript::_placeholder_erased(PlaceHolderScriptInstance *p_placeholder) {
	instances.erase(p_placeholder->get_owner());
	placeholders.erase(p_placeholder);
}
#endif

bool ECMAScript::has_method(const StringName &p_method) const {
	if (!ecma_class) return false;
	return ecma_class->methods.getptr(p_method) != NULL;
}

MethodInfo ECMAScript::get_method_info(const StringName &p_method) const {
	MethodInfo mi;
	ERR_FAIL_NULL_V(ecma_class, mi);
	if (const MethodInfo *ptr = ecma_class->methods.getptr(p_method)) {
		mi = *ptr;
	}
	return mi;
}

bool ECMAScript::is_tool() const {
	if (!ecma_class) return false;
	return ecma_class->tool;
}

void ECMAScript::get_script_method_list(List<MethodInfo> *p_list) const {
	if (!ecma_class) return;
	for (const KeyValue<StringName, MethodInfo> &pair : ecma_class->methods) {
		p_list->push_back(pair.value);
	}
}

void ECMAScript::get_script_property_list(List<PropertyInfo> *p_list) const {
	if (!ecma_class) return;
	for (const KeyValue<StringName, ECMAProperyInfo> &pair : ecma_class->properties) {
		p_list->push_back(pair.value);
	}
}

bool ECMAScript::get_property_default_value(const StringName &p_property, Variant &r_value) const {
	if (!ecma_class)
		return false;

	if (const ECMAProperyInfo *pi = ecma_class->properties.getptr(p_property)) {
		r_value = pi->default_value;
		return true;
	}

	return false;
}

void ECMAScript::update_exports() {

#ifdef TOOLS_ENABLED
	if (!ecma_class) return;

	List<PropertyInfo> props;
	HashMap<StringName, Variant> values;
	for (const KeyValue<StringName, ECMAProperyInfo> &pair : ecma_class->properties) {
		const ECMAProperyInfo &pi = pair.value;
		props.push_back(pi);
		values[pair.key] = pi.default_value;
	}

	for (PlaceHolderScriptInstance *s : placeholders) {
		s->update(props, values);
	}
#endif
}

bool ECMAScript::has_script_signal(const StringName &p_signal) const {
	if (!ecma_class) return false;
	return ecma_class->signals.has(p_signal);
}

void ECMAScript::get_script_signal_list(List<MethodInfo> *r_signals) const {
	if (!ecma_class) return;
	for (const KeyValue<StringName, MethodInfo> &pair : ecma_class->signals) {
		r_signals->push_back(pair.value);
	}
}

bool ECMAScript::is_valid() const {
	return ecma_class != NULL;
}

void ECMAScript::_bind_methods() {
}

Ref<Resource> ResourceFormatLoaderECMAScript::load(const String &p_path, const String &p_original_path, Error *r_error) {
	Error err = OK;
	Ref<ECMAScriptModule> module = ResourceFormatLoaderECMAScriptModule::load_static(p_path, p_original_path, &err);
	if (r_error) *r_error = err;
	ERR_FAIL_COND_V_MSG(err != OK, Ref<Resource>(), "Cannot load script file '" + p_path + "'.");
	Ref<ECMAScript> script;
	script.instantiate();
	script->set_script_path(p_path);
	script->bytecode = module->get_bytecode();
	script->set_source_code(module->get_source_code());
	err = script->reload();
	if (r_error) *r_error = err;
	ERR_FAIL_COND_V_MSG(err != OK, Ref<Resource>(), "Parse source code from file '" + p_path + "' failed.");
#ifdef TOOLS_ENABLED
	ECMAScriptLanguage::get_singleton()->get_scripts().insert(script);
#endif
	return script;
}

void ResourceFormatLoaderECMAScript::get_recognized_extensions(List<String> *p_extensions) const {
	p_extensions->push_front(EXT_JSCLASS);
	p_extensions->push_front(EXT_JSCLASS_BYTECODE);
	p_extensions->push_front(EXT_JSCLASS_ENCRYPTED);
}

void ResourceFormatLoaderECMAScript::get_recognized_extensions_for_type(const String &p_type, List<String> *p_extensions) const {
	get_recognized_extensions(p_extensions);
}

bool ResourceFormatLoaderECMAScript::handles_type(const String &p_type) const {
	return p_type == ECMAScript::get_class_static();
}

String ResourceFormatLoaderECMAScript::get_resource_type(const String &p_path) const {
	String el = p_path.get_extension().to_lower();
	if (el == EXT_JSCLASS || el == EXT_JSCLASS_BYTECODE || el == EXT_JSCLASS_ENCRYPTED) return ECMAScript::get_class_static();
	return "";
}

Error ResourceFormatSaverECMAScript::save(const String &p_path, const Ref<Resource> &p_resource, uint32_t p_flags) {

	Ref<ECMAScript> script = p_resource;
	ERR_FAIL_COND_V(script.is_null(), ERR_INVALID_PARAMETER);

	String source = script->get_source_code();

	Error err;
	Ref<FileAccess> file = FileAccess::open(p_path, FileAccess::WRITE, &err);
	ERR_FAIL_COND_V_MSG(err, err, "Cannot save file '" + p_path + "'.");
	file->store_string(source);
	if (file->get_error() != OK && file->get_error() != ERR_FILE_EOF) {
		return ERR_CANT_CREATE;
	}

	if (ScriptServer::is_reload_scripts_on_save_enabled()) {
		script->reload();
	}

	return OK;
}

void ResourceFormatSaverECMAScript::get_recognized_extensions(const Ref<Resource> &p_resource, List<String> *p_extensions) const {
	if (Object::cast_to<ECMAScript>(*p_resource)) {
		p_extensions->push_back(EXT_JSCLASS);
	}
}

bool ResourceFormatSaverECMAScript::recognize(const Ref<Resource> &p_resource) const {
	return Object::cast_to<ECMAScript>(*p_resource) != nullptr;
}

void ECMAScriptModule::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_script_path", "script_path"), &ECMAScriptModule::set_script_path);
	ClassDB::bind_method(D_METHOD("get_script_path"), &ECMAScriptModule::get_script_path);
	ClassDB::bind_method(D_METHOD("set_source_code", "source_code"), &ECMAScriptModule::set_source_code);
	ClassDB::bind_method(D_METHOD("get_source_code"), &ECMAScriptModule::get_source_code);
	ClassDB::bind_method(D_METHOD("set_bytecode", "bytecode"), &ECMAScriptModule::set_bytecode);
	ClassDB::bind_method(D_METHOD("get_bytecode"), &ECMAScriptModule::get_bytecode);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "script_path"), "set_script_path", "get_script_path");
}

ECMAScriptModule::ECMAScriptModule() {
	set_source_code("module.exports = {};" ENDL);
}

Ref<Resource> ResourceFormatLoaderECMAScriptModule::load(const String &p_path, const String &p_original_path, Error *r_error) {
	return load_static(p_path, p_original_path, r_error);
}

void ResourceFormatLoaderECMAScriptModule::get_recognized_extensions(List<String> *p_extensions) const {
	p_extensions->push_front(EXT_JSMODULE);
	p_extensions->push_front(EXT_JSMODULE_BYTECODE);
	p_extensions->push_front(EXT_JSMODULE_ENCRYPTED);
}

void ResourceFormatLoaderECMAScriptModule::get_recognized_extensions_for_type(const String &p_type, List<String> *p_extensions) const {
	get_recognized_extensions(p_extensions);
}

bool ResourceFormatLoaderECMAScriptModule::handles_type(const String &p_type) const {
	return p_type == ECMAScriptModule::get_class_static();
}

String ResourceFormatLoaderECMAScriptModule::get_resource_type(const String &p_path) const {
	String el = p_path.get_extension().to_lower();
	if (el == EXT_JSMODULE || el == EXT_JSMODULE_BYTECODE || el == EXT_JSMODULE_ENCRYPTED) return ECMAScriptModule::get_class_static();
	return "";
}

Ref<Resource> ResourceFormatLoaderECMAScriptModule::load_static(const String &p_path, const String &p_original_path, Error *r_error) {
	Error err = ERR_FILE_CANT_OPEN;
	Ref<ECMAScriptModule> module;
	module.instantiate();
	module->set_script_path(p_path);
	if (p_path.ends_with("." EXT_JSMODULE) || p_path.ends_with("." EXT_JSCLASS) || p_path.ends_with("." EXT_JSON)) {
		String code = FileAccess::get_file_as_string(p_path, &err);
		if (r_error) *r_error = err;
		ERR_FAIL_COND_V_MSG(err != OK, Ref<Resource>(), "Cannot load source code from file '" + p_path + "'.");
		module->set_source_code(code);
	} else if (p_path.ends_with("." EXT_JSMODULE_BYTECODE) || p_path.ends_with("." EXT_JSCLASS_BYTECODE)) {
		module->set_bytecode(FileAccess::get_file_as_array(p_path, &err));
		if (r_error) *r_error = err;
		ERR_FAIL_COND_V_MSG(err != OK, Ref<Resource>(), "Cannot load bytecode from file '" + p_path + "'.");
	} else if (p_path.ends_with("." EXT_JSMODULE_ENCRYPTED) || p_path.ends_with("." EXT_JSCLASS_ENCRYPTED)) {
		Ref<FileAccess> fa = FileAccess::open(p_path, FileAccess::READ);
		if (fa.is_valid() && fa->is_open()) {
			Ref<FileAccessEncrypted> fae = memnew(FileAccessEncrypted);
			Vector<uint8_t> key;
			key.resize(32);
			for (int i = 0; i < key.size(); i++) {
				key.write[i] = script_encryption_key[i];
			}
			err = fae->open_and_parse(fa, key, FileAccessEncrypted::MODE_READ);
			if (err == OK) {
				Vector<uint8_t> encrypted_code;
				encrypted_code.resize(fae->get_length());
				fae->get_buffer(encrypted_code.ptrw(), encrypted_code.size());

				String code;
				if (code.parse_utf8((const char *)encrypted_code.ptr(), encrypted_code.size())) {
					err = ERR_PARSE_ERROR;
				} else {
					module->set_source_code(code);
				}
			}
		} else {
			err = ERR_CANT_OPEN;
		}
	}
	if (r_error) *r_error = err;
	ERR_FAIL_COND_V(err != OK, Ref<Resource>());
	return module;
}

Error ResourceFormatSaverECMAScriptModule::save(const String &p_path, const Ref<Resource> &p_resource, uint32_t p_flags) {
	Ref<ECMAScriptModule> module = p_resource;
	ERR_FAIL_COND_V(module.is_null(), ERR_INVALID_PARAMETER);
	String source = module->get_source_code();
	Error err;
	Ref<FileAccess> file = FileAccess::open(p_path, FileAccess::WRITE, &err);
	ERR_FAIL_COND_V_MSG(err, err, "Cannot save file '" + p_path + "'.");
	file->store_string(source);
	if (file->get_error() != OK && file->get_error() != ERR_FILE_EOF) {
		return ERR_CANT_CREATE;
	}
	return OK;
}

void ResourceFormatSaverECMAScriptModule::get_recognized_extensions(const Ref<Resource> &p_resource, List<String> *p_extensions) const {
	if (Object::cast_to<ECMAScriptModule>(*p_resource)) {
		p_extensions->push_back(EXT_JSMODULE);
	}
}

bool ResourceFormatSaverECMAScriptModule::recognize(const Ref<Resource> &p_resource) const {
	return Object::cast_to<ECMAScriptModule>(*p_resource) != nullptr;
}
