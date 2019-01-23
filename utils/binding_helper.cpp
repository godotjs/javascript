#include "binding_helper.h"
#include "core/os/memory.h"
#include "core/print_string.h"

void *alloc_function(void *udata, duk_size_t size) {
	return memalloc(size);
}

void *realloc_function(void *udata, void *ptr, duk_size_t size) {
	return memrealloc(ptr, size);
}

void free_function(void *udata, void *ptr) {
	if (ptr) {
		memfree(ptr);
	}
}

void fatal_function(void *udata, const char *msg) {
	fprintf(stderr, "*** FATAL ERROR: %s\n", (msg ? msg : "no message"));
	fflush(stderr);
	abort();
}

duk_ret_t native_print(duk_context *ctx) {
	int size = duk_get_top(ctx);
	String msg;
	for (int i = 0; i < size; ++i) {
		msg += duk_to_string(ctx, i);
		if (i < size - 1) {
			msg += " ";
		}
	}
	print_line(msg);
	return NO_RET_VAL;
}
