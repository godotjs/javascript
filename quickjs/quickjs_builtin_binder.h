#ifndef QUICKJS_BUILTIN_BINDER_H
#define QUICKJS_BUILTIN_BINDER_H

#include "quickjs/quickjs.h"
#include <core/variant.h>
class ECMAScriptGCHandler;
class QuickJSBinder;

typedef JSValue (*JSConstructorFunc)(JSContext *ctx, JSValueConst new_target, int argc, JSValueConst *argv);
typedef void (*JSFinalizerFunc)(JSRuntime *rt, JSValue val);

class QuickJSBuiltinBinder {
public:
	struct BuiltinClass {
		JSClassID id;
		JSValue class_function;
		JSValue class_prototype;
		JSClassDef js_class;
	};

private:
	QuickJSBinder *binder;
	JSContext *ctx;
	BuiltinClass *builtin_class_map;
	JSValue to_string_function;
	JSAtom js_key_to_string;

public:
	static void builtin_finalizer(ECMAScriptGCHandler *p_bind);
	static JSValue bind_builtin_object(JSContext *ctx, Variant::Type p_type, void *p_object);

	void register_builtin_class(Variant::Type p_type, const char *p_name, JSConstructorFunc p_constructor, int argc);
	void register_property(Variant::Type p_type, const char *p_name, JSCFunctionMagic *p_getter, JSCFunctionMagic *p_setter, int magic);
	void register_operator(Variant::Type p_type, JSAtom p_operator, JSCFunction *p_func, int p_length);
	void register_method(Variant::Type p_type, const char *p_name, JSCFunction *p_func, int p_length);
	void register_constant(Variant::Type p_type, const char *p_name, const Variant &p_value);

public:
	QuickJSBuiltinBinder();
	~QuickJSBuiltinBinder();

	void initialize(JSContext *p_context, QuickJSBinder *p_binder);
	void uninitialize();

	void bind_builtin_classes_gen();
	void bind_builtin_propties_manually();

	_FORCE_INLINE_ void set_classid(Variant::Type p_type, JSClassID p_id) { builtin_class_map[p_type].id = p_id; }
	_FORCE_INLINE_ JSClassID get_classid(Variant::Type p_type) { return builtin_class_map[p_type].id; }
	_FORCE_INLINE_ BuiltinClass &get_class(Variant::Type p_type) { return *(builtin_class_map + p_type); }

	static JSValue new_object_from(JSContext *ctx, const Variant &p_val);
	_FORCE_INLINE_ static JSValue new_object_from(JSContext *ctx, const Vector2 &p_val) { return bind_builtin_object(ctx, Variant::VECTOR2, memnew(Vector2(p_val))); }
	_FORCE_INLINE_ static JSValue new_object_from(JSContext *ctx, const Vector3 &p_val) { return bind_builtin_object(ctx, Variant::VECTOR3, memnew(Vector3(p_val))); }
	_FORCE_INLINE_ static JSValue new_object_from(JSContext *ctx, const Rect2 &p_val) { return bind_builtin_object(ctx, Variant::RECT2, memnew(Rect2(p_val))); }
	_FORCE_INLINE_ static JSValue new_object_from(JSContext *ctx, const Color &p_val) { return bind_builtin_object(ctx, Variant::COLOR, memnew(Color(p_val))); }
	_FORCE_INLINE_ static JSValue new_object_from(JSContext *ctx, const Transform2D &p_val) { return bind_builtin_object(ctx, Variant::TRANSFORM2D, memnew(Transform2D(p_val))); }
	_FORCE_INLINE_ static JSValue new_object_from(JSContext *ctx, const Transform &p_val) { return bind_builtin_object(ctx, Variant::TRANSFORM, memnew(Transform(p_val))); }
	_FORCE_INLINE_ static JSValue new_object_from(JSContext *ctx, const Quat &p_val) { return bind_builtin_object(ctx, Variant::QUAT, memnew(Quat(p_val))); }
	_FORCE_INLINE_ static JSValue new_object_from(JSContext *ctx, const Plane &p_val) { return bind_builtin_object(ctx, Variant::PLANE, memnew(Plane(p_val))); }
	_FORCE_INLINE_ static JSValue new_object_from(JSContext *ctx, const RID &p_val) { return bind_builtin_object(ctx, Variant::_RID, memnew(RID(p_val))); }
	_FORCE_INLINE_ static JSValue new_object_from(JSContext *ctx, const AABB &p_val) { return bind_builtin_object(ctx, Variant::AABB, memnew(AABB(p_val))); }
	_FORCE_INLINE_ static JSValue new_object_from(JSContext *ctx, const Basis &p_val) { return bind_builtin_object(ctx, Variant::BASIS, memnew(Basis(p_val))); }
	_FORCE_INLINE_ static JSValue new_object_from(JSContext *ctx, const PoolIntArray &p_val) { return bind_builtin_object(ctx, Variant::POOL_INT_ARRAY, memnew(PoolIntArray(p_val))); }
	_FORCE_INLINE_ static JSValue new_object_from(JSContext *ctx, const PoolByteArray &p_val) { return bind_builtin_object(ctx, Variant::POOL_BYTE_ARRAY, memnew(PoolByteArray(p_val))); }
	_FORCE_INLINE_ static JSValue new_object_from(JSContext *ctx, const PoolRealArray &p_val) { return bind_builtin_object(ctx, Variant::POOL_REAL_ARRAY, memnew(PoolRealArray(p_val))); }
	_FORCE_INLINE_ static JSValue new_object_from(JSContext *ctx, const PoolColorArray &p_val) { return bind_builtin_object(ctx, Variant::POOL_COLOR_ARRAY, memnew(PoolColorArray(p_val))); }
	_FORCE_INLINE_ static JSValue new_object_from(JSContext *ctx, const PoolStringArray &p_val) { return bind_builtin_object(ctx, Variant::POOL_STRING_ARRAY, memnew(PoolStringArray(p_val))); }
	_FORCE_INLINE_ static JSValue new_object_from(JSContext *ctx, const PoolVector2Array &p_val) { return bind_builtin_object(ctx, Variant::POOL_VECTOR2_ARRAY, memnew(PoolVector2Array(p_val))); }
	_FORCE_INLINE_ static JSValue new_object_from(JSContext *ctx, const PoolVector3Array &p_val) { return bind_builtin_object(ctx, Variant::POOL_VECTOR3_ARRAY, memnew(PoolVector3Array(p_val))); }
};

#endif // QUICKJS_BUILTIN_BINDER_H
