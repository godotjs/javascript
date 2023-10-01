/**************************************************************************/
/*  quickjs_callable.cpp                                                  */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#include "quickjs_callable.h"
#include "../../quickjs_binder.h"
#include "../javascript_language.h"
#include "quickjs/quickjs.h"
#include "../../quickjs_binder.h"

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

QuickJSCallable::QuickJSCallable(const JavaScriptGCHandler &p_function) :
		JavaScriptCallable(p_function) {
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
