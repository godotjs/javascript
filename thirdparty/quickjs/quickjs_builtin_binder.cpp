#include "quickjs_builtin_binder.h"
#include "core/math/color.h"
#include "core/variant/variant.h"
#include "quickjs_binder.h"
#include <core/io/compression.h>
#include <core/os/memory.h>
#include <cstring>

QuickJSBuiltinBinder::QuickJSBuiltinBinder() {
	ctx = NULL;
	builtin_class_map = memnew_arr(BuiltinClass, Variant::VARIANT_MAX);
}

QuickJSBuiltinBinder::~QuickJSBuiltinBinder() {
	memdelete_arr(builtin_class_map);
}

void QuickJSBuiltinBinder::bind_builtin_object(JSContext *ctx, JSValue target, Variant::Type p_type, const void *p_object) {

	void *ptr = NULL;
	JavaScriptGCHandler *bind = NULL;
	switch (p_type) {
		case Variant::VECTOR2:
			ptr = memalloc(sizeof(JavaScriptGCHandler) + sizeof(Vector2));
			bind = memnew_placement(ptr, JavaScriptGCHandler);
			memnew_placement(bind + 1, Vector2(*static_cast<const Vector2 *>(p_object)));
			break;
		case Variant::RECT2:
			ptr = memalloc(sizeof(JavaScriptGCHandler) + sizeof(Rect2));
			bind = memnew_placement(ptr, JavaScriptGCHandler);
			memnew_placement(bind + 1, Rect2(*static_cast<const Rect2 *>(p_object)));
			break;
		case Variant::COLOR:
			ptr = memalloc(sizeof(JavaScriptGCHandler) + sizeof(Color));
			bind = memnew_placement(ptr, JavaScriptGCHandler);
			memnew_placement(bind + 1, Color(*static_cast<const Color *>(p_object)));
			break;
		case Variant::VECTOR3:
			ptr = memalloc(sizeof(JavaScriptGCHandler) + sizeof(Vector3));
			bind = memnew_placement(ptr, JavaScriptGCHandler);
			memnew_placement(bind + 1, Vector3(*static_cast<const Vector3 *>(p_object)));
			break;
		case Variant::BASIS:
			ptr = memalloc(sizeof(JavaScriptGCHandler) + sizeof(Basis));
			bind = memnew_placement(ptr, JavaScriptGCHandler);
			memnew_placement(bind + 1, Basis(*static_cast<const Basis *>(p_object)));
			break;
		case Variant::QUATERNION:
			ptr = memalloc(sizeof(JavaScriptGCHandler) + sizeof(Quaternion));
			bind = memnew_placement(ptr, JavaScriptGCHandler);
			memnew_placement(bind + 1, Quaternion(*static_cast<const Quaternion *>(p_object)));
			break;
		case Variant::PLANE:
			ptr = memalloc(sizeof(JavaScriptGCHandler) + sizeof(Plane));
			bind = memnew_placement(ptr, JavaScriptGCHandler);
			memnew_placement(bind + 1, Plane(*static_cast<const Plane *>(p_object)));
			break;
		case Variant::TRANSFORM2D:
			ptr = memalloc(sizeof(JavaScriptGCHandler) + sizeof(Transform2D));
			bind = memnew_placement(ptr, JavaScriptGCHandler);
			memnew_placement(bind + 1, Transform2D(*static_cast<const Transform2D *>(p_object)));
			break;
		case Variant::RID:
			ptr = memalloc(sizeof(JavaScriptGCHandler) + sizeof(RID));
			bind = memnew_placement(ptr, JavaScriptGCHandler);
			memnew_placement(bind + 1, RID(*static_cast<const RID *>(p_object)));
			break;
		case Variant::TRANSFORM3D:
			ptr = memalloc(sizeof(JavaScriptGCHandler) + sizeof(Transform3D));
			bind = memnew_placement(ptr, JavaScriptGCHandler);
			memnew_placement(bind + 1, Transform3D(*static_cast<const Transform3D *>(p_object)));
			break;
		case Variant::AABB:
			ptr = memalloc(sizeof(JavaScriptGCHandler) + sizeof(AABB));
			bind = memnew_placement(ptr, JavaScriptGCHandler);
			memnew_placement(bind + 1, AABB(*static_cast<const AABB *>(p_object)));
			break;
		case Variant::PACKED_INT32_ARRAY:
			ptr = memalloc(sizeof(JavaScriptGCHandler) + sizeof(PackedInt32Array));
			bind = memnew_placement(ptr, JavaScriptGCHandler);
			memnew_placement(bind + 1, PackedInt32Array(*static_cast<const PackedInt32Array *>(p_object)));
			break;
		case Variant::PACKED_BYTE_ARRAY:
			ptr = memalloc(sizeof(JavaScriptGCHandler) + sizeof(PackedByteArray));
			bind = memnew_placement(ptr, JavaScriptGCHandler);
			memnew_placement(bind + 1, PackedByteArray(*static_cast<const PackedByteArray *>(p_object)));
			break;
		case Variant::PACKED_FLOAT32_ARRAY:
			ptr = memalloc(sizeof(JavaScriptGCHandler) + sizeof(PackedFloat32Array));
			bind = memnew_placement(ptr, JavaScriptGCHandler);
			memnew_placement(bind + 1, PackedFloat32Array(*static_cast<const PackedFloat32Array *>(p_object)));
			break;
		case Variant::PACKED_COLOR_ARRAY:
			ptr = memalloc(sizeof(JavaScriptGCHandler) + sizeof(PackedColorArray));
			bind = memnew_placement(ptr, JavaScriptGCHandler);
			memnew_placement(bind + 1, PackedColorArray(*static_cast<const PackedColorArray *>(p_object)));
			break;
		case Variant::PACKED_STRING_ARRAY:
			ptr = memalloc(sizeof(JavaScriptGCHandler) + sizeof(PackedStringArray));
			bind = memnew_placement(ptr, JavaScriptGCHandler);
			memnew_placement(bind + 1, PackedStringArray(*static_cast<const PackedStringArray *>(p_object)));
			break;
		case Variant::PACKED_VECTOR2_ARRAY:
			ptr = memalloc(sizeof(JavaScriptGCHandler) + sizeof(PackedVector2Array));
			bind = memnew_placement(ptr, JavaScriptGCHandler);
			memnew_placement(bind + 1, PackedVector2Array(*static_cast<const PackedVector2Array *>(p_object)));
			break;
		case Variant::PACKED_VECTOR3_ARRAY:
			ptr = memalloc(sizeof(JavaScriptGCHandler) + sizeof(PackedVector3Array));
			bind = memnew_placement(ptr, JavaScriptGCHandler);
			memnew_placement(bind + 1, PackedVector3Array(*static_cast<const PackedVector3Array *>(p_object)));
			break;
		default:
			break;
	}
	ERR_FAIL_NULL(bind);

	bind->context = ctx;
	bind->type = p_type;
	bind->flags |= JavaScriptGCHandler::FLAG_BUILTIN_CLASS;
	bind->godot_builtin_object_ptr = bind + 1;
	bind->javascript_object = JS_VALUE_GET_PTR(target);
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

void QuickJSBuiltinBinder::builtin_finalizer(JavaScriptGCHandler *p_bind) {
	switch (p_bind->type) {
		case Variant::PACKED_BYTE_ARRAY:
			p_bind->getPackedByteArray()->~PackedByteArray();
			break;
		case Variant::PACKED_INT32_ARRAY:
			p_bind->getPackedInt32Array()->~PackedInt32Array();
			break;
		case Variant::PACKED_INT64_ARRAY:
			p_bind->getPackedInt64Array()->~PackedInt64Array();
			break;
		case Variant::PACKED_FLOAT32_ARRAY:
			p_bind->getPackedFloat32Array()->~PackedFloat32Array();
			break;
		case Variant::PACKED_FLOAT64_ARRAY:
			p_bind->getPackedFloat64Array()->~PackedFloat64Array();
			break;
		case Variant::PACKED_STRING_ARRAY:
			p_bind->getPackedStringArray()->~PackedStringArray();
			break;
		case Variant::PACKED_VECTOR2_ARRAY:
			p_bind->getPackedVector2Array()->~PackedVector2Array();
			break;
		case Variant::PACKED_VECTOR3_ARRAY:
			p_bind->getPackedVector3Array()->~PackedVector3Array();
			break;
		case Variant::PACKED_COLOR_ARRAY:
			p_bind->getPackedColorArray()->~PackedColorArray();
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
								JavaScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, argv[1]);
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
								JavaScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, argv[0]);
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
								JavaScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, argv[0]);
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
								JavaScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, argv[1]);
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
								JavaScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, argv[0]);
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
								JavaScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, argv[0]);
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
								JavaScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, argv[1]);
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
								JavaScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, argv[0]);
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
								JavaScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, argv[0]);
								Color *ptr = bind->getColor();
								real_t scalar = QuickJSBinder::js_to_number(ctx, argv[1]);
								Color ret = ptr->operator/(scalar);
								return QuickJSBuiltinBinder::new_object_from(ctx, ret);
							},
							"divide_number_right", 2),
					QuickJSBinder::PROP_DEF_DEFAULT);
		} break;
		case Variant::QUATERNION: {
			JSValue number_left = JS_NewObject(ctx);
			JS_DefinePropertyValueStr(ctx, number_left, "left", JS_DupValue(ctx, Number), QuickJSBinder::PROP_DEF_DEFAULT);
			r_operators.push_back(number_left);

			JSValue number_right = JS_NewObject(ctx);
			JS_DefinePropertyValueStr(ctx, number_right, "right", JS_DupValue(ctx, Number), QuickJSBinder::PROP_DEF_DEFAULT);
			r_operators.push_back(number_right);

			// 2 * new godot.Quaternion(0.5, 0.5, 0.5, 0.5)
			JS_DefinePropertyValueStr(ctx, number_left, "*",
					JS_NewCFunction(
							ctx, [](JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) -> JSValue {
								real_t scalar = QuickJSBinder::js_to_number(ctx, argv[0]);
								JavaScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, argv[1]);
								Quaternion *ptr = bind->getQuaternion();
								Quaternion ret = ptr->operator*(scalar);
								return QuickJSBuiltinBinder::new_object_from(ctx, ret);
							},
							"multiply_number_left", 2),
					QuickJSBinder::PROP_DEF_DEFAULT);

			// new godot.Quaternion(0.5, 0.5, 0.5, 0.5) * 2
			JS_DefinePropertyValueStr(ctx, number_right, "*",
					JS_NewCFunction(
							ctx, [](JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) -> JSValue {
								JavaScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, argv[0]);
								Quaternion *ptr = bind->getQuaternion();
								real_t scalar = QuickJSBinder::js_to_number(ctx, argv[1]);
								Quaternion ret = ptr->operator*(scalar);
								return QuickJSBuiltinBinder::new_object_from(ctx, ret);
							},
							"multiply_number_right", 2),
					QuickJSBinder::PROP_DEF_DEFAULT);

			// new godot.Quaternion(0.5, 0.5, 0.5, 0.5) / 2
			JS_DefinePropertyValueStr(ctx, number_right, "/",
					JS_NewCFunction(
							ctx, [](JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) -> JSValue {
								JavaScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, argv[0]);
								Quaternion *ptr = bind->getQuaternion();
								real_t scalar = QuickJSBinder::js_to_number(ctx, argv[1]);
								Quaternion ret = ptr->operator/(scalar);
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
								JavaScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, argv[1]);
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
								JavaScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, argv[0]);
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
		case Variant::QUATERNION: {
			Quaternion tmp = p_val;
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
		case Variant::RID: {
			RID tmp = p_val;
			obj = create_builtin_value(ctx, type, &tmp);
		} break;
		case Variant::TRANSFORM3D: {
			Transform3D tmp = p_val;
			obj = create_builtin_value(ctx, type, &tmp);
		} break;
		case Variant::AABB: {
			AABB tmp = p_val;
			obj = create_builtin_value(ctx, type, &tmp);
		} break;
		case Variant::PACKED_INT32_ARRAY: {
			PackedInt32Array tmp = p_val;
			obj = create_builtin_value(ctx, type, &tmp);
		} break;
		case Variant::PACKED_BYTE_ARRAY: {
			PackedByteArray tmp = p_val;
			obj = create_builtin_value(ctx, type, &tmp);
		} break;
		case Variant::PACKED_FLOAT32_ARRAY: {
			PackedFloat32Array tmp = p_val;
			obj = create_builtin_value(ctx, type, &tmp);
		} break;
		case Variant::PACKED_COLOR_ARRAY: {
			PackedColorArray tmp = p_val;
			obj = create_builtin_value(ctx, type, &tmp);
		} break;
		case Variant::PACKED_STRING_ARRAY: {
			PackedStringArray tmp = p_val;
			obj = create_builtin_value(ctx, type, &tmp);
		} break;
		case Variant::PACKED_VECTOR2_ARRAY: {
			PackedVector2Array tmp = p_val;
			obj = create_builtin_value(ctx, type, &tmp);
		} break;
		case Variant::PACKED_VECTOR3_ARRAY: {
			PackedVector3Array tmp = p_val;
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

JSValue QuickJSBuiltinBinder::new_object_from(JSContext *ctx, const Transform3D &p_val) {
	return create_builtin_value(ctx, Variant::TRANSFORM3D, &p_val);
}

JSValue QuickJSBuiltinBinder::new_object_from(JSContext *ctx, const Quaternion &p_val) {
	return create_builtin_value(ctx, Variant::QUATERNION, &p_val);
}

JSValue QuickJSBuiltinBinder::new_object_from(JSContext *ctx, const Plane &p_val) {
	return create_builtin_value(ctx, Variant::PLANE, &p_val);
}

JSValue QuickJSBuiltinBinder::new_object_from(JSContext *ctx, const RID &p_val) {
	return create_builtin_value(ctx, Variant::RID, &p_val);
}

JSValue QuickJSBuiltinBinder::new_object_from(JSContext *ctx, const AABB &p_val) {
	return create_builtin_value(ctx, Variant::AABB, &p_val);
}

JSValue QuickJSBuiltinBinder::new_object_from(JSContext *ctx, const Basis &p_val) {
	return create_builtin_value(ctx, Variant::BASIS, &p_val);
}

JSValue QuickJSBuiltinBinder::new_object_from(JSContext *ctx, const PackedInt32Array &p_val) {
	return create_builtin_value(ctx, Variant::PACKED_INT32_ARRAY, &p_val);
}

JSValue QuickJSBuiltinBinder::new_object_from(JSContext *ctx, const PackedByteArray &p_val) {
	return create_builtin_value(ctx, Variant::PACKED_BYTE_ARRAY, &p_val);
}

JSValue QuickJSBuiltinBinder::new_object_from(JSContext *ctx, const PackedFloat32Array &p_val) {
	return create_builtin_value(ctx, Variant::PACKED_FLOAT32_ARRAY, &p_val);
}

JSValue QuickJSBuiltinBinder::new_object_from(JSContext *ctx, const PackedColorArray &p_val) {
	return create_builtin_value(ctx, Variant::PACKED_COLOR_ARRAY, &p_val);
}

JSValue QuickJSBuiltinBinder::new_object_from(JSContext *ctx, const PackedStringArray &p_val) {
	return create_builtin_value(ctx, Variant::PACKED_STRING_ARRAY, &p_val);
}

JSValue QuickJSBuiltinBinder::new_object_from(JSContext *ctx, const PackedVector2Array &p_val) {
	return create_builtin_value(ctx, Variant::PACKED_VECTOR2_ARRAY, &p_val);
}

JSValue QuickJSBuiltinBinder::new_object_from(JSContext *ctx, const PackedVector3Array &p_val) {
	return create_builtin_value(ctx, Variant::PACKED_VECTOR3_ARRAY, &p_val);
}

void QuickJSBuiltinBinder::bind_builtin_propties_manually() {

	{ // Color
		JSCFunctionMagic *getter = [](JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv, int magic) -> JSValue {
			JavaScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, this_val);
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
			JavaScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, this_val);
			Color *ptr = bind->getColor();
#ifdef DEBUG_METHODS_ENABLED
			ERR_FAIL_COND_V(!QuickJSBinder::validate_type(ctx, Variant::FLOAT, argv[0]), (JS_ThrowTypeError(ctx, "number value expected")));
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
			JavaScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, this_val);
			const Rect2 *ptr = bind->getRect2();
			switch (magic) {
				case 0:
					return QuickJSBinder::variant_to_var(ctx, ptr->size + ptr->position);
			}
			return JS_UNDEFINED;
		};
		JSCFunctionMagic *setter = [](JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv, int magic) -> JSValue {
			JavaScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, this_val);
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
			"grow_side",
			[](JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
#ifdef DEBUG_METHODS_ENABLED
				ERR_FAIL_COND_V(argc < 2, (JS_ThrowTypeError(ctx, "Two arguments expected for Rect2.grow_margin")));
#endif
				JavaScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, this_val);
				Rect2 *ptr = bind->getRect2();
#ifdef DEBUG_METHODS_ENABLED
				ERR_FAIL_COND_V(!QuickJSBinder::validate_type(ctx, Variant::INT, argv[0]), (JS_ThrowTypeError(ctx, "number expected for argument 0 of Rect2.grow_margin")));
				ERR_FAIL_COND_V(!QuickJSBinder::validate_type(ctx, Variant::FLOAT, argv[0]), (JS_ThrowTypeError(ctx, "number expected for argument 1 of Rect2.grow_margin")));
#endif
				Rect2 ret = ptr->grow_side(Side(QuickJSBinder::js_to_int(ctx, argv[0])), QuickJSBinder::js_to_number(ctx, argv[1]));
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
					JavaScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, this_val);
					Transform2D *ptr = bind->getTransform2D();
					if (QuickJSBinder::validate_type(ctx, Variant::VECTOR2, argv[0])) {
						Vector2 ret = ptr->xform(Vector2(QuickJSBinder::var_to_variant(ctx, argv[0])));
						return QuickJSBinder::variant_to_var(ctx, ret);
					} else if (QuickJSBinder::validate_type(ctx, Variant::RECT2, argv[0])) {
						Rect2 ret = ptr->xform(Rect2(QuickJSBinder::var_to_variant(ctx, argv[0])));
						return QuickJSBinder::variant_to_var(ctx, ret);
					} else if (QuickJSBinder::validate_type(ctx, Variant::PACKED_VECTOR2_ARRAY, argv[0])) {
						PackedVector2Array ret = ptr->xform(PackedVector2Array(QuickJSBinder::var_to_variant(ctx, argv[0])));
						return QuickJSBinder::variant_to_var(ctx, ret);
					}
					ERR_FAIL_V(JS_ThrowTypeError(ctx, "Vector2, Rect2 or PackedVector2Array expected for argument #0 of Transform2D.xform"));
				},
				1);
		binder->get_builtin_binder().register_method(
				Variant::TRANSFORM2D,
				"xform_inv",
				[](JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
					ERR_FAIL_COND_V(argc < 1, (JS_ThrowTypeError(ctx, "Argument expected for Transform2D.xform_inv")));
					JavaScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, this_val);
					Transform2D *ptr = bind->getTransform2D();
					if (QuickJSBinder::validate_type(ctx, Variant::VECTOR2, argv[0])) {
						Vector2 ret = ptr->xform_inv(Vector2(QuickJSBinder::var_to_variant(ctx, argv[0])));
						return QuickJSBinder::variant_to_var(ctx, ret);
					} else if (QuickJSBinder::validate_type(ctx, Variant::RECT2, argv[0])) {
						Rect2 ret = ptr->xform_inv(Rect2(QuickJSBinder::var_to_variant(ctx, argv[0])));
						return QuickJSBinder::variant_to_var(ctx, ret);
					} else if (QuickJSBinder::validate_type(ctx, Variant::PACKED_VECTOR2_ARRAY, argv[0])) {
						PackedVector2Array ret = ptr->xform_inv(PackedVector2Array(QuickJSBinder::var_to_variant(ctx, argv[0])));
						return QuickJSBinder::variant_to_var(ctx, ret);
					}
					ERR_FAIL_V(JS_ThrowTypeError(ctx, "Vector2, Rect2 or PackedVector2Array expected for argument #0 of Transform2D.xform_inv"));
				},
				1);
	}

	{ // Basis
		binder->get_builtin_binder().register_method(
				Variant::BASIS,
				"is_equal_approx",
				[](JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
					ERR_FAIL_COND_V(argc < 1, (JS_ThrowTypeError(ctx, "Argument expected for Basis.is_equal_approx")));
					JavaScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, this_val);
					Basis *ptr = bind->getBasis();
#ifdef DEBUG_METHODS_ENABLED
					ERR_FAIL_COND_V(!QuickJSBinder::validate_type(ctx, Variant::BASIS, argv[0]), (JS_ThrowTypeError(ctx, "Basis expected for Basis.is_equal_approx")));
#endif
					JavaScriptGCHandler *argv_bind = BINDING_DATA_FROM_JS(ctx, argv[0]);
					bool ret = ptr->is_equal_approx(*argv_bind->getBasis());
					return QuickJSBinder::to_js_bool(ctx, ret);
				},
				1);
	}

	{ // AABB
		JSCFunctionMagic *getter = [](JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv, int magic) -> JSValue {
			JavaScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, this_val);
			const AABB *ptr = bind->getAABB();
			switch (magic) {
				case 0:
					return QuickJSBinder::variant_to_var(ctx, ptr->size + ptr->position);
			}
			return JS_UNDEFINED;
		};
		JSCFunctionMagic *setter = [](JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv, int magic) -> JSValue {
			JavaScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, this_val);
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
					JavaScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, this_val);
					Plane *ptr = bind->getPlane();
#ifdef DEBUG_METHODS_ENABLED
					ERR_FAIL_COND_V(!QuickJSBinder::validate_type(ctx, Variant::PLANE, argv[0]), (JS_ThrowTypeError(ctx, "Plane expected for arguments #0 Plane.intersect_3")));
					ERR_FAIL_COND_V(!QuickJSBinder::validate_type(ctx, Variant::PLANE, argv[1]), (JS_ThrowTypeError(ctx, "Plane expected for arguments #1 Plane.intersect_3")));
#endif
					Vector3 ret;
					JavaScriptGCHandler *argv0_bind = BINDING_DATA_FROM_JS(ctx, argv[0]);
					JavaScriptGCHandler *argv1_bind = BINDING_DATA_FROM_JS(ctx, argv[1]);
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
					JavaScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, this_val);
					Plane *ptr = bind->getPlane();
#ifdef DEBUG_METHODS_ENABLED
					ERR_FAIL_COND_V(!QuickJSBinder::validate_type(ctx, Variant::VECTOR3, argv[0]), (JS_ThrowTypeError(ctx, "Vector3 expected for arguments #0 Plane.intersects_ray")));
					ERR_FAIL_COND_V(!QuickJSBinder::validate_type(ctx, Variant::VECTOR3, argv[1]), (JS_ThrowTypeError(ctx, "Vector3 expected for arguments #1 Plane.intersects_ray")));
#endif
					Vector3 ret;
					JavaScriptGCHandler *argv0_bind = BINDING_DATA_FROM_JS(ctx, argv[0]);
					JavaScriptGCHandler *argv1_bind = BINDING_DATA_FROM_JS(ctx, argv[1]);
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
					JavaScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, this_val);
					Plane *ptr = bind->getPlane();
#ifdef DEBUG_METHODS_ENABLED
					ERR_FAIL_COND_V(!QuickJSBinder::validate_type(ctx, Variant::VECTOR3, argv[0]), (JS_ThrowTypeError(ctx, "Vector3 expected for arguments #0 Plane.intersects_segment")));
					ERR_FAIL_COND_V(!QuickJSBinder::validate_type(ctx, Variant::VECTOR3, argv[1]), (JS_ThrowTypeError(ctx, "Vector3 expected for arguments #1 Plane.intersects_segment")));
#endif
					Vector3 ret;
					JavaScriptGCHandler *argv0_bind = BINDING_DATA_FROM_JS(ctx, argv[0]);
					JavaScriptGCHandler *argv1_bind = BINDING_DATA_FROM_JS(ctx, argv[1]);
					if (ptr->intersects_segment(*argv0_bind->getVector3(), *argv1_bind->getVector3(), &ret)) {
						return QuickJSBinder::variant_to_var(ctx, ret);
					}
					return JS_NULL;
				},
				2);
	}
	{ // Transform
		binder->get_builtin_binder().register_method(
			Variant::TRANSFORM3D,
			"xform",
			[](JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
				ERR_FAIL_COND_V(argc < 1, (JS_ThrowTypeError(ctx, "Argument expected for Transform3D.xform")));
				JavaScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, this_val);
				Transform3D *ptr = bind->getTransform3D();
				if (QuickJSBinder::validate_type(ctx, Variant::VECTOR3, argv[0])) {
					Vector3 ret = ptr->xform(Vector3(QuickJSBinder::var_to_variant(ctx, argv[0])));
					return QuickJSBinder::variant_to_var(ctx, ret);
				} else if (QuickJSBinder::validate_type(ctx, Variant::PLANE, argv[0])) {
					Plane ret = ptr->xform(Plane(QuickJSBinder::var_to_variant(ctx, argv[0])));
					return QuickJSBinder::variant_to_var(ctx, ret);
				} else if (QuickJSBinder::validate_type(ctx, Variant::AABB, argv[0])) {
					AABB ret = ptr->xform(AABB(QuickJSBinder::var_to_variant(ctx, argv[0])));
					return QuickJSBinder::variant_to_var(ctx, ret);
				} else if (QuickJSBinder::validate_type(ctx, Variant::PACKED_VECTOR3_ARRAY, argv[0])) {
					PackedVector3Array ret = ptr->xform(PackedVector3Array(QuickJSBinder::var_to_variant(ctx, argv[0])));
					return QuickJSBinder::variant_to_var(ctx, ret);
				}
				ERR_FAIL_V(JS_ThrowTypeError(ctx, "Vector3, Plane, AABB or PackedVector3Array expected for argument #0 of Transform3D.xform"));
			},
			1);
		binder->get_builtin_binder().register_method(
			Variant::TRANSFORM3D,
			"xform_inv",
			[](JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
				ERR_FAIL_COND_V(argc < 1, (JS_ThrowTypeError(ctx, "Argument expected for Transform3D.xform_inv")));
				JavaScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, this_val);
				Transform3D *ptr = bind->getTransform3D();
				if (QuickJSBinder::validate_type(ctx, Variant::VECTOR3, argv[0])) {
					Vector3 ret = ptr->xform_inv(Vector3(QuickJSBinder::var_to_variant(ctx, argv[0])));
					return QuickJSBinder::variant_to_var(ctx, ret);
				} else if (QuickJSBinder::validate_type(ctx, Variant::PLANE, argv[0])) {
					Plane ret = ptr->xform_inv(Plane(QuickJSBinder::var_to_variant(ctx, argv[0])));
					return QuickJSBinder::variant_to_var(ctx, ret);
				} else if (QuickJSBinder::validate_type(ctx, Variant::AABB, argv[0])) {
					AABB ret = ptr->xform_inv(AABB(QuickJSBinder::var_to_variant(ctx, argv[0])));
					return QuickJSBinder::variant_to_var(ctx, ret);
				} else if (QuickJSBinder::validate_type(ctx, Variant::PACKED_VECTOR3_ARRAY, argv[0])) {
					PackedVector3Array ret = ptr->xform_inv(PackedVector3Array(QuickJSBinder::var_to_variant(ctx, argv[0])));
					return QuickJSBinder::variant_to_var(ctx, ret);
				}
				ERR_FAIL_V(JS_ThrowTypeError(ctx, "Vector3, Plane, AABB or PackedVector3Array expected for argument #0 of Transform3D.xform_inv"));
			},
			1);
	}

	{ // PackedByteArray
		// PackedByteArray.prototype.compress
		binder->get_builtin_binder().register_method(
				Variant::PACKED_BYTE_ARRAY,
				"compress",
				[](JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
					JavaScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, this_val);
					PackedByteArray *ptr = bind->getPackedByteArray();
					PackedByteArray compressed;
					if (ptr->size() > 0) {
#ifdef DEBUG_METHODS_ENABLED
						ERR_FAIL_COND_V(!QuickJSBinder::validate_type(ctx, Variant::INT, argv[0]), (JS_ThrowTypeError(ctx, "number expected for argument 0 of PackedByteArray.compress")));
#endif
						Compression::Mode mode = (Compression::Mode)(QuickJSBinder::js_to_int(ctx, argv[0]));
						compressed.resize(Compression::get_max_compressed_buffer_size(ptr->size(), mode));
						int result = Compression::compress(compressed.ptrw(), ptr->ptr(), ptr->size(), mode);
						result = result >= 0 ? result : 0;
						compressed.resize(result);
					}
					return QuickJSBinder::variant_to_var(ctx, compressed);
				},
				1);
		// PackedByteArray.prototype.decompress
		binder->get_builtin_binder().register_method(
				Variant::PACKED_BYTE_ARRAY,
				"decompress",
				[](JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
					JavaScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, this_val);
					PackedByteArray *ptr = bind->getPackedByteArray();
					PackedByteArray decompressed;
#ifdef DEBUG_METHODS_ENABLED
					ERR_FAIL_COND_V(!QuickJSBinder::validate_type(ctx, Variant::INT, argv[0]), (JS_ThrowTypeError(ctx, "number expected for argument 0 of PackedByteArray.decompress")));
#endif
					int buffer_size = QuickJSBinder::js_to_int(ctx, argv[0]);
					ERR_FAIL_COND_V(buffer_size <= 0, (JS_ThrowTypeError(ctx, "Decompression buffer size must be greater than zero.")));

#ifdef DEBUG_METHODS_ENABLED
					ERR_FAIL_COND_V(!QuickJSBinder::validate_type(ctx, Variant::INT, argv[0]), (JS_ThrowTypeError(ctx, "number expected for argument 1 of PackedByteArray.decompress")));
#endif
					Compression::Mode mode = (Compression::Mode)(QuickJSBinder::js_to_int(ctx, argv[1]));
					decompressed.resize(buffer_size);
					int result = Compression::decompress(decompressed.ptrw(), buffer_size, ptr->ptr(), ptr->size(), mode);
					result = result >= 0 ? result : 0;
					decompressed.resize(result);
					return QuickJSBinder::variant_to_var(ctx, decompressed);
				},
				1);
		// PackedByteArray.prototype.get_string_from_utf8
		binder->get_builtin_binder().register_method(
				Variant::PACKED_BYTE_ARRAY,
				"get_string_from_utf8",
				[](JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
					JavaScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, this_val);
					PackedByteArray *ptr = bind->getPackedByteArray();
					String ret;
					if (ptr->size() > 0) {
						ret.parse_utf8((const char *)ptr->ptr(), ptr->size());
					}
					return QuickJSBinder::to_js_string(ctx, ret);
				},
				0);
		// PackedByteArray.prototype.get_string_from_ascii
		binder->get_builtin_binder().register_method(
				Variant::PACKED_BYTE_ARRAY,
				"get_string_from_ascii",
				[](JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
					JavaScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, this_val);
					PackedByteArray *ptr = bind->getPackedByteArray();
					String ret;
					if (ptr->size() > 0) {
						CharString cs;
						cs.resize(ptr->size() + 1);
						memcpy(cs.ptrw(), ptr->ptr(), ptr->size());
						cs[ptr->size()] = 0;
						ret = cs.get_data();
					}
					return QuickJSBinder::to_js_string(ctx, ret);
				},
				0);
		// PackedByteArray.prototype.hex_encode
		binder->get_builtin_binder().register_method(
				Variant::PACKED_BYTE_ARRAY,
				"hex_encode",
				[](JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
					JavaScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, this_val);
					PackedByteArray *ptr = bind->getPackedByteArray();
					String ret;
					if (ptr->size() > 0) {
						ret = String::hex_encode_buffer(ptr->ptr(), ptr->size());
					}
					return QuickJSBinder::to_js_string(ctx, ret);
				},
				0);
		// PackedByteArray.prototype.get_buffer
		binder->get_builtin_binder().register_method(
				Variant::PACKED_BYTE_ARRAY,
				"get_buffer",
				[](JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
					JavaScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, this_val);
					PackedByteArray *array = memnew(PackedByteArray(*bind->getPackedByteArray()));
					JSValue ret = JS_NewArrayBuffer(
							ctx, const_cast<uint8_t *>(array->ptr()), array->size(), [](JSRuntime *rt, void *opaque, void *ptr) {
								memdelete(static_cast<PackedByteArray *>(opaque));
							},
							array, false);
					return ret;
				},
				0);
	}
	{
		// PackedInt32Array.prototype.get_buffer
		binder->get_builtin_binder().register_method(
				Variant::PACKED_INT32_ARRAY,
				"get_buffer",
				[](JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
					JavaScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, this_val);
					PackedInt32Array *array = memnew(PackedInt32Array(*bind->getPackedInt32Array()));
					JSValue ret = JS_NewArrayBuffer(
							ctx, (uint8_t *)(array->ptr()), array->size() * sizeof(int), [](JSRuntime *rt, void *opaque, void *ptr) {
								memdelete(static_cast<PackedInt32Array *>(opaque));
							},
							array, false);
					return ret;
				},
				0);
	}
	{
		// PackedFloat32Array.prototype.get_buffer
		binder->get_builtin_binder().register_method(
				Variant::PACKED_FLOAT32_ARRAY,
				"get_buffer",
				[](JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
					JavaScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, this_val);
					PackedFloat32Array *array = memnew(PackedFloat32Array(*bind->getPackedFloat32Array()));
					JSValue ret = JS_NewArrayBuffer(
							ctx, (uint8_t *)(array->ptr()), array->size() * sizeof(real_t), [](JSRuntime *rt, void *opaque, void *ptr) {
								memdelete(static_cast<PackedFloat32Array *>(opaque));
							},
							array, false);
					return ret;
				},
				0);
	}
	{
		// PackedVector2Array.prototype.get_buffer
		binder->get_builtin_binder().register_method(
				Variant::PACKED_VECTOR2_ARRAY,
				"get_buffer",
				[](JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
					JavaScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, this_val);
					PackedVector2Array *array = memnew(PackedVector2Array(*bind->getPackedVector2Array()));
					JSValue ret = JS_NewArrayBuffer(
							ctx, (uint8_t *)(array->ptr()), array->size() * sizeof(Vector2), [](JSRuntime *rt, void *opaque, void *ptr) {
								memdelete(static_cast<PackedVector2Array *>(opaque));
							},
							array, false);
					return ret;
				},
				0);
	}

	{
		// PackedVector3Array.prototype.get_buffer
		binder->get_builtin_binder().register_method(
				Variant::PACKED_VECTOR3_ARRAY,
				"get_buffer",
				[](JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
					JavaScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, this_val);
					PackedVector3Array *array = memnew(PackedVector3Array(*bind->getPackedVector3Array()));
					JSValue ret = JS_NewArrayBuffer(
							ctx, (uint8_t *)(array->ptr()), array->size() * sizeof(Vector3), [](JSRuntime *rt, void *opaque, void *ptr) {
								memdelete(static_cast<PackedVector3Array *>(opaque));
							},
							array, false);
					return ret;
				},
				0);
	}

	{
		// PackedColorArray.prototype.get_buffer
		binder->get_builtin_binder().register_method(
				Variant::PACKED_COLOR_ARRAY,
				"get_buffer",
				[](JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
					JavaScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, this_val);
					PackedColorArray *array = memnew(PackedColorArray(*bind->getPackedColorArray()));
					JSValue ret = JS_NewArrayBuffer(
						ctx, (uint8_t *)(array->ptr()), array->size() * sizeof(Color), [](JSRuntime *rt, void *opaque, void *ptr) {
							memdelete(static_cast<PackedColorArray *>(opaque));
						},
						array, false);
					return ret;
				},
				0);
	}
}
