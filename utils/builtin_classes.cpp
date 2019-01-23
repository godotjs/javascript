#include "builtin_classes.h"
#include "core/math/vector2.h"

static duk_ret_t vector_property_x(duk_context *ctx) {
	duk_idx_t argc = duk_get_top(ctx);

	duk_push_this(ctx);

	duk_get_prop_string(ctx, -1, DUK_HIDDEN_SYMBOL("ptr"));
	Vector2 *ptr = static_cast<Vector2 *>(duk_get_pointer(ctx, -1));
	if (ptr) {
		if (argc) {
			ptr->x = duk_get_number_default(ctx, 0, 0);
		} else {
			duk_push_number(ctx, ptr->x);
		}
	} else {
		duk_push_nan(ctx);
	}
	return HAS_RET_VAL;
}

static duk_ret_t vector_property_y(duk_context *ctx) {
	duk_idx_t argc = duk_get_top(ctx);

	duk_push_this(ctx);

	duk_get_prop_string(ctx, -1, DUK_HIDDEN_SYMBOL("ptr"));
	Vector2 *ptr = static_cast<Vector2 *>(duk_get_pointer(ctx, -1));
	if (ptr) {
		if (argc) {
			ptr->y = duk_get_number_default(ctx, 0, 0);
		} else {
			duk_push_number(ctx, ptr->y);
		}
	} else {
		duk_push_nan(ctx);
	}
	return HAS_RET_VAL;
}

void binding_vector2(duk_context *ctx) {
	duk_push_c_function(ctx, static_constructor_2f<Vector2>, 2);
	{ // prototype
		duk_push_object(ctx);
		{ // members
			duk_push_literal(ctx, "x");
			duk_push_c_function(ctx, vector_property_x, 0);
			duk_push_c_function(ctx, vector_property_x, 1);
			duk_def_prop(ctx, -4, DUK_DEFPROP_HAVE_GETTER | DUK_DEFPROP_HAVE_SETTER);

			duk_push_literal(ctx, "y");
			duk_push_c_function(ctx, vector_property_y, 0);
			duk_push_c_function(ctx, vector_property_y, 1);
			duk_def_prop(ctx, -4, DUK_DEFPROP_HAVE_GETTER | DUK_DEFPROP_HAVE_SETTER);

			//			duk_push_c_function(ctx, vector_setters, 1);
			//			duk_push_string(ctx, "y");
			//			duk_put_prop_string(ctx, -2, "key");

			//			duk_push_c_function(ctx, vector2_getters, 0);
			//			duk_push_string(ctx, "y");
			//			duk_put_prop_string(ctx, -2, "key");
		}
		duk_put_prop_literal(ctx, -2, "prototype");
	}
	duk_put_prop_literal(ctx, -2, "Vector2");
}

void register_builtin_classes(duk_context *ctx) {
	binding_vector2(ctx);
}
