#ifndef ECMASCRIPT_BINDDING_HELPER
#define ECMASCRIPT_BINDDING_HELPER

#include "core/os/memory.h"
#include "duktape/duktape.h"

#define NO_RET_VAL 0
#define HAS_RET_VAL 1

// Memory managerment
void *alloc_function(void *udata, duk_size_t size);
void *realloc_function(void *udata, void *ptr, duk_size_t size);
void free_function(void *udata, void *ptr);

void fatal_function(void *udata, const char *msg);

duk_ret_t native_print(duk_context *ctx);

template <typename T>
duk_ret_t finalizer_static(duk_context *ctx) {

	duk_get_prop_string(ctx, -1, DUK_HIDDEN_SYMBOL("ptr"));
	T *ptr = static_cast<T *>(duk_get_pointer(ctx, -1));
	if (ptr) {
		memdelete(ptr);
		duk_pop(ctx);
	}

	return NO_RET_VAL;
}

template <typename T>
duk_ret_t static_constructor(duk_context *ctx) {

	if (!duk_is_constructor_call(ctx)) {
		return DUK_ERR_TYPE_ERROR;
	}

	size_t this_pos = duk_get_top(ctx);
	T *ptr = memnew(T);
	duk_push_this(ctx);
	duk_push_pointer(ctx, ptr);
	duk_put_prop_string(ctx, -2, DUK_HIDDEN_SYMBOL("ptr"));

	duk_push_c_function(ctx, finalizer_static<T>, 1);
	duk_set_finalizer(ctx, this_pos);

	return NO_RET_VAL;
}

template <typename T>
duk_ret_t static_constructor_2f(duk_context *ctx) {

	if (!duk_is_constructor_call(ctx)) {
		return DUK_ERR_TYPE_ERROR;
	}

	duk_double_t p0 = duk_get_number_default(ctx, 0, 0);
	duk_double_t p1 = duk_get_number_default(ctx, 1, 0);
	T *ptr = memnew(T(p0, p1));

	size_t this_pos = duk_get_top(ctx);
	duk_push_this(ctx);
	duk_push_pointer(ctx, ptr);
	duk_put_prop_literal(ctx, -2, DUK_HIDDEN_SYMBOL("ptr"));

	duk_push_c_function(ctx, finalizer_static<T>, 1);
	duk_set_finalizer(ctx, this_pos);

	return NO_RET_VAL;
}

#endif // ECMASCRIPT_BINDDING_HELPER
