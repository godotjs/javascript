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

JSValue QuickJSBuiltinBinder::bind_builtin_object(Variant::Type p_type, const void *p_object) {

	const QuickJSBuiltinBinder::BuiltinClass &cls = binder->builtin_binder.get_class(p_type);
	JSValue obj = JS_NewObjectProtoClass(ctx, cls.class_prototype, binder->get_origin_class_id());

	size_t size = 0;
	switch (p_type) {
		case Variant::VECTOR2:
			size += sizeof(Vector2);
			break;
		case Variant::RECT2:
			size += sizeof(Rect2);
			break;
		case Variant::COLOR:
			size += sizeof(Color);
			break;
		case Variant::VECTOR3:
			size += sizeof(Vector3);
			break;
		case Variant::BASIS:
			size += sizeof(Basis);
			break;
		case Variant::QUAT:
			size += sizeof(Quat);
			break;
		case Variant::PLANE:
			size += sizeof(Plane);
			break;
		case Variant::TRANSFORM2D:
			size += sizeof(Transform2D);
			break;
		case Variant::_RID:
			size += sizeof(RID);
			break;
		case Variant::TRANSFORM:
			size += sizeof(Transform);
			break;
		case Variant::AABB:
			size += sizeof(AABB);
			break;
		case Variant::PACKED_INT32_ARRAY:
			size += sizeof(PackedInt32Array);
			break;
		case Variant::PACKED_BYTE_ARRAY:
			size += sizeof(PackedByteArray);
			break;
		case Variant::PACKED_FLOAT32_ARRAY:
			size += sizeof(PackedFloat32Array);
			break;
		case Variant::PACKED_COLOR_ARRAY:
			size += sizeof(PackedColorArray);
			break;
		case Variant::PACKED_STRING_ARRAY:
			size += sizeof(PackedStringArray);
			break;
		case Variant::PACKED_VECTOR2_ARRAY:
			size += sizeof(PackedVector2Array);
			break;
		case Variant::PACKED_VECTOR3_ARRAY:
			size += sizeof(PackedVector3Array);
			break;
		default:
			break;
	}
	void *ptr = memalloc(sizeof(ECMAScriptGCHandler) + size);
	ECMAScriptGCHandler *bind = memnew_placement(ptr, ECMAScriptGCHandler);
	bind->context = ctx;
	bind->type = p_type;
	bind->flags |= ECMAScriptGCHandler::FLAG_BUILTIN_CLASS;
	bind->godot_builtin_object_ptr = bind + 1;
	copymem(bind->godot_builtin_object_ptr, p_object, size);
	JS_SetOpaque(obj, bind);

#ifdef DUMP_LEAKS
	QuickJSBinder::add_debug_binding_info(ctx, obj, bind);
#endif

	return obj;
}

void QuickJSBuiltinBinder::builtin_finalizer(ECMAScriptGCHandler *p_bind) {
	memfree(p_bind);
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

JSValue QuickJSBuiltinBinder::bind_builtin_object_static(JSContext *ctx, Variant::Type p_type, const void *p_object) {
	return QuickJSBinder::get_context_binder(ctx)->builtin_binder.bind_builtin_object(p_type, p_object);
}

JSValue QuickJSBuiltinBinder::new_object_from(JSContext *ctx, const Variant &p_val) {
	const GodotVariantParser *p = reinterpret_cast<const GodotVariantParser *>(&p_val);
	const void *ptr = NULL;
	switch (p_val.get_type()) {
		case Variant::Type::AABB:
			ptr = p->data._aabb;
			break;
		case Variant::Type::BASIS:
			ptr = p->data._basis;
			break;
		case Variant::Type::TRANSFORM:
			ptr = p->data._transform;
			break;
		case Variant::Type::TRANSFORM2D:
			ptr = p->data._transform2d;
			break;
		default:
			ptr = p->data._mem;
			break;
	}
	ERR_FAIL_NULL_V_MSG(ptr, JS_UNDEFINED, vformat("Cannot construct builtin type for %s", Variant::get_type_name(p_val.get_type())));
	return QuickJSBinder::get_context_binder(ctx)->builtin_binder.bind_builtin_object(p_val.get_type(), ptr);
}

JSValue QuickJSBuiltinBinder::new_object_from(JSContext *ctx, const Vector2 &p_val) {
	return QuickJSBinder::get_context_binder(ctx)->builtin_binder.bind_builtin_object(Variant::VECTOR2, &p_val);
}

JSValue QuickJSBuiltinBinder::new_object_from(JSContext *ctx, const Vector3 &p_val) {
	return QuickJSBinder::get_context_binder(ctx)->builtin_binder.bind_builtin_object(Variant::VECTOR3, &p_val);
}

JSValue QuickJSBuiltinBinder::new_object_from(JSContext *ctx, const Rect2 &p_val) {
	return QuickJSBinder::get_context_binder(ctx)->builtin_binder.bind_builtin_object(Variant::RECT2, &p_val);
}

JSValue QuickJSBuiltinBinder::new_object_from(JSContext *ctx, const Color &p_val) {
	return QuickJSBinder::get_context_binder(ctx)->builtin_binder.bind_builtin_object(Variant::COLOR, &p_val);
}

JSValue QuickJSBuiltinBinder::new_object_from(JSContext *ctx, const Transform2D &p_val) {
	return QuickJSBinder::get_context_binder(ctx)->builtin_binder.bind_builtin_object(Variant::TRANSFORM2D, &p_val);
}

JSValue QuickJSBuiltinBinder::new_object_from(JSContext *ctx, const Transform &p_val) {
	return QuickJSBinder::get_context_binder(ctx)->builtin_binder.bind_builtin_object(Variant::TRANSFORM, &p_val);
}

JSValue QuickJSBuiltinBinder::new_object_from(JSContext *ctx, const Quat &p_val) {
	return QuickJSBinder::get_context_binder(ctx)->builtin_binder.bind_builtin_object(Variant::QUAT, &p_val);
}

JSValue QuickJSBuiltinBinder::new_object_from(JSContext *ctx, const Plane &p_val) {
	return QuickJSBinder::get_context_binder(ctx)->builtin_binder.bind_builtin_object(Variant::PLANE, &p_val);
}

JSValue QuickJSBuiltinBinder::new_object_from(JSContext *ctx, const RID &p_val) {
	return QuickJSBinder::get_context_binder(ctx)->builtin_binder.bind_builtin_object(Variant::_RID, &p_val);
}

JSValue QuickJSBuiltinBinder::new_object_from(JSContext *ctx, const AABB &p_val) {
	return QuickJSBinder::get_context_binder(ctx)->builtin_binder.bind_builtin_object(Variant::AABB, &p_val);
}

JSValue QuickJSBuiltinBinder::new_object_from(JSContext *ctx, const Basis &p_val) {
	return QuickJSBinder::get_context_binder(ctx)->builtin_binder.bind_builtin_object(Variant::BASIS, &p_val);
}

JSValue QuickJSBuiltinBinder::new_object_from(JSContext *ctx, const PackedInt32Array &p_val) {
	return QuickJSBinder::get_context_binder(ctx)->builtin_binder.bind_builtin_object(Variant::PACKED_INT32_ARRAY, &p_val);
}

JSValue QuickJSBuiltinBinder::new_object_from(JSContext *ctx, const PackedByteArray &p_val) {
	return QuickJSBinder::get_context_binder(ctx)->builtin_binder.bind_builtin_object(Variant::PACKED_BYTE_ARRAY, &p_val);
}

JSValue QuickJSBuiltinBinder::new_object_from(JSContext *ctx, const PackedFloat32Array &p_val) {
	return QuickJSBinder::get_context_binder(ctx)->builtin_binder.bind_builtin_object(Variant::PACKED_FLOAT32_ARRAY, &p_val);
}

JSValue QuickJSBuiltinBinder::new_object_from(JSContext *ctx, const PackedColorArray &p_val) {
	return QuickJSBinder::get_context_binder(ctx)->builtin_binder.bind_builtin_object(Variant::PACKED_COLOR_ARRAY, &p_val);
}

JSValue QuickJSBuiltinBinder::new_object_from(JSContext *ctx, const PackedStringArray &p_val) {
	return QuickJSBinder::get_context_binder(ctx)->builtin_binder.bind_builtin_object(Variant::PACKED_STRING_ARRAY, &p_val);
}

JSValue QuickJSBuiltinBinder::new_object_from(JSContext *ctx, const PackedVector2Array &p_val) {
	return QuickJSBinder::get_context_binder(ctx)->builtin_binder.bind_builtin_object(Variant::PACKED_VECTOR2_ARRAY, &p_val);
}

JSValue QuickJSBuiltinBinder::new_object_from(JSContext *ctx, const PackedVector3Array &p_val) {
	return QuickJSBinder::get_context_binder(ctx)->builtin_binder.bind_builtin_object(Variant::PACKED_VECTOR3_ARRAY, &p_val);
}

void QuickJSBuiltinBinder::bind_builtin_propties_manually() {

	{ // PoolByteArray
		// PoolByteArray.prototype.compress
		binder->get_builtin_binder().register_method(
				Variant::PACKED_BYTE_ARRAY,
				"compress",
				[](JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
					ECMAScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, this_val);
					PackedByteArray *ptr = bind->getPackedByteArray();
					PackedByteArray compressed;
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
				Variant::PACKED_BYTE_ARRAY,
				"decompress",
				[](JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
					ECMAScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, this_val);
					PackedByteArray *ptr = bind->getPackedByteArray();
					PackedByteArray decompressed;
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
				Variant::PACKED_BYTE_ARRAY,
				"get_string_from_utf8",
				[](JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
					ECMAScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, this_val);
					PackedByteArray *ptr = bind->getPackedByteArray();
					String ret;
					if (ptr->size() > 0) {
						PackedByteArray::Read r = ptr->read();
						ret.parse_utf8((const char *)r.ptr(), ptr->size());
					}
					return QuickJSBinder::to_js_string(ctx, ret);
				},
				0);
		// PoolByteArray.prototype.get_string_from_ascii
		binder->get_builtin_binder().register_method(
				Variant::PACKED_BYTE_ARRAY,
				"get_string_from_ascii",
				[](JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
					ECMAScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, this_val);
					PackedByteArray *ptr = bind->getPackedByteArray();
					String ret;
					if (ptr->size() > 0) {
						PackedByteArray::Read r = ptr->read();
						CharString cs;
						cs.resize(ptr->size() + 1);
						copymem(cs.ptrw(), r.ptr(), ptr->size());
						cs[ptr->size()] = 0;
						ret = cs.get_data();
					}
					return QuickJSBinder::to_js_string(ctx, ret);
				},
				0);
		// PoolByteArray.prototype.hex_encode
		binder->get_builtin_binder().register_method(
				Variant::PACKED_BYTE_ARRAY,
				"hex_encode",
				[](JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
					ECMAScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, this_val);
					PackedByteArray *ptr = bind->getPackedByteArray();
					String ret;
					if (ptr->size() > 0) {
						PackedByteArray::Read r = ptr->read();
						ret = String::hex_encode_buffer(&r[0], ptr->size());
					}
					return QuickJSBinder::to_js_string(ctx, ret);
				},
				0);
		// PoolByteArray.prototype.get_buffer
		binder->get_builtin_binder().register_method(
				Variant::PACKED_BYTE_ARRAY,
				"get_buffer",
				[](JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
					ECMAScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, this_val);
					PackedByteArray *array = memnew(PackedByteArray(*bind->getPackedByteArray()));
					JSValue ret = JS_NewArrayBuffer(
							ctx, const_cast<uint8_t *>(array->read().ptr()), array->size(), [](JSRuntime *rt, void *opaque, void *ptr) {
								memdelete(static_cast<PackedByteArray *>(opaque));
							},
							array, false);
					return ret;
				},
				0);
	}
	{
		// PoolIntArray.prototype.get_buffer
		binder->get_builtin_binder().register_method(
				Variant::PACKED_INT32_ARRAY,
				"get_buffer",
				[](JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
					ECMAScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, this_val);
					PackedInt32Array *array = memnew(PackedInt32Array(*bind->getPackedInt32Array()));
					JSValue ret = JS_NewArrayBuffer(
							ctx, (uint8_t *)(array->read().ptr()), array->size() * sizeof(int), [](JSRuntime *rt, void *opaque, void *ptr) {
								memdelete(static_cast<PackedInt32Array *>(opaque));
							},
							array, false);
					return ret;
				},
				0);
	}
	{
		// PoolRealArray.prototype.get_buffer
		binder->get_builtin_binder().register_method(
				Variant::PACKED_FLOAT32_ARRAY,
				"get_buffer",
				[](JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
					ECMAScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, this_val);
					PackedFloat32Array *array = memnew(PackedFloat32Array(*bind->getPackedFloat32Array()));
					JSValue ret = JS_NewArrayBuffer(
							ctx, (uint8_t *)(array->read().ptr()), array->size() * sizeof(real_t), [](JSRuntime *rt, void *opaque, void *ptr) {
								memdelete(static_cast<PackedFloat32Array *>(opaque));
							},
							array, false);
					return ret;
				},
				0);
	}
	{
		// PoolVector2Array.prototype.get_buffer
		binder->get_builtin_binder().register_method(
				Variant::PACKED_VECTOR2_ARRAY,
				"get_buffer",
				[](JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
					ECMAScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, this_val);
					PackedVector2Array *array = memnew(PackedVector2Array(*bind->getPackedVector2Array()));
					JSValue ret = JS_NewArrayBuffer(
							ctx, (uint8_t *)(array->read().ptr()), array->size() * sizeof(Vector2), [](JSRuntime *rt, void *opaque, void *ptr) {
								memdelete(static_cast<PackedVector2Array *>(opaque));
							},
							array, false);
					return ret;
				},
				0);
	}

	{
		// PoolVector3Array.prototype.get_buffer
		binder->get_builtin_binder().register_method(
				Variant::PACKED_VECTOR3_ARRAY,
				"get_buffer",
				[](JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
					ECMAScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, this_val);
					PackedVector3Array *array = memnew(PackedVector3Array(*bind->getPackedVector3Array()));
					JSValue ret = JS_NewArrayBuffer(
							ctx, (uint8_t *)(array->read().ptr()), array->size() * sizeof(Vector3), [](JSRuntime *rt, void *opaque, void *ptr) {
								memdelete(static_cast<PackedVector3Array *>(opaque));
							},
							array, false);
					return ret;
				},
				0);
	}

	{
		// PoolColorArray.prototype.get_buffer
		binder->get_builtin_binder().register_method(
				Variant::PACKED_COLOR_ARRAY,
				"get_buffer",
				[](JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
					ECMAScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, this_val);
					PackedColorArray *array = memnew(PackedColorArray(*bind->getPackedColorArray()));
					JSValue ret = JS_NewArrayBuffer(
							ctx, (uint8_t *)(array->read().ptr()), array->size() * sizeof(Color), [](JSRuntime *rt, void *opaque, void *ptr) {
								memdelete(static_cast<PackedColorArray *>(opaque));
							},
							array, false);
					return ret;
				},
				0);
	}
}
