#include "duktape_builtin_bindings.h"
#include "core/resource.h"

HashMap<Variant::Type, DuktapeHeapObject *> *class_prototypes = NULL;
Variant (*duk_get_variant)(duk_context *ctx, duk_idx_t idx) = NULL;
void (*duk_push_variant)(duk_context *ctx, const Variant &var) = NULL;
DuktapeHeapObject *godot_to_string_ptr = NULL;

duk_ret_t vector2_constructor(duk_context *ctx);
void vector2_properties(duk_context *ctx);
duk_ret_t vector3_constructor(duk_context *ctx);
void vector3_properties(duk_context *ctx);
duk_ret_t rect2_constructor(duk_context *ctx);
void rect2_properties(duk_context *ctx);
duk_ret_t color_constructor(duk_context *ctx);
void color_properties(duk_context *ctx);
duk_ret_t rid_constructor(duk_context *ctx);

void DuktapeBindingHelper::register_builtin_classes(duk_context *ctx) {

	class_prototypes = &builtin_class_prototypes;
	godot_to_string_ptr = duk_ptr_godot_to_string;
	duk_get_variant = duk_get_godot_variant;
	duk_push_variant = duk_push_godot_variant;

	// register builtin classes
	register_builtin_class<Vector2>(ctx, vector2_constructor, 2, Variant::VECTOR2, "Vector2");
	register_builtin_class<Rect2>(ctx, rect2_constructor, 4, Variant::RECT2, "Rect2");
	register_builtin_class<Color>(ctx, color_constructor, 4, Variant::COLOR, "Color");
	register_builtin_class<Vector3>(ctx, vector3_constructor, 3, Variant::VECTOR3, "Vector3");
	register_builtin_class<RID>(ctx, rid_constructor, 1, Variant::_RID, "RID");

	// define properties of builtin classes
	register_builtin_class_properties(ctx);
	register_builtin_class_properties_gen(ctx);
}

void register_builtin_class_properties(duk_context *ctx) {
	vector2_properties(ctx);
	vector3_properties(ctx);
	color_properties(ctx);
	rect2_properties(ctx);
}

duk_ret_t vector2_constructor(duk_context *ctx) {
	ERR_FAIL_COND_V(!duk_is_constructor_call(ctx), DUK_ERR_SYNTAX_ERROR);

	duk_double_t x = duk_get_number_default(ctx, 0, 0);
	duk_double_t y = duk_get_number_default(ctx, 1, 0);

	duk_push_this(ctx);

	duk_push_pointer(ctx, memnew(Vector2(x, y)));
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
	},
			1);
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
	},
			1);
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
	},
			1);
	duk_put_prop_literal(ctx, -2, "divide");

	duk_push_c_function(ctx, [](duk_context *ctx) -> duk_ret_t {
		duk_push_this(ctx);
		Vector2 *ptr = duk_get_builtin_ptr<Vector2>(ctx, -1);
		ERR_FAIL_NULL_V(ptr, DUK_ERR_TYPE_ERROR);
		Variant arg0 = duk_get_variant(ctx, 0);
		ERR_FAIL_COND_V(arg0.get_type() != Variant::REAL, DUK_ERR_TYPE_ERROR);

		ptr->operator/=(arg0);

		duk_push_this(ctx);
		return DUK_HAS_RET_VAL;
	},
			1);
	duk_put_prop_literal(ctx, -2, "divide_assign");

	duk_push_c_function(ctx, [](duk_context *ctx) -> duk_ret_t {
		duk_push_this(ctx);
		Vector2 *ptr = duk_get_builtin_ptr<Vector2>(ctx, -1);
		ERR_FAIL_NULL_V(ptr, DUK_ERR_TYPE_ERROR);
		ptr->operator=(ptr->operator-());
		duk_push_this(ctx);
		return DUK_HAS_RET_VAL;
	},
			1);
	duk_put_prop_literal(ctx, -2, "negate_assign");

	duk_pop(ctx);
}

duk_ret_t vector3_constructor(duk_context *ctx) {
	ERR_FAIL_COND_V(!duk_is_constructor_call(ctx), DUK_ERR_SYNTAX_ERROR);

	duk_double_t x = duk_get_number_default(ctx, 0, 0);
	duk_double_t y = duk_get_number_default(ctx, 1, 0);
	duk_double_t z = duk_get_number_default(ctx, 2, 0);

	duk_push_this(ctx);

	duk_push_pointer(ctx, memnew(Vector3(x, y, z)));
	duk_put_prop_literal(ctx, -2, DUK_HIDDEN_SYMBOL("ptr"));

	duk_push_int(ctx, Variant::VECTOR3);
	duk_put_prop_literal(ctx, -2, DUK_HIDDEN_SYMBOL("type"));

	return DUK_NO_RET_VAL;
}

void vector3_properties(duk_context *ctx) {
}

duk_ret_t rect2_constructor(duk_context *ctx) {
	ERR_FAIL_COND_V(!duk_is_constructor_call(ctx), DUK_ERR_SYNTAX_ERROR);

	duk_double_t x = duk_get_number_default(ctx, 0, 0);
	duk_double_t y = duk_get_number_default(ctx, 1, 0);
	duk_double_t w = duk_get_number_default(ctx, 2, 0);
	duk_double_t h = duk_get_number_default(ctx, 3, 0);

	duk_push_this(ctx);

	duk_push_pointer(ctx, memnew(Rect2(x, y, w, h)));
	duk_put_prop_literal(ctx, -2, DUK_HIDDEN_SYMBOL("ptr"));

	duk_push_int(ctx, Variant::RECT2);
	duk_put_prop_literal(ctx, -2, DUK_HIDDEN_SYMBOL("type"));

	return DUK_NO_RET_VAL;
}

void rect2_properties(duk_context *ctx) {

	duk_push_heapptr(ctx, class_prototypes->get(Variant::COLOR));

	duk_push_literal(ctx, "end");
	duk_push_c_function(ctx, [](duk_context *ctx) -> duk_ret_t {
		duk_push_this(ctx);
		Rect2 *ptr = duk_get_builtin_ptr<Rect2>(ctx, -1);
		ERR_FAIL_NULL_V(ptr, DUK_ERR_TYPE_ERROR);
		duk_push_variant(ctx, ptr->position + ptr->size);
		return DUK_HAS_RET_VAL;
	},
			1);
	duk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_GETTER | DUK_DEFPROP_ENUMERABLE);

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
	},
			1);
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
	},
			1);
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
	},
			1);
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
	},
			1);
	duk_put_prop_literal(ctx, -2, "divide_assign");

	duk_push_c_function(ctx, [](duk_context *ctx) -> duk_ret_t {
		duk_push_this(ctx);
		Color *ptr = duk_get_builtin_ptr<Color>(ctx, -1);
		ERR_FAIL_NULL_V(ptr, DUK_ERR_TYPE_ERROR);
		ptr->operator=(ptr->operator-());
		duk_push_this(ctx);
		return DUK_HAS_RET_VAL;
	},
			1);
	duk_put_prop_literal(ctx, -2, "negate_assign");

	duk_push_literal(ctx, "h");
	duk_push_c_function(ctx, [](duk_context *ctx) -> duk_ret_t {
		duk_push_this(ctx);
		Color *ptr = duk_get_builtin_ptr<Color>(ctx, -1);
		ERR_FAIL_NULL_V(ptr, DUK_ERR_TYPE_ERROR);
		duk_push_number(ctx, ptr->get_h());
		return DUK_HAS_RET_VAL;
	},
			1);
	duk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_GETTER | DUK_DEFPROP_ENUMERABLE);

	duk_push_literal(ctx, "s");
	duk_push_c_function(ctx, [](duk_context *ctx) -> duk_ret_t {
		duk_push_this(ctx);
		Color *ptr = duk_get_builtin_ptr<Color>(ctx, -1);
		ERR_FAIL_NULL_V(ptr, DUK_ERR_TYPE_ERROR);
		duk_push_number(ctx, ptr->get_s());
		return DUK_HAS_RET_VAL;
	},
			1);
	duk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_GETTER | DUK_DEFPROP_ENUMERABLE);

	duk_push_literal(ctx, "v");
	duk_push_c_function(ctx, [](duk_context *ctx) -> duk_ret_t {
		duk_push_this(ctx);
		Color *ptr = duk_get_builtin_ptr<Color>(ctx, -1);
		ERR_FAIL_NULL_V(ptr, DUK_ERR_TYPE_ERROR);
		duk_push_number(ctx, ptr->get_v());
		return DUK_HAS_RET_VAL;
	},
			1);
	duk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_GETTER | DUK_DEFPROP_ENUMERABLE);

	duk_push_literal(ctx, "r8");
	duk_push_c_function(ctx, [](duk_context *ctx) -> duk_ret_t {
		duk_push_this(ctx);
		Color *ptr = duk_get_builtin_ptr<Color>(ctx, -1);
		ERR_FAIL_NULL_V(ptr, DUK_ERR_TYPE_ERROR);
		duk_push_uint(ctx, ptr->r * 255);
		return DUK_HAS_RET_VAL;
	},
			1);
	duk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_GETTER | DUK_DEFPROP_ENUMERABLE);

	duk_push_literal(ctx, "g8");
	duk_push_c_function(ctx, [](duk_context *ctx) -> duk_ret_t {
		duk_push_this(ctx);
		Color *ptr = duk_get_builtin_ptr<Color>(ctx, -1);
		ERR_FAIL_NULL_V(ptr, DUK_ERR_TYPE_ERROR);
		duk_push_uint(ctx, ptr->g * 255);
		return DUK_HAS_RET_VAL;
	},
			1);
	duk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_GETTER | DUK_DEFPROP_ENUMERABLE);

	duk_push_literal(ctx, "b8");
	duk_push_c_function(ctx, [](duk_context *ctx) -> duk_ret_t {
		duk_push_this(ctx);
		Color *ptr = duk_get_builtin_ptr<Color>(ctx, -1);
		ERR_FAIL_NULL_V(ptr, DUK_ERR_TYPE_ERROR);
		duk_push_uint(ctx, ptr->b * 255);
		return DUK_HAS_RET_VAL;
	},
			1);
	duk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_GETTER | DUK_DEFPROP_ENUMERABLE);

	duk_push_literal(ctx, "a8");
	duk_push_c_function(ctx, [](duk_context *ctx) -> duk_ret_t {
		duk_push_this(ctx);
		Color *ptr = duk_get_builtin_ptr<Color>(ctx, -1);
		ERR_FAIL_NULL_V(ptr, DUK_ERR_TYPE_ERROR);
		duk_push_uint(ctx, ptr->a * 255);
		return DUK_HAS_RET_VAL;
	},
			1);
	duk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_GETTER | DUK_DEFPROP_ENUMERABLE);

	duk_pop(ctx);
}

duk_ret_t rid_constructor(duk_context *ctx) {
	ERR_FAIL_COND_V(!duk_is_constructor_call(ctx), DUK_ERR_SYNTAX_ERROR);

	duk_push_this(ctx);

	Variant from = duk_get_variant(ctx, 0);
	ERR_FAIL_COND_V(from.get_type() != Variant::OBJECT, DUK_ERR_TYPE_ERROR);
	Object * obj = from;
	RID * ptr = NULL;
	if(Resource * res = Object::cast_to<Resource>(obj)) {
		ptr = memnew(RID(res->get_rid()));
	}

	duk_push_pointer(ctx, ptr);
	duk_put_prop_literal(ctx, -2, DUK_HIDDEN_SYMBOL("ptr"));
	duk_push_int(ctx, Variant::_RID);
	duk_put_prop_literal(ctx, -2, DUK_HIDDEN_SYMBOL("type"));

	return DUK_NO_RET_VAL;
}

