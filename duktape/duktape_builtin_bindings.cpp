#include "duktape_builtin_bindings.h"
#include "core/resource.h"
#include "core/io/compression.h"

HashMap<Variant::Type, DuktapeHeapObject *> *class_prototypes = NULL;
HashMap<Variant::Type, DuktapeHeapObject *> *class_constructors = NULL;
Variant (*duk_get_variant)(duk_context *ctx, duk_idx_t idx) = NULL;
void (*duk_push_variant)(duk_context *ctx, const Variant &var) = NULL;
DuktapeHeapObject *godot_to_string_ptr = NULL;

duk_ret_t vector2_constructor(duk_context *ctx);
void vector2_properties(duk_context *ctx);
duk_ret_t rect2_constructor(duk_context *ctx);
void rect2_properties(duk_context *ctx);
duk_ret_t color_constructor(duk_context *ctx);
void color_properties(duk_context *ctx);
duk_ret_t rid_constructor(duk_context *ctx);
duk_ret_t transform2d_constructor(duk_context *ctx);
void transform2d_properties(duk_context *ctx);

duk_ret_t vector3_constructor(duk_context *ctx);
void vector3_properties(duk_context *ctx);
duk_ret_t basis_constructor(duk_context *ctx);
void basis_properties(duk_context *ctx);
duk_ret_t quat_constructor(duk_context *ctx);
void quat_properties(duk_context *ctx);
duk_ret_t plane_constructor(duk_context *ctx);
void plane_properties(duk_context *ctx);
duk_ret_t aabb_constructor(duk_context *ctx);
void aabb_properties(duk_context *ctx);
duk_ret_t transform_constructor(duk_context *ctx);
void transform_properties(duk_context *ctx);

duk_ret_t pool_byte_array_constructor(duk_context *ctx);
duk_ret_t pool_int_array_constructor(duk_context *ctx);
duk_ret_t pool_real_array_constructor(duk_context *ctx);
duk_ret_t pool_string_array_constructor(duk_context *ctx);
duk_ret_t pool_vector2_array_constructor(duk_context *ctx);
duk_ret_t pool_vector3_array_constructor(duk_context *ctx);
duk_ret_t pool_color_array_constructor(duk_context *ctx);
void pool_array_properties(duk_context *ctx);

void DuktapeBindingHelper::register_builtin_classes(duk_context *ctx) {

	class_prototypes = &builtin_class_prototypes;
	class_constructors = &builtin_class_constructors;
	godot_to_string_ptr = duk_ptr_godot_to_string;
	duk_get_variant = duk_get_godot_variant;
	duk_push_variant = duk_push_godot_variant;

	// register builtin classes
	register_builtin_class<Vector2>(ctx, vector2_constructor, 2, Variant::VECTOR2, "Vector2");
	register_builtin_class<Rect2>(ctx, rect2_constructor, 4, Variant::RECT2, "Rect2");
	register_builtin_class<Color>(ctx, color_constructor, 4, Variant::COLOR, "Color");
	register_builtin_class<RID>(ctx, rid_constructor, 1, Variant::_RID, "RID");
	register_builtin_class<Transform2D>(ctx, transform2d_constructor, 3, Variant::TRANSFORM2D, "Transform2D");

	register_builtin_class<Vector3>(ctx, vector3_constructor, 3, Variant::VECTOR3, "Vector3");
	register_builtin_class<Basis>(ctx, basis_constructor, 3, Variant::BASIS, "Basis");
	register_builtin_class<Quat>(ctx, quat_constructor, 4, Variant::QUAT, "Quat");
	register_builtin_class<Plane>(ctx, plane_constructor, 4, Variant::PLANE, "Plane");
	register_builtin_class<AABB>(ctx, aabb_constructor, 2, Variant::AABB, "AABB");
	register_builtin_class<Transform>(ctx, transform_constructor, 4, Variant::TRANSFORM, "Transform");

	register_builtin_class<PoolByteArray>(ctx, pool_byte_array_constructor, 1, Variant::POOL_BYTE_ARRAY, "PoolByteArray");
	register_builtin_class<PoolIntArray>(ctx, pool_int_array_constructor, 1, Variant::POOL_INT_ARRAY, "PoolIntArray");
	register_builtin_class<PoolRealArray>(ctx, pool_real_array_constructor, 1, Variant::POOL_REAL_ARRAY, "PoolRealArray");
	register_builtin_class<PoolStringArray>(ctx, pool_string_array_constructor, 1, Variant::POOL_STRING_ARRAY, "PoolStringArray");
	register_builtin_class<PoolVector2Array>(ctx, pool_vector2_array_constructor, 1, Variant::POOL_VECTOR2_ARRAY, "PoolVector2Array");
	register_builtin_class<PoolVector3Array>(ctx, pool_vector3_array_constructor, 1, Variant::POOL_VECTOR3_ARRAY, "PoolVector3Array");
	register_builtin_class<PoolColorArray>(ctx, pool_color_array_constructor, 1, Variant::POOL_COLOR_ARRAY, "PoolColorArray");

	// define properties of builtin classes
	register_builtin_class_properties(ctx);
	register_builtin_class_properties_gen(ctx);
}

void register_builtin_class_properties(duk_context *ctx) {
	vector2_properties(ctx);
	color_properties(ctx);
	rect2_properties(ctx);
	transform2d_properties(ctx);

	vector3_properties(ctx);
	basis_properties(ctx);
	quat_properties(ctx);
	plane_properties(ctx);
	aabb_properties(ctx);
	transform_properties(ctx);

	pool_array_properties(ctx);
}

duk_ret_t vector2_constructor(duk_context *ctx) {
	ERR_FAIL_COND_V(!duk_is_constructor_call(ctx), DUK_ERR_SYNTAX_ERROR);

	Vector2 *ptr = NULL;
	if (duk_is_object(ctx, 0)) {
		Vector2 v = duk_get_variant(ctx, 0);
		ptr = memnew(Vector2(v));
	} else {
		duk_double_t x = duk_get_number_default(ctx, 0, 0);
		duk_double_t y = duk_get_number_default(ctx, 1, 0);
		ptr = memnew(Vector2(x, y));
	}

	duk_push_this(ctx);
	duk_push_pointer(ctx, ptr);
	duk_put_prop_literal(ctx, -2, DUK_HIDDEN_SYMBOL("ptr"));
	duk_push_int(ctx, Variant::VECTOR2);
	duk_put_prop_literal(ctx, -2, DUK_HIDDEN_SYMBOL("type"));

	return DUK_NO_RET_VAL;
}

void vector2_properties(duk_context *ctx) {

	duk_push_heapptr(ctx, class_prototypes->get(Variant::VECTOR2));

	duk_push_c_function(ctx, [](duk_context *ctx) -> duk_ret_t {
		duk_push_this(ctx);
		Vector2 *ptr = duk_get_builtin_ptr<Vector2>(ctx, -1);
		ERR_FAIL_NULL_V(ptr, DUK_ERR_TYPE_ERROR);
		Variant arg0 = duk_get_variant(ctx, 0);
		Variant::Type type = arg0.get_type();
		ERR_FAIL_COND_V(!(type == Variant::VECTOR2 || type == Variant::REAL), DUK_ERR_TYPE_ERROR);
		Variant ret;
		if (type == Variant::VECTOR2) {
			Vector2 arg = arg0;
			ret = ptr->operator*(arg);
		} else if (type == Variant::REAL) {
			real_t arg = arg0;
			ret = ptr->operator*(arg);
		}
		duk_push_variant(ctx, ret);
		return DUK_HAS_RET_VAL;
	}, 1);
	duk_put_prop_literal(ctx, -2, "multiply");

	duk_push_c_function(ctx, [](duk_context *ctx) -> duk_ret_t {
		duk_push_this(ctx);
		Vector2 *ptr = duk_get_builtin_ptr<Vector2>(ctx, -1);
		ERR_FAIL_NULL_V(ptr, DUK_ERR_TYPE_ERROR);
		Variant arg0 = duk_get_variant(ctx, 0);
		Variant::Type type = arg0.get_type();
		ERR_FAIL_COND_V(!(type == Variant::VECTOR2 || type == Variant::REAL), DUK_ERR_TYPE_ERROR);

		if (type == Variant::VECTOR2) {
			Vector2 arg = arg0;
			ptr->operator*=(arg);
		} else if (type == Variant::REAL) {
			real_t arg = arg0;
			ptr->operator*=(arg);
		}
		duk_push_this(ctx);
		return DUK_HAS_RET_VAL;
	}, 1);
	duk_put_prop_literal(ctx, -2, "multiply_assign");

	duk_push_c_function(ctx, [](duk_context *ctx) -> duk_ret_t {
		duk_push_this(ctx);
		Vector2 *ptr = duk_get_builtin_ptr<Vector2>(ctx, -1);
		ERR_FAIL_NULL_V(ptr, DUK_ERR_TYPE_ERROR);
		Variant arg0 = duk_get_variant(ctx, 0);
		Variant::Type type = arg0.get_type();
		ERR_FAIL_COND_V(!(type == Variant::VECTOR2 || type == Variant::REAL), DUK_ERR_TYPE_ERROR);
		Variant ret;
		if (type == Variant::VECTOR2) {
			Vector2 arg = arg0;
			ret = ptr->operator/(arg);
		} else if (type == Variant::REAL) {
			real_t arg = arg0;
			ret = ptr->operator/(arg);
		}
		duk_push_variant(ctx, ret);
		return DUK_HAS_RET_VAL;
	}, 1);
	duk_put_prop_literal(ctx, -2, "divide");

	duk_push_c_function(ctx, [](duk_context *ctx) -> duk_ret_t {
		duk_push_this(ctx);
		Vector2 *ptr = duk_get_builtin_ptr<Vector2>(ctx, -1);
		ERR_FAIL_NULL_V(ptr, DUK_ERR_TYPE_ERROR);
		Variant arg0 = duk_get_variant(ctx, 0);
		Variant::Type type = arg0.get_type();
		ERR_FAIL_COND_V(!(type == Variant::VECTOR2 || type == Variant::REAL), DUK_ERR_TYPE_ERROR);
		Variant ret;
		if (type == Variant::VECTOR2) {
			Vector2 arg = arg0;
			ptr->operator/=(arg);
		} else if (type == Variant::REAL) {
			real_t arg = arg0;
			ptr->operator/=(arg);
		}
		duk_push_this(ctx);
		return DUK_HAS_RET_VAL;
	}, 1);
	duk_put_prop_literal(ctx, -2, "divide_assign");

	duk_push_c_function(ctx, [](duk_context *ctx) -> duk_ret_t {
		duk_push_this(ctx);
		Vector2 *ptr = duk_get_builtin_ptr<Vector2>(ctx, -1);
		ERR_FAIL_NULL_V(ptr, DUK_ERR_TYPE_ERROR);
		ptr->operator=(ptr->operator-());
		duk_push_this(ctx);
		return DUK_HAS_RET_VAL;
	}, 1);
	duk_put_prop_literal(ctx, -2, "negate_assign");

	duk_pop(ctx);
}

duk_ret_t vector3_constructor(duk_context *ctx) {
	ERR_FAIL_COND_V(!duk_is_constructor_call(ctx), DUK_ERR_SYNTAX_ERROR);

	Vector3 *ptr = NULL;
	if (duk_is_object(ctx, 0)) {
		Vector3 v = duk_get_variant(ctx, 0);
		ptr = memnew(Vector3(v));
	} else {
		duk_double_t x = duk_get_number_default(ctx, 0, 0);
		duk_double_t y = duk_get_number_default(ctx, 1, 0);
		duk_double_t z = duk_get_number_default(ctx, 2, 0);
		ptr = memnew(Vector3(x, y, z));
	}

	duk_push_this(ctx);
	duk_push_pointer(ctx, ptr);
	duk_put_prop_literal(ctx, -2, DUK_HIDDEN_SYMBOL("ptr"));
	duk_push_int(ctx, Variant::VECTOR3);
	duk_put_prop_literal(ctx, -2, DUK_HIDDEN_SYMBOL("type"));

	return DUK_NO_RET_VAL;
}

void vector3_properties(duk_context *ctx) {

	duk_push_heapptr(ctx, class_prototypes->get(Variant::VECTOR3));

	duk_push_c_function(ctx, [](duk_context *ctx) -> duk_ret_t {
		duk_push_this(ctx);
		Vector3 *ptr = duk_get_builtin_ptr<Vector3>(ctx, -1);
		ERR_FAIL_NULL_V(ptr, DUK_ERR_TYPE_ERROR);
		Variant arg0 = duk_get_variant(ctx, 0);
		Variant::Type type = arg0.get_type();
		ERR_FAIL_COND_V(!(type == Variant::VECTOR3 || type == Variant::REAL), DUK_ERR_TYPE_ERROR);
		Variant ret;
		if (type == Variant::VECTOR3) {
			Vector3 arg = arg0;
			ret = ptr->operator*(arg);
		} else if (type == Variant::REAL) {
			real_t arg = arg0;
			ret = ptr->operator*(arg);
		}
		duk_push_variant(ctx, ret);
		return DUK_HAS_RET_VAL;
	}, 1);
	duk_put_prop_literal(ctx, -2, "multiply");

	duk_push_c_function(ctx, [](duk_context *ctx) -> duk_ret_t {
		duk_push_this(ctx);
		Vector3 *ptr = duk_get_builtin_ptr<Vector3>(ctx, -1);
		ERR_FAIL_NULL_V(ptr, DUK_ERR_TYPE_ERROR);
		Variant arg0 = duk_get_variant(ctx, 0);
		Variant::Type type = arg0.get_type();
		ERR_FAIL_COND_V(!(type == Variant::VECTOR3 || type == Variant::REAL), DUK_ERR_TYPE_ERROR);

		if (type == Variant::VECTOR3) {
			Vector3 arg = arg0;
			ptr->operator*=(arg);
		} else if (type == Variant::REAL) {
			real_t arg = arg0;
			ptr->operator*=(arg);
		}
		duk_push_this(ctx);
		return DUK_HAS_RET_VAL;
	}, 1);
	duk_put_prop_literal(ctx, -2, "multiply_assign");

	duk_push_c_function(ctx, [](duk_context *ctx) -> duk_ret_t {
		duk_push_this(ctx);
		Vector3 *ptr = duk_get_builtin_ptr<Vector3>(ctx, -1);
		ERR_FAIL_NULL_V(ptr, DUK_ERR_TYPE_ERROR);
		Variant arg0 = duk_get_variant(ctx, 0);
		Variant::Type type = arg0.get_type();
		ERR_FAIL_COND_V(!(type == Variant::VECTOR3 || type == Variant::REAL), DUK_ERR_TYPE_ERROR);
		Variant ret;
		if (type == Variant::VECTOR3) {
			Vector3 arg = arg0;
			ret = ptr->operator/(arg);
		} else if (type == Variant::REAL) {
			real_t arg = arg0;
			ret = ptr->operator/(arg);
		}
		duk_push_variant(ctx, ret);
		return DUK_HAS_RET_VAL;
	}, 1);
	duk_put_prop_literal(ctx, -2, "divide");

	duk_push_c_function(ctx, [](duk_context *ctx) -> duk_ret_t {
		duk_push_this(ctx);
		Vector3 *ptr = duk_get_builtin_ptr<Vector3>(ctx, -1);
		ERR_FAIL_NULL_V(ptr, DUK_ERR_TYPE_ERROR);
		Variant arg0 = duk_get_variant(ctx, 0);
		Variant::Type type = arg0.get_type();
		ERR_FAIL_COND_V(!(type == Variant::VECTOR3 || type == Variant::REAL), DUK_ERR_TYPE_ERROR);

		if (type == Variant::VECTOR3) {
			Vector3 arg = arg0;
			ptr->operator/=(arg);
		} else if (type == Variant::REAL) {
			real_t arg = arg0;
			ptr->operator/=(arg);
		}
		duk_push_this(ctx);
		return DUK_HAS_RET_VAL;
	}, 1);
	duk_put_prop_literal(ctx, -2, "divide_assign");

	duk_push_c_function(ctx, [](duk_context *ctx) -> duk_ret_t {
		duk_push_this(ctx);
		Vector3 *ptr = duk_get_builtin_ptr<Vector3>(ctx, -1);
		ERR_FAIL_NULL_V(ptr, DUK_ERR_TYPE_ERROR);
		ptr->operator=(ptr->operator-());
		duk_push_this(ctx);
		return DUK_HAS_RET_VAL;
	}, 1);
	duk_put_prop_literal(ctx, -2, "negate_assign");

	duk_pop(ctx);
}

duk_ret_t rect2_constructor(duk_context *ctx) {
	ERR_FAIL_COND_V(!duk_is_constructor_call(ctx), DUK_ERR_SYNTAX_ERROR);

	Rect2 *ptr = NULL;
	Variant arg0 = duk_get_variant(ctx, 0);
	switch (arg0.get_type()) {
		case Variant::REAL: {
			real_t x = arg0;
			real_t y = duk_get_number_default(ctx, 1, 0);
			real_t w = duk_get_number_default(ctx, 2, 0);
			real_t h = duk_get_number_default(ctx, 3, 0);
			ptr = memnew(Rect2(x, y, w, h));
		} break;
		case Variant::VECTOR2: {
			Vector2 pos = arg0;
			Vector2 size = duk_get_variant(ctx, 1);
			ptr = memnew(Rect2(pos, size));
		} break;
		case Variant::RECT2: {
			Rect2 r = arg0;
			ptr = memnew(Rect2(r));
		} break;
		default:
			ptr = memnew(Rect2());
			break;
	}
	ERR_FAIL_NULL_V(ptr, DUK_ERR_TYPE_ERROR);

	duk_push_this(ctx);
	duk_push_pointer(ctx, ptr);
	duk_put_prop_literal(ctx, -2, DUK_HIDDEN_SYMBOL("ptr"));
	duk_push_int(ctx, Variant::RECT2);
	duk_put_prop_literal(ctx, -2, DUK_HIDDEN_SYMBOL("type"));

	return DUK_NO_RET_VAL;
}

void rect2_properties(duk_context *ctx) {

	duk_push_heapptr(ctx, class_prototypes->get(Variant::COLOR));

	duk_push_literal(ctx, "end");
	duk_c_function func = [](duk_context *ctx) -> duk_ret_t {
		duk_int_t argc = duk_get_top(ctx);
		duk_push_this(ctx);
		Rect2 *ptr = duk_get_builtin_ptr<Rect2>(ctx, -1);
		ERR_FAIL_NULL_V(ptr, DUK_ERR_TYPE_ERROR);
		if (argc > 0) {
			Vector2 arg = (duk_get_variant(ctx, 0));
			ptr->size = arg - ptr->position;
		}
		duk_push_variant(ctx, ptr->position + ptr->size);
		return DUK_HAS_RET_VAL;
	};
	duk_push_c_function(ctx, func, 0);
	duk_push_c_function(ctx, func, 1);
	duk_def_prop(ctx, -4, DUK_DEFPROP_HAVE_GETTER | DUK_DEFPROP_HAVE_SETTER| DUK_DEFPROP_ENUMERABLE);

	duk_pop(ctx);
}

duk_ret_t color_constructor(duk_context *ctx) {
	ERR_FAIL_COND_V(!duk_is_constructor_call(ctx), DUK_ERR_SYNTAX_ERROR);

	duk_push_this(ctx);
	Color *ptr = NULL;

	if (duk_is_string(ctx, 0)) {
		ptr = memnew(Color(Color::html(duk_get_string(ctx, 0))));
	} else if (duk_is_undefined(ctx, 1) && duk_is_number(ctx, 0)) {
		ptr = memnew(Color(Color::hex(duk_get_uint(ctx, 0))));
	} else if (duk_is_object(ctx, 0)) {
		Color c = duk_get_variant(ctx, 0);
		ptr = memnew(Color(c));
	} else {
		duk_double_t r = duk_get_number_default(ctx, 0, 0);
		duk_double_t g = duk_get_number_default(ctx, 1, 0);
		duk_double_t b = duk_get_number_default(ctx, 2, 0);
		duk_double_t a = duk_get_number_default(ctx, 3, 1);
		ptr = memnew(Color(r, g, b, a));
	}

	duk_push_pointer(ctx, ptr);
	duk_put_prop_literal(ctx, -2, DUK_HIDDEN_SYMBOL("ptr"));
	duk_push_int(ctx, Variant::COLOR);
	duk_put_prop_literal(ctx, -2, DUK_HIDDEN_SYMBOL("type"));

	return DUK_NO_RET_VAL;
}

void color_properties(duk_context *ctx) {

	duk_push_heapptr(ctx, class_prototypes->get(Variant::COLOR));

	duk_push_c_function(ctx, [](duk_context *ctx) -> duk_ret_t {
		duk_push_this(ctx);
		Color *ptr = duk_get_builtin_ptr<Color>(ctx, -1);
		ERR_FAIL_NULL_V(ptr, DUK_ERR_TYPE_ERROR);
		Variant arg0 = duk_get_variant(ctx, 0);
		Variant::Type type = arg0.get_type();
		ERR_FAIL_COND_V(!(type == Variant::COLOR || type == Variant::REAL), DUK_ERR_TYPE_ERROR);
		Variant ret;
		if (type == Variant::COLOR) {
			Color arg = arg0;
			ret = ptr->operator*(arg);
		} else if (type == Variant::REAL) {
			real_t arg = arg0;
			ret = ptr->operator*(arg);
		}
		duk_push_variant(ctx, ret);
		return DUK_HAS_RET_VAL;
	}, 1);
	duk_put_prop_literal(ctx, -2, "multiply");

	duk_push_c_function(ctx, [](duk_context *ctx) -> duk_ret_t {
		duk_push_this(ctx);
		Color *ptr = duk_get_builtin_ptr<Color>(ctx, -1);
		ERR_FAIL_NULL_V(ptr, DUK_ERR_TYPE_ERROR);
		Variant arg0 = duk_get_variant(ctx, 0);
		Variant::Type type = arg0.get_type();
		ERR_FAIL_COND_V(!(type == Variant::COLOR || type == Variant::REAL), DUK_ERR_TYPE_ERROR);

		if (type == Variant::COLOR) {
			Color arg = arg0;
			ptr->operator*=(arg);
		} else if (type == Variant::REAL) {
			real_t arg = arg0;
			ptr->operator*=(arg);
		}
		duk_push_this(ctx);
		return DUK_HAS_RET_VAL;
	}, 1);
	duk_put_prop_literal(ctx, -2, "multiply_assign");

	duk_push_c_function(ctx, [](duk_context *ctx) -> duk_ret_t {
		duk_push_this(ctx);
		Color *ptr = duk_get_builtin_ptr<Color>(ctx, -1);
		ERR_FAIL_NULL_V(ptr, DUK_ERR_TYPE_ERROR);
		Variant arg0 = duk_get_variant(ctx, 0);
		Variant::Type type = arg0.get_type();
		ERR_FAIL_COND_V(!(type == Variant::COLOR || type == Variant::REAL), DUK_ERR_TYPE_ERROR);
		Variant ret;
		if (type == Variant::COLOR) {
			Color arg = arg0;
			ret = ptr->operator/(arg);
		} else if (type == Variant::REAL) {
			real_t arg = arg0;
			ret = ptr->operator/(arg);
		}
		duk_push_variant(ctx, ret);
		return DUK_HAS_RET_VAL;
	}, 1);
	duk_put_prop_literal(ctx, -2, "divide");

	duk_push_c_function(ctx, [](duk_context *ctx) -> duk_ret_t {
		duk_push_this(ctx);
		Color *ptr = duk_get_builtin_ptr<Color>(ctx, -1);
		ERR_FAIL_NULL_V(ptr, DUK_ERR_TYPE_ERROR);
		Variant arg0 = duk_get_variant(ctx, 0);
		Variant::Type type = arg0.get_type();
		ERR_FAIL_COND_V(!(type == Variant::COLOR || type == Variant::REAL), DUK_ERR_TYPE_ERROR);

		if (type == Variant::COLOR) {
			Color arg = arg0;
			ptr->operator/=(arg);
		} else if (type == Variant::REAL) {
			real_t arg = arg0;
			ptr->operator/=(arg);
		}
		duk_push_this(ctx);
		return DUK_HAS_RET_VAL;
	}, 1);
	duk_put_prop_literal(ctx, -2, "divide_assign");

	duk_push_c_function(ctx, [](duk_context *ctx) -> duk_ret_t {
		duk_push_this(ctx);
		Color *ptr = duk_get_builtin_ptr<Color>(ctx, -1);
		ERR_FAIL_NULL_V(ptr, DUK_ERR_TYPE_ERROR);
		ptr->operator=(ptr->operator-());
		duk_push_this(ctx);
		return DUK_HAS_RET_VAL;
	}, 1);
	duk_put_prop_literal(ctx, -2, "negate_assign");

	duk_push_literal(ctx, "h");
	duk_c_function func = [](duk_context *ctx) -> duk_ret_t {
		duk_idx_t argc = duk_get_top(ctx);
		duk_push_this(ctx);
		Color *ptr = duk_get_builtin_ptr<Color>(ctx, -1);
		ERR_FAIL_NULL_V(ptr, DUK_ERR_TYPE_ERROR);
		if (argc > 0) {
			ptr->set_hsv(duk_get_number_default(ctx, 0, 0), ptr->get_s(), ptr->get_v(), ptr->a);
		}
		duk_push_number(ctx, ptr->get_h());
		return DUK_HAS_RET_VAL;
	};
	duk_push_c_function(ctx, func, 0);
	duk_push_c_function(ctx, func, 1);
	duk_def_prop(ctx, -4, DUK_DEFPROP_HAVE_GETTER | DUK_DEFPROP_HAVE_SETTER| DUK_DEFPROP_ENUMERABLE);

	duk_push_literal(ctx, "s");
	func = [](duk_context *ctx) -> duk_ret_t {
		duk_idx_t argc = duk_get_top(ctx);
		duk_push_this(ctx);
		Color *ptr = duk_get_builtin_ptr<Color>(ctx, -1);
		ERR_FAIL_NULL_V(ptr, DUK_ERR_TYPE_ERROR);
		if (argc > 0) {
			ptr->set_hsv(ptr->get_h(), duk_get_number_default(ctx, 0, 0), ptr->get_v(), ptr->a);
		}
		duk_push_number(ctx, ptr->get_s());
		return DUK_HAS_RET_VAL;
	};
	duk_push_c_function(ctx, func, 0);
	duk_push_c_function(ctx, func, 1);
	duk_def_prop(ctx, -4, DUK_DEFPROP_HAVE_GETTER | DUK_DEFPROP_HAVE_SETTER| DUK_DEFPROP_ENUMERABLE);

	duk_push_literal(ctx, "v");
	func = [](duk_context *ctx) -> duk_ret_t {
		duk_idx_t argc = duk_get_top(ctx);
		duk_push_this(ctx);
		Color *ptr = duk_get_builtin_ptr<Color>(ctx, -1);
		ERR_FAIL_NULL_V(ptr, DUK_ERR_TYPE_ERROR);
		if (argc > 0) {
			ptr->set_hsv(ptr->get_h(), ptr->get_s(), duk_get_number_default(ctx, 0, 0), ptr->a);
		}
		duk_push_number(ctx, ptr->get_v());
		return DUK_HAS_RET_VAL;
	};
	duk_push_c_function(ctx, func, 0);
	duk_push_c_function(ctx, func, 1);
	duk_def_prop(ctx, -4, DUK_DEFPROP_HAVE_GETTER | DUK_DEFPROP_HAVE_SETTER| DUK_DEFPROP_ENUMERABLE);

	duk_push_literal(ctx, "r8");
	func = [](duk_context *ctx) -> duk_ret_t {
		duk_idx_t argc = duk_get_top(ctx);
		duk_push_this(ctx);
		Color *ptr = duk_get_builtin_ptr<Color>(ctx, -1);
		ERR_FAIL_NULL_V(ptr, DUK_ERR_TYPE_ERROR);
		if (argc > 0) {
			ptr->r =  duk_get_number_default(ctx, 0, 0) / 255.0;
		}
		duk_push_number(ctx, Math::round(ptr->r * 255.0));
		return DUK_HAS_RET_VAL;
	};
	duk_push_c_function(ctx, func, 0);
	duk_push_c_function(ctx, func, 1);
	duk_def_prop(ctx, -4, DUK_DEFPROP_HAVE_GETTER | DUK_DEFPROP_HAVE_SETTER| DUK_DEFPROP_ENUMERABLE);

	duk_push_literal(ctx, "g8");
	func = [](duk_context *ctx) -> duk_ret_t {
		duk_idx_t argc = duk_get_top(ctx);
		duk_push_this(ctx);
		Color *ptr = duk_get_builtin_ptr<Color>(ctx, -1);
		ERR_FAIL_NULL_V(ptr, DUK_ERR_TYPE_ERROR);
		if (argc) {
			ptr->g = duk_get_number_default(ctx, 0, 0) / 255.0;
		}
		duk_push_number(ctx, Math::round(ptr->g * 255.0));
		return DUK_HAS_RET_VAL;
	};
	duk_push_c_function(ctx, func, 0);
	duk_push_c_function(ctx, func, 1);
	duk_def_prop(ctx, -4, DUK_DEFPROP_HAVE_GETTER | DUK_DEFPROP_HAVE_SETTER| DUK_DEFPROP_ENUMERABLE);

	duk_push_literal(ctx, "b8");
	func = [](duk_context *ctx) -> duk_ret_t {
		duk_idx_t argc = duk_get_top(ctx);
		duk_push_this(ctx);
		Color *ptr = duk_get_builtin_ptr<Color>(ctx, -1);
		ERR_FAIL_NULL_V(ptr, DUK_ERR_TYPE_ERROR);
		if (argc) {
			ptr->b = duk_get_number_default(ctx, 0, 0) / 255.0;
		}
		duk_push_number(ctx, Math::round(ptr->b * 255.0));
		return DUK_HAS_RET_VAL;
	};
	duk_push_c_function(ctx, func, 0);
	duk_push_c_function(ctx, func, 1);
	duk_def_prop(ctx, -4, DUK_DEFPROP_HAVE_GETTER | DUK_DEFPROP_HAVE_SETTER| DUK_DEFPROP_ENUMERABLE);

	duk_push_literal(ctx, "a8");
	func = [](duk_context *ctx) -> duk_ret_t {
		duk_idx_t argc = duk_get_top(ctx);
		duk_push_this(ctx);
		Color *ptr = duk_get_builtin_ptr<Color>(ctx, -1);
		ERR_FAIL_NULL_V(ptr, DUK_ERR_TYPE_ERROR);
		if (argc) {
			ptr->a = duk_get_number_default(ctx, 0, 0) / 255.0;
		}
		duk_push_number(ctx, Math::round(ptr->a * 255.0));
		return DUK_HAS_RET_VAL;
	};
	duk_push_c_function(ctx, func, 0);
	duk_push_c_function(ctx, func, 1);
	duk_def_prop(ctx, -4, DUK_DEFPROP_HAVE_GETTER | DUK_DEFPROP_HAVE_SETTER| DUK_DEFPROP_ENUMERABLE);

	duk_pop(ctx);
}

duk_ret_t rid_constructor(duk_context *ctx) {
	ERR_FAIL_COND_V(!duk_is_constructor_call(ctx), DUK_ERR_SYNTAX_ERROR);

	duk_push_this(ctx);

	RID *ptr = NULL;
	Variant from = duk_get_variant(ctx, 0);
	if (from.get_type() == Variant::_RID) {
		RID rid = from;
		ptr = memnew(RID(rid));
	} else {
		Object * obj = from;
		if(Resource * res = Object::cast_to<Resource>(obj)) {
			ptr = memnew(RID(res->get_rid()));
		}
	}
	ERR_FAIL_NULL_V(ptr, DUK_ERR_TYPE_ERROR);

	duk_push_pointer(ctx, ptr);
	duk_put_prop_literal(ctx, -2, DUK_HIDDEN_SYMBOL("ptr"));
	duk_push_int(ctx, Variant::_RID);
	duk_put_prop_literal(ctx, -2, DUK_HIDDEN_SYMBOL("type"));

	return DUK_NO_RET_VAL;
}

duk_ret_t transform2d_constructor(duk_context *ctx) {
	ERR_FAIL_COND_V(!duk_is_constructor_call(ctx), DUK_ERR_SYNTAX_ERROR);

	duk_push_this(ctx);

	Transform2D *ptr = NULL;
	Variant arg0 = duk_get_variant(ctx, 0);
	switch (arg0.get_type()) {
		case Variant::TRANSFORM: {
			Transform xf = arg0;
			// TODO
		} break;
		case Variant::VECTOR2: {
				Vector2 arg1 = duk_get_variant(ctx, 1);
				Vector2 arg2 = duk_get_variant(ctx, 2);
				ptr = memnew(Transform2D());
				ptr->elements[0] = arg0;
				ptr->elements[1] = arg1;
				ptr->elements[2] = arg2;
			} break;
		case Variant::REAL: {
			Vector2 arg1 = duk_get_variant(ctx, 1);
			ptr = memnew(Transform2D(arg0, arg1));
		} break;
		case Variant::TRANSFORM2D: {
			Transform2D xf = arg0;
			ptr = memnew(Transform2D(xf));
		} break;
		default:
			ptr = memnew(Transform2D());
			break;
	}
	ERR_FAIL_NULL_V(ptr, DUK_ERR_TYPE_ERROR);

	duk_push_pointer(ctx, ptr);
	duk_put_prop_literal(ctx, -2, DUK_HIDDEN_SYMBOL("ptr"));
	duk_push_int(ctx, Variant::TRANSFORM2D);
	duk_put_prop_literal(ctx, -2, DUK_HIDDEN_SYMBOL("type"));

	return DUK_NO_RET_VAL;
}

void transform2d_properties(duk_context *ctx) {
	duk_push_heapptr(ctx, class_prototypes->get(Variant::TRANSFORM2D));

	duk_push_c_function(ctx, [](duk_context *ctx) -> duk_ret_t {
		duk_push_this(ctx);
		Transform2D *ptr = duk_get_builtin_ptr<Transform2D>(ctx, -1);
		ERR_FAIL_NULL_V(ptr, DUK_ERR_TYPE_ERROR);
		Variant arg0 = duk_get_variant(ctx, 0);
		Variant ret;
		switch (arg0.get_type()) {
			case Variant::VECTOR2: ret = ptr->xform(Vector2(arg0));
			case Variant::RECT2: ret = ptr->xform(Rect2(arg0));
			default:
				return DUK_ERR_TYPE_ERROR;
		}
		duk_push_variant(ctx, ret);
		return DUK_HAS_RET_VAL;
	}, 1);
	duk_put_prop_literal(ctx, -2, "xform");

	duk_push_c_function(ctx, [](duk_context *ctx) -> duk_ret_t {
		duk_push_this(ctx);
		Transform2D *ptr = duk_get_builtin_ptr<Transform2D>(ctx, -1);
		ERR_FAIL_NULL_V(ptr, DUK_ERR_TYPE_ERROR);
		Variant arg0 = duk_get_variant(ctx, 0);
		Variant ret;
		switch (arg0.get_type()) {
			case Variant::VECTOR2: ret = ptr->xform_inv(Vector2(arg0));
			case Variant::RECT2: ret = ptr->xform_inv(Rect2(arg0));
			default:
				return DUK_ERR_TYPE_ERROR;
		}
		duk_push_variant(ctx, ret);
		return DUK_HAS_RET_VAL;
	}, 1);
	duk_put_prop_literal(ctx, -2, "xform_inv");

	duk_push_literal(ctx, "x");
	duk_c_function func = [](duk_context *ctx) -> duk_ret_t {
		duk_int_t argc = duk_get_top(ctx);
		duk_push_this(ctx);
		Transform2D *ptr = duk_get_builtin_ptr<Transform2D>(ctx, -1);
		ERR_FAIL_NULL_V(ptr, DUK_ERR_TYPE_ERROR);
		if (argc) {
			Vector2 arg = duk_get_variant(ctx, 0);
			ptr->elements[0] = arg;
		}
		duk_push_variant(ctx, ptr->elements[0]);
		return DUK_HAS_RET_VAL;
	};
	duk_push_c_function(ctx, func, 0);
	duk_push_c_function(ctx, func, 1);
	duk_def_prop(ctx, -4, DUK_DEFPROP_HAVE_GETTER | DUK_DEFPROP_HAVE_SETTER| DUK_DEFPROP_ENUMERABLE);


	duk_push_literal(ctx, "y");
	func = [](duk_context *ctx) -> duk_ret_t {
		duk_int_t argc = duk_get_top(ctx);
		duk_push_this(ctx);
		Transform2D *ptr = duk_get_builtin_ptr<Transform2D>(ctx, -1);
		ERR_FAIL_NULL_V(ptr, DUK_ERR_TYPE_ERROR);
		if (argc) {
			Vector2 arg = duk_get_variant(ctx, 0);
			ptr->elements[1] = arg;
		}
		duk_push_variant(ctx, ptr->elements[1]);
		return DUK_HAS_RET_VAL;
	};
	duk_push_c_function(ctx, func, 0);
	duk_push_c_function(ctx, func, 1);
	duk_def_prop(ctx, -4, DUK_DEFPROP_HAVE_GETTER | DUK_DEFPROP_HAVE_SETTER| DUK_DEFPROP_ENUMERABLE);

	duk_push_literal(ctx, "origin");
	func = [](duk_context *ctx) -> duk_ret_t {
		duk_int_t argc = duk_get_top(ctx);
		duk_push_this(ctx);
		Transform2D *ptr = duk_get_builtin_ptr<Transform2D>(ctx, -1);
		ERR_FAIL_NULL_V(ptr, DUK_ERR_TYPE_ERROR);
		if (argc) {
			Vector2 arg = duk_get_variant(ctx, 0);
			ptr->elements[2] = arg;
		}
		duk_push_variant(ctx, ptr->elements[2]);
		return DUK_HAS_RET_VAL;
	};
	duk_push_c_function(ctx, func, 0);
	duk_push_c_function(ctx, func, 1);
	duk_def_prop(ctx, -4, DUK_DEFPROP_HAVE_GETTER | DUK_DEFPROP_HAVE_SETTER| DUK_DEFPROP_ENUMERABLE);

	duk_pop(ctx);
}

duk_ret_t basis_constructor(duk_context *ctx) {
	ERR_FAIL_COND_V(!duk_is_constructor_call(ctx), DUK_ERR_SYNTAX_ERROR);

	duk_push_this(ctx);

	Basis *ptr = NULL;
	Variant arg0 = duk_get_variant(ctx, 0);
	switch (arg0.get_type()) {
		case Variant::QUAT: {
			Quat q = arg0;
			ptr = memnew(Basis(q));
		} break;
		case Variant::VECTOR3: {
			Vector3 p1 = arg0;
			Variant arg1 = duk_get_variant(ctx, 1);
			if (arg1.get_type() == Variant::REAL) {
				ptr = memnew(Basis(p1, real_t(arg1)));
			} else {
				Vector3 p2 = arg1;
				Vector3 p3 = duk_get_variant(ctx, 2);
				ptr = memnew(Basis(p1, p2, p3));
			}
		} break;
		case Variant::BASIS: {
			Basis b = arg0;
			ptr = memnew(Basis(b));
		} break;
		default:
			ptr = memnew(Basis);
			break;
	}
	ERR_FAIL_NULL_V(ptr, DUK_ERR_TYPE_ERROR);

	duk_push_pointer(ctx, ptr);
	duk_put_prop_literal(ctx, -2, DUK_HIDDEN_SYMBOL("ptr"));
	duk_push_int(ctx, Variant::BASIS);
	duk_put_prop_literal(ctx, -2, DUK_HIDDEN_SYMBOL("type"));
	return DUK_NO_RET_VAL;
}

void basis_properties(duk_context *ctx) {
	duk_push_heapptr(ctx, class_prototypes->get(Variant::BASIS));

	duk_push_literal(ctx, "x");
	duk_c_function func = [](duk_context *ctx) -> duk_ret_t {
		duk_int_t argc = duk_get_top(ctx);
		duk_push_this(ctx);
		Basis *ptr = duk_get_builtin_ptr<Basis>(ctx, -1);
		ERR_FAIL_NULL_V(ptr, DUK_ERR_TYPE_ERROR);
		if (argc) {
			Vector3 arg = duk_get_variant(ctx, 0);
			ptr->elements[0] = arg;
		}
		duk_push_variant(ctx, ptr->elements[0]);
		return DUK_HAS_RET_VAL;
	};
	duk_push_c_function(ctx, func, 0);
	duk_push_c_function(ctx, func, 1);
	duk_def_prop(ctx, -4, DUK_DEFPROP_HAVE_GETTER | DUK_DEFPROP_HAVE_SETTER| DUK_DEFPROP_ENUMERABLE);

	duk_push_literal(ctx, "y");
	func = [](duk_context *ctx) -> duk_ret_t {
		duk_int_t argc = duk_get_top(ctx);
		duk_push_this(ctx);
		Basis *ptr = duk_get_builtin_ptr<Basis>(ctx, -1);
		ERR_FAIL_NULL_V(ptr, DUK_ERR_TYPE_ERROR);
		if (argc) {
			Vector3 arg = duk_get_variant(ctx, 0);
			ptr->elements[1] = arg;
		}
		duk_push_variant(ctx, ptr->elements[1]);
		return DUK_HAS_RET_VAL;
	};
	duk_push_c_function(ctx, func, 0);
	duk_push_c_function(ctx, func, 1);
	duk_def_prop(ctx, -4, DUK_DEFPROP_HAVE_GETTER | DUK_DEFPROP_HAVE_SETTER| DUK_DEFPROP_ENUMERABLE);

	duk_push_literal(ctx, "z");
	func = [](duk_context *ctx) -> duk_ret_t {
		duk_int_t argc = duk_get_top(ctx);
		duk_push_this(ctx);
		Basis *ptr = duk_get_builtin_ptr<Basis>(ctx, -1);
		ERR_FAIL_NULL_V(ptr, DUK_ERR_TYPE_ERROR);
		if (argc) {
			Vector3 arg = duk_get_variant(ctx, 0);
			ptr->elements[2] = arg;
		}
		duk_push_variant(ctx, ptr->elements[2]);
		return DUK_HAS_RET_VAL;
	};
	duk_push_c_function(ctx, func, 0);
	duk_push_c_function(ctx, func, 1);
	duk_def_prop(ctx, -4, DUK_DEFPROP_HAVE_GETTER | DUK_DEFPROP_HAVE_SETTER| DUK_DEFPROP_ENUMERABLE);

	duk_push_c_function(ctx, [](duk_context *ctx) -> duk_ret_t {
		duk_push_this(ctx);
		Basis *ptr = duk_get_builtin_ptr<Basis>(ctx, -1);
		ERR_FAIL_NULL_V(ptr, DUK_ERR_TYPE_ERROR);
		duk_push_variant(ctx, ptr->operator Quat());
		return DUK_HAS_RET_VAL;
	}, 0);
	duk_put_prop_literal(ctx, -2, "to_quat");

	duk_push_c_function(ctx, [](duk_context *ctx) -> duk_ret_t {
		duk_push_this(ctx);
		Basis *ptr = duk_get_builtin_ptr<Basis>(ctx, -1);
		ERR_FAIL_NULL_V(ptr, DUK_ERR_TYPE_ERROR);
		Variant arg0 = duk_get_variant(ctx, 0);
		Variant::Type type = arg0.get_type();
		ERR_FAIL_COND_V(!(type == Variant::BASIS || type == Variant::REAL), DUK_ERR_TYPE_ERROR);
		Variant ret;
		if (type == Variant::BASIS) {
			Basis arg = arg0;
			ret = ptr->operator*(arg);
		} else if (type == Variant::REAL) {
			real_t arg = arg0;
			ret = ptr->operator*(arg);
		}
		duk_push_variant(ctx, ret);
		return DUK_HAS_RET_VAL;
	}, 1);
	duk_put_prop_literal(ctx, -2, "multiply");

	duk_push_c_function(ctx, [](duk_context *ctx) -> duk_ret_t {
		duk_push_this(ctx);
		Basis *ptr = duk_get_builtin_ptr<Basis>(ctx, -1);
		ERR_FAIL_NULL_V(ptr, DUK_ERR_TYPE_ERROR);
		Variant arg0 = duk_get_variant(ctx, 0);
		Variant::Type type = arg0.get_type();
		ERR_FAIL_COND_V(!(type == Variant::VECTOR3 || type == Variant::REAL), DUK_ERR_TYPE_ERROR);

		if (type == Variant::BASIS) {
			Basis arg = arg0;
			ptr->operator*=(arg);
		} else if (type == Variant::REAL) {
			real_t arg = arg0;
			ptr->operator*=(arg);
		}
		duk_push_this(ctx);
		return DUK_HAS_RET_VAL;
	}, 1);
	duk_put_prop_literal(ctx, -2, "multiply_assign");

	duk_pop(ctx);
}

duk_ret_t quat_constructor(duk_context *ctx) {

	ERR_FAIL_COND_V(!duk_is_constructor_call(ctx), DUK_ERR_SYNTAX_ERROR);

	duk_push_this(ctx);

	Quat *ptr = NULL;
	Variant arg0 = duk_get_variant(ctx, 0);
	switch (arg0.get_type()) {
		case Variant::BASIS: {
			Basis b = arg0;
			ptr = memnew(Quat(b));
		} break;
		case Variant::VECTOR3: {
			Vector3 p1 = arg0;
			Variant arg1 = duk_get_variant(ctx, 1);
			if (arg1.get_type() == Variant::REAL) {
				ptr = memnew(Quat(p1, real_t(arg1)));
			} else {
				ptr = memnew(Quat(p1));
			}
		} break;
		case Variant::REAL: {
			real_t x = arg0;
			real_t y = duk_get_number_default(ctx, 1, 0);
			real_t z = duk_get_number_default(ctx, 2, 0);
			real_t w = duk_get_number_default(ctx, 3, 0);
			ptr = memnew(Quat(x, y, z, w));
		} break;
		case Variant::QUAT: {
			Quat q = arg0;
			ptr = memnew(Quat(q));
		} break;
		default:
			ptr = memnew(Quat);
			break;
	}
	ERR_FAIL_NULL_V(ptr, DUK_ERR_TYPE_ERROR);

	duk_push_pointer(ctx, ptr);
	duk_put_prop_literal(ctx, -2, DUK_HIDDEN_SYMBOL("ptr"));
	duk_push_int(ctx, Variant::BASIS);
	duk_put_prop_literal(ctx, -2, DUK_HIDDEN_SYMBOL("type"));
	return DUK_NO_RET_VAL;

}

void quat_properties(duk_context *ctx) {

	duk_push_heapptr(ctx, class_prototypes->get(Variant::QUAT));

	duk_push_c_function(ctx, [](duk_context *ctx) -> duk_ret_t {
		duk_push_this(ctx);
		Quat *ptr = duk_get_builtin_ptr<Quat>(ctx, -1);
		ERR_FAIL_NULL_V(ptr, DUK_ERR_TYPE_ERROR);
		ptr->operator=(ptr->operator-());
		duk_push_this(ctx);
		return DUK_HAS_RET_VAL;
	}, 1);
	duk_put_prop_literal(ctx, -2, "negate_assign");

	duk_push_c_function(ctx, [](duk_context *ctx) -> duk_ret_t {
		duk_push_this(ctx);
		Quat *ptr = duk_get_builtin_ptr<Quat>(ctx, -1);
		ERR_FAIL_NULL_V(ptr, DUK_ERR_TYPE_ERROR);
		Variant arg0 = duk_get_variant(ctx, 0);
		Variant::Type type = arg0.get_type();
		ERR_FAIL_COND_V(!(type == Variant::VECTOR3 || type == Variant::REAL), DUK_ERR_TYPE_ERROR);
		Variant ret;
		if (type == Variant::VECTOR3) {
			Vector3 arg = arg0;
			ret = ptr->operator*(arg);
		} else if (type == Variant::REAL) {
			real_t arg = arg0;
			ret = ptr->operator*(arg);
		} else if (type == Variant::QUAT) {
			Quat arg = arg0;
			ret = ptr->operator*(arg);
		}
		duk_push_variant(ctx, ret);
		return DUK_HAS_RET_VAL;
	}, 1);
	duk_put_prop_literal(ctx, -2, "multiply");

	duk_push_c_function(ctx, [](duk_context *ctx) -> duk_ret_t {
		duk_push_this(ctx);
		Quat *ptr = duk_get_builtin_ptr<Quat>(ctx, -1);
		ERR_FAIL_NULL_V(ptr, DUK_ERR_TYPE_ERROR);
		Variant arg0 = duk_get_variant(ctx, 0);
		Variant::Type type = arg0.get_type();
		ERR_FAIL_COND_V(!(type == Variant::VECTOR3 || type == Variant::REAL), DUK_ERR_TYPE_ERROR);

		if (type == Variant::VECTOR3) {
			Vector3 arg = arg0;
			ptr->operator*=(arg);
		} else if (type == Variant::REAL) {
			real_t arg = arg0;
			ptr->operator*=(arg);
		} else if (type == Variant::QUAT) {
			Quat arg = arg0;
			ptr->operator*=(arg);
		}

		duk_push_this(ctx);
		return DUK_HAS_RET_VAL;
	}, 1);
	duk_put_prop_literal(ctx, -2, "multiply_assign");

	duk_push_c_function(ctx, [](duk_context *ctx) -> duk_ret_t {
		duk_push_this(ctx);
		Quat *ptr = duk_get_builtin_ptr<Quat>(ctx, -1);
		ERR_FAIL_NULL_V(ptr, DUK_ERR_TYPE_ERROR);
		real_t arg = duk_get_number_default(ctx, 0, 0);
		Variant ret = ptr->operator/(arg);
		duk_push_variant(ctx, ret);
		return DUK_HAS_RET_VAL;
	}, 1);
	duk_put_prop_literal(ctx, -2, "divide");

	duk_push_c_function(ctx, [](duk_context *ctx) -> duk_ret_t {
		duk_push_this(ctx);
		Quat *ptr = duk_get_builtin_ptr<Quat>(ctx, -1);
		ERR_FAIL_NULL_V(ptr, DUK_ERR_TYPE_ERROR);
		real_t arg = duk_get_number_default(ctx, 0, 0);
		ptr->operator=( ptr->operator/(arg) );
		duk_push_this(ctx);
		return DUK_HAS_RET_VAL;
	}, 1);
	duk_put_prop_literal(ctx, -2, "divide_assign");

	duk_pop(ctx);
}

duk_ret_t plane_constructor(duk_context *ctx) {
	ERR_FAIL_COND_V(!duk_is_constructor_call(ctx), DUK_ERR_SYNTAX_ERROR);

	duk_push_this(ctx);

	Plane *ptr = NULL;
	Variant arg0 = duk_get_variant(ctx, 0);
	switch (arg0.get_type()) {
		case Variant::VECTOR3: {
			Vector3 p1 = arg0;
			Variant arg1 = duk_get_variant(ctx, 1);
			if (arg1.get_type() == Variant::REAL) {
				ptr = memnew(Plane(p1, real_t(arg1)));
			} else {
				Vector3 p2 = arg1;
				Vector3 p3 = duk_get_variant(ctx, 2);
				ptr = memnew(Plane(p1, p2, p3));
			}
		} break;
		case Variant::REAL: {
			real_t x = arg0;
			real_t y = duk_get_number_default(ctx, 1, 0);
			real_t z = duk_get_number_default(ctx, 2, 0);
			real_t d = duk_get_number_default(ctx, 3, 0);
			ptr = memnew(Plane(x, y, z, d));
		} break;
		case Variant::PLANE: {
			Plane p = arg0;
			ptr = memnew(Plane(p));
		} break;
		default:
			ptr = memnew(Plane);
			break;
	}
	ERR_FAIL_NULL_V(ptr, DUK_ERR_TYPE_ERROR);

	duk_push_pointer(ctx, ptr);
	duk_put_prop_literal(ctx, -2, DUK_HIDDEN_SYMBOL("ptr"));
	duk_push_int(ctx, Variant::PLANE);
	duk_put_prop_literal(ctx, -2, DUK_HIDDEN_SYMBOL("type"));
	return DUK_NO_RET_VAL;
}

void plane_properties(duk_context *ctx) {
	duk_push_heapptr(ctx, class_prototypes->get(Variant::PLANE));

	duk_push_c_function(ctx, [](duk_context *ctx) -> duk_ret_t {
		duk_push_this(ctx);
		Plane *ptr = duk_get_builtin_ptr<Plane>(ctx, -1);
		ERR_FAIL_NULL_V(ptr, DUK_ERR_TYPE_ERROR);
		ptr->operator=(ptr->operator-());
		duk_push_this(ctx);
		return DUK_HAS_RET_VAL;
	}, 1);
	duk_put_prop_literal(ctx, -2, "negate_assign");

	duk_push_c_function(ctx, [](duk_context *ctx) -> duk_ret_t {
		duk_push_this(ctx);
		Plane *ptr = duk_get_builtin_ptr<Plane>(ctx, -1);
		ERR_FAIL_NULL_V(ptr, DUK_ERR_TYPE_ERROR);

		Plane p1 = duk_get_variant(ctx, 0);
		Plane p2 = duk_get_variant(ctx, 1);

		Vector3 ret;
		ptr->intersect_3(p1, p2, &ret);
		duk_push_variant(ctx, ret);

		return DUK_HAS_RET_VAL;
	}, 2);
	duk_put_prop_literal(ctx, -2, "intersect_3");

	duk_push_c_function(ctx, [](duk_context *ctx) -> duk_ret_t {
		duk_push_this(ctx);
		Plane *ptr = duk_get_builtin_ptr<Plane>(ctx, -1);
		ERR_FAIL_NULL_V(ptr, DUK_ERR_TYPE_ERROR);

		Vector3 p1 = duk_get_variant(ctx, 0);
		Vector3 p2 = duk_get_variant(ctx, 1);

		Vector3 ret;
		ptr->intersects_ray(p1, p2, &ret);
		duk_push_variant(ctx, ret);

		return DUK_HAS_RET_VAL;
	}, 2);
	duk_put_prop_literal(ctx, -2, "intersects_ray");

	duk_push_c_function(ctx, [](duk_context *ctx) -> duk_ret_t {
		duk_push_this(ctx);
		Plane *ptr = duk_get_builtin_ptr<Plane>(ctx, -1);
		ERR_FAIL_NULL_V(ptr, DUK_ERR_TYPE_ERROR);

		Vector3 p1 = duk_get_variant(ctx, 0);
		Vector3 p2 = duk_get_variant(ctx, 1);

		Vector3 ret;
		ptr->intersects_segment(p1, p2, &ret);
		duk_push_variant(ctx, ret);

		return DUK_HAS_RET_VAL;
	}, 2);
	duk_put_prop_literal(ctx, -2, "intersects_segment");

	duk_push_literal(ctx, "x");
	duk_c_function func = [](duk_context *ctx) -> duk_ret_t {
		duk_int_t argc = duk_get_top(ctx);
		duk_push_this(ctx);
		Plane *ptr = duk_get_builtin_ptr<Plane>(ctx, -1);
		ERR_FAIL_NULL_V(ptr, DUK_ERR_TYPE_ERROR);
		if (argc) {
			ptr->normal.x = duk_get_number_default(ctx, 0, 0);
		}
		duk_push_variant(ctx, ptr->normal.x);
		return DUK_HAS_RET_VAL;
	};
	duk_push_c_function(ctx, func, 0);
	duk_push_c_function(ctx, func, 1);
	duk_def_prop(ctx, -4, DUK_DEFPROP_HAVE_GETTER | DUK_DEFPROP_HAVE_SETTER| DUK_DEFPROP_ENUMERABLE);

	duk_push_literal(ctx, "y");
	func = [](duk_context *ctx) -> duk_ret_t {
		duk_int_t argc = duk_get_top(ctx);
		duk_push_this(ctx);
		Plane *ptr = duk_get_builtin_ptr<Plane>(ctx, -1);
		ERR_FAIL_NULL_V(ptr, DUK_ERR_TYPE_ERROR);
		if (argc) {
			ptr->normal.y = duk_get_number_default(ctx, 0, 0);
		}
		duk_push_variant(ctx, ptr->normal.y);
		return DUK_HAS_RET_VAL;
	};
	duk_push_c_function(ctx, func, 0);
	duk_push_c_function(ctx, func, 1);
	duk_def_prop(ctx, -4, DUK_DEFPROP_HAVE_GETTER | DUK_DEFPROP_HAVE_SETTER| DUK_DEFPROP_ENUMERABLE);

	duk_push_literal(ctx, "z");
	func = [](duk_context *ctx) -> duk_ret_t {
		duk_int_t argc = duk_get_top(ctx);
		duk_push_this(ctx);
		Plane *ptr = duk_get_builtin_ptr<Plane>(ctx, -1);
		ERR_FAIL_NULL_V(ptr, DUK_ERR_TYPE_ERROR);
		if (argc) {
			ptr->normal.z = duk_get_number_default(ctx, 0, 0);
		}
		duk_push_variant(ctx, ptr->normal.z);
		return DUK_HAS_RET_VAL;
	};
	duk_push_c_function(ctx, func, 0);
	duk_push_c_function(ctx, func, 1);
	duk_def_prop(ctx, -4, DUK_DEFPROP_HAVE_GETTER | DUK_DEFPROP_HAVE_SETTER| DUK_DEFPROP_ENUMERABLE);

	duk_pop(ctx);
}

duk_ret_t aabb_constructor(duk_context *ctx) {
	ERR_FAIL_COND_V(!duk_is_constructor_call(ctx), DUK_ERR_SYNTAX_ERROR);

	duk_push_this(ctx);

	AABB *ptr = NULL;
	Variant arg0 = duk_get_variant(ctx, 0);
	switch (arg0.get_type()) {
		case Variant::VECTOR3: {
			Vector3 p1 = arg0;
			Vector3 p2 = duk_get_variant(ctx, 1);
			ptr = memnew(AABB(p1, p2));
		} break;
		case Variant::AABB: {
			AABB p = arg0;
			ptr = memnew(AABB(p));
		} break;
		default:
			ptr = memnew(AABB);
			break;
	}
	ERR_FAIL_NULL_V(ptr, DUK_ERR_TYPE_ERROR);

	duk_push_pointer(ctx, ptr);
	duk_put_prop_literal(ctx, -2, DUK_HIDDEN_SYMBOL("ptr"));
	duk_push_int(ctx, Variant::AABB);
	duk_put_prop_literal(ctx, -2, DUK_HIDDEN_SYMBOL("type"));
	return DUK_NO_RET_VAL;
}

void aabb_properties(duk_context *ctx) {
	duk_push_heapptr(ctx, class_prototypes->get(Variant::AABB));

	duk_push_literal(ctx, "end");
	duk_c_function func = [](duk_context *ctx) -> duk_ret_t {
		duk_int_t argc = duk_get_top(ctx);
		duk_push_this(ctx);
		AABB *ptr = duk_get_builtin_ptr<AABB>(ctx, -1);
		ERR_FAIL_NULL_V(ptr, DUK_ERR_TYPE_ERROR);
		if (argc) {
			Vector3 arg = duk_get_variant(ctx, 0);
			ptr->size = arg - ptr->position;
		}
		duk_push_variant(ctx, ptr->size + ptr->position);
		return DUK_HAS_RET_VAL;
	};
	duk_push_c_function(ctx, func, 0);
	duk_push_c_function(ctx, func, 1);
	duk_def_prop(ctx, -4, DUK_DEFPROP_HAVE_GETTER | DUK_DEFPROP_HAVE_SETTER| DUK_DEFPROP_ENUMERABLE);

	duk_pop(ctx);
}

duk_ret_t transform_constructor(duk_context *ctx) {
	ERR_FAIL_COND_V(!duk_is_constructor_call(ctx), DUK_ERR_SYNTAX_ERROR);

	duk_push_this(ctx);

	Transform *ptr = NULL;
	Variant arg0 = duk_get_variant(ctx, 0);
	switch (arg0.get_type()) {
		case Variant::VECTOR3: {
			Basis b;
			b.elements[0] = arg0;
			b.elements[1] = duk_get_variant(ctx, 1);
			b.elements[2] = duk_get_variant(ctx, 2);
			Vector3 origin = duk_get_variant(ctx, 3);
			ptr = memnew(Transform(b, origin));
		} break;
		case Variant::BASIS: {
			Basis p1 = arg0;
			Variant p2 = duk_get_variant(ctx, 1);
			if (p2.get_type() == Variant::VECTOR3) {
				Vector3 arg2 = p2;
				ptr = memnew(Transform(p1, arg2));
			} else {
				ptr = memnew(Transform(p1));
			}
		} break;
		case Variant::QUAT: {
			Quat p1 = arg0;
			ptr = memnew(Transform(p1));
		} break;
		case Variant::TRANSFORM2D: {
			Transform2D p1 = arg0;
			// TODO: construct from Transform2D
//			ptr = memnew(Transform(p1));
		} break;
		case Variant::TRANSFORM: {
			Transform p = arg0;
			ptr = memnew(Transform(p));
		} break;
		default:
			ptr = memnew(Transform);
			break;
	}
	ERR_FAIL_NULL_V(ptr, DUK_ERR_TYPE_ERROR);

	duk_push_pointer(ctx, ptr);
	duk_put_prop_literal(ctx, -2, DUK_HIDDEN_SYMBOL("ptr"));
	duk_push_int(ctx, Variant::TRANSFORM);
	duk_put_prop_literal(ctx, -2, DUK_HIDDEN_SYMBOL("type"));
	return DUK_NO_RET_VAL;
}

void transform_properties(duk_context *ctx) {
	duk_push_heapptr(ctx, class_constructors->get(Variant::TRANSFORM));

	duk_push_literal(ctx, "IDENTITY");
	duk_push_variant(ctx, Transform(Basis(1, 0, 0, 0, 1, 0, 0, 0, 1), Vector3(0, 0, 0 )));
	duk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_VALUE|DUK_DEFPROP_ENUMERABLE);

	duk_push_literal(ctx, "FLIP_X");
	duk_push_variant(ctx, Transform(Basis(-1, 0, 0, 0, 1, 0, 0, 0, 1), Vector3(0, 0, 0 )));
	duk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_VALUE|DUK_DEFPROP_ENUMERABLE);

	duk_push_literal(ctx, "FLIP_Y");
	duk_push_variant(ctx, Transform(Basis(1, 0, 0, 0, 1, 0, 0, 0, 1), Vector3(0, 0, 0 )));
	duk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_VALUE|DUK_DEFPROP_ENUMERABLE);

	duk_push_literal(ctx, "FLIP_Z");
	duk_push_variant(ctx, Transform(Basis(1, 0, 0, 0, 1, 0, 0, 0, 1), Vector3(0, 0, 0 )));
	duk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_VALUE|DUK_DEFPROP_ENUMERABLE);

	duk_pop(ctx);



	duk_push_heapptr(ctx, class_prototypes->get(Variant::TRANSFORM));

	duk_push_c_function(ctx, [](duk_context *ctx) -> duk_ret_t {
		duk_push_this(ctx);
		Transform *ptr = duk_get_builtin_ptr<Transform>(ctx, -1);
		ERR_FAIL_NULL_V(ptr, DUK_ERR_TYPE_ERROR);

		Variant arg0 = duk_get_variant(ctx, 0);
		Variant r_ret;
		switch (arg0.get_type()) {
			case Variant::VECTOR3: {
				Vector3 v = arg0;
				r_ret = ptr->xform(v);
			} break;
			case Variant::PLANE: {
				Plane v = arg0;
				r_ret = ptr->xform(v);
			} break;
			case Variant::AABB: {
				AABB v = arg0;
				r_ret = ptr->xform(v);
			} break;
			default: r_ret = Variant();
		}
		duk_push_variant(ctx, r_ret);
		return DUK_HAS_RET_VAL;
	}, 1);
	duk_put_prop_literal(ctx, -2, "xform");

	duk_push_c_function(ctx, [](duk_context *ctx) -> duk_ret_t {
		duk_push_this(ctx);
		Transform *ptr = duk_get_builtin_ptr<Transform>(ctx, -1);
		ERR_FAIL_NULL_V(ptr, DUK_ERR_TYPE_ERROR);

		Variant arg0 = duk_get_variant(ctx, 0);
		Variant r_ret;
		switch (arg0.get_type()) {
			case Variant::VECTOR3: {
				Vector3 v = arg0;
				r_ret = ptr->xform_inv(v);
			} break;
			case Variant::PLANE: {
				Plane v = arg0;
				r_ret = ptr->xform_inv(v);
			} break;
			case Variant::AABB: {
				AABB v = arg0;
				r_ret = ptr->xform_inv(v);
			} break;
			default: r_ret = Variant();
		}
		duk_push_variant(ctx, r_ret);
		return DUK_HAS_RET_VAL;
	}, 1);
	duk_put_prop_literal(ctx, -2, "xform_inv");

	duk_pop(ctx);
}

duk_ret_t pool_byte_array_constructor(duk_context *ctx) {
	ERR_FAIL_COND_V(!duk_is_constructor_call(ctx), DUK_ERR_SYNTAX_ERROR);
	duk_push_this(ctx);

	PoolByteArray *ptr = NULL;
	Variant arg0 = duk_get_variant(ctx, 0);
	switch (arg0.get_type()) {

		case Variant::ARRAY: {
			// TODO: constuct from array
		} break;
		case Variant::POOL_BYTE_ARRAY: {
			PoolByteArray p = arg0;
			ptr = memnew(PoolByteArray(p));
		} break;
		default:
			ptr = memnew(PoolByteArray);
			break;
	}
	ERR_FAIL_NULL_V(ptr, DUK_ERR_TYPE_ERROR);
	duk_push_pointer(ctx, ptr);
	duk_put_prop_literal(ctx, -2, DUK_HIDDEN_SYMBOL("ptr"));
	duk_push_int(ctx, Variant::POOL_BYTE_ARRAY);
	duk_put_prop_literal(ctx, -2, DUK_HIDDEN_SYMBOL("type"));

	return DUK_NO_RET_VAL;
}

duk_ret_t pool_int_array_constructor(duk_context *ctx) {
	ERR_FAIL_COND_V(!duk_is_constructor_call(ctx), DUK_ERR_SYNTAX_ERROR);
	duk_push_this(ctx);

	PoolIntArray *ptr = NULL;
	Variant arg0 = duk_get_variant(ctx, 0);
	switch (arg0.get_type()) {

		case Variant::ARRAY: {
			// TODO: constuct from array
		} break;
		case Variant::POOL_INT_ARRAY: {
			PoolIntArray p = arg0;
			ptr = memnew(PoolIntArray(p));
		} break;
		default:
			ptr = memnew(PoolIntArray);
			break;
	}
	ERR_FAIL_NULL_V(ptr, DUK_ERR_TYPE_ERROR);
	duk_push_pointer(ctx, ptr);
	duk_put_prop_literal(ctx, -2, DUK_HIDDEN_SYMBOL("ptr"));
	duk_push_int(ctx, Variant::POOL_INT_ARRAY);
	duk_put_prop_literal(ctx, -2, DUK_HIDDEN_SYMBOL("type"));

	return DUK_NO_RET_VAL;
}

duk_ret_t pool_real_array_constructor(duk_context *ctx) {
	ERR_FAIL_COND_V(!duk_is_constructor_call(ctx), DUK_ERR_SYNTAX_ERROR);
	duk_push_this(ctx);

	PoolRealArray *ptr = NULL;
	Variant arg0 = duk_get_variant(ctx, 0);
	switch (arg0.get_type()) {

		case Variant::ARRAY: {
			// TODO: constuct from array
		} break;
		case Variant::POOL_REAL_ARRAY: {
			PoolRealArray p = arg0;
			ptr = memnew(PoolRealArray(p));
		} break;
		default:
			ptr = memnew(PoolRealArray);
			break;
	}
	ERR_FAIL_NULL_V(ptr, DUK_ERR_TYPE_ERROR);
	duk_push_pointer(ctx, ptr);
	duk_put_prop_literal(ctx, -2, DUK_HIDDEN_SYMBOL("ptr"));
	duk_push_int(ctx, Variant::POOL_REAL_ARRAY);
	duk_put_prop_literal(ctx, -2, DUK_HIDDEN_SYMBOL("type"));

	return DUK_NO_RET_VAL;
}

duk_ret_t pool_string_array_constructor(duk_context *ctx) {
	ERR_FAIL_COND_V(!duk_is_constructor_call(ctx), DUK_ERR_SYNTAX_ERROR);
	duk_push_this(ctx);

	PoolStringArray *ptr = NULL;
	Variant arg0 = duk_get_variant(ctx, 0);
	switch (arg0.get_type()) {

		case Variant::ARRAY: {
			// TODO: constuct from array
		} break;
		case Variant::POOL_STRING_ARRAY: {
			PoolStringArray p = arg0;
			ptr = memnew(PoolStringArray(p));
		} break;
		default:
			ptr = memnew(PoolStringArray);
			break;
	}
	ERR_FAIL_NULL_V(ptr, DUK_ERR_TYPE_ERROR);
	duk_push_pointer(ctx, ptr);
	duk_put_prop_literal(ctx, -2, DUK_HIDDEN_SYMBOL("ptr"));
	duk_push_int(ctx, Variant::POOL_STRING_ARRAY);
	duk_put_prop_literal(ctx, -2, DUK_HIDDEN_SYMBOL("type"));

	return DUK_NO_RET_VAL;
}

duk_ret_t pool_vector2_array_constructor(duk_context *ctx) {
	ERR_FAIL_COND_V(!duk_is_constructor_call(ctx), DUK_ERR_SYNTAX_ERROR);
	duk_push_this(ctx);

	PoolVector2Array *ptr = NULL;
	Variant arg0 = duk_get_variant(ctx, 0);
	switch (arg0.get_type()) {

		case Variant::ARRAY: {
			// TODO: constuct from array
		} break;
		case Variant::POOL_VECTOR2_ARRAY: {
			PoolVector2Array p = arg0;
			ptr = memnew(PoolVector2Array(p));
		} break;
		default:
			ptr = memnew(PoolVector2Array);
			break;
	}
	ERR_FAIL_NULL_V(ptr, DUK_ERR_TYPE_ERROR);
	duk_push_pointer(ctx, ptr);
	duk_put_prop_literal(ctx, -2, DUK_HIDDEN_SYMBOL("ptr"));
	duk_push_int(ctx, Variant::POOL_VECTOR2_ARRAY);
	duk_put_prop_literal(ctx, -2, DUK_HIDDEN_SYMBOL("type"));

	return DUK_NO_RET_VAL;
}

duk_ret_t pool_vector3_array_constructor(duk_context *ctx) {
	ERR_FAIL_COND_V(!duk_is_constructor_call(ctx), DUK_ERR_SYNTAX_ERROR);
	duk_push_this(ctx);

	PoolVector3Array *ptr = NULL;
	Variant arg0 = duk_get_variant(ctx, 0);
	switch (arg0.get_type()) {

		case Variant::ARRAY: {
			// TODO: constuct from array
		} break;
		case Variant::POOL_VECTOR3_ARRAY: {
			PoolVector3Array p = arg0;
			ptr = memnew(PoolVector3Array(p));
		} break;
		default:
			ptr = memnew(PoolVector3Array);
			break;
	}
	ERR_FAIL_NULL_V(ptr, DUK_ERR_TYPE_ERROR);
	duk_push_pointer(ctx, ptr);
	duk_put_prop_literal(ctx, -2, DUK_HIDDEN_SYMBOL("ptr"));
	duk_push_int(ctx, Variant::POOL_VECTOR3_ARRAY);
	duk_put_prop_literal(ctx, -2, DUK_HIDDEN_SYMBOL("type"));

	return DUK_NO_RET_VAL;
}


duk_ret_t pool_color_array_constructor(duk_context *ctx) {
	ERR_FAIL_COND_V(!duk_is_constructor_call(ctx), DUK_ERR_SYNTAX_ERROR);
	duk_push_this(ctx);

	PoolColorArray *ptr = NULL;
	Variant arg0 = duk_get_variant(ctx, 0);
	switch (arg0.get_type()) {

		case Variant::ARRAY: {
			// TODO: constuct from array
		} break;
		case Variant::POOL_COLOR_ARRAY: {
			PoolColorArray p = arg0;
			ptr = memnew(PoolColorArray(p));
		} break;
		default:
			ptr = memnew(PoolColorArray);
			break;
	}
	ERR_FAIL_NULL_V(ptr, DUK_ERR_TYPE_ERROR);
	duk_push_pointer(ctx, ptr);
	duk_put_prop_literal(ctx, -2, DUK_HIDDEN_SYMBOL("ptr"));
	duk_push_int(ctx, Variant::POOL_COLOR_ARRAY);
	duk_put_prop_literal(ctx, -2, DUK_HIDDEN_SYMBOL("type"));

	return DUK_NO_RET_VAL;
}

template<class T>
duk_ret_t pool_array_index_getter(duk_context* ctx) {
	duk_push_this(ctx);
	T *ptr = duk_get_builtin_ptr<T>(ctx, -1);
	ERR_FAIL_NULL_V(ptr, DUK_ERR_TYPE_ERROR);
	int idx = duk_get_int_default(ctx, 0, -1);
	ERR_FAIL_COND_V(idx < 0 || idx >= ptr->size(), DUK_ERR_RANGE_ERROR);
	duk_push_variant(ctx, (*ptr)[idx]);
	return DUK_HAS_RET_VAL;
};

void pool_array_properties(duk_context *ctx) {

	duk_push_heapptr(ctx, class_prototypes->get(Variant::POOL_BYTE_ARRAY));

	duk_push_c_function(ctx, pool_array_index_getter<PoolByteArray>, 1);
	duk_put_prop_literal(ctx, -2, "get");

	duk_push_c_function(ctx, ([](duk_context *ctx) -> duk_ret_t{
							duk_push_this(ctx);
							PoolByteArray *ptr = duk_get_builtin_ptr<PoolByteArray>(ctx, -1);
							ERR_FAIL_NULL_V(ptr, DUK_ERR_TYPE_ERROR);

							PoolByteArray compressed;
							Compression::Mode mode = (Compression::Mode)(int)(duk_get_int_default(ctx, 0, 0));
							compressed.resize(Compression::get_max_compressed_buffer_size(ptr->size(), mode));
							int result = Compression::compress(compressed.write().ptr(), ptr->read().ptr(), ptr->size(), mode);
							result = result >= 0 ? result : 0;
							compressed.resize(result);

							duk_push_variant(ctx, compressed);
							return DUK_HAS_RET_VAL;
						}), 1);
	duk_put_prop_literal(ctx, -2, "compress");

	duk_push_c_function(ctx, ([](duk_context *ctx) -> duk_ret_t{
							duk_push_this(ctx);
							PoolByteArray *ptr = duk_get_builtin_ptr<PoolByteArray>(ctx, -1);
							ERR_FAIL_NULL_V(ptr, DUK_ERR_TYPE_ERROR);

							PoolByteArray decompressed;
							Compression::Mode mode = (Compression::Mode)(int)(duk_get_int_default(ctx, 1, 0));

							int buffer_size = (int)(duk_get_int_default(ctx, 0, 0));
							if (buffer_size < 0) {
								ERR_EXPLAIN("Decompression buffer size is less than zero");
								ERR_FAIL_V(DUK_ERR_RANGE_ERROR);
							}

							decompressed.resize(buffer_size);
							int result = Compression::decompress(decompressed.write().ptr(), buffer_size, ptr->read().ptr(), ptr->size(), mode);

							result = result >= 0 ? result : 0;
							decompressed.resize(result);

							duk_push_variant(ctx, decompressed);
							return DUK_HAS_RET_VAL;
						}), 2);
	duk_put_prop_literal(ctx, -2, "decompress");

	duk_push_c_function(ctx, ([](duk_context *ctx) -> duk_ret_t{
							duk_push_this(ctx);
							PoolByteArray *ptr = duk_get_builtin_ptr<PoolByteArray>(ctx, -1);
							ERR_FAIL_NULL_V(ptr, DUK_ERR_TYPE_ERROR);

							String s;
							if (ptr->size() >= 0) {
								PoolByteArray::Read r = ptr->read();
								CharString cs;
								cs.resize(ptr->size() + 1);
								copymem(cs.ptrw(), r.ptr(), ptr->size());
								cs[ptr->size()] = 0;

								s = cs.get_data();
							}

							duk_push_variant(ctx, s);
							return DUK_HAS_RET_VAL;
						}), 0);
	duk_put_prop_literal(ctx, -2, "get_string_from_ascii");

	duk_push_c_function(ctx, ([](duk_context *ctx) -> duk_ret_t{
							duk_push_this(ctx);
							PoolByteArray *ptr = duk_get_builtin_ptr<PoolByteArray>(ctx, -1);
							ERR_FAIL_NULL_V(ptr, DUK_ERR_TYPE_ERROR);

							String s;
							if (ptr->size() >= 0) {
								PoolByteArray::Read r = ptr->read();
								s.parse_utf8((const char *)r.ptr(), ptr->size());
							}

							duk_push_variant(ctx, s);
							return DUK_HAS_RET_VAL;
						}), 0);
	duk_put_prop_literal(ctx, -2, "get_string_from_utf8");

	duk_pop(ctx);

	duk_push_heapptr(ctx, class_prototypes->get(Variant::POOL_INT_ARRAY));
	duk_push_c_function(ctx, pool_array_index_getter<PoolIntArray>, 1);
	duk_put_prop_literal(ctx, -2, "get");
	duk_pop(ctx);

	duk_push_heapptr(ctx, class_prototypes->get(Variant::POOL_REAL_ARRAY));
	duk_push_c_function(ctx, pool_array_index_getter<PoolRealArray>, 1);
	duk_put_prop_literal(ctx, -2, "get");
	duk_pop(ctx);

	duk_push_heapptr(ctx, class_prototypes->get(Variant::POOL_STRING_ARRAY));
	duk_push_c_function(ctx, pool_array_index_getter<PoolStringArray>, 1);
	duk_put_prop_literal(ctx, -2, "get");
	duk_pop(ctx);

	duk_push_heapptr(ctx, class_prototypes->get(Variant::POOL_VECTOR2_ARRAY));
	duk_push_c_function(ctx, pool_array_index_getter<PoolVector2Array>, 1);
	duk_put_prop_literal(ctx, -2, "get");
	duk_pop(ctx);

	duk_push_heapptr(ctx, class_prototypes->get(Variant::POOL_VECTOR3_ARRAY));
	duk_push_c_function(ctx, pool_array_index_getter<PoolVector3Array>, 1);
	duk_put_prop_literal(ctx, -2, "get");
	duk_pop(ctx);

	duk_push_heapptr(ctx, class_prototypes->get(Variant::POOL_COLOR_ARRAY));
	duk_push_c_function(ctx, pool_array_index_getter<PoolColorArray>, 1);
	duk_put_prop_literal(ctx, -2, "get");
	duk_pop(ctx);
}
