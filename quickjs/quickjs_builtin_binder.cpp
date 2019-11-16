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

void QuickJSBuiltinBinder::register_operator(Variant::Type p_type, JSAtom p_operator, JSCFunctionMagic *p_func, int p_length) {
	const BuiltinClass &cls = get_class(p_type);
	JSValue func = JS_NewCFunctionMagic(ctx, p_func, "", p_length, JS_CFUNC_generic_magic, int(p_operator));
	JS_DefinePropertyValue(ctx, cls.class_function, p_operator, func, 0);
}

static JSValue vector2_constructor(JSContext *ctx, JSValueConst new_target, int argc, JSValueConst *argv);
static void bind_vector2_properties(JSContext *ctx);

static JSValue rect2_constructor(JSContext *ctx, JSValueConst new_target, int argc, JSValueConst *argv);
static void bind_rect2_properties(JSContext *ctx);

void QuickJSBuiltinBinder::initialize(JSContext *p_context, QuickJSBinder *p_binder) {
	ctx = p_context;
	binder = p_binder;

	js_key_to_string = JS_NewAtom(ctx, TO_STRING_LITERAL);
	to_string_function = JS_NewCFunction(ctx, QuickJSBinder::godot_to_string, TO_STRING_LITERAL, 0);

	register_builtin_class(Variant::VECTOR2, "Vector2", vector2_constructor, 2);
	bind_vector2_properties(ctx);

	register_builtin_class(Variant::RECT2, "Rect2", rect2_constructor, 4);
	bind_rect2_properties(ctx);
}

void QuickJSBuiltinBinder::uninitialize() {
	JS_FreeValue(ctx, to_string_function);
	JS_FreeAtom(ctx, js_key_to_string);
}

JSValue QuickJSBuiltinBinder::new_object_from(JSContext *ctx, const Variant &p_val) {
	void *ptr = NULL;
	switch (p_val.get_type()) {
		case Variant::VECTOR2: {
			Vector2 *v = memnew(Vector2(p_val));
			ptr = v;
		} break;
		case Variant::RECT2: {
			Rect2 *v = memnew(Rect2(p_val));
			ptr = v;
		} break;
	}
	return bind_builtin_object(ctx, p_val.get_type(), ptr);
}

static JSValue vector2_constructor(JSContext *ctx, JSValueConst new_target, int argc, JSValueConst *argv) {

	Vector2 *ptr = memnew(Vector2);
	if (argc >= 2) {
		double x, y;
		JS_ToFloat64(ctx, &x, argv[0]);
		JS_ToFloat64(ctx, &y, argv[1]);
		ptr->x = x;
		ptr->y = y;
	}

	return QuickJSBuiltinBinder::bind_builtin_object(ctx, Variant::VECTOR2, ptr);
}

static void bind_vector2_properties(JSContext *ctx) {

	QuickJSBinder *binder = QuickJSBinder::get_context_binder(ctx);

	JSCFunctionMagic *getter = [](JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv, int magic) -> JSValue {
		ECMAScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, this_val);
		const Vector2 *ptr = static_cast<Vector2 *>(bind->godot_builtin_object_ptr);
		switch (magic) {
			case 0:
				return JS_NewFloat64(ctx, ptr->x);
			case 1:
				return JS_NewFloat64(ctx, ptr->y);
		}
		return JS_UNDEFINED;
	};
	JSCFunctionMagic *setter = [](JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv, int magic) -> JSValue {
		ECMAScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, this_val);
		Vector2 *ptr = static_cast<Vector2 *>(bind->godot_builtin_object_ptr);
		double param = 0;
		if (JS_ToFloat64(ctx, &param, argv[0])) return JS_EXCEPTION;
		switch (magic) {
			case 0:
				ptr->x = param;
				break;
			case 1:
				ptr->y = param;
				break;
		}
		return argv[0];
	};

	JSCFunctionMagic *operators = [](JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv, int magic) -> JSValue {
		ECMAScriptGCHandler *bind0 = BINDING_DATA_FROM_JS(ctx, argv[0]);
		ECMAScriptGCHandler *bind1 = BINDING_DATA_FROM_JS(ctx, argv[1]);
		Vector2 *ptr0 = static_cast<Vector2 *>(bind0->godot_builtin_object_ptr);
		Vector2 *ptr1 = static_cast<Vector2 *>(bind1->godot_builtin_object_ptr);
		switch (magic) {
			case QuickJSBinder::JS_ATOM_Symbol_operatorAdd:
				return QuickJSBuiltinBinder::new_object_from(ctx, ptr0->operator+(*ptr1));
			case QuickJSBinder::JS_ATOM_Symbol_operatorSub:
				return QuickJSBuiltinBinder::new_object_from(ctx, ptr0->operator-(*ptr1));
			case QuickJSBinder::JS_ATOM_Symbol_operatorMul:
				return QuickJSBuiltinBinder::new_object_from(ctx, ptr0->operator*(*ptr1));
			case QuickJSBinder::JS_ATOM_Symbol_operatorDiv:
				return QuickJSBuiltinBinder::new_object_from(ctx, ptr0->operator/(*ptr1));
		}
		return JS_UNDEFINED;
	};

	binder->get_builtin_binder().register_property(Variant::VECTOR2, "x", getter, setter, 0);
	binder->get_builtin_binder().register_property(Variant::VECTOR2, "y", getter, setter, 1);

	binder->get_builtin_binder().register_operator(Variant::VECTOR2, QuickJSBinder::JS_ATOM_Symbol_operatorAdd, operators, 2);
	binder->get_builtin_binder().register_operator(Variant::VECTOR2, QuickJSBinder::JS_ATOM_Symbol_operatorSub, operators, 2);
	binder->get_builtin_binder().register_operator(Variant::VECTOR2, QuickJSBinder::JS_ATOM_Symbol_operatorMul, operators, 2);
	binder->get_builtin_binder().register_operator(Variant::VECTOR2, QuickJSBinder::JS_ATOM_Symbol_operatorDiv, operators, 2);
}

static JSValue rect2_constructor(JSContext *ctx, JSValueConst new_target, int argc, JSValueConst *argv) {
	Rect2 *ptr = memnew(Rect2);
	if (argc >= 4) {
		double x, y, w, h;
		JS_ToFloat64(ctx, &x, argv[0]);
		JS_ToFloat64(ctx, &y, argv[1]);
		JS_ToFloat64(ctx, &w, argv[2]);
		JS_ToFloat64(ctx, &h, argv[3]);
		ptr->position.x = x;
		ptr->position.y = y;
		ptr->size.x = w;
		ptr->size.y = h;
	} else if (argc >= 2) {
		ECMAScriptGCHandler *param0 = BINDING_DATA_FROM_JS(ctx, argv[0]);
		ECMAScriptGCHandler *param1 = BINDING_DATA_FROM_JS(ctx, argv[1]);
		if (!param0 || !param1) return JS_EXCEPTION;
		if (param0->type != Variant::VECTOR2 || param1->type != Variant::VECTOR2) return JS_EXCEPTION;
		ptr->position = *(static_cast<Vector2 *>(param0->godot_builtin_object_ptr));
		ptr->size = *(static_cast<Vector2 *>(param1->godot_builtin_object_ptr));
	}
	return QuickJSBuiltinBinder::bind_builtin_object(ctx, Variant::RECT2, ptr);
}

static void bind_rect2_properties(JSContext *ctx) {
	JSCFunctionMagic *getter = [](JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv, int magic) -> JSValue {
		ECMAScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, this_val);
		const Rect2 *ptr = static_cast<Rect2 *>(bind->godot_builtin_object_ptr);
		switch (magic) {
			case 0:
				return QuickJSBuiltinBinder::new_object_from(ctx, ptr->position);
			case 1:
				return QuickJSBuiltinBinder::new_object_from(ctx, ptr->size);
			case 2:
				return QuickJSBuiltinBinder::new_object_from(ctx, ptr->position + ptr->size);
		}
		return JS_UNDEFINED;
	};

	JSCFunctionMagic *setter = [](JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv, int magic) -> JSValue {
		ECMAScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, this_val);
		Rect2 *ptr = static_cast<Rect2 *>(bind->godot_builtin_object_ptr);
		ECMAScriptGCHandler *param_bind = BINDING_DATA_FROM_JS(ctx, argv[0]);
		if (!param_bind || param_bind->type != Variant::VECTOR2) return JS_EXCEPTION;
		switch (magic) {
			case 0:
				ptr->position = *(static_cast<Vector2 *>(param_bind->godot_builtin_object_ptr));
				break;
			case 1:
				ptr->size = *(static_cast<Vector2 *>(param_bind->godot_builtin_object_ptr));
				break;
			case 2:
				// TODO
				return JS_EXCEPTION;
				break;
		}
		return argv[0];
	};
	QuickJSBinder *binder = QuickJSBinder::get_context_binder(ctx);
	binder->get_builtin_binder().register_property(Variant::RECT2, "position", getter, setter, 0);
	binder->get_builtin_binder().register_property(Variant::RECT2, "size", getter, setter, 1);
	binder->get_builtin_binder().register_property(Variant::RECT2, "end", getter, setter, 2);
}
