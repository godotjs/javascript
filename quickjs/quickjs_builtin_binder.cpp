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

JSValue QuickJSBuiltinBinder::bind_builtin_object(Variant::Type p_type, void *p_object) {

	const QuickJSBuiltinBinder::BuiltinClass &cls = binder->builtin_binder.get_class(p_type);
	JSValue obj = JS_NewObjectProtoClass(ctx, cls.class_prototype, binder->get_origin_class_id());

	ECMAScriptGCHandler *bind = QuickJSBinder::new_gc_handler(ctx);
	bind->type = p_type;
	bind->flags |= ECMAScriptGCHandler::FLAG_BUILTIN_CLASS;
	bind->godot_builtin_object_ptr = p_object;
	JS_SetOpaque(obj, bind);

#ifdef DUMP_LEAKS
	QuickJSBinder::add_debug_binding_info(ctx, obj, bind);
#endif

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
		case Variant::COLOR:
			memdelete(static_cast<Color *>(p_bind->godot_builtin_object_ptr));
			break;
		case Variant::VECTOR3:
			memdelete(static_cast<Vector3 *>(p_bind->godot_builtin_object_ptr));
			break;
		case Variant::BASIS:
			memdelete(static_cast<Basis *>(p_bind->godot_builtin_object_ptr));
			break;
		case Variant::QUAT:
			memdelete(static_cast<Quat *>(p_bind->godot_builtin_object_ptr));
			break;
		case Variant::PLANE:
			memdelete(static_cast<Plane *>(p_bind->godot_builtin_object_ptr));
			break;
		case Variant::TRANSFORM2D:
			memdelete(static_cast<Transform2D *>(p_bind->godot_builtin_object_ptr));
			break;
		case Variant::_RID:
			memdelete(static_cast<RID *>(p_bind->godot_builtin_object_ptr));
			break;
		case Variant::TRANSFORM:
			memdelete(static_cast<Transform *>(p_bind->godot_builtin_object_ptr));
			break;
		case Variant::AABB:
			memdelete(static_cast<AABB *>(p_bind->godot_builtin_object_ptr));
			break;
		case Variant::POOL_INT_ARRAY:
			memdelete(static_cast<PoolIntArray *>(p_bind->godot_builtin_object_ptr));
			break;
		case Variant::POOL_BYTE_ARRAY:
			memdelete(static_cast<PoolByteArray *>(p_bind->godot_builtin_object_ptr));
			break;
		case Variant::POOL_REAL_ARRAY:
			memdelete(static_cast<PoolRealArray *>(p_bind->godot_builtin_object_ptr));
			break;
		case Variant::POOL_COLOR_ARRAY:
			memdelete(static_cast<PoolColorArray *>(p_bind->godot_builtin_object_ptr));
			break;
		case Variant::POOL_STRING_ARRAY:
			memdelete(static_cast<PoolStringArray *>(p_bind->godot_builtin_object_ptr));
			break;
		case Variant::POOL_VECTOR2_ARRAY:
			memdelete(static_cast<PoolVector2Array *>(p_bind->godot_builtin_object_ptr));
			break;
		case Variant::POOL_VECTOR3_ARRAY:
			memdelete(static_cast<PoolVector3Array *>(p_bind->godot_builtin_object_ptr));
			break;
	}
	memdelete(p_bind);
}

void QuickJSBuiltinBinder::register_builtin_class(Variant::Type p_type, const char *p_name, JSConstructorFunc p_constructor, int argc) {
	QuickJSBinder *binder = QuickJSBinder::get_context_binder(ctx);
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
	JS_SetPrototype(ctx, cls.class_prototype, QuickJSBinder::get_context_binder(ctx)->get_origin_class().prototype);

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

void QuickJSBuiltinBinder::register_operators(Variant::Type p_type, Vector<JSValue> &p_operators) {
	const BuiltinClass &cls = get_class(p_type);
	QuickJSBinder::define_operators(ctx, cls.class_prototype, p_operators.ptrw(), p_operators.size());
	for (int i = 0; i < p_operators.size(); i++) {
		JS_FreeValue(ctx, p_operators[i]);
	}
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

void QuickJSBuiltinBinder::get_cross_type_operators(Variant::Type p_type, Vector<JSValue> &r_operators) {
	JSValue Number = JS_GetProperty(ctx, binder->global_object, QuickJSBinder::JS_ATOM_Number);

	switch (p_type) {
		case Variant::VECTOR2: {
			JSValue number_left = JS_NewObject(ctx);
			JS_DefinePropertyValueStr(ctx, number_left, "left", JS_DupValue(ctx, Number), QuickJSBinder::PROP_DEF_DEFAULT);
			r_operators.push_back(number_left);

			JSValue number_right = JS_NewObject(ctx);
			JS_DefinePropertyValueStr(ctx, number_right, "right", JS_DupValue(ctx, Number), QuickJSBinder::PROP_DEF_DEFAULT);
			r_operators.push_back(number_right);

			// 2 * new godot.Vector2(2, 3)
			JS_DefinePropertyValueStr(ctx, number_left, "*",
					JS_NewCFunction(
							ctx, [](JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) -> JSValue {
								real_t scalar = QuickJSBinder::js_to_number(ctx, argv[0]);
								ECMAScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, argv[1]);
								Vector2 *ptr = bind->getVector2();
								Vector2 ret = ptr->operator*(scalar);
								return QuickJSBuiltinBinder::new_object_from(ctx, ret);
							},
							"multiply_number_left", 2),
					QuickJSBinder::PROP_DEF_DEFAULT);

			// new godot.Vector2(2, 3) * 2
			JS_DefinePropertyValueStr(ctx, number_right, "*",
					JS_NewCFunction(
							ctx, [](JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) -> JSValue {
								ECMAScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, argv[0]);
								Vector2 *ptr = bind->getVector2();
								real_t scalar = QuickJSBinder::js_to_number(ctx, argv[1]);
								Vector2 ret = ptr->operator*(scalar);
								return QuickJSBuiltinBinder::new_object_from(ctx, ret);
							},
							"multiply_number_right", 2),
					QuickJSBinder::PROP_DEF_DEFAULT);

			// new godot.Vector2(2, 3) / 2
			JS_DefinePropertyValueStr(ctx, number_right, "/",
					JS_NewCFunction(
							ctx, [](JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) -> JSValue {
								ECMAScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, argv[0]);
								Vector2 *ptr = bind->getVector2();
								real_t scalar = QuickJSBinder::js_to_number(ctx, argv[1]);
								Vector2 ret = ptr->operator/(scalar);
								return QuickJSBuiltinBinder::new_object_from(ctx, ret);
							},
							"divide_number_right", 2),
					QuickJSBinder::PROP_DEF_DEFAULT);
		} break;
		case Variant::VECTOR3: {
			JSValue number_left = JS_NewObject(ctx);
			JS_DefinePropertyValueStr(ctx, number_left, "left", JS_DupValue(ctx, Number), QuickJSBinder::PROP_DEF_DEFAULT);
			r_operators.push_back(number_left);

			JSValue number_right = JS_NewObject(ctx);
			JS_DefinePropertyValueStr(ctx, number_right, "right", JS_DupValue(ctx, Number), QuickJSBinder::PROP_DEF_DEFAULT);
			r_operators.push_back(number_right);

			// 2 * new godot.Vector3(2, 3, 4)
			JS_DefinePropertyValueStr(ctx, number_left, "*",
					JS_NewCFunction(
							ctx, [](JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) -> JSValue {
								real_t scalar = QuickJSBinder::js_to_number(ctx, argv[0]);
								ECMAScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, argv[1]);
								Vector3 *ptr = bind->getVector3();
								Vector3 ret = ptr->operator*(scalar);
								return QuickJSBuiltinBinder::new_object_from(ctx, ret);
							},
							"multiply_number_left", 2),
					QuickJSBinder::PROP_DEF_DEFAULT);

			// new godot.Vector3(2, 3, 4) * 2
			JS_DefinePropertyValueStr(ctx, number_right, "*",
					JS_NewCFunction(
							ctx, [](JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) -> JSValue {
								ECMAScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, argv[0]);
								Vector3 *ptr = bind->getVector3();
								real_t scalar = QuickJSBinder::js_to_number(ctx, argv[1]);
								Vector3 ret = ptr->operator*(scalar);
								return QuickJSBuiltinBinder::new_object_from(ctx, ret);
							},
							"multiply_number_right", 2),
					QuickJSBinder::PROP_DEF_DEFAULT);

			// new godot.Vector3(2, 3, 4) / 2
			JS_DefinePropertyValueStr(ctx, number_right, "/",
					JS_NewCFunction(
							ctx, [](JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) -> JSValue {
								ECMAScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, argv[0]);
								Vector3 *ptr = bind->getVector3();
								real_t scalar = QuickJSBinder::js_to_number(ctx, argv[1]);
								Vector3 ret = ptr->operator/(scalar);
								return QuickJSBuiltinBinder::new_object_from(ctx, ret);
							},
							"divide_number_right", 2),
					QuickJSBinder::PROP_DEF_DEFAULT);
		} break;
		case Variant::COLOR: {
			JSValue number_left = JS_NewObject(ctx);
			JS_DefinePropertyValueStr(ctx, number_left, "left", JS_DupValue(ctx, Number), QuickJSBinder::PROP_DEF_DEFAULT);
			r_operators.push_back(number_left);

			JSValue number_right = JS_NewObject(ctx);
			JS_DefinePropertyValueStr(ctx, number_right, "right", JS_DupValue(ctx, Number), QuickJSBinder::PROP_DEF_DEFAULT);
			r_operators.push_back(number_right);

			// 2 * new godot.Color(0.5, 0.5, 0.5)
			JS_DefinePropertyValueStr(ctx, number_left, "*",
					JS_NewCFunction(
							ctx, [](JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) -> JSValue {
								real_t scalar = QuickJSBinder::js_to_number(ctx, argv[0]);
								ECMAScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, argv[1]);
								Color *ptr = bind->getColor();
								Color ret = ptr->operator*(scalar);
								return QuickJSBuiltinBinder::new_object_from(ctx, ret);
							},
							"multiply_number_left", 2),
					QuickJSBinder::PROP_DEF_DEFAULT);

			// new godot.Color(0.5, 0.5, 0.5) * 2
			JS_DefinePropertyValueStr(ctx, number_right, "*",
					JS_NewCFunction(
							ctx, [](JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) -> JSValue {
								ECMAScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, argv[0]);
								Color *ptr = bind->getColor();
								real_t scalar = QuickJSBinder::js_to_number(ctx, argv[1]);
								Color ret = ptr->operator*(scalar);
								return QuickJSBuiltinBinder::new_object_from(ctx, ret);
							},
							"multiply_number_right", 2),
					QuickJSBinder::PROP_DEF_DEFAULT);

			// new godot.Color(0.5, 0.5, 0.5) / 2
			JS_DefinePropertyValueStr(ctx, number_right, "/",
					JS_NewCFunction(
							ctx, [](JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) -> JSValue {
								ECMAScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, argv[0]);
								Color *ptr = bind->getColor();
								real_t scalar = QuickJSBinder::js_to_number(ctx, argv[1]);
								Color ret = ptr->operator/(scalar);
								return QuickJSBuiltinBinder::new_object_from(ctx, ret);
							},
							"divide_number_right", 2),
					QuickJSBinder::PROP_DEF_DEFAULT);
		} break;
		case Variant::QUAT: {
			JSValue number_left = JS_NewObject(ctx);
			JS_DefinePropertyValueStr(ctx, number_left, "left", JS_DupValue(ctx, Number), QuickJSBinder::PROP_DEF_DEFAULT);
			r_operators.push_back(number_left);

			JSValue number_right = JS_NewObject(ctx);
			JS_DefinePropertyValueStr(ctx, number_right, "right", JS_DupValue(ctx, Number), QuickJSBinder::PROP_DEF_DEFAULT);
			r_operators.push_back(number_right);

			// 2 * new godot.Quat(0.5, 0.5, 0.5, 0.5)
			JS_DefinePropertyValueStr(ctx, number_left, "*",
					JS_NewCFunction(
							ctx, [](JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) -> JSValue {
								real_t scalar = QuickJSBinder::js_to_number(ctx, argv[0]);
								ECMAScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, argv[1]);
								Quat *ptr = bind->getQuat();
								Quat ret = ptr->operator*(scalar);
								return QuickJSBuiltinBinder::new_object_from(ctx, ret);
							},
							"multiply_number_left", 2),
					QuickJSBinder::PROP_DEF_DEFAULT);

			// new godot.Quat(0.5, 0.5, 0.5, 0.5) * 2
			JS_DefinePropertyValueStr(ctx, number_right, "*",
					JS_NewCFunction(
							ctx, [](JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) -> JSValue {
								ECMAScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, argv[0]);
								Quat *ptr = bind->getQuat();
								real_t scalar = QuickJSBinder::js_to_number(ctx, argv[1]);
								Quat ret = ptr->operator*(scalar);
								return QuickJSBuiltinBinder::new_object_from(ctx, ret);
							},
							"multiply_number_right", 2),
					QuickJSBinder::PROP_DEF_DEFAULT);

			// new godot.Quat(0.5, 0.5, 0.5, 0.5) / 2
			JS_DefinePropertyValueStr(ctx, number_right, "/",
					JS_NewCFunction(
							ctx, [](JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) -> JSValue {
								ECMAScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, argv[0]);
								Quat *ptr = bind->getQuat();
								real_t scalar = QuickJSBinder::js_to_number(ctx, argv[1]);
								Quat ret = ptr->operator/(scalar);
								return QuickJSBuiltinBinder::new_object_from(ctx, ret);
							},
							"divide_number_right", 2),
					QuickJSBinder::PROP_DEF_DEFAULT);
		} break;
		case Variant::BASIS: {
			JSValue number_left = JS_NewObject(ctx);
			JS_DefinePropertyValueStr(ctx, number_left, "left", JS_DupValue(ctx, Number), QuickJSBinder::PROP_DEF_DEFAULT);
			r_operators.push_back(number_left);

			JSValue number_right = JS_NewObject(ctx);
			JS_DefinePropertyValueStr(ctx, number_right, "right", JS_DupValue(ctx, Number), QuickJSBinder::PROP_DEF_DEFAULT);
			r_operators.push_back(number_right);

			// 2 * new godot.Basis()
			JS_DefinePropertyValueStr(ctx, number_left, "*",
					JS_NewCFunction(
							ctx, [](JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) -> JSValue {
								real_t scalar = QuickJSBinder::js_to_number(ctx, argv[0]);
								ECMAScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, argv[1]);
								Basis *ptr = bind->getBasis();
								Basis ret = ptr->operator*(scalar);
								return QuickJSBuiltinBinder::new_object_from(ctx, ret);
							},
							"multiply_number_left", 2),
					QuickJSBinder::PROP_DEF_DEFAULT);

			// new godot.Basis() * 2
			JS_DefinePropertyValueStr(ctx, number_right, "*",
					JS_NewCFunction(
							ctx, [](JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) -> JSValue {
								ECMAScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, argv[0]);
								Basis *ptr = bind->getBasis();
								real_t scalar = QuickJSBinder::js_to_number(ctx, argv[1]);
								Basis ret = ptr->operator*(scalar);
								return QuickJSBuiltinBinder::new_object_from(ctx, ret);
							},
							"multiply_number_right", 2),
					QuickJSBinder::PROP_DEF_DEFAULT);
		} break;
		default:
			break;
	}

	JS_FreeValue(ctx, Number);
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

JSValue QuickJSBuiltinBinder::bind_builtin_object_static(JSContext *ctx, Variant::Type p_type, void *p_object) {
	return QuickJSBinder::get_context_binder(ctx)->builtin_binder.bind_builtin_object(p_type, p_object);
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

	return QuickJSBinder::get_context_binder(ctx)->builtin_binder.bind_builtin_object(p_val.get_type(), ptr);
}

JSValue QuickJSBuiltinBinder::new_object_from(JSContext *ctx, const Vector2 &p_val) {
	return QuickJSBinder::get_context_binder(ctx)->builtin_binder.bind_builtin_object(Variant::VECTOR2, memnew(Vector2(p_val)));
}

JSValue QuickJSBuiltinBinder::new_object_from(JSContext *ctx, const Vector3 &p_val) {
	return QuickJSBinder::get_context_binder(ctx)->builtin_binder.bind_builtin_object(Variant::VECTOR3, memnew(Vector3(p_val)));
}

JSValue QuickJSBuiltinBinder::new_object_from(JSContext *ctx, const Rect2 &p_val) {
	return QuickJSBinder::get_context_binder(ctx)->builtin_binder.bind_builtin_object(Variant::RECT2, memnew(Rect2(p_val)));
}

JSValue QuickJSBuiltinBinder::new_object_from(JSContext *ctx, const Color &p_val) {
	return QuickJSBinder::get_context_binder(ctx)->builtin_binder.bind_builtin_object(Variant::COLOR, memnew(Color(p_val)));
}

JSValue QuickJSBuiltinBinder::new_object_from(JSContext *ctx, const Transform2D &p_val) {
	return QuickJSBinder::get_context_binder(ctx)->builtin_binder.bind_builtin_object(Variant::TRANSFORM2D, memnew(Transform2D(p_val)));
}

JSValue QuickJSBuiltinBinder::new_object_from(JSContext *ctx, const Transform &p_val) {
	return QuickJSBinder::get_context_binder(ctx)->builtin_binder.bind_builtin_object(Variant::TRANSFORM, memnew(Transform(p_val)));
}

JSValue QuickJSBuiltinBinder::new_object_from(JSContext *ctx, const Quat &p_val) {
	return QuickJSBinder::get_context_binder(ctx)->builtin_binder.bind_builtin_object(Variant::QUAT, memnew(Quat(p_val)));
}

JSValue QuickJSBuiltinBinder::new_object_from(JSContext *ctx, const Plane &p_val) {
	return QuickJSBinder::get_context_binder(ctx)->builtin_binder.bind_builtin_object(Variant::PLANE, memnew(Plane(p_val)));
}

JSValue QuickJSBuiltinBinder::new_object_from(JSContext *ctx, const RID &p_val) {
	return QuickJSBinder::get_context_binder(ctx)->builtin_binder.bind_builtin_object(Variant::_RID, memnew(RID(p_val)));
}

JSValue QuickJSBuiltinBinder::new_object_from(JSContext *ctx, const AABB &p_val) {
	return QuickJSBinder::get_context_binder(ctx)->builtin_binder.bind_builtin_object(Variant::AABB, memnew(AABB(p_val)));
}

JSValue QuickJSBuiltinBinder::new_object_from(JSContext *ctx, const Basis &p_val) {
	return QuickJSBinder::get_context_binder(ctx)->builtin_binder.bind_builtin_object(Variant::BASIS, memnew(Basis(p_val)));
}

JSValue QuickJSBuiltinBinder::new_object_from(JSContext *ctx, const PoolIntArray &p_val) {
	return QuickJSBinder::get_context_binder(ctx)->builtin_binder.bind_builtin_object(Variant::POOL_INT_ARRAY, memnew(PoolIntArray(p_val)));
}

JSValue QuickJSBuiltinBinder::new_object_from(JSContext *ctx, const PoolByteArray &p_val) {
	return QuickJSBinder::get_context_binder(ctx)->builtin_binder.bind_builtin_object(Variant::POOL_BYTE_ARRAY, memnew(PoolByteArray(p_val)));
}

JSValue QuickJSBuiltinBinder::new_object_from(JSContext *ctx, const PoolRealArray &p_val) {
	return QuickJSBinder::get_context_binder(ctx)->builtin_binder.bind_builtin_object(Variant::POOL_REAL_ARRAY, memnew(PoolRealArray(p_val)));
}

JSValue QuickJSBuiltinBinder::new_object_from(JSContext *ctx, const PoolColorArray &p_val) {
	return QuickJSBinder::get_context_binder(ctx)->builtin_binder.bind_builtin_object(Variant::POOL_COLOR_ARRAY, memnew(PoolColorArray(p_val)));
}

JSValue QuickJSBuiltinBinder::new_object_from(JSContext *ctx, const PoolStringArray &p_val) {
	return QuickJSBinder::get_context_binder(ctx)->builtin_binder.bind_builtin_object(Variant::POOL_STRING_ARRAY, memnew(PoolStringArray(p_val)));
}

JSValue QuickJSBuiltinBinder::new_object_from(JSContext *ctx, const PoolVector2Array &p_val) {
	return QuickJSBinder::get_context_binder(ctx)->builtin_binder.bind_builtin_object(Variant::POOL_VECTOR2_ARRAY, memnew(PoolVector2Array(p_val)));
}

JSValue QuickJSBuiltinBinder::new_object_from(JSContext *ctx, const PoolVector3Array &p_val) {
	return QuickJSBinder::get_context_binder(ctx)->builtin_binder.bind_builtin_object(Variant::POOL_VECTOR3_ARRAY, memnew(PoolVector3Array(p_val)));
}

void QuickJSBuiltinBinder::bind_builtin_propties_manually() {
}
