#include "builtin_classes.h"
#include "core/math/vector2.h"

void binding_vector2(duk_context *ctx) {
	duk_push_c_function(ctx, static_constructor_2f<Vector2>, 2);
	duk_put_prop_string(ctx, -2, "Vector2");
}

void register_builtin_classes(duk_context *ctx) {
	binding_vector2(ctx);
}
