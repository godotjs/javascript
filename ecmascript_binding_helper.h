#ifndef ECMASCRIPT_BINDING_HELPER_H
#define ECMASCRIPT_BINDING_HELPER_H

#include "core/object.h"
#include "core/reference.h"

#define PROTOTYPE_LITERAL "prototype"
#define PROTO_LITERAL "__proto__"
#define TO_STRING_LITERAL "toString"
#define ECMA_CLASS_NAME_LITERAL "class_name"

struct ECMAScriptGCHandler {
	void *ecma_object;
	bool is_null() const { return ecma_object == NULL; }
};

struct ECMAScriptObjectBindingData : public ECMAScriptGCHandler {
	enum {
		FLAG_NONE = 0,
		FLAG_OBJECT = 1,
		FLAG_REFERENCE = 1 << 1,
		FLAG_FROM_SCRIPT = 1 << 2,
		FLAG_HOLDING_SCRIPT_REF = 1 << 3,
		FLAG_SCRIPT_FINALIZED = 1 << 4,
	};
	union {
		Object *godot_object;
		REF *godot_reference;
	};

	uint16_t flags = FLAG_NONE;

	_FORCE_INLINE_ Variant get_value() const {
		if (flags & FLAG_REFERENCE) {
			return *godot_reference;
		} else if (flags & FLAG_OBJECT) {
			return godot_object;
		}
		return Variant();
	}

	_FORCE_INLINE_ Object *get_godot_object() {
		if (flags & FLAG_REFERENCE && godot_reference) {
			return godot_reference->ptr();
		} else if (flags & FLAG_OBJECT) {
			return godot_object;
		}
		return NULL;
	}

	_FORCE_INLINE_ bool is_object() const {
		return flags & FLAG_OBJECT && !(flags & FLAG_REFERENCE);
	}

	_FORCE_INLINE_ bool is_reference() const {
		return flags & FLAG_REFERENCE;
	}

	_FORCE_INLINE_ void clear() {
		godot_object = NULL;
		ecma_object = NULL;
	}
};

typedef ECMAScriptGCHandler ECMAMethodInfo;

struct ECMAProperyInfo {
	Variant::Type type;
	Variant default_value;
};

struct ECMAClassInfo {
	bool tool;
	StringName class_name;
	String icon_path;
	ECMAScriptGCHandler ecma_constructor;
	ClassDB::ClassInfo *native_class;
	HashMap<StringName, ECMAMethodInfo> methods;
	HashMap<StringName, MethodInfo> signals;
	HashMap<StringName, ECMAProperyInfo> properties;
};

class ECMAScriptBindingHelper {
	friend class ECMAScript;

protected:
	HashMap<StringName, ECMAClassInfo> ecma_classes;

public:
	virtual void clear_classes() { ecma_classes.clear(); }

	virtual void initialize() = 0;
	virtual void uninitialize() = 0;

	virtual void *alloc_object_binding_data(Object *p_object) = 0;
	virtual void free_object_binding_data(void *p_gc_handle) = 0;

	virtual void godot_refcount_incremented(Reference *p_object) = 0;
	virtual bool godot_refcount_decremented(Reference *p_object) = 0;

	virtual Error eval_string(const String &p_source) = 0;
	virtual Error safe_eval_text(const String &p_source, String &r_error) = 0;

	virtual ECMAScriptGCHandler create_ecma_instance_for_godot_object(const StringName &ecma_class_name, Object *p_object) = 0;
	virtual Variant call_method(const ECMAScriptGCHandler &p_object, const ECMAMethodInfo &p_method, const Variant **p_args, int p_argcount, Variant::CallError &r_error) = 0;
	virtual bool get_instance_property(const ECMAScriptGCHandler &p_object, const StringName &p_name, Variant &r_ret) = 0;
	virtual bool set_instance_property(const ECMAScriptGCHandler &p_object, const StringName &p_name, const Variant &p_value) = 0;
};

#endif
