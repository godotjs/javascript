#include "quickjs_builtin_binder.h"
#include "core/color.h"
#include "core/variant.h"
#include "quickjs_binder.h"
#include <core/io/compression.h>
#include <core/os/memory.h>

QuickJSBuiltinBinder::QuickJSBuiltinBinder() {
	ctx = NULL;
	builtin_class_map = memnew_arr(BuiltinClass, Variant::VARIANT_MAX);
}

QuickJSBuiltinBinder::~QuickJSBuiltinBinder() {
	memdelete_arr(builtin_class_map);
}

void QuickJSBuiltinBinder::bind_builtin_object(JSContext *ctx, JSValue target, Variant::Type p_type, const void *p_object) {

	void *ptr = NULL;
	ECMAScriptGCHandler *bind = NULL;
	switch (p_type) {
		case Variant::VECTOR2:
			ptr = memalloc(sizeof(ECMAScriptGCHandler) + sizeof(Vector2));
			bind = memnew_placement(ptr, ECMAScriptGCHandler);
			memnew_placement(bind + 1, Vector2(*static_cast<const Vector2 *>(p_object)));
			break;
		case Variant::RECT2:
			ptr = memalloc(sizeof(ECMAScriptGCHandler) + sizeof(Rect2));
			bind = memnew_placement(ptr, ECMAScriptGCHandler);
			memnew_placement(bind + 1, Rect2(*static_cast<const Rect2 *>(p_object)));
			break;
		case Variant::COLOR:
			ptr = memalloc(sizeof(ECMAScriptGCHandler) + sizeof(Color));
			bind = memnew_placement(ptr, ECMAScriptGCHandler);
			memnew_placement(bind + 1, Color(*static_cast<const Color *>(p_object)));
			break;
		case Variant::VECTOR3:
			ptr = memalloc(sizeof(ECMAScriptGCHandler) + sizeof(Vector3));
			bind = memnew_placement(ptr, ECMAScriptGCHandler);
			memnew_placement(bind + 1, Vector3(*static_cast<const Vector3 *>(p_object)));
			break;
		case Variant::BASIS:
			ptr = memalloc(sizeof(ECMAScriptGCHandler) + sizeof(Basis));
			bind = memnew_placement(ptr, ECMAScriptGCHandler);
			memnew_placement(bind + 1, Basis(*static_cast<const Basis *>(p_object)));
			break;
		case Variant::QUAT:
			ptr = memalloc(sizeof(ECMAScriptGCHandler) + sizeof(Quat));
			bind = memnew_placement(ptr, ECMAScriptGCHandler);
			memnew_placement(bind + 1, Quat(*static_cast<const Quat *>(p_object)));
			break;
		case Variant::PLANE:
			ptr = memalloc(sizeof(ECMAScriptGCHandler) + sizeof(Plane));
			bind = memnew_placement(ptr, ECMAScriptGCHandler);
			memnew_placement(bind + 1, Plane(*static_cast<const Plane *>(p_object)));
			break;
		case Variant::TRANSFORM2D:
			ptr = memalloc(sizeof(ECMAScriptGCHandler) + sizeof(Transform2D));
			bind = memnew_placement(ptr, ECMAScriptGCHandler);
			memnew_placement(bind + 1, Transform2D(*static_cast<const Transform2D *>(p_object)));
			break;
		case Variant::_RID:
			ptr = memalloc(sizeof(ECMAScriptGCHandler) + sizeof(RID));
			bind = memnew_placement(ptr, ECMAScriptGCHandler);
			memnew_placement(bind + 1, RID(*static_cast<const RID *>(p_object)));
			break;
		case Variant::TRANSFORM:
			ptr = memalloc(sizeof(ECMAScriptGCHandler) + sizeof(Transform));
			bind = memnew_placement(ptr, ECMAScriptGCHandler);
			memnew_placement(bind + 1, Transform(*static_cast<const Transform *>(p_object)));
			break;
		case Variant::AABB:
			ptr = memalloc(sizeof(ECMAScriptGCHandler) + sizeof(AABB));
			bind = memnew_placement(ptr, ECMAScriptGCHandler);
			memnew_placement(bind + 1, AABB(*static_cast<const AABB *>(p_object)));
			break;
		case Variant::POOL_INT_ARRAY:
			ptr = memalloc(sizeof(ECMAScriptGCHandler) + sizeof(PoolIntArray));
			bind = memnew_placement(ptr, ECMAScriptGCHandler);
			memnew_placement(bind + 1, PoolIntArray(*static_cast<const PoolIntArray *>(p_object)));
			break;
		case Variant::POOL_BYTE_ARRAY:
			ptr = memalloc(sizeof(ECMAScriptGCHandler) + sizeof(PoolByteArray));
			bind = memnew_placement(ptr, ECMAScriptGCHandler);
			memnew_placement(bind + 1, PoolByteArray(*static_cast<const PoolByteArray *>(p_object)));
			break;
		case Variant::POOL_REAL_ARRAY:
			ptr = memalloc(sizeof(ECMAScriptGCHandler) + sizeof(PoolRealArray));
			bind = memnew_placement(ptr, ECMAScriptGCHandler);
			memnew_placement(bind + 1, PoolRealArray(*static_cast<const PoolRealArray *>(p_object)));
			break;
		case Variant::POOL_COLOR_ARRAY:
			ptr = memalloc(sizeof(ECMAScriptGCHandler) + sizeof(PoolColorArray));
			bind = memnew_placement(ptr, ECMAScriptGCHandler);
			memnew_placement(bind + 1, PoolColorArray(*static_cast<const PoolColorArray *>(p_object)));
			break;
		case Variant::POOL_STRING_ARRAY:
			ptr = memalloc(sizeof(ECMAScriptGCHandler) + sizeof(PoolStringArray));
			bind = memnew_placement(ptr, ECMAScriptGCHandler);
			memnew_placement(bind + 1, PoolStringArray(*static_cast<const PoolStringArray *>(p_object)));
			break;
		case Variant::POOL_VECTOR2_ARRAY:
			ptr = memalloc(sizeof(ECMAScriptGCHandler) + sizeof(PoolVector2Array));
			bind = memnew_placement(ptr, ECMAScriptGCHandler);
			memnew_placement(bind + 1, PoolVector2Array(*static_cast<const PoolVector2Array *>(p_object)));
			break;
		case Variant::POOL_VECTOR3_ARRAY:
			ptr = memalloc(sizeof(ECMAScriptGCHandler) + sizeof(PoolVector3Array));
			bind = memnew_placement(ptr, ECMAScriptGCHandler);
			memnew_placement(bind + 1, PoolVector3Array(*static_cast<const PoolVector3Array *>(p_object)));
			break;
		default:
			break;
	}
	ERR_FAIL_NULL(bind);

	bind->context = ctx;
	bind->type = p_type;
	bind->flags |= ECMAScriptGCHandler::FLAG_BUILTIN_CLASS;
	bind->godot_builtin_object_ptr = bind + 1;
	bind->ecma_object = JS_VALUE_GET_PTR(target);
	JS_SetOpaque(target, bind);
#ifdef DUMP_LEAKS
	QuickJSBinder::add_debug_binding_info(ctx, target, bind);
#endif
}

JSValue QuickJSBuiltinBinder::create_builtin_value(JSContext *ctx, Variant::Type p_type, const void *p_val) {
	QuickJSBinder *binder = QuickJSBinder::get_context_binder(ctx);
	const QuickJSBuiltinBinder::BuiltinClass &cls = binder->builtin_binder.get_class(p_type);
	JSValue obj = JS_NewObjectProtoClass(ctx, cls.class_prototype, binder->get_origin_class_id());
	bind_builtin_object(ctx, obj, p_type, p_val);
	return obj;
}

void QuickJSBuiltinBinder::builtin_finalizer(ECMAScriptGCHandler *p_bind) {
	switch (p_bind->type) {
		case Variant::POOL_BYTE_ARRAY:
			p_bind->getPoolByteArray()->~PoolVector<uint8_t>();
			break;
		case Variant::POOL_INT_ARRAY:
			p_bind->getPoolIntArray()->~PoolVector<int>();
			break;
		case Variant::POOL_REAL_ARRAY:
			p_bind->getPoolRealArray()->~PoolVector<real_t>();
			break;
		case Variant::POOL_STRING_ARRAY:
			p_bind->getPoolStringArray()->~PoolVector<String>();
			break;
		case Variant::POOL_VECTOR2_ARRAY:
			p_bind->getPoolVector2Array()->~PoolVector<Vector2>();
			break;
		case Variant::POOL_VECTOR3_ARRAY:
			p_bind->getPoolVector3Array()->~PoolVector<Vector3>();
			break;
		case Variant::POOL_COLOR_ARRAY:
			p_bind->getPoolColorArray()->~PoolVector<Color>();
			break;
		default:
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

JSValue QuickJSBuiltinBinder::new_object_from(JSContext *ctx, const Variant &p_val) {
	Variant::Type type = p_val.get_type();
	JSValue obj = JS_UNDEFINED;
	switch (type) {
		case Variant::VECTOR2: {
			Vector2 tmp = p_val;
			obj = create_builtin_value(ctx, type, &tmp);
		} break;
		case Variant::RECT2: {
			Rect2 tmp = p_val;
			obj = create_builtin_value(ctx, type, &tmp);
		} break;
		case Variant::COLOR: {
			Color tmp = p_val;
			obj = create_builtin_value(ctx, type, &tmp);
		} break;
		case Variant::VECTOR3: {
			Vector3 tmp = p_val;
			obj = create_builtin_value(ctx, type, &tmp);
		} break;
		case Variant::BASIS: {
			Basis tmp = p_val;
			obj = create_builtin_value(ctx, type, &tmp);
		} break;
		case Variant::QUAT: {
			Quat tmp = p_val;
			obj = create_builtin_value(ctx, type, &tmp);
		} break;
		case Variant::PLANE: {
			Plane tmp = p_val;
			obj = create_builtin_value(ctx, type, &tmp);
		} break;
		case Variant::TRANSFORM2D: {
			Transform2D tmp = p_val;
			obj = create_builtin_value(ctx, type, &tmp);
		} break;
		case Variant::_RID: {
			RID tmp = p_val;
			obj = create_builtin_value(ctx, type, &tmp);
		} break;
		case Variant::TRANSFORM: {
			Transform tmp = p_val;
			obj = create_builtin_value(ctx, type, &tmp);
		} break;
		case Variant::AABB: {
			AABB tmp = p_val;
			obj = create_builtin_value(ctx, type, &tmp);
		} break;
		case Variant::POOL_INT_ARRAY: {
			PoolIntArray tmp = p_val;
			obj = create_builtin_value(ctx, type, &tmp);
		} break;
		case Variant::POOL_BYTE_ARRAY: {
			PoolByteArray tmp = p_val;
			obj = create_builtin_value(ctx, type, &tmp);
		} break;
		case Variant::POOL_REAL_ARRAY: {
			PoolRealArray tmp = p_val;
			obj = create_builtin_value(ctx, type, &tmp);
		} break;
		case Variant::POOL_COLOR_ARRAY: {
			PoolColorArray tmp = p_val;
			obj = create_builtin_value(ctx, type, &tmp);
		} break;
		case Variant::POOL_STRING_ARRAY: {
			PoolStringArray tmp = p_val;
			obj = create_builtin_value(ctx, type, &tmp);
		} break;
		case Variant::POOL_VECTOR2_ARRAY: {
			PoolVector2Array tmp = p_val;
			obj = create_builtin_value(ctx, type, &tmp);
		} break;
		case Variant::POOL_VECTOR3_ARRAY: {
			PoolVector3Array tmp = p_val;
			obj = create_builtin_value(ctx, type, &tmp);
		} break;
		default:
			break;
	}
	return obj;
}

JSValue QuickJSBuiltinBinder::new_object_from(JSContext *ctx, const Vector2 &p_val) {
	return create_builtin_value(ctx, Variant::VECTOR2, &p_val);
}

JSValue QuickJSBuiltinBinder::new_object_from(JSContext *ctx, const Vector3 &p_val) {
	return create_builtin_value(ctx, Variant::VECTOR3, &p_val);
}

JSValue QuickJSBuiltinBinder::new_object_from(JSContext *ctx, const Rect2 &p_val) {
	return create_builtin_value(ctx, Variant::RECT2, &p_val);
}

JSValue QuickJSBuiltinBinder::new_object_from(JSContext *ctx, const Color &p_val) {
	return create_builtin_value(ctx, Variant::COLOR, &p_val);
}

JSValue QuickJSBuiltinBinder::new_object_from(JSContext *ctx, const Transform2D &p_val) {
	return create_builtin_value(ctx, Variant::TRANSFORM2D, &p_val);
}

JSValue QuickJSBuiltinBinder::new_object_from(JSContext *ctx, const Transform &p_val) {
	return create_builtin_value(ctx, Variant::TRANSFORM, &p_val);
}

JSValue QuickJSBuiltinBinder::new_object_from(JSContext *ctx, const Quat &p_val) {
	return create_builtin_value(ctx, Variant::QUAT, &p_val);
}

JSValue QuickJSBuiltinBinder::new_object_from(JSContext *ctx, const Plane &p_val) {
	return create_builtin_value(ctx, Variant::PLANE, &p_val);
}

JSValue QuickJSBuiltinBinder::new_object_from(JSContext *ctx, const RID &p_val) {
	return create_builtin_value(ctx, Variant::_RID, &p_val);
}

JSValue QuickJSBuiltinBinder::new_object_from(JSContext *ctx, const AABB &p_val) {
	return create_builtin_value(ctx, Variant::AABB, &p_val);
}

JSValue QuickJSBuiltinBinder::new_object_from(JSContext *ctx, const Basis &p_val) {
	return create_builtin_value(ctx, Variant::BASIS, &p_val);
}

JSValue QuickJSBuiltinBinder::new_object_from(JSContext *ctx, const PoolIntArray &p_val) {
	return create_builtin_value(ctx, Variant::POOL_INT_ARRAY, &p_val);
}

JSValue QuickJSBuiltinBinder::new_object_from(JSContext *ctx, const PoolByteArray &p_val) {
	return create_builtin_value(ctx, Variant::POOL_BYTE_ARRAY, &p_val);
}

JSValue QuickJSBuiltinBinder::new_object_from(JSContext *ctx, const PoolRealArray &p_val) {
	return create_builtin_value(ctx, Variant::POOL_REAL_ARRAY, &p_val);
}

JSValue QuickJSBuiltinBinder::new_object_from(JSContext *ctx, const PoolColorArray &p_val) {
	return create_builtin_value(ctx, Variant::POOL_COLOR_ARRAY, &p_val);
}

JSValue QuickJSBuiltinBinder::new_object_from(JSContext *ctx, const PoolStringArray &p_val) {
	return create_builtin_value(ctx, Variant::POOL_STRING_ARRAY, &p_val);
}

JSValue QuickJSBuiltinBinder::new_object_from(JSContext *ctx, const PoolVector2Array &p_val) {
	return create_builtin_value(ctx, Variant::POOL_VECTOR2_ARRAY, &p_val);
}

JSValue QuickJSBuiltinBinder::new_object_from(JSContext *ctx, const PoolVector3Array &p_val) {
	return create_builtin_value(ctx, Variant::POOL_VECTOR3_ARRAY, &p_val);
}

void QuickJSBuiltinBinder::bind_builtin_propties_manually() {

	{ // Color
		JSCFunctionMagic *getter = [](JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv, int magic) -> JSValue {
			ECMAScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, this_val);
			const Color *ptr = bind->getColor();
			switch (magic) {
				case 0:
					return QuickJSBinder::to_js_number(ctx, Math::round(ptr->r * 255));
				case 1:
					return QuickJSBinder::to_js_number(ctx, Math::round(ptr->g * 255));
				case 2:
					return QuickJSBinder::to_js_number(ctx, Math::round(ptr->b * 255));
				case 3:
					return QuickJSBinder::to_js_number(ctx, Math::round(ptr->a * 255));
				case 4:
					return QuickJSBinder::to_js_number(ctx, ptr->get_h());
				case 5:
					return QuickJSBinder::to_js_number(ctx, ptr->get_s());
				case 6:
					return QuickJSBinder::to_js_number(ctx, ptr->get_v());
			}
			return JS_UNDEFINED;
		};

		JSCFunctionMagic *setter = [](JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv, int magic) -> JSValue {
			ECMAScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, this_val);
			Color *ptr = bind->getColor();
#ifdef DEBUG_METHODS_ENABLED
			ERR_FAIL_COND_V(!QuickJSBinder::validate_type(ctx, Variant::REAL, argv[0]), (JS_ThrowTypeError(ctx, "number value expected")));
#endif
			real_t value = QuickJSBinder::js_to_number(ctx, argv[0]);
			switch (magic) {
				case 0:
					ptr->r = value / 255.0;
					break;
				case 1:
					ptr->g = value / 255.0;
					break;
				case 2:
					ptr->b = value / 255.0;
					break;
				case 3:
					ptr->a = value / 255.0;
					break;
				case 4:
					ptr->set_hsv(value, ptr->get_s(), ptr->get_v(), ptr->a);
					break;
				case 5:
					ptr->set_hsv(ptr->get_h(), value, ptr->get_v(), ptr->a);
					break;
				case 6:
					ptr->set_hsv(ptr->get_h(), ptr->get_s(), value, ptr->a);
					break;
			}
			return JS_DupValue(ctx, argv[0]);
		};
		binder->get_builtin_binder().register_property(Variant::COLOR, "r8", getter, setter, 0);
		binder->get_builtin_binder().register_property(Variant::COLOR, "g8", getter, setter, 1);
		binder->get_builtin_binder().register_property(Variant::COLOR, "b8", getter, setter, 2);
		binder->get_builtin_binder().register_property(Variant::COLOR, "a8", getter, setter, 3);
		binder->get_builtin_binder().register_property(Variant::COLOR, "h", getter, setter, 4);
		binder->get_builtin_binder().register_property(Variant::COLOR, "s", getter, setter, 5);
		binder->get_builtin_binder().register_property(Variant::COLOR, "v", getter, setter, 6);
	}

	{ // Rect2
		JSCFunctionMagic *getter = [](JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv, int magic) -> JSValue {
			ECMAScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, this_val);
			const Rect2 *ptr = bind->getRect2();
			switch (magic) {
				case 0:
					return QuickJSBinder::variant_to_var(ctx, ptr->size + ptr->position);
			}
			return JS_UNDEFINED;
		};
		JSCFunctionMagic *setter = [](JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv, int magic) -> JSValue {
			ECMAScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, this_val);
			Rect2 *ptr = bind->getRect2();
			switch (magic) {
				case 0:
#ifdef DEBUG_METHODS_ENABLED
					ERR_FAIL_COND_V(!QuickJSBinder::validate_type(ctx, Variant::VECTOR2, argv[0]), (JS_ThrowTypeError(ctx, "Vector2 expected for Rect2.end")));
#endif
					ptr->size = Vector2(QuickJSBinder::var_to_variant(ctx, argv[0])) - ptr->position;
					break;
			}
			return JS_DupValue(ctx, argv[0]);
		};
		binder->get_builtin_binder().register_property(Variant::RECT2, "end", getter, setter, 0);

		binder->get_builtin_binder().register_method(
				Variant::RECT2,
				"grow_margin",
				[](JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
#ifdef DEBUG_METHODS_ENABLED
					ERR_FAIL_COND_V(argc < 2, (JS_ThrowTypeError(ctx, "Two arguments expected for Rect2.grow_margin")));
#endif
					ECMAScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, this_val);
					Rect2 *ptr = bind->getRect2();
#ifdef DEBUG_METHODS_ENABLED
					ERR_FAIL_COND_V(!QuickJSBinder::validate_type(ctx, Variant::INT, argv[0]), (JS_ThrowTypeError(ctx, "number expected for argument 0 of Rect2.grow_margin")));
					ERR_FAIL_COND_V(!QuickJSBinder::validate_type(ctx, Variant::REAL, argv[0]), (JS_ThrowTypeError(ctx, "number expected for argument 1 of Rect2.grow_margin")));
#endif
					Rect2 ret = ptr->grow_margin(Margin(QuickJSBinder::js_to_int(ctx, argv[0])), QuickJSBinder::js_to_number(ctx, argv[1]));
					return QuickJSBinder::variant_to_var(ctx, ret);
				},
				2);
	}

	{ // Transform2D
		binder->get_builtin_binder().register_method(
				Variant::TRANSFORM2D,
				"xform",
				[](JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
					ERR_FAIL_COND_V(argc < 1, (JS_ThrowTypeError(ctx, "Argument expected for Transform2D.xform")));
					ECMAScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, this_val);
					Transform2D *ptr = bind->getTransform2D();
					if (QuickJSBinder::validate_type(ctx, Variant::VECTOR2, argv[0])) {
						Vector2 ret = ptr->xform(Vector2(QuickJSBinder::var_to_variant(ctx, argv[0])));
						return QuickJSBinder::variant_to_var(ctx, ret);
					} else if (QuickJSBinder::validate_type(ctx, Variant::RECT2, argv[0])) {
						Rect2 ret = ptr->xform(Rect2(QuickJSBinder::var_to_variant(ctx, argv[0])));
						return QuickJSBinder::variant_to_var(ctx, ret);
					} else if (QuickJSBinder::validate_type(ctx, Variant::POOL_VECTOR2_ARRAY, argv[0])) {
						PoolVector2Array ret = ptr->xform(PoolVector2Array(QuickJSBinder::var_to_variant(ctx, argv[0])));
						return QuickJSBinder::variant_to_var(ctx, ret);
					}
					ERR_FAIL_V(JS_ThrowTypeError(ctx, "Vector2, Rect2 or PoolVector2Array expected for argument #0 of Transform2D.xform"));
				},
				1);
		binder->get_builtin_binder().register_method(
				Variant::TRANSFORM2D,
				"xform_inv",
				[](JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
					ERR_FAIL_COND_V(argc < 1, (JS_ThrowTypeError(ctx, "Argument expected for Transform2D.xform_inv")));
					ECMAScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, this_val);
					Transform2D *ptr = bind->getTransform2D();
					if (QuickJSBinder::validate_type(ctx, Variant::VECTOR2, argv[0])) {
						Vector2 ret = ptr->xform_inv(Vector2(QuickJSBinder::var_to_variant(ctx, argv[0])));
						return QuickJSBinder::variant_to_var(ctx, ret);
					} else if (QuickJSBinder::validate_type(ctx, Variant::RECT2, argv[0])) {
						Rect2 ret = ptr->xform_inv(Rect2(QuickJSBinder::var_to_variant(ctx, argv[0])));
						return QuickJSBinder::variant_to_var(ctx, ret);
					} else if (QuickJSBinder::validate_type(ctx, Variant::POOL_VECTOR2_ARRAY, argv[0])) {
						PoolVector2Array ret = ptr->xform_inv(PoolVector2Array(QuickJSBinder::var_to_variant(ctx, argv[0])));
						return QuickJSBinder::variant_to_var(ctx, ret);
					}
					ERR_FAIL_V(JS_ThrowTypeError(ctx, "Vector2, Rect2 or PoolVector2Array expected for argument #0 of Transform2D.xform_inv"));
				},
				1);
	}

	{ // Basis
		binder->get_builtin_binder().register_method(
				Variant::BASIS,
				"is_equal_approx",
				[](JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
					ERR_FAIL_COND_V(argc < 1, (JS_ThrowTypeError(ctx, "Argument expected for Basis.is_equal_approx")));
					ECMAScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, this_val);
					Basis *ptr = bind->getBasis();
#ifdef DEBUG_METHODS_ENABLED
					ERR_FAIL_COND_V(!QuickJSBinder::validate_type(ctx, Variant::BASIS, argv[0]), (JS_ThrowTypeError(ctx, "Basis expected for Basis.is_equal_approx")));
#endif
					ECMAScriptGCHandler *argv_bind = BINDING_DATA_FROM_JS(ctx, argv[0]);
					bool ret = ptr->is_equal_approx(*argv_bind->getBasis());
					return QuickJSBinder::to_js_bool(ctx, ret);
				},
				1);
	}

	{ // AABB
		JSCFunctionMagic *getter = [](JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv, int magic) -> JSValue {
			ECMAScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, this_val);
			const AABB *ptr = bind->getAABB();
			switch (magic) {
				case 0:
					return QuickJSBinder::variant_to_var(ctx, ptr->size + ptr->position);
			}
			return JS_UNDEFINED;
		};
		JSCFunctionMagic *setter = [](JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv, int magic) -> JSValue {
			ECMAScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, this_val);
			AABB *ptr = bind->getAABB();
			switch (magic) {
				case 0:
#ifdef DEBUG_METHODS_ENABLED
					ERR_FAIL_COND_V(!QuickJSBinder::validate_type(ctx, Variant::VECTOR3, argv[0]), (JS_ThrowTypeError(ctx, "Vector3 expected for AABB.end")));
#endif
					ptr->size = Vector3(QuickJSBinder::var_to_variant(ctx, argv[0])) - ptr->position;
					break;
			}
			return JS_DupValue(ctx, argv[0]);
		};
		binder->get_builtin_binder().register_property(Variant::AABB, "end", getter, setter, 0);
	}

	{ // Plane
		binder->get_builtin_binder().register_method(
				Variant::PLANE,
				"intersect_3",
				[](JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
					ERR_FAIL_COND_V(argc < 2, (JS_ThrowTypeError(ctx, "Two arguments expected for Plane.intersect_3")));
					ECMAScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, this_val);
					Plane *ptr = bind->getPlane();
#ifdef DEBUG_METHODS_ENABLED
					ERR_FAIL_COND_V(!QuickJSBinder::validate_type(ctx, Variant::PLANE, argv[0]), (JS_ThrowTypeError(ctx, "Plane expected for arguments #0 Plane.intersect_3")));
					ERR_FAIL_COND_V(!QuickJSBinder::validate_type(ctx, Variant::PLANE, argv[1]), (JS_ThrowTypeError(ctx, "Plane expected for arguments #1 Plane.intersect_3")));
#endif
					Vector3 ret;
					ECMAScriptGCHandler *argv0_bind = BINDING_DATA_FROM_JS(ctx, argv[0]);
					ECMAScriptGCHandler *argv1_bind = BINDING_DATA_FROM_JS(ctx, argv[1]);
					if (ptr->intersect_3(*argv0_bind->getPlane(), *argv1_bind->getPlane(), &ret)) {
						return QuickJSBinder::variant_to_var(ctx, ret);
					}
					return JS_NULL;
				},
				2);
		binder->get_builtin_binder().register_method(
				Variant::PLANE,
				"intersects_ray",
				[](JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
					ERR_FAIL_COND_V(argc < 2, (JS_ThrowTypeError(ctx, "Two arguments expected for Plane.intersects_ray")));
					ECMAScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, this_val);
					Plane *ptr = bind->getPlane();
#ifdef DEBUG_METHODS_ENABLED
					ERR_FAIL_COND_V(!QuickJSBinder::validate_type(ctx, Variant::VECTOR3, argv[0]), (JS_ThrowTypeError(ctx, "Vector3 expected for arguments #0 Plane.intersects_ray")));
					ERR_FAIL_COND_V(!QuickJSBinder::validate_type(ctx, Variant::VECTOR3, argv[1]), (JS_ThrowTypeError(ctx, "Vector3 expected for arguments #1 Plane.intersects_ray")));
#endif
					Vector3 ret;
					ECMAScriptGCHandler *argv0_bind = BINDING_DATA_FROM_JS(ctx, argv[0]);
					ECMAScriptGCHandler *argv1_bind = BINDING_DATA_FROM_JS(ctx, argv[1]);
					if (ptr->intersects_ray(*argv0_bind->getVector3(), *argv1_bind->getVector3(), &ret)) {
						return QuickJSBinder::variant_to_var(ctx, ret);
					}
					return JS_NULL;
				},
				2);
		binder->get_builtin_binder().register_method(
				Variant::PLANE,
				"intersects_segment",
				[](JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
					ERR_FAIL_COND_V(argc < 2, (JS_ThrowTypeError(ctx, "Two arguments expected for Plane.intersects_segment")));
					ECMAScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, this_val);
					Plane *ptr = bind->getPlane();
#ifdef DEBUG_METHODS_ENABLED
					ERR_FAIL_COND_V(!QuickJSBinder::validate_type(ctx, Variant::VECTOR3, argv[0]), (JS_ThrowTypeError(ctx, "Vector3 expected for arguments #0 Plane.intersects_segment")));
					ERR_FAIL_COND_V(!QuickJSBinder::validate_type(ctx, Variant::VECTOR3, argv[1]), (JS_ThrowTypeError(ctx, "Vector3 expected for arguments #1 Plane.intersects_segment")));
#endif
					Vector3 ret;
					ECMAScriptGCHandler *argv0_bind = BINDING_DATA_FROM_JS(ctx, argv[0]);
					ECMAScriptGCHandler *argv1_bind = BINDING_DATA_FROM_JS(ctx, argv[1]);
					if (ptr->intersects_segment(*argv0_bind->getVector3(), *argv1_bind->getVector3(), &ret)) {
						return QuickJSBinder::variant_to_var(ctx, ret);
					}
					return JS_NULL;
				},
				2);
	}
	{ // Transform
		binder->get_builtin_binder().register_method(
				Variant::TRANSFORM,
				"xform",
				[](JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
					ERR_FAIL_COND_V(argc < 1, (JS_ThrowTypeError(ctx, "Argument expected for Transform.xform")));
					ECMAScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, this_val);
					Transform *ptr = bind->getTransform();
					if (QuickJSBinder::validate_type(ctx, Variant::VECTOR3, argv[0])) {
						Vector3 ret = ptr->xform(Vector3(QuickJSBinder::var_to_variant(ctx, argv[0])));
						return QuickJSBinder::variant_to_var(ctx, ret);
					} else if (QuickJSBinder::validate_type(ctx, Variant::PLANE, argv[0])) {
						Plane ret = ptr->xform(Plane(QuickJSBinder::var_to_variant(ctx, argv[0])));
						return QuickJSBinder::variant_to_var(ctx, ret);
					} else if (QuickJSBinder::validate_type(ctx, Variant::AABB, argv[0])) {
						AABB ret = ptr->xform(AABB(QuickJSBinder::var_to_variant(ctx, argv[0])));
						return QuickJSBinder::variant_to_var(ctx, ret);
					} else if (QuickJSBinder::validate_type(ctx, Variant::POOL_VECTOR3_ARRAY, argv[0])) {
						PoolVector3Array ret = ptr->xform(PoolVector3Array(QuickJSBinder::var_to_variant(ctx, argv[0])));
						return QuickJSBinder::variant_to_var(ctx, ret);
					}
					ERR_FAIL_V(JS_ThrowTypeError(ctx, "Vector3, Plane, AABB or PoolVector3Array expected for argument #0 of Transform.xform"));
				},
				1);
		binder->get_builtin_binder().register_method(
				Variant::TRANSFORM,
				"xform_inv",
				[](JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
					ERR_FAIL_COND_V(argc < 1, (JS_ThrowTypeError(ctx, "Argument expected for Transform.xform_inv")));
					ECMAScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, this_val);
					Transform *ptr = bind->getTransform();
					if (QuickJSBinder::validate_type(ctx, Variant::VECTOR3, argv[0])) {
						Vector3 ret = ptr->xform_inv(Vector3(QuickJSBinder::var_to_variant(ctx, argv[0])));
						return QuickJSBinder::variant_to_var(ctx, ret);
					} else if (QuickJSBinder::validate_type(ctx, Variant::PLANE, argv[0])) {
						Plane ret = ptr->xform_inv(Plane(QuickJSBinder::var_to_variant(ctx, argv[0])));
						return QuickJSBinder::variant_to_var(ctx, ret);
					} else if (QuickJSBinder::validate_type(ctx, Variant::AABB, argv[0])) {
						AABB ret = ptr->xform_inv(AABB(QuickJSBinder::var_to_variant(ctx, argv[0])));
						return QuickJSBinder::variant_to_var(ctx, ret);
					} else if (QuickJSBinder::validate_type(ctx, Variant::POOL_VECTOR3_ARRAY, argv[0])) {
						PoolVector3Array ret = ptr->xform_inv(PoolVector3Array(QuickJSBinder::var_to_variant(ctx, argv[0])));
						return QuickJSBinder::variant_to_var(ctx, ret);
					}
					ERR_FAIL_V(JS_ThrowTypeError(ctx, "Vector3, Plane, AABB or PoolVector3Array expected for argument #0 of Transform.xform_inv"));
				},
				1);
	}

	{ // PoolByteArray
		// PoolByteArray.prototype.compress
		binder->get_builtin_binder().register_method(
				Variant::POOL_BYTE_ARRAY,
				"compress",
				[](JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
					ECMAScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, this_val);
					PoolByteArray *ptr = bind->getPoolByteArray();
					PoolByteArray compressed;
					if (ptr->size() > 0) {
#ifdef DEBUG_METHODS_ENABLED
						ERR_FAIL_COND_V(!QuickJSBinder::validate_type(ctx, Variant::INT, argv[0]), (JS_ThrowTypeError(ctx, "number expected for argument 0 of PoolByteArray.compress")));
#endif
						Compression::Mode mode = (Compression::Mode)(QuickJSBinder::js_to_int(ctx, argv[0]));
						compressed.resize(Compression::get_max_compressed_buffer_size(ptr->size(), mode));
						int result = Compression::compress(compressed.write().ptr(), ptr->read().ptr(), ptr->size(), mode);
						result = result >= 0 ? result : 0;
						compressed.resize(result);
					}
					return QuickJSBinder::variant_to_var(ctx, compressed);
				},
				1);
		// PoolByteArray.prototype.decompress
		binder->get_builtin_binder().register_method(
				Variant::POOL_BYTE_ARRAY,
				"decompress",
				[](JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
					ECMAScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, this_val);
					PoolByteArray *ptr = bind->getPoolByteArray();
					PoolByteArray decompressed;
#ifdef DEBUG_METHODS_ENABLED
					ERR_FAIL_COND_V(!QuickJSBinder::validate_type(ctx, Variant::INT, argv[0]), (JS_ThrowTypeError(ctx, "number expected for argument 0 of PoolByteArray.decompress")));
#endif
					int buffer_size = QuickJSBinder::js_to_int(ctx, argv[0]);
					ERR_FAIL_COND_V(buffer_size <= 0, (JS_ThrowTypeError(ctx, "Decompression buffer size must be greater than zero.")));

#ifdef DEBUG_METHODS_ENABLED
					ERR_FAIL_COND_V(!QuickJSBinder::validate_type(ctx, Variant::INT, argv[0]), (JS_ThrowTypeError(ctx, "number expected for argument 1 of PoolByteArray.decompress")));
#endif
					Compression::Mode mode = (Compression::Mode)(QuickJSBinder::js_to_int(ctx, argv[1]));
					decompressed.resize(buffer_size);
					int result = Compression::decompress(decompressed.write().ptr(), buffer_size, ptr->read().ptr(), ptr->size(), mode);
					result = result >= 0 ? result : 0;
					decompressed.resize(result);
					return QuickJSBinder::variant_to_var(ctx, decompressed);
				},
				1);
		// PoolByteArray.prototype.get_string_from_utf8
		binder->get_builtin_binder().register_method(
				Variant::POOL_BYTE_ARRAY,
				"get_string_from_utf8",
				[](JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
					ECMAScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, this_val);
					PoolByteArray *ptr = bind->getPoolByteArray();
					String ret;
					if (ptr->size() > 0) {
						PoolByteArray::Read r = ptr->read();
						ret.parse_utf8((const char *)r.ptr(), ptr->size());
					}
					return QuickJSBinder::to_js_string(ctx, ret);
				},
				0);
		// PoolByteArray.prototype.get_string_from_ascii
		binder->get_builtin_binder().register_method(
				Variant::POOL_BYTE_ARRAY,
				"get_string_from_ascii",
				[](JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
					ECMAScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, this_val);
					PoolByteArray *ptr = bind->getPoolByteArray();
					String ret;
					if (ptr->size() > 0) {
						PoolByteArray::Read r = ptr->read();
						CharString cs;
						cs.resize(ptr->size() + 1);
						memcpy(cs.ptrw(), r.ptr(), ptr->size());
						cs[ptr->size()] = 0;
						ret = cs.get_data();
					}
					return QuickJSBinder::to_js_string(ctx, ret);
				},
				0);
		// PoolByteArray.prototype.hex_encode
		binder->get_builtin_binder().register_method(
				Variant::POOL_BYTE_ARRAY,
				"hex_encode",
				[](JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
					ECMAScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, this_val);
					PoolByteArray *ptr = bind->getPoolByteArray();
					String ret;
					if (ptr->size() > 0) {
						PoolByteArray::Read r = ptr->read();
						ret = String::hex_encode_buffer(&r[0], ptr->size());
					}
					return QuickJSBinder::to_js_string(ctx, ret);
				},
				0);
		// PoolByteArray.prototype.get_buffer
		binder->get_builtin_binder().register_method(
				Variant::POOL_BYTE_ARRAY,
				"get_buffer",
				[](JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
					ECMAScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, this_val);
					PoolByteArray *array = memnew(PoolByteArray(*bind->getPoolByteArray()));
					JSValue ret = JS_NewArrayBuffer(
							ctx, const_cast<uint8_t *>(array->read().ptr()), array->size(), [](JSRuntime *rt, void *opaque, void *ptr) {
								memdelete(static_cast<PoolByteArray *>(opaque));
							},
							array, false);
					return ret;
				},
				0);
	}
	{
		// PoolIntArray.prototype.get_buffer
		binder->get_builtin_binder().register_method(
				Variant::POOL_INT_ARRAY,
				"get_buffer",
				[](JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
					ECMAScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, this_val);
					PoolIntArray *array = memnew(PoolIntArray(*bind->getPoolIntArray()));
					JSValue ret = JS_NewArrayBuffer(
							ctx, (uint8_t *)(array->read().ptr()), array->size() * sizeof(int), [](JSRuntime *rt, void *opaque, void *ptr) {
								memdelete(static_cast<PoolIntArray *>(opaque));
							},
							array, false);
					return ret;
				},
				0);
	}
	{
		// PoolRealArray.prototype.get_buffer
		binder->get_builtin_binder().register_method(
				Variant::POOL_REAL_ARRAY,
				"get_buffer",
				[](JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
					ECMAScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, this_val);
					PoolRealArray *array = memnew(PoolRealArray(*bind->getPoolRealArray()));
					JSValue ret = JS_NewArrayBuffer(
							ctx, (uint8_t *)(array->read().ptr()), array->size() * sizeof(real_t), [](JSRuntime *rt, void *opaque, void *ptr) {
								memdelete(static_cast<PoolRealArray *>(opaque));
							},
							array, false);
					return ret;
				},
				0);
	}
	{
		// PoolVector2Array.prototype.get_buffer
		binder->get_builtin_binder().register_method(
				Variant::POOL_VECTOR2_ARRAY,
				"get_buffer",
				[](JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
					ECMAScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, this_val);
					PoolVector2Array *array = memnew(PoolVector2Array(*bind->getPoolVector2Array()));
					JSValue ret = JS_NewArrayBuffer(
							ctx, (uint8_t *)(array->read().ptr()), array->size() * sizeof(Vector2), [](JSRuntime *rt, void *opaque, void *ptr) {
								memdelete(static_cast<PoolVector2Array *>(opaque));
							},
							array, false);
					return ret;
				},
				0);
	}

	{
		// PoolVector3Array.prototype.get_buffer
		binder->get_builtin_binder().register_method(
				Variant::POOL_VECTOR3_ARRAY,
				"get_buffer",
				[](JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
					ECMAScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, this_val);
					PoolVector3Array *array = memnew(PoolVector3Array(*bind->getPoolVector3Array()));
					JSValue ret = JS_NewArrayBuffer(
							ctx, (uint8_t *)(array->read().ptr()), array->size() * sizeof(Vector3), [](JSRuntime *rt, void *opaque, void *ptr) {
								memdelete(static_cast<PoolVector3Array *>(opaque));
							},
							array, false);
					return ret;
				},
				0);
	}

	{
		// PoolColorArray.prototype.get_buffer
		binder->get_builtin_binder().register_method(
				Variant::POOL_COLOR_ARRAY,
				"get_buffer",
				[](JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
					ECMAScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, this_val);
					PoolColorArray *array = memnew(PoolColorArray(*bind->getPoolColorArray()));
					JSValue ret = JS_NewArrayBuffer(
							ctx, (uint8_t *)(array->read().ptr()), array->size() * sizeof(Color), [](JSRuntime *rt, void *opaque, void *ptr) {
								memdelete(static_cast<PoolColorArray *>(opaque));
							},
							array, false);
					return ret;
				},
				0);
	}
}
