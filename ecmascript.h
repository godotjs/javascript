#ifndef ECMASCRIPT_H
#define ECMASCRIPT_H

#include "core/io/resource_loader.h"
#include "core/script_language.h"
#include "ecmascript_binding_helper.h"
#include "ecmascript_library.h"

class ECMAScript : public Script {
	GDCLASS(ECMAScript, Script);

private:
	friend class ECMAScriptInstance;
	friend class DuktapeBindingHelper;

	Set<Object *> instances;

	StringName class_name;
	Ref<ECMAScriptLibrary> library;

	ECMAClassInfo *get_ecma_class() const;

protected:
	/* TODO */ virtual bool editor_can_reload_from_file() { return false; } // this is handled by editor better
	/* TODO */ void _notification(int p_what) {}
	/* TODO */ static void _bind_methods();

	friend class PlaceHolderScriptInstance;
	/* TODO */ virtual void _placeholder_erased(PlaceHolderScriptInstance *p_placeholder) {}

public:
	virtual bool can_instance() const;

	/* TODO */ virtual Ref<Script> get_base_script() const { return NULL; } //for script inheritance

	/* TODO */ virtual StringName get_instance_base_type() const { return StringName(); } // this may not work in all scripts, will return empty if so
	virtual ScriptInstance *instance_create(Object *p_this);
	/* TODO */ virtual PlaceHolderScriptInstance *placeholder_instance_create(Object *p_this) { return NULL; }
	virtual bool instance_has(const Object *p_this) const;

	/* TODO */ virtual bool has_source_code() const { return false; }
	/* TODO */ virtual String get_source_code() const { return ""; }
	/* TODO */ virtual void set_source_code(const String &p_code) {}
	/* TODO */ virtual Error reload(bool p_keep_state = false) { return ERR_UNAVAILABLE; }

	virtual bool has_method(const StringName &p_method) const;
	virtual MethodInfo get_method_info(const StringName &p_method) const;

	/* TODO */ virtual bool is_tool() const { return false; }
	virtual bool is_valid() const;

	virtual ScriptLanguage *get_language() const;

	virtual bool has_script_signal(const StringName &p_signal) const;
	virtual void get_script_signal_list(List<MethodInfo> *r_signals) const;;

	virtual bool get_property_default_value(const StringName &p_property, Variant &r_value) const;

	/* TODO */ virtual void update_exports() {} //editor tool
	virtual void get_script_method_list(List<MethodInfo> *p_list) const;
	virtual void get_script_property_list(List<PropertyInfo> *p_list) const;;

	/* TODO */ virtual int get_member_line(const StringName &p_member) const { return -1; }

	/* TODO */ virtual void get_constants(Map<StringName, Variant> *p_constants) {}
	/* TODO */ virtual void get_members(Set<StringName> *p_constants) {}

	/* TODO */ virtual bool is_placeholder_fallback_enabled() const { return false; }

	_FORCE_INLINE_ void set_class_name(const StringName &p_class_name) { class_name = p_class_name; }
	_FORCE_INLINE_ StringName get_class_name() const { return class_name; }

	_FORCE_INLINE_ void set_library(const Ref<ECMAScriptLibrary> &p_library) { library = p_library; }
	_FORCE_INLINE_ Ref<ECMAScriptLibrary> get_library() const { return library; }

	ECMAScript();
	~ECMAScript();
};

class ResourceFormatLoaderECMAScript : public ResourceFormatLoader {
	GDCLASS(ResourceFormatLoaderECMAScript, ResourceFormatLoader)
public:
	virtual RES load(const String &p_path, const String &p_original_path = "", Error *r_error = NULL);
	virtual void get_recognized_extensions(List<String> *p_extensions) const;
	virtual void get_recognized_extensions_for_type(const String &p_type, List<String> *p_extensions) const;
	virtual bool handles_type(const String &p_type) const;
	virtual String get_resource_type(const String &p_path) const;
};

class ResourceFormatSaverECMAScript : public ResourceFormatSaver {
	GDCLASS(ResourceFormatSaverECMAScript, ResourceFormatSaver)
public:
	virtual Error save(const String &p_path, const RES &p_resource, uint32_t p_flags = 0);
	virtual void get_recognized_extensions(const RES &p_resource, List<String> *p_extensions) const;
	virtual bool recognize(const RES &p_resource) const;
};

#endif // ECMASCRIPT_H
