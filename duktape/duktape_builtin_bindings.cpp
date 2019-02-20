#include "duktape_binding_helper.h"

static HashMap<Variant::Type, DuktapeHeapObject *> *class_prototypes = NULL;
static DuktapeHeapObject *godot_to_string_ptr = NULL;

template<class T>
static duk_ret_t builtin_finalizer(duk_context *ctx) {
	duk_get_prop_string(ctx, -1, DUK_HIDDEN_SYMBOL("ptr"));
	T *ptr = static_cast<T *>(duk_get_pointer(ctx, -1));
	if (ptr) {
		memdelete(ptr);
	}
	return DUK_NO_RET_VAL;
}

template<class T>
static void register_builtin_class(duk_context *ctx, duk_c_function ctor, int ctor_argc, Variant::Type type, const char *name) {

	duk_push_c_function(ctx, ctor, ctor_argc);
	duk_push_object(ctx);
	class_prototypes->set(type, duk_get_heapptr(ctx, -1));

	duk_push_heapptr(ctx, godot_to_string_ptr);
	duk_put_prop_literal(ctx, -2, "toString");

	duk_push_c_function(ctx, builtin_finalizer<T>, 1);
	duk_set_finalizer(ctx, -2);

	duk_put_prop_literal(ctx, -2, PROTOTYPE_LITERAL);
	duk_put_prop_string(ctx, -2, name);
}

template <class T>
static duk_ret_t builtin_property_x(duk_context *ctx) {

	duk_idx_t argc = duk_get_top(ctx);

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, DUK_HIDDEN_SYMBOL("ptr"));
	T *ptr = static_cast<T *>(duk_get_pointer(ctx, -1));
	ERR_FAIL_NULL_V(ptr, DUK_ERR_TYPE_ERROR);

	if (argc) {
		ptr->x = duk_get_number_default(ctx, 0, DUK_DOUBLE_NAN);
	}
	duk_push_number(ctx, ptr->x);

	return DUK_HAS_RET_VAL;
}

template <class T>
static duk_ret_t builtin_property_y(duk_context *ctx) {

	duk_idx_t argc = duk_get_top(ctx);

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, DUK_HIDDEN_SYMBOL("ptr"));
	T *ptr = static_cast<T *>(duk_get_pointer(ctx, -1));
	ERR_FAIL_NULL_V(ptr, DUK_ERR_TYPE_ERROR);

	if (argc) {
		ptr->y = duk_get_number_default(ctx, 0, DUK_DOUBLE_NAN);
	}
	duk_push_number(ctx, ptr->y);

	return DUK_HAS_RET_VAL;
}

static duk_ret_t vector2_constructor(duk_context *ctx) {
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

static void register_properties_vector2(duk_context *ctx) {

	duk_push_heapptr(ctx, class_prototypes->get(Variant::VECTOR2));

	duk_push_literal(ctx, "x");
	duk_push_c_function(ctx, builtin_property_x<Vector2>, 0);
	duk_push_c_function(ctx, builtin_property_x<Vector2>, 1);
	duk_def_prop(ctx, -4, DUK_DEFPROP_HAVE_GETTER | DUK_DEFPROP_HAVE_SETTER);

	duk_push_literal(ctx, "y");
	duk_push_c_function(ctx, builtin_property_y<Vector2>, 0);
	duk_push_c_function(ctx, builtin_property_y<Vector2>, 1);
	duk_def_prop(ctx, -4, DUK_DEFPROP_HAVE_GETTER | DUK_DEFPROP_HAVE_SETTER);

	duk_push_literal(ctx, "width");
	duk_push_c_function(ctx, builtin_property_x<Vector2>, 0);
	duk_push_c_function(ctx, builtin_property_x<Vector2>, 1);
	duk_def_prop(ctx, -4, DUK_DEFPROP_HAVE_GETTER | DUK_DEFPROP_HAVE_SETTER);

	duk_push_literal(ctx, "height");
	duk_push_c_function(ctx, builtin_property_y<Vector2>, 0);
	duk_push_c_function(ctx, builtin_property_y<Vector2>, 1);
	duk_def_prop(ctx, -4, DUK_DEFPROP_HAVE_GETTER | DUK_DEFPROP_HAVE_SETTER);

	duk_pop(ctx);
}

void DuktapeBindingHelper::register_builtin_classes(duk_context *ctx) {
	class_prototypes = &builtin_class_prototypes;
	godot_to_string_ptr = duk_ptr_godot_to_string;

	// Vector2
	register_builtin_class<Vector2>(ctx, vector2_constructor, 2, Variant::VECTOR2, "Vector2");
	register_properties_vector2(ctx);
}
