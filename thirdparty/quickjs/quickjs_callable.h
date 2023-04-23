#ifndef QUICKJS_CALLABLE_H
#define QUICKJS_CALLABLE_H
#include "javascript_callable.h"
#include "quickjs/quickjs.h"

#if !defined(JS_NAN_BOXING)
///typedef uint64_t JSValue; defined in quickjs.h if defined(JS_NAN_BOXING)
struct JSValue;
#endif

class QuickJSCallable : public JavaScriptCallable {
	static bool compare_equal(const CallableCustom *p_a, const CallableCustom *p_b);
	static bool compare_less(const CallableCustom *p_a, const CallableCustom *p_b);

public:
	QuickJSCallable(JSContext *ctx, const JSValue &p_value);
	QuickJSCallable(const JavaScriptGCHandler &p_function);
	virtual ~QuickJSCallable();

	virtual uint32_t hash() const override;
	virtual String get_as_text() const override;

	virtual CompareEqualFunc get_compare_equal_func() const override { return QuickJSCallable::compare_equal; }
	virtual CompareLessFunc get_compare_less_func() const override { return QuickJSCallable::compare_less; }

	virtual ObjectID get_object() const override;
	virtual void call(const Variant **p_arguments, int p_argcount, Variant &r_return_value, Callable::CallError &r_call_error) const override;
};

#endif // QUICKJS_CALLABLE_H
