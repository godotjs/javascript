#ifndef JAVASCRIPT_GC_HANDLER_H
#define JAVASCRIPT_GC_HANDLER_H

#include "core/object/object.h"
#include "core/object/ref_counted.h"
#include "core/variant/variant.h"

#define PROTOTYPE_LITERAL "prototype"
#define PROTO_LITERAL "__proto__"
#define TO_STRING_LITERAL "toString"
#define JS_CLASS_NAME_LITERAL "class_name"
#define GODOT_OBJECT_NAME "godot"

struct JavaScriptGCHandler {
	enum {
		FLAG_NONE = 0,
		FLAG_ATOMIC_VALUE = 1 << 1,
		FLAG_BUILTIN_CLASS = 1 << 2,
		FLAG_OBJECT = 1 << 3,
		FLAG_REF_COUNTED = 1 << 4,
		FLAG_FINALIZED = 1 << 5,
		FLAG_TRANSFERABLE = 1 << 6,
	};
	Variant::Type type;
	uint8_t flags;
	void *context;
	void *javascript_object;
	union {
		Object *godot_object;
		void *godot_builtin_object_ptr;
		void *native_ptr;
	};

	_FORCE_INLINE_ Variant get_value() const {
		switch (type) {
			case Variant::OBJECT:
				return godot_object;
			case Variant::VECTOR2:
				return *(static_cast<Vector2 *>(godot_builtin_object_ptr));
			case Variant::RECT2:
				return *(static_cast<Rect2 *>(godot_builtin_object_ptr));
			case Variant::QUATERNION:
				return *(static_cast<Quaternion *>(godot_builtin_object_ptr));
			case Variant::COLOR:
				return *(static_cast<Color *>(godot_builtin_object_ptr));
			case Variant::RID:
				return *(static_cast<RID *>(godot_builtin_object_ptr));
			case Variant::TRANSFORM3D:
				return *(static_cast<Transform3D *>(godot_builtin_object_ptr));
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
			case Variant::PACKED_INT32_ARRAY:
				return *(static_cast<PackedInt32Array *>(godot_builtin_object_ptr));
			case Variant::PACKED_INT64_ARRAY:
				return *(static_cast<PackedInt64Array *>(godot_builtin_object_ptr));
			case Variant::PACKED_BYTE_ARRAY:
				return *(static_cast<PackedByteArray *>(godot_builtin_object_ptr));
			case Variant::PACKED_FLOAT32_ARRAY:
				return *(static_cast<PackedFloat32Array *>(godot_builtin_object_ptr));
			case Variant::PACKED_FLOAT64_ARRAY:
				return *(static_cast<PackedFloat64Array *>(godot_builtin_object_ptr));
			case Variant::PACKED_COLOR_ARRAY:
				return *(static_cast<PackedColorArray *>(godot_builtin_object_ptr));
			case Variant::PACKED_STRING_ARRAY:
				return *(static_cast<PackedStringArray *>(godot_builtin_object_ptr));
			case Variant::PACKED_VECTOR2_ARRAY:
				return *(static_cast<PackedVector2Array *>(godot_builtin_object_ptr));
			case Variant::PACKED_VECTOR3_ARRAY:
				return *(static_cast<PackedVector3Array *>(godot_builtin_object_ptr));
			default:
				return Variant();
		}
		return Variant();
	}

	_FORCE_INLINE_ Object *get_godot_object() {
		if (flags & FLAG_OBJECT) {
			return godot_object;
		}
		return nullptr;
	}

	_FORCE_INLINE_ Vector2 *getVector2() const { return static_cast<Vector2 *>(godot_builtin_object_ptr); }
	_FORCE_INLINE_ Rect2 *getRect2() const { return static_cast<Rect2 *>(godot_builtin_object_ptr); }
	_FORCE_INLINE_ Color *getColor() const { return static_cast<Color *>(godot_builtin_object_ptr); }
	_FORCE_INLINE_ AABB *getAABB() const { return static_cast<AABB *>(godot_builtin_object_ptr); }
	_FORCE_INLINE_ Vector3 *getVector3() const { return static_cast<Vector3 *>(godot_builtin_object_ptr); }
	_FORCE_INLINE_ Transform2D *getTransform2D() const { return static_cast<Transform2D *>(godot_builtin_object_ptr); }
	_FORCE_INLINE_ Transform3D *getTransform3D() const { return static_cast<Transform3D *>(godot_builtin_object_ptr); }
	_FORCE_INLINE_ Basis *getBasis() const { return static_cast<Basis *>(godot_builtin_object_ptr); }
	_FORCE_INLINE_ RID *getRID() const { return static_cast<RID *>(godot_builtin_object_ptr); }
	_FORCE_INLINE_ Plane *getPlane() const { return static_cast<Plane *>(godot_builtin_object_ptr); }
	_FORCE_INLINE_ Quaternion *getQuaternion() const { return static_cast<Quaternion *>(godot_builtin_object_ptr); }
	_FORCE_INLINE_ PackedInt32Array *getPackedInt32Array() const { return static_cast<PackedInt32Array *>(godot_builtin_object_ptr); }
	_FORCE_INLINE_ PackedInt64Array *getPackedInt64Array() const { return static_cast<PackedInt64Array *>(godot_builtin_object_ptr); }
	_FORCE_INLINE_ PackedByteArray *getPackedByteArray() const { return static_cast<PackedByteArray *>(godot_builtin_object_ptr); }
	_FORCE_INLINE_ PackedFloat32Array *getPackedFloat32Array() const { return static_cast<PackedFloat32Array *>(godot_builtin_object_ptr); }
	_FORCE_INLINE_ PackedFloat64Array *getPackedFloat64Array() const { return static_cast<PackedFloat64Array *>(godot_builtin_object_ptr); }
	_FORCE_INLINE_ PackedStringArray *getPackedStringArray() const { return static_cast<PackedStringArray *>(godot_builtin_object_ptr); }
	_FORCE_INLINE_ PackedColorArray *getPackedColorArray() const { return static_cast<PackedColorArray *>(godot_builtin_object_ptr); }
	_FORCE_INLINE_ PackedVector2Array *getPackedVector2Array() const { return static_cast<PackedVector2Array *>(godot_builtin_object_ptr); }
	_FORCE_INLINE_ PackedVector3Array *getPackedVector3Array() const { return static_cast<PackedVector3Array *>(godot_builtin_object_ptr); }

	_FORCE_INLINE_ bool is_object() const {
		return flags & FLAG_OBJECT;
	}

	_FORCE_INLINE_ bool is_ref_counted() const {
		return flags & FLAG_REF_COUNTED;
	}

	_FORCE_INLINE_ bool is_transferable() const {
		return flags & FLAG_TRANSFERABLE;
	}

	_FORCE_INLINE_ bool is_atomic_type() const {
		return flags & FLAG_ATOMIC_VALUE;
	}

	_FORCE_INLINE_ bool is_finalized() const {
		return flags & FLAG_FINALIZED;
	}

	_FORCE_INLINE_ bool is_valid_javascript_object() const {
		return context != NULL && javascript_object != NULL && !is_finalized();
	}

	_FORCE_INLINE_ void clear() {
		type = Variant::NIL;
		godot_object = NULL;
		javascript_object = NULL;
		context = NULL;
	}

	JavaScriptGCHandler() {
		type = Variant::NIL;
		flags = FLAG_NONE;
		godot_object = NULL;
		javascript_object = NULL;
		context = NULL;
	}
};

#endif // JAVASCRIPT_GC_HANDLER_H
