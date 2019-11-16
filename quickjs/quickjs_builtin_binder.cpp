#include "quickjs_builtin_binder.h"
#include "quickjs_binder.h"
#include <core/os/memory.h>

QuickJSBuiltinBinder::QuickJSBuiltinBinder() {
	ctx = NULL;
	builtin_class_map = memnew_arr(BuiltinClass, Variant::VARIANT_MAX);
}

QuickJSBuiltinBinder::~QuickJSBuiltinBinder() {
	memdelete_arr(builtin_class_map);
}

#include <core/math/vector2.h>

JSValue QuickJSBuiltinBinder::bind_builtin_object(JSContext *ctx, Variant::Type p_type, void *p_object) {

	ERR_FAIL_NULL_V(ctx, JS_UNDEFINED);

	QuickJSBinder *binder = QuickJSBinder::context_binders.get(ctx);
	const QuickJSBuiltinBinder::BuiltinClass &cls = binder->builtin_binder.get_class(p_type);
	JSValue obj = JS_NewObjectProtoClass(ctx, cls.class_prototype, binder->get_origin_class_id());

	ECMAScriptGCHandler *bind = memnew(ECMAScriptGCHandler);
	bind->type = p_type;
	bind->flags |= ECMAScriptGCHandler::FLAG_BUILTIN_CLASS;
	bind->godot_builtin_object_ptr = p_object;
	JS_SetOpaque(obj, bind);

	return obj;
}

void QuickJSBuiltinBinder::builtin_finalizer(ECMAScriptGCHandler *p_bind) {
	switch (p_bind->type) {
		case Variant::VECTOR2:
			memdelete(static_cast<Vector2 *>(p_bind->godot_builtin_object_ptr));
			break;
		case Variant::RECT2:
			memdelete(static_cast<Rect2 *>(p_bind->godot_builtin_object_ptr));
			break;
	}
	memdelete(p_bind);
}

void QuickJSBuiltinBinder::register_builtin_class(Variant::Type p_type, const char *p_name, JSConstructorFunc p_constructor, int argc) {
	QuickJSBinder *binder = QuickJSBinder::context_binders.get(ctx);
	QuickJSBuiltinBinder::BuiltinClass &cls = binder->builtin_binder.get_class(p_type);
	cls.id = 0;
	cls.js_class.class_name = p_name;
	cls.js_class.finalizer = NULL;
	cls.js_class.exotic = NULL;
	cls.js_class.gc_mark = NULL;
	cls.js_class.call = NULL;

	JS_NewClassID(&cls.id);
	JS_NewClass(JS_GetRuntime(ctx), cls.id, &cls.js_class);

	cls.class_prototype = JS_NewObject(ctx);
	JS_SetClassProto(ctx, cls.id, cls.class_prototype);

	cls.class_function = JS_NewCFunction2(ctx, p_constructor, p_name, argc, JS_CFUNC_constructor, 0);
	JS_SetConstructor(ctx, cls.class_function, cls.class_prototype);

	// toString
	JS_DupValue(ctx, to_string_function);
	JS_DefinePropertyValue(ctx, cls.class_prototype, js_key_to_string, to_string_function, QuickJSBinder::PROP_DEF_DEFAULT);
	// Add to godot object
	JS_DefinePropertyValueStr(ctx, binder->godot_object, p_name, cls.class_function, QuickJSBinder::PROP_DEF_DEFAULT);
}

void QuickJSBuiltinBinder::register_property(Variant::Type p_type, const char *p_name, JSCFunctionMagic *p_getter, JSCFunctionMagic *p_setter, int magic) {
	const BuiltinClass &cls = get_class(p_type);
	JSAtom atom = JS_NewAtom(ctx, p_name);
	JSValue getter = JS_NewCFunctionMagic(ctx, p_getter, p_name, 0, JS_CFUNC_generic_magic, magic);
	JSValue setter = JS_NewCFunctionMagic(ctx, p_setter, p_name, 1, JS_CFUNC_generic_magic, magic);
	JS_DefinePropertyGetSet(ctx, cls.class_prototype, atom, getter, setter, QuickJSBinder::PROP_DEF_DEFAULT);
	JS_FreeAtom(ctx, atom);
}

void QuickJSBuiltinBinder::register_operator(Variant::Type p_type, JSAtom p_operator, JSCFunction *p_func, int p_length) {
	const BuiltinClass &cls = get_class(p_type);
	JSValue func = JS_NewCFunction(ctx, p_func, "", p_length);
	JS_DefinePropertyValue(ctx, cls.class_function, p_operator, func, 0);
}

void QuickJSBuiltinBinder::register_method(Variant::Type p_type, const char *p_name, JSCFunction *p_func, int p_length) {
	const BuiltinClass &cls = get_class(p_type);
	JSValue func = JS_NewCFunction(ctx, p_func, p_name, p_length);
	JSAtom atom = JS_NewAtom(ctx, p_name);
	JS_DefinePropertyValue(ctx, cls.class_prototype, atom, func, QuickJSBinder::PROP_DEF_DEFAULT);
	JS_FreeAtom(ctx, atom);
}

void QuickJSBuiltinBinder::register_constant(Variant::Type p_type, const char *p_name, const Variant &p_value) {
	const BuiltinClass &cls = get_class(p_type);
	JSValue val = QuickJSBinder::variant_to_var(ctx, p_value);
	JSAtom atom = JS_NewAtom(ctx, p_name);
	JS_DefinePropertyValue(ctx, cls.class_function, atom, val, QuickJSBinder::PROP_DEF_DEFAULT);
	JS_FreeAtom(ctx, atom);
}

void QuickJSBuiltinBinder::initialize(JSContext *p_context, QuickJSBinder *p_binder) {
	ctx = p_context;
	binder = p_binder;
	js_key_to_string = JS_NewAtom(ctx, TO_STRING_LITERAL);
	to_string_function = JS_NewCFunction(ctx, QuickJSBinder::godot_to_string, TO_STRING_LITERAL, 0);
	bind_builtin_classes_gen();
	bind_builtin_propties_manually();
}

void QuickJSBuiltinBinder::uninitialize() {
	JS_FreeValue(ctx, to_string_function);
	JS_FreeAtom(ctx, js_key_to_string);
}

JSValue QuickJSBuiltinBinder::new_object_from(JSContext *ctx, const Variant &p_val) {
	void *ptr = NULL;
	switch (p_val.get_type()) {
		case Variant::COLOR: {
			Color *v = memnew(Color(p_val));
			ptr = v;
		} break;
		case Variant::VECTOR2: {
			Vector2 *v = memnew(Vector2(p_val));
			ptr = v;
		} break;
		case Variant::RECT2: {
			Rect2 *v = memnew(Rect2(p_val));
			ptr = v;
		} break;
		case Variant::TRANSFORM2D: {
			Transform2D *v = memnew(Transform2D(p_val));
			ptr = v;
		} break;
		case Variant::VECTOR3: {
			Vector3 *v = memnew(Vector3(p_val));
			ptr = v;
		} break;
		case Variant::QUAT: {
			Quat *v = memnew(Quat());
			*v = p_val;
			ptr = v;
		} break;
		case Variant::AABB: {
			AABB *v = memnew(AABB(p_val));
			ptr = v;
		} break;
		case Variant::PLANE: {
			Plane *v = memnew(Plane(p_val));
			ptr = v;
		} break;
		case Variant::BASIS: {
			Basis *v = memnew(Basis());
			*v = p_val;
			ptr = v;
		} break;
		case Variant::TRANSFORM: {
			Transform *v = memnew(Transform());
			*v = p_val;
			ptr = v;
		} break;
		case Variant::_RID: {
			RID *v = memnew(RID(p_val));
			ptr = v;
		} break;

		case Variant::POOL_INT_ARRAY: {
			PoolIntArray *v = memnew(PoolIntArray(p_val));
			ptr = v;
		} break;
		case Variant::POOL_BYTE_ARRAY: {
			PoolByteArray *v = memnew(PoolByteArray(p_val));
			ptr = v;
		} break;
		case Variant::POOL_REAL_ARRAY: {
			PoolRealArray *v = memnew(PoolRealArray(p_val));
			ptr = v;
		} break;
		case Variant::POOL_COLOR_ARRAY: {
			PoolColorArray *v = memnew(PoolColorArray(p_val));
			ptr = v;
		} break;
		case Variant::POOL_STRING_ARRAY: {
			PoolStringArray *v = memnew(PoolStringArray(p_val));
			ptr = v;
		} break;
		case Variant::POOL_VECTOR2_ARRAY: {
			PoolVector2Array *v = memnew(PoolVector2Array(p_val));
			ptr = v;
		} break;
		case Variant::POOL_VECTOR3_ARRAY: {
			PoolVector3Array *v = memnew(PoolVector3Array(p_val));
			ptr = v;
		} break;
	}
	return bind_builtin_object(ctx, p_val.get_type(), ptr);
}

void QuickJSBuiltinBinder::bind_builtin_propties_manually() {
}
