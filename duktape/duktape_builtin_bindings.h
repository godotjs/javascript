#ifndef DUKTAPE_BUILTIN_BINDINGS_H
#define DUKTAPE_BUILTIN_BINDINGS_H

#include "duktape_binding_helper.h"

extern HashMap<Variant::Type, DuktapeHeapObject *> *class_prototypes;
extern Variant (*duk_get_variant)(duk_context *ctx, duk_idx_t idx);
extern void (*duk_push_variant)(duk_context *ctx, const Variant &var);
extern DuktapeHeapObject *godot_to_string_ptr;

template<class T>
T* duk_get_builtin_ptr(duk_context *ctx, duk_idx_t idx) {
	duk_get_prop_string(ctx,idx, DUK_HIDDEN_SYMBOL("ptr"));
	T *ptr = static_cast<T *>(duk_get_pointer_default(ctx, -1, NULL));
	duk_pop(ctx);
	return ptr;
}

template<class T>
duk_ret_t builtin_finalizer(duk_context *ctx) {
	duk_get_prop_string(ctx, -1, DUK_HIDDEN_SYMBOL("ptr"));
	T *ptr = static_cast<T *>(duk_get_pointer(ctx, -1));
	if (ptr) {
		memdelete(ptr);
	}
	return DUK_NO_RET_VAL;
}

template<class T>
void register_builtin_class(duk_context *ctx, duk_c_function ctor, int ctor_argc, Variant::Type type, const char *name) {

	duk_push_string(ctx, name);
	duk_push_c_function(ctx, ctor, ctor_argc);
	duk_push_object(ctx);
	class_prototypes->set(type, duk_get_heapptr(ctx, -1));

	duk_push_heapptr(ctx, godot_to_string_ptr);
	duk_put_prop_literal(ctx, -2, "toString");

	duk_push_c_function(ctx, builtin_finalizer<T>, 1);
	duk_set_finalizer(ctx, -2);

	duk_put_prop_literal(ctx, -2, PROTOTYPE_LITERAL);
	duk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_VALUE|DUK_DEFPROP_ENUMERABLE);
}

void register_builtin_class_properties(duk_context *ctx);
void register_builtin_class_properties_gen(duk_context *ctx);

#endif
