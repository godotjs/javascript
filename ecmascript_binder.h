#ifndef ECMASCRIPT_BINDING_HELPER_H
#define ECMASCRIPT_BINDING_HELPER_H

#include "core/object.h"
#include "core/reference.h"
#include "core/variant.h"

#define PROTOTYPE_LITERAL "prototype"
#define PROTO_LITERAL "__proto__"
#define TO_STRING_LITERAL "toString"
#define ECMA_CLASS_NAME_LITERAL "class_name"

struct ECMAScriptGCHandler {
	enum {
		FLAG_NONE = 0,
		FLAG_OBJECT = 1,
		FLAG_REFERENCE = 1 << 1,
		FLAG_FROM_SCRIPT = 1 << 2,
		FLAG_FROM_NATIVE = 1 << 3,
		FLAG_HOLDING_SCRIPT_REF = 1 << 4,
		FLAG_SCRIPT_FINALIZED = 1 << 5,
		FLAG_BUILTIN_CLASS = 1 << 6,
	};
	uint16_t flags;
	Variant::Type type;
	void *ecma_object;
	union {
		Object *godot_object;
		REF *godot_reference;
		void *godot_builtin_object_ptr;
	};

	_FORCE_INLINE_ Variant get_value() const {
		switch (type) {
			case Variant::OBJECT: {
				if (flags & FLAG_REFERENCE) {
					return *godot_reference;
				} else if (flags & FLAG_OBJECT) {
					return godot_object;
				}
			} break;
			case Variant::VECTOR2:
				return *(static_cast<Vector2 *>(godot_builtin_object_ptr));
			case Variant::RECT2:
				return *(static_cast<Rect2 *>(godot_builtin_object_ptr));
			case Variant::QUAT:
				return *(static_cast<Quat *>(godot_builtin_object_ptr));
			case Variant::COLOR:
				return *(static_cast<Color *>(godot_builtin_object_ptr));
			case Variant::_RID:
				return *(static_cast<RID *>(godot_builtin_object_ptr));
			case Variant::TRANSFORM:
				return *(static_cast<Transform *>(godot_builtin_object_ptr));
			case Variant::TRANSFORM2D:
				return *(static_cast<Transform2D *>(godot_builtin_object_ptr));
			case Variant::BASIS:
				return *(static_cast<Basis *>(godot_builtin_object_ptr));
			case Variant::VECTOR3:
				return *(static_cast<Vector3 *>(godot_builtin_object_ptr));
			case Variant::PLANE:
				return *(static_cast<Plane *>(godot_builtin_object_ptr));
			case Variant::AABB:
				return *(static_cast<AABB *>(godot_builtin_object_ptr));
			case Variant::POOL_INT_ARRAY:
				return *(static_cast<PoolIntArray *>(godot_builtin_object_ptr));
			case Variant::POOL_BYTE_ARRAY:
				return *(static_cast<PoolByteArray *>(godot_builtin_object_ptr));
			case Variant::POOL_REAL_ARRAY:
				return *(static_cast<PoolRealArray *>(godot_builtin_object_ptr));
			case Variant::POOL_COLOR_ARRAY:
				return *(static_cast<PoolColorArray *>(godot_builtin_object_ptr));
			case Variant::POOL_STRING_ARRAY:
				return *(static_cast<PoolStringArray *>(godot_builtin_object_ptr));
			case Variant::POOL_VECTOR2_ARRAY:
				return *(static_cast<PoolVector2Array *>(godot_builtin_object_ptr));
			case Variant::POOL_VECTOR3_ARRAY:
				return *(static_cast<PoolVector3Array *>(godot_builtin_object_ptr));
			default:
				return Variant();
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
		type = Variant::NIL;
	}

	ECMAScriptGCHandler() {
		flags = FLAG_NONE;
		godot_object = NULL;
		ecma_object = NULL;
		type = Variant::NIL;
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
	ECMAScriptGCHandler ecma_class_function;
	ECMAScriptGCHandler ecma_prototype;
	const ClassDB::ClassInfo *native_class;
	HashMap<StringName, MethodInfo> signals;
	HashMap<StringName, ECMAProperyInfo> properties;
};

class ECMAScriptBinder {
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
	virtual Variant call_method(const ECMAScriptGCHandler &p_object, const StringName &p_method, const Variant **p_args, int p_argcount, Variant::CallError &r_error) = 0;
	virtual bool get_instance_property(const ECMAScriptGCHandler &p_object, const StringName &p_name, Variant &r_ret) = 0;
	virtual bool set_instance_property(const ECMAScriptGCHandler &p_object, const StringName &p_name, const Variant &p_value) = 0;
	virtual bool has_method(const ECMAScriptGCHandler &p_object, const StringName &p_name) = 0;
};

#endif
