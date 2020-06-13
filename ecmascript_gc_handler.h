#ifndef ECMASCRIPT_GC_HANDLER_H
#define ECMASCRIPT_GC_HANDLER_H

#include "core/object.h"
#include "core/reference.h"
#include "core/variant.h"

#define PROTOTYPE_LITERAL "prototype"
#define PROTO_LITERAL "__proto__"
#define TO_STRING_LITERAL "toString"
#define ECMA_CLASS_NAME_LITERAL "class_name"
#define GODOT_OBJECT_NAME "godot"

struct ECMAScriptGCHandler {
	enum {
		FLAG_NONE = 0,
		FLAG_ATOMIC_VALUE = 1 << 1,
		FLAG_BUILTIN_CLASS = 1 << 2,
		FLAG_OBJECT = 1 << 3,
		FLAG_REFERENCE = 1 << 4,
		FLAG_SCRIPT_FINALIZED = 1 << 5,
		FLAG_CONTEXT_TRANSFERABLE = 1 << 6,
	};
	Variant::Type type;
	uint8_t flags;
	void *context;
	void *ecma_object;
	union {
		Object *godot_object;
		REF *godot_reference;
		void *godot_builtin_object_ptr;
		void *native_ptr;
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

	_FORCE_INLINE_ Vector2 *getVector2() const { return static_cast<Vector2 *>(godot_builtin_object_ptr); }
	_FORCE_INLINE_ Rect2 *getRect2() const { return static_cast<Rect2 *>(godot_builtin_object_ptr); }
	_FORCE_INLINE_ Color *getColor() const { return static_cast<Color *>(godot_builtin_object_ptr); }
	_FORCE_INLINE_ AABB *getAABB() const { return static_cast<AABB *>(godot_builtin_object_ptr); }
	_FORCE_INLINE_ Vector3 *getVector3() const { return static_cast<Vector3 *>(godot_builtin_object_ptr); }
	_FORCE_INLINE_ Transform2D *getTransform2D() const { return static_cast<Transform2D *>(godot_builtin_object_ptr); }
	_FORCE_INLINE_ Transform *getTransform() const { return static_cast<Transform *>(godot_builtin_object_ptr); }
	_FORCE_INLINE_ Basis *getBasis() const { return static_cast<Basis *>(godot_builtin_object_ptr); }
	_FORCE_INLINE_ RID *getRID() const { return static_cast<RID *>(godot_builtin_object_ptr); }
	_FORCE_INLINE_ Plane *getPlane() const { return static_cast<Plane *>(godot_builtin_object_ptr); }
	_FORCE_INLINE_ Quat *getQuat() const { return static_cast<Quat *>(godot_builtin_object_ptr); }
	_FORCE_INLINE_ PoolIntArray *getPoolIntArray() const { return static_cast<PoolIntArray *>(godot_builtin_object_ptr); }
	_FORCE_INLINE_ PoolByteArray *getPoolByteArray() const { return static_cast<PoolByteArray *>(godot_builtin_object_ptr); }
	_FORCE_INLINE_ PoolRealArray *getPoolRealArray() const { return static_cast<PoolRealArray *>(godot_builtin_object_ptr); }
	_FORCE_INLINE_ PoolStringArray *getPoolStringArray() const { return static_cast<PoolStringArray *>(godot_builtin_object_ptr); }
	_FORCE_INLINE_ PoolColorArray *getPoolColorArray() const { return static_cast<PoolColorArray *>(godot_builtin_object_ptr); }
	_FORCE_INLINE_ PoolVector2Array *getPoolVector2Array() const { return static_cast<PoolVector2Array *>(godot_builtin_object_ptr); }
	_FORCE_INLINE_ PoolVector3Array *getPoolVector3Array() const { return static_cast<PoolVector3Array *>(godot_builtin_object_ptr); }

	_FORCE_INLINE_ bool is_object() const {
		return flags & FLAG_OBJECT && !(flags & FLAG_REFERENCE);
	}

	_FORCE_INLINE_ bool is_reference() const {
		return flags & FLAG_REFERENCE;
	}

	_FORCE_INLINE_ bool is_transferable() const {
		return flags & FLAG_CONTEXT_TRANSFERABLE;
	}

	_FORCE_INLINE_ bool is_atomic_type() const {
		return flags & FLAG_ATOMIC_VALUE;
	}

	_FORCE_INLINE_ bool is_finalized() const {
		return flags & FLAG_SCRIPT_FINALIZED;
	}
	_FORCE_INLINE_ bool is_valid_ecma_object() const {
		return context != NULL && ecma_object != NULL && !is_finalized();
	}


	_FORCE_INLINE_ void clear() {
		type = Variant::NIL;
		godot_object = NULL;
		ecma_object = NULL;
		context = NULL;
	}

	ECMAScriptGCHandler() {
		type = Variant::NIL;
		flags = FLAG_NONE;
		godot_object = NULL;
		ecma_object = NULL;
		context = NULL;
	}
};

#endif
