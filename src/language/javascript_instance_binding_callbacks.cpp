/* Instantiates global functions to allocate/free objets for godot */

#include "javascript_language.h"

void *JavaScriptLanguage::create_callback(void *p_token, void *p_instance) {
	if (JavaScriptBinder *binder = get_singleton()->get_thread_binder(Thread::get_caller_id())) {
		return binder->alloc_object_binding_data(static_cast<Object *>(p_instance));
	}
	return nullptr;
}

void JavaScriptLanguage::free_callback(void *p_token, void *p_instance, void *p_binding) {
	if (JavaScriptBinder *binder = get_singleton()->get_thread_binder(Thread::get_caller_id())) {
		return binder->free_object_binding_data(static_cast<JavaScriptGCHandler *>(p_binding));
	}
}

GDExtensionBool JavaScriptLanguage::reference_callback(void *p_token, void *p_binding, GDExtensionBool p_reference) {
	if (JavaScriptBinder *binder = get_singleton()->get_thread_binder(Thread::get_caller_id())) {
		if (p_reference) {
			binder->godot_refcount_incremented(static_cast<JavaScriptGCHandler *>(p_binding));
			return false;
		} else {
			return binder->godot_refcount_decremented(static_cast<JavaScriptGCHandler *>(p_binding));
		}
	}
	return true;
}
