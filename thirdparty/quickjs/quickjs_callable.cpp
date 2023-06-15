#include "quickjs_callable.h"
#include "javascript_language.h"
#include "quickjs/quickjs.h"

#include "quickjs_builtin_binder.h"
#include "quickjs_binder.h"

bool QuickJSCallable::compare_equal(const CallableCustom *p_a, const CallableCustom *p_b) {
	const QuickJSCallable *a = static_cast<const QuickJSCallable *>(p_a);
	const QuickJSCallable *b = static_cast<const QuickJSCallable *>(p_b);
	return a->js_function.javascript_object == b->js_function.javascript_object;
}

bool QuickJSCallable::compare_less(const CallableCustom *p_a, const CallableCustom *p_b) {
	if (compare_equal(p_a, p_b)) {
		return false;
	}
	return p_a < p_b;
}

QuickJSCallable::QuickJSCallable(JSContext *ctx, const JSValue &p_value) {
	ERR_FAIL_COND(!JS_IsFunction(ctx, p_value));
	JSValue v = JS_DupValue(ctx, p_value);
	js_function.context = ctx;
	js_function.javascript_object = JS_VALUE_GET_PTR(v);
}

QuickJSCallable::QuickJSCallable(const JavaScriptGCHandler &p_function) : JavaScriptCallable(p_function) {
	JSValue js_func = JS_MKPTR(JS_TAG_OBJECT, p_function.javascript_object);
	JSContext *ctx = static_cast<JSContext *>(p_function.context);
	ERR_FAIL_COND(JS_IsFunction(ctx, js_func));
	JS_DupValue(ctx, js_func);
}

QuickJSCallable::~QuickJSCallable() {
	if (js_function.is_valid_javascript_object()) {
		JSContext *ctx = static_cast<JSContext *>(js_function.context);
		JSValue js_func = JS_MKPTR(JS_TAG_OBJECT, js_function.javascript_object);
		JS_FreeValue(ctx, js_func);
	}
}

uint32_t QuickJSCallable::hash() const {
	JSValue js_func = JS_MKPTR(JS_TAG_OBJECT, js_function.javascript_object);
	return hash_murmur3_one_64((uint64_t)JS_VALUE_GET_PTR(js_func));
}

String QuickJSCallable::get_as_text() const {
	QuickJSBinder *binder = QuickJSBinder::get_context_binder(static_cast<JSContext *>(js_function.context));
	JSValue js_func = JS_MKPTR(JS_TAG_OBJECT, js_function.javascript_object);
	JSContext *ctx = static_cast<JSContext *>(js_function.context);
	String text = binder->js_to_string(ctx, js_func);
	return text;
}

ObjectID QuickJSCallable::get_object() const {
	return JavaScriptLanguage::get_singleton()->get_callable_middleman()->get_instance_id();
}

void QuickJSCallable::call(const Variant **p_arguments, int p_argcount, Variant &r_return_value, Callable::CallError &r_call_error) const {
	QuickJSBinder *binder = QuickJSBinder::get_context_binder(static_cast<JSContext *>(js_function.context));
	JSValue js_func = JS_MKPTR(JS_TAG_OBJECT, js_function.javascript_object);
	JavaScriptGCHandler func;
	func.javascript_object = JS_VALUE_GET_PTR(js_func);
	JavaScriptGCHandler caller;
	r_return_value = binder->call(func, caller, p_arguments, p_argcount, r_call_error);
}
