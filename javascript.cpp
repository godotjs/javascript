/* This is implementation is for loading/saving JS files. */
/* After loading it contains the code/bytecode of the loaded file. */

#include "javascript.h"
#include "core/config/engine.h"
#include "core/io/file_access_encrypted.h"
#include "javascript_instance.h"
#include "javascript_language.h"
#include "scene/resources/resource_format_text.h"

ScriptLanguage *JavaScript::get_language() const {
	return JavaScriptLanguage::get_singleton();
}

JavaScript::JavaScript() {
}

JavaScript::~JavaScript() {
}

bool JavaScript::can_instantiate() const {
#ifdef TOOLS_ENABLED
	return is_valid() && (is_tool() || ScriptServer::is_scripting_enabled());
#else
	return is_valid();
#endif
}

StringName JavaScript::get_global_name() const {
	return StringName();
}

StringName JavaScript::get_instance_base_type() const {
	static StringName empty;
	ERR_FAIL_NULL_V(javascript_class, empty);
	ERR_FAIL_NULL_V(javascript_class->native_class, empty);
	return javascript_class->native_class->name;
}

ScriptInstance *JavaScript::instance_create(Object *p_this) {
	JavaScriptBinder *binder = JavaScriptLanguage::get_thread_binder(Thread::get_caller_id());
	ERR_FAIL_NULL_V_MSG(binder, NULL, "Cannot create instance from this thread");
	const JavaScriptClassInfo *cls = NULL;
	JavaScriptError js_err;
	if (!bytecode.is_empty()) {
		cls = binder->parse_javascript_class(bytecode, script_path, false, &js_err);
	} else {
		cls = binder->parse_javascript_class(code, script_path, false, &js_err);
	}
	ERR_FAIL_NULL_V_MSG(cls, NULL, vformat("Cannot parse class from %s", get_script_path()));

	if (!ClassDB::is_parent_class(p_this->get_class_name(), cls->native_class->name)) {
		ERR_FAIL_V_MSG(NULL, vformat("Script inherits from native type '%s', so it can't be instanced in object of type: '%s'", cls->native_class->name, p_this->get_class()));
	}

	JavaScriptGCHandler js_instance = binder->create_js_instance_for_godot_object(cls, p_this);
	ERR_FAIL_NULL_V(js_instance.javascript_object, NULL);

	JavaScriptInstance *instance = memnew(JavaScriptInstance);
	instance->script = Ref<JavaScript>(this);
	instance->owner = p_this;
	instance->binder = binder;
	instance->javascript_object = js_instance;
	instance->javascript_class = cls;
	instance->owner->set_script_instance(instance);
	instances.insert(p_this);
	return instance;
}

PlaceHolderScriptInstance *JavaScript::placeholder_instance_create(Object *p_this) {
#ifdef TOOLS_ENABLED
	PlaceHolderScriptInstance *si = memnew(PlaceHolderScriptInstance(JavaScriptLanguage::get_singleton(), Ref<Script>(this), p_this));
	instances.insert(p_this);
	placeholders.insert(si);
	update_exports();
	return si;
#else
	return NULL;
#endif
}

Error JavaScript::reload(bool p_keep_state) {
	javascript_class = NULL;
	Error err = OK;
	JavaScriptBinder *binder = JavaScriptLanguage::get_thread_binder(Thread::get_caller_id());
#ifdef TOOLS_ENABLED
	// This is a workaround for files generated outside of the godot editor
	if (binder == NULL) {
		binder = JavaScriptLanguage::get_thread_binder(Thread::MAIN_ID);
	}
#endif
	ERR_FAIL_COND_V_MSG(binder == NULL, ERR_INVALID_DATA, "Cannot load script in this thread");
	JavaScriptError js_err;

	// TODO: We should have a setting/option to skip parsing or reading .mjs files which aren't Godot classes for example "chunk-xxx.mjs" build from esbuild

	if (!bytecode.is_empty()) {
		javascript_class = binder->parse_javascript_class(bytecode, script_path, true, &js_err);
	} else {
		javascript_class = binder->parse_javascript_class(code, script_path, true, &js_err);
	}

	if (!javascript_class) {
		err = ERR_PARSE_ERROR;
		ERR_PRINT(binder->error_to_string(js_err));
	} else {
#ifdef TOOLS_ENABLED
		set_last_modified_time(FileAccess::get_modified_time(script_path));
		p_keep_state = true;

		for (Object *owner : instances) {
			HashMap<StringName, Variant> values;
			if (p_keep_state) {
				for (const KeyValue<StringName, JavaScriptProperyInfo> &pair : javascript_class->properties) {
					values.insert(pair.key, owner->get(pair.key));
				}
			}

			ScriptInstance *si = owner->get_script_instance();
			if (si->is_placeholder()) {
				PlaceHolderScriptInstance *psi = static_cast<PlaceHolderScriptInstance *>(si);
				if (javascript_class->tool) {
					_placeholder_erased(psi);
				}
			} else if (!javascript_class->tool) { // from tooled to !tooled
				PlaceHolderScriptInstance *psi = placeholder_instance_create(owner);
				owner->set_script_instance(psi);
				instances.insert(owner);
			}
			if (javascript_class->tool) { // re-create as an instance
				instance_create(owner);
			}

			if (p_keep_state) {
				for (const KeyValue<StringName, Variant> &pair : values) {
					if (const JavaScriptProperyInfo *epi = javascript_class->properties.getptr(pair.key)) {
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

bool JavaScript::instance_has(const Object *p_this) const {
	return instances.has(const_cast<Object *>(p_this));
}

#ifdef TOOLS_ENABLED
void JavaScript::_placeholder_erased(PlaceHolderScriptInstance *p_placeholder) {
	instances.erase(p_placeholder->get_owner());
	placeholders.erase(p_placeholder);
}
#endif

bool JavaScript::has_method(const StringName &p_method) const {
	if (!javascript_class)
		return false;
	return javascript_class->methods.getptr(p_method) != NULL;
}

MethodInfo JavaScript::get_method_info(const StringName &p_method) const {
	MethodInfo mi;
	ERR_FAIL_NULL_V(javascript_class, mi);
	if (const MethodInfo *ptr = javascript_class->methods.getptr(p_method)) {
		mi = *ptr;
	}
	return mi;
}

bool JavaScript::is_tool() const {
	if (!javascript_class)
		return false;
	return javascript_class->tool;
}

void JavaScript::get_script_method_list(List<MethodInfo> *p_list) const {
	if (!javascript_class)
		return;
	for (const KeyValue<StringName, MethodInfo> &pair : javascript_class->methods) {
		p_list->push_back(pair.value);
	}
}

void JavaScript::get_script_property_list(List<PropertyInfo> *p_list) const {
	if (!javascript_class)
		return;
	for (const KeyValue<StringName, JavaScriptProperyInfo> &pair : javascript_class->properties) {
		p_list->push_back(pair.value);
	}
}

bool JavaScript::get_property_default_value(const StringName &p_property, Variant &r_value) const {
	if (!javascript_class)
		return false;

	if (const JavaScriptProperyInfo *pi = javascript_class->properties.getptr(p_property)) {
		r_value = pi->default_value;
		return true;
	}

	return false;
}

void JavaScript::update_exports() {
#ifdef TOOLS_ENABLED
	if (!javascript_class)
		return;

	List<PropertyInfo> props;
	HashMap<StringName, Variant> values;
	for (const KeyValue<StringName, JavaScriptProperyInfo> &pair : javascript_class->properties) {
		const JavaScriptProperyInfo &pi = pair.value;
		props.push_back(pi);
		values[pair.key] = pi.default_value;
	}

	for (PlaceHolderScriptInstance *s : placeholders) {
		s->update(props, values);
	}
#endif
}

bool JavaScript::has_script_signal(const StringName &p_signal) const {
	if (!javascript_class)
		return false;
	return javascript_class->signals.has(p_signal);
}

void JavaScript::get_script_signal_list(List<MethodInfo> *r_signals) const {
	if (!javascript_class)
		return;
	for (const KeyValue<StringName, MethodInfo> &pair : javascript_class->signals) {
		r_signals->push_back(pair.value);
	}
}

bool JavaScript::is_valid() const {
	return javascript_class != NULL;
}

void JavaScript::_bind_methods() {
}

Ref<Resource> ResourceFormatLoaderJavaScript::load(const String &p_path, const String &p_original_path, Error *r_error, bool p_use_sub_threads, float *r_progress, CacheMode p_cache_mode) {
	Error err = OK;
	Ref<JavaScriptModule> module = ResourceFormatLoaderJavaScriptModule::load_static(p_path, p_original_path, &err);
	if (r_error)
		*r_error = err;

	if (err == ERR_FILE_NOT_FOUND) {
		return module;
	}

	ERR_FAIL_COND_V_MSG(err != OK, Ref<Resource>(), "Cannot load script file '" + p_path + "'.");
	Ref<JavaScript> javaScript;
	javaScript.instantiate();
	javaScript->set_script_path(p_path);
	javaScript->bytecode = module->get_bytecode();
	javaScript->set_source_code(module->get_source_code());
	err = javaScript->reload();
	if (r_error)
		*r_error = err;
	ERR_FAIL_COND_V_MSG(err != OK, Ref<Resource>(), "Parse source code from file '" + p_path + "' failed.");
#ifdef TOOLS_ENABLED
	JavaScriptLanguage::get_singleton()->get_scripts().insert(javaScript);
#endif
	return javaScript;
}

void ResourceFormatLoaderJavaScript::get_recognized_extensions(List<String> *p_extensions) const {
	p_extensions->push_front(EXT_JSCLASS);
}

void ResourceFormatLoaderJavaScript::get_recognized_extensions_for_type(const String &p_type, List<String> *p_extensions) const {
	get_recognized_extensions(p_extensions);
}

bool ResourceFormatLoaderJavaScript::handles_type(const String &p_type) const {
	return p_type == JavaScript::get_class_static();
}

String ResourceFormatLoaderJavaScript::get_resource_type(const String &p_path) const {
	String el = p_path.get_extension().to_lower();
	if (el == EXT_JSCLASS)
		return JavaScript::get_class_static();
	return "";
}

bool ResourceFormatLoaderJavaScript::recognize_path(const String &p_path, const String &p_for_type) const {
	return p_path.get_extension() == EXT_JSCLASS;
}

Error ResourceFormatSaverJavaScript::save(const Ref<Resource> &p_resource, const String &p_path, uint32_t p_flags) {
	Ref<JavaScript> javaScript = p_resource;
	ERR_FAIL_COND_V(javaScript.is_null(), ERR_INVALID_PARAMETER);

	String source = javaScript->get_source_code();

	Error err;
	Ref<FileAccess> file = FileAccess::open(p_path, FileAccess::WRITE, &err);
	ERR_FAIL_COND_V_MSG(err, err, "Cannot save file '" + p_path + "'.");
	file->store_string(source);
	if (file->get_error() != OK && file->get_error() != ERR_FILE_EOF) {
		return ERR_CANT_CREATE;
	}

	if (ScriptServer::is_reload_scripts_on_save_enabled()) {
		javaScript->reload();
	}

	return OK;
}

void ResourceFormatSaverJavaScript::get_recognized_extensions(const Ref<Resource> &p_resource, List<String> *p_extensions) const {
	if (Object::cast_to<JavaScript>(*p_resource)) {
		p_extensions->push_back(EXT_JSCLASS);
	}
}

bool ResourceFormatSaverJavaScript::recognize(const Ref<Resource> &p_resource) const {
	return Object::cast_to<JavaScript>(*p_resource) != nullptr;
}

void JavaScriptModule::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_script_path", "script_path"), &JavaScriptModule::set_script_path);
	ClassDB::bind_method(D_METHOD("get_script_path"), &JavaScriptModule::get_script_path);
	ClassDB::bind_method(D_METHOD("set_source_code", "source_code"), &JavaScriptModule::set_source_code);
	ClassDB::bind_method(D_METHOD("get_source_code"), &JavaScriptModule::get_source_code);
	ClassDB::bind_method(D_METHOD("set_bytecode", "bytecode"), &JavaScriptModule::set_bytecode);
	ClassDB::bind_method(D_METHOD("get_bytecode"), &JavaScriptModule::get_bytecode);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "script_path"), "set_script_path", "get_script_path");
}

JavaScriptModule::JavaScriptModule() {
	set_source_code("module.exports = {};\n");
}

Ref<Resource> ResourceFormatLoaderJavaScriptModule::load(const String &p_path, const String &p_original_path, Error *r_error, bool p_use_sub_threads, float *r_progress, ResourceFormatLoader::CacheMode p_cache_mode) {
	return ResourceFormatLoaderJavaScriptModule::load_static(p_path, p_original_path, r_error);
}

void ResourceFormatLoaderJavaScriptModule::get_recognized_extensions(List<String> *p_extensions) const {
	p_extensions->push_front(EXT_JSMODULE);
}

void ResourceFormatLoaderJavaScriptModule::get_recognized_extensions_for_type(const String &p_type, List<String> *p_extensions) const {
	get_recognized_extensions(p_extensions);
}

bool ResourceFormatLoaderJavaScriptModule::handles_type(const String &p_type) const {
	return p_type == JavaScriptModule::get_class_static();
}

String ResourceFormatLoaderJavaScriptModule::get_resource_type(const String &p_path) const {
	String el = p_path.get_extension().to_lower();
	if (el == EXT_JSMODULE)
		return JavaScriptModule::get_class_static();
	return "";
}

Ref<Resource> ResourceFormatLoaderJavaScriptModule::load_static(const String &p_path, const String &p_original_path, Error *r_error) {
	Error err = ERR_FILE_CANT_OPEN;
	bool fileExists = FileAccess::exists(p_path);
	if (!fileExists) {
		*r_error = ERR_FILE_NOT_FOUND;
		WARN_PRINT("Cannot find file '" + p_path + "'. Maybe you deleted the file.");
		return Ref<Resource>();
	}

	Ref<JavaScriptModule> module;
	module.instantiate();
	module->set_script_path(p_path);

	if (p_path.ends_with("." EXT_JSMODULE) || p_path.ends_with("." EXT_JSCLASS) || p_path.ends_with("." EXT_JSON)) {
		String code = FileAccess::get_file_as_string(p_path, &err);
		if (r_error)
			*r_error = err;
		ERR_FAIL_COND_V_MSG(err != OK, Ref<Resource>(), "Cannot load source code from file '" + p_path + "'.");
		module->set_source_code(code);
	}

// TODO: Check what this block is for
#if 0
	else if (p_path.ends_with("." EXT_JSMODULE_BYTECODE) || p_path.ends_with("." EXT_JSCLASS_BYTECODE)) {
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
#endif

	if (r_error)
		*r_error = err;
	ERR_FAIL_COND_V(err != OK, Ref<Resource>());
	return module;
}

Error ResourceFormatSaverJavaScriptModule::save(const Ref<Resource> &p_resource, const String &p_path, uint32_t p_flags) {
	Ref<JavaScriptModule> module = p_resource;
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

void ResourceFormatSaverJavaScriptModule::get_recognized_extensions(const Ref<Resource> &p_resource, List<String> *p_extensions) const {
	if (Object::cast_to<JavaScriptModule>(*p_resource)) {
		p_extensions->push_back(EXT_JSMODULE);
	}
}

bool ResourceFormatSaverJavaScriptModule::recognize(const Ref<Resource> &p_resource) const {
	return Object::cast_to<JavaScriptModule>(*p_resource) != nullptr;
}
