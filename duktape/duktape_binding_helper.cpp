#include "duktape_binding_helper.h"
#include "../ecmascript_language.h"

void DuktapeBindingHelper::fatal_function(void *udata, const char *msg) {
	fprintf(stderr, "*** FATAL ERROR: %s\n", (msg ? msg : "no message"));
	fflush(stderr);
#ifdef DEBUG_ENABLED
	CRASH_NOW();
#elif
	abort();
#endif
}

void DuktapeBindingHelper::set_weak_ref(Object *obj, DuktapeHeapObject *ptr) {
	ERR_FAIL_NULL(obj);
	weakref_pool.set(obj->get_instance_id(), ptr);
}

DuktapeHeapObject *DuktapeBindingHelper::get_weak_ref(Object *obj) {
	ERR_FAIL_NULL_V(obj, NULL);
	void **ele_ptr = get_singleton()->weakref_pool.getptr(obj->get_instance_id());
	if (ele_ptr) {
		return *ele_ptr;
	}
	return NULL;
}

void DuktapeBindingHelper::set_strong_ref(Object *obj, DuktapeHeapObject *ptr) {

	ERR_FAIL_NULL(obj);

	weakref_pool.set(obj->get_instance_id(), ptr);
	strongref_pool.set(obj->get_instance_id(), ptr);

	// fill the script instance binding data to make sure godot_refcount_decremented can get when unreference
	obj->get_script_instance_binding(get_language()->get_language_index());

	duk_push_strong_ref_container(ctx);
	duk_push_heapptr(ctx, ptr);
	duk_put_prop_index(ctx, -2, obj->get_instance_id());
	duk_pop(ctx);
}

DuktapeHeapObject *DuktapeBindingHelper::get_strong_ref(Object *obj) {
	DuktapeHeapObject **val_ptr = strongref_pool.getptr(obj->get_instance_id());
	if (val_ptr) {
		return *val_ptr;
	}
	return NULL;
}

void DuktapeBindingHelper::duk_push_strong_ref_container(duk_context *ctx) {
	duk_push_heapptr(ctx, strongref_pool_ptr);
}

duk_ret_t DuktapeBindingHelper::duk_godot_object_constructor(duk_context *ctx) {

	duk_push_current_function(ctx);
	duk_get_prop_string(ctx, -1, DUK_HIDDEN_SYMBOL("cls"));
	ClassDB::ClassInfo *cls = static_cast<ClassDB::ClassInfo *>(duk_get_pointer_default(ctx, -1, NULL));
	duk_pop_2(ctx);

	ERR_FAIL_NULL_V(cls, DUK_ERR_TYPE_ERROR);
	ERR_FAIL_NULL_V(cls->creation_func, DUK_ERR_TYPE_ERROR);

	duk_push_godot_object(ctx, cls->creation_func(), true);
	ERR_FAIL_COND_V(duk_is_null_or_undefined(ctx, -1), DUK_ERR_TYPE_ERROR);

	//	call _init method
	duk_get_prop_literal(ctx, -1, "_init");
	ERR_FAIL_COND_V(!duk_is_function(ctx, -1), DUK_ERR_ERROR);
	duk_dup(ctx, -2);
	duk_call_method(ctx, 0);
	duk_pop(ctx);

	return DUK_NO_RET_VAL;
}

duk_ret_t DuktapeBindingHelper::duk_godot_object_finalizer(duk_context *ctx) {
	Object *ptr = duk_get_godot_object(ctx, -1);
	if (NULL == ptr) return DUK_NO_RET_VAL;

	if (Reference *ref = Object::cast_to<Reference>(ptr)) {
		if (ref->unreference()) {
			// A reference without C++ usage
			get_singleton()->weakref_pool.erase(ptr->get_instance_id());
			memdelete(ref);
		} else if (ref->get_script_instance_binding(get_language()->get_language_index())) {
			// A reference not used in C++
			memdelete(ref);
		} else {
			// A reference with other C++ reference
			// Rescue the ecmascript object as the reference is still alive
			get_singleton()->set_strong_ref(ref, duk_get_heapptr(ctx, 0));
		}
	}
	return DUK_NO_RET_VAL;
}

void DuktapeBindingHelper::godot_refcount_incremented(Reference *p_object) {
}

bool DuktapeBindingHelper::godot_refcount_decremented(Reference *p_object) {
	int refcount = p_object->reference_get_count();
	ECMAScriptBindingData *gc_handler = static_cast<ECMAScriptBindingData *>(p_object->get_script_instance_binding(get_language()->get_language_index()));
	if (gc_handler) {
		if (refcount == 0) {
			// clear taged strong reference in script
			// the life of this refernce is given to script gc
			set_strong_ref(p_object, NULL);
		}
		return false;
	}
	return refcount == 0;
}

Error DuktapeBindingHelper::eval_string(const String &p_source) {
	ERR_FAIL_NULL_V(ctx, ERR_SKIP);
	duk_eval_string(ctx, p_source.utf8().ptr());
	return OK;
}

void *DuktapeBindingHelper::alloc_object_binding_data(Object *p_object) {
	ECMAScriptBindingData *handler = NULL;
	if (DuktapeHeapObject *heap_ptr = get_strong_ref(p_object)) {
		handler = memnew(ECMAScriptBindingData);
		handler->godot_object = p_object;
		handler->ecma_object = heap_ptr;
	}
	return handler;
}

void DuktapeBindingHelper::free_object_binding_data(void *p_gc_handler) {
	if (ECMAScriptBindingData *handler = static_cast<ECMAScriptBindingData *>(p_gc_handler)) {
		if (Object::cast_to<Reference>(handler->godot_object)) {
			// References don't need do this as they are weak referenced or they
			return;
		} else if (handler->ecma_object) {
			// clear taged script reference
			set_strong_ref(handler->godot_object, NULL);
		}
		memdelete(handler);
	}
}

duk_ret_t DuktapeBindingHelper::godot_object_free(duk_context *ctx) {
	duk_push_this(ctx);
	Object *obj = duk_get_godot_object(ctx, -1);
	ERR_FAIL_NULL_V(obj, DUK_NO_RET_VAL);
	if (Object::cast_to<Reference>(obj)) {
		ERR_FAIL_V(DUK_ERR_TYPE_ERROR);
	} else {
		memdelete(obj);
		duk_del_prop_literal(ctx, -2, DUK_HIDDEN_SYMBOL("ptr"));
	}
	return DUK_NO_RET_VAL;
}

duk_ret_t DuktapeBindingHelper::godot_print_function(duk_context *ctx) {
	int size = duk_get_top(ctx);
	String msg;
	for (int i = 0; i < size; ++i) {
		msg += DuktapeBindingHelper::duk_get_godot_string(ctx, i, true);
		if (i < size - 1) {
			msg += " ";
		}
	}
	print_line(msg);
	return DUK_NO_RET_VAL;
}

duk_ret_t DuktapeBindingHelper::duk_godot_object_method(duk_context *ctx) {

	duk_idx_t argc = duk_get_top(ctx);

	duk_push_current_function(ctx);
	duk_get_prop_string(ctx, -1, DUK_HIDDEN_SYMBOL("mb"));
	MethodBind *mb = static_cast<MethodBind *>(duk_get_pointer_default(ctx, -1, NULL));
	ERR_FAIL_NULL_V(mb, DUK_NO_RET_VAL);

	duk_push_this(ctx);
	Object *ptr = duk_get_godot_object(ctx, -1);
	ERR_FAIL_NULL_V(ptr, DUK_NO_RET_VAL);

	Variant::CallError err;

	Vector<Variant> args;
	args.resize(argc);
	for (duk_idx_t i = 0; i < argc; ++i) {
		args.ptrw()->operator=(duk_get_godot_variant(ctx, i));
	}

	const Variant *args_ptr = args.ptr();
	const Variant &ret_val = mb->call(ptr, &args_ptr, argc, err);
	ERR_FAIL_COND_V(err.error != Variant::CallError::CALL_OK, DUK_ERR_TYPE_ERROR);

	if (mb->has_return()) {
		duk_push_godot_variant(ctx, ret_val);
		return DUK_HAS_RET_VAL;
	}
	return DUK_NO_RET_VAL;
}

duk_ret_t DuktapeBindingHelper::godot_object_to_string(duk_context *ctx) {
	duk_push_this(ctx);
	Variant var = duk_get_godot_object(ctx, -1);
	duk_push_godot_string(ctx, var);
	return DUK_HAS_RET_VAL;
}

void DuktapeBindingHelper::duk_push_godot_variant(duk_context *ctx, const Variant &var) {
	switch (var.get_type()) {
		case Variant::NIL:
			duk_push_null(ctx);
			break;
		case Variant::BOOL:
			duk_push_boolean(ctx, duk_bool_t((bool)var));
			break;
		case Variant::REAL:
		case Variant::INT:
			duk_push_number(ctx, var);
			break;
		case Variant::STRING: {
			duk_push_godot_string(ctx, var);
		} break;
		case Variant::OBJECT:
			duk_push_godot_object(ctx, var);
			break;
		default:
			// not supported
			duk_push_undefined(ctx);
			break;
	}
}

Variant DuktapeBindingHelper::duk_get_godot_variant(duk_context *ctx, duk_idx_t idx) {
	duk_int_t type = duk_get_type(ctx, idx);
	switch (type) {
		case DUK_TYPE_NUMBER:
			return Variant(duk_get_number(ctx, idx));
		case DUK_TYPE_STRING: {
			String str;
			str.parse_utf8(duk_get_string(ctx, idx));
			return str;
		} break;
		case DUK_TYPE_OBJECT: {
			Object *obj = duk_get_godot_object(ctx, idx);
			if (Reference *r = Object::cast_to<Reference>(obj)) {
				// Add reference count as the REF construct from Reference* don't increase the reference count
				// This reference count will minused in the desctuctor of REF
				REF ref(r);
				ref->reference();
				return ref;
			}
			return obj;
		}
		case DUK_TYPE_NULL:
		case DUK_TYPE_UNDEFINED:
		default:
			return Variant();
	}
}

String DuktapeBindingHelper::duk_get_godot_string(duk_context *ctx, duk_idx_t idx, bool convert_type) {
	const char *raw_str = convert_type ? duk_to_string(ctx, idx) : duk_get_string(ctx, idx);
	String str;
	str.parse_utf8(raw_str);
	return str;
}

Object *DuktapeBindingHelper::duk_get_godot_object(duk_context *ctx, duk_idx_t idx) {
	duk_get_prop_string(ctx, idx, DUK_HIDDEN_SYMBOL("ptr"));
	return static_cast<Object *>(duk_get_pointer_default(ctx, -1, NULL));
}

void DuktapeBindingHelper::duk_push_godot_object(duk_context *ctx, Object *obj, bool from_constructor) {
	if (obj) {
		DuktapeHeapObject *heap_obj = get_singleton()->get_weak_ref(obj);
		if (heap_obj) {
			duk_push_heapptr(ctx, heap_obj);
		} else {
			if (from_constructor) {
				duk_push_this(ctx);
			} else {
				duk_push_object(ctx);
				duk_push_heapptr(ctx, get_singleton()->class_prototypes.get(obj->get_class_name()));
				duk_put_prop_literal(ctx, -2, PROTO_LITERAL);
			}

			duk_push_pointer(ctx, obj);
			duk_put_prop_literal(ctx, -2, DUK_HIDDEN_SYMBOL("ptr"));

			heap_obj = duk_get_heapptr(ctx, -1);
			if (Reference *ref = Object::cast_to<Reference>(obj)) {
				get_singleton()->set_weak_ref(ref, heap_obj);
			} else {
				// The strong reference is released when object is going to die
				// See godot_free_instance_callback and godot_refcount_decremented
				get_singleton()->set_strong_ref(obj, heap_obj);
			}
		}
	} else {
		duk_push_undefined(ctx);
	}
}

void DuktapeBindingHelper::duk_push_godot_string(duk_context *ctx, const String &str) {
	if (str.empty()) {
		duk_push_lstring(ctx, NULL, 0);
	} else {
		duk_push_string(ctx, str.utf8().ptr());
	}
}

void DuktapeBindingHelper::duk_push_godot_string_name(duk_context *ctx, const StringName &str) {
	duk_push_godot_string(ctx, str);
}

void DuktapeBindingHelper::duk_put_prop_godot_string(duk_context *ctx, duk_idx_t idx, const String &str) {
	duk_put_prop_string(ctx, idx, str.utf8().ptr());
}

void DuktapeBindingHelper::duk_put_prop_godot_string_name(duk_context *ctx, duk_idx_t idx, const StringName &str) {
	duk_put_prop_godot_string(ctx, idx, str);
}

void DuktapeBindingHelper::rigister_class(duk_context *ctx, const ClassDB::ClassInfo *cls) {
	duk_require_stack(ctx, 2);

	// Class constuctor function
	duk_push_c_function(ctx, duk_godot_object_constructor, 0);
	duk_push_pointer(ctx, (void *)cls);
	duk_put_prop_literal(ctx, -2, DUK_HIDDEN_SYMBOL("cls"));
	{
		// Class.prototype
		duk_push_object(ctx);
		{
			// members
			register_class_members(ctx, cls);
		}

		DuktapeHeapObject *proto_ptr = duk_get_heapptr(ctx, -1);
		get_singleton()->class_prototypes[cls->name] = proto_ptr;

		duk_put_prop_literal(ctx, -2, PROTOTYPE_LITERAL);
	}
	duk_put_prop_godot_string_name(ctx, -2, cls->name);
}

DuktapeBindingHelper *DuktapeBindingHelper::get_singleton() {
	return dynamic_cast<DuktapeBindingHelper *>(ECMAScriptLanguage::get_binder());
}

ECMAScriptLanguage *DuktapeBindingHelper::get_language() {
	return ECMAScriptLanguage::get_singleton();
}

void DuktapeBindingHelper::initialize() {

	this->ctx = duk_create_heap(alloc_function, realloc_function, free_function, this, fatal_function);
	ERR_FAIL_NULL(ctx);

	// strong reference object pool
	duk_push_heap_stash(ctx);

	duk_push_object(ctx);
	this->strongref_pool_ptr = duk_get_heapptr(ctx, -1);
	duk_put_prop_literal(ctx, -2, "object_pool");
	{
		// pr-edefined functions for godot classes
		duk_push_c_function(ctx, duk_godot_object_finalizer, 1);
		this->duk_ptr_godot_object_finalizer = duk_get_heapptr(ctx, -1);
		duk_put_prop_literal(ctx, -2, "duk_godot_object_finalizer");

		duk_push_c_function(ctx, godot_object_virtual_method, 0);
		this->duk_ptr_godot_object_virtual_method = duk_get_heapptr(ctx, -1);
		duk_put_prop_literal(ctx, -2, "godot_object_virtual_method");

		duk_push_c_function(ctx, godot_object_free, 0);
		this->duk_ptr_godot_object_free = duk_get_heapptr(ctx, -1);
		duk_put_prop_literal(ctx, -2, "godot_object_free");

		duk_push_c_function(ctx, godot_object_to_string, 0);
		this->duk_ptr_godot_object_to_string = duk_get_heapptr(ctx, -1);
		duk_put_prop_literal(ctx, -2, "godot_object_to_string");
	}
	duk_pop(ctx);

	// global scope
	duk_push_c_function(ctx, godot_print_function, DUK_VARARGS);
	duk_put_global_literal(ctx, "print");

	// godot namespace
	duk_push_object(ctx);
	{
		// TODO: register builtin classes
		// register classes
		const StringName *key = ClassDB::classes.next(NULL);
		while (key) {
			const ClassDB::ClassInfo *cls = ClassDB::classes.getptr(*key);
			rigister_class(ctx, cls);
			key = ClassDB::classes.next(key);
		}
		// setup proto chain for prototypes
		key = ClassDB::classes.next(NULL);
		while (key) {
			const ClassDB::ClassInfo *cls = ClassDB::classes.getptr(*key);
			if (cls->inherits_ptr) {
				duk_require_stack(ctx, 2);
				DuktapeHeapObject *prototype_ptr = class_prototypes.get(cls->name);
				duk_push_heapptr(ctx, prototype_ptr);
				DuktapeHeapObject *base_prototype_ptr = class_prototypes.get(cls->inherits_ptr->name);
				duk_push_heapptr(ctx, base_prototype_ptr);
				duk_put_prop_literal(ctx, -2, PROTO_LITERAL);
				duk_pop(ctx);
			}
			key = ClassDB::classes.next(key);
		}

		// godot.register_class
		duk_push_c_function(ctx, register_ecma_class, 3);
		this->duk_ptr_register_ecma_class = duk_get_heapptr(ctx, -1);
		duk_put_prop_literal(ctx, -2, "register_class");

#if 0
		// godot.GDCLASS
		duk_push_c_function(ctx, decorator_register_ecma_class, 3);
		duk_put_prop_literal(ctx, -2, "GDCLASS");
#endif
	}

	duk_put_global_literal(ctx, "godot");
}

void DuktapeBindingHelper::uninitialize() {
	duk_destroy_heap(ctx);
	this->ctx = NULL;
}

void DuktapeBindingHelper::register_class_members(duk_context *ctx, const ClassDB::ClassInfo *cls) {

	if (cls->name == "Object") {
		// Object.prototype.fin
		duk_push_heapptr(ctx, duk_ptr_godot_object_finalizer);
		duk_set_finalizer(ctx, -2);
		// Object.prototype._init
		duk_push_heapptr(ctx, duk_ptr_godot_object_virtual_method);
		duk_put_prop_literal(ctx, -2, "_init");
		// Object.prototype.free
		duk_push_heapptr(ctx, duk_ptr_godot_object_free);
		duk_put_prop_literal(ctx, -2, "free");
		// Object.prototype.toString
		duk_push_heapptr(ctx, duk_ptr_godot_object_to_string);
		duk_put_prop_literal(ctx, -2, "toString");
	}

	const duk_idx_t prototype_idx = duk_get_top(ctx) - 1;

	{
		// methods
		const StringName *key = cls->method_map.next(NULL);
		while (key) {

			duk_require_stack(ctx, 2);

			MethodBind *mb = cls->method_map.get(*key);
			duk_push_godot_method(ctx, mb);
			duk_put_prop_godot_string_name(ctx, -2, mb->get_name());

			key = cls->method_map.next(key);
		}
	}

	{
		// properties
		const StringName *key = cls->property_setget.next(NULL);
		while (key) {

			duk_require_stack(ctx, 3);

			const ClassDB::PropertySetGet &prop = cls->property_setget[*key];
			duk_uidx_t masks = DUK_DEFPROP_FORCE;
			duk_push_godot_string_name(ctx, *key);
			if (prop._getptr) {
				duk_push_godot_method(ctx, prop._getptr);
				masks |= DUK_DEFPROP_HAVE_GETTER;
			}
			if (prop._setptr) {
				duk_push_godot_method(ctx, prop._setptr);
				masks |= DUK_DEFPROP_HAVE_SETTER;
			}

			if (masks) {
				duk_def_prop(ctx, prototype_idx, masks);
			} else {
				duk_pop(ctx);
			}
			key = cls->property_setget.next(key);
		}
	}
}

void DuktapeBindingHelper::duk_push_godot_method(duk_context *ctx, const MethodBind *mb) {

	DuktapeHeapObject **val_ele = method_bindings.getptr(mb);
	DuktapeHeapObject *heap_ptr = val_ele ? (*val_ele) : NULL;

	if (heap_ptr) {
		duk_push_heapptr(ctx, heap_ptr);
	} else {
		if (mb->get_hint_flags() & MethodFlags::METHOD_FLAG_VIRTUAL) {
			duk_push_heapptr(ctx, duk_ptr_godot_object_virtual_method);
			heap_ptr = duk_ptr_godot_object_virtual_method;
		} else {
			duk_size_t argc = mb->is_vararg() ? DUK_VARARGS : mb->get_argument_count();
			duk_push_c_function(ctx, duk_godot_object_method, argc);
			duk_push_pointer(ctx, (void *)mb);
			duk_put_prop_literal(ctx, -2, DUK_HIDDEN_SYMBOL("mb"));
			heap_ptr = duk_get_heapptr(ctx, -1);
		}
		method_bindings[mb] = heap_ptr;
	}
}

duk_ret_t DuktapeBindingHelper::register_ecma_class(duk_context *ctx) {
	// godot.register_class(cls, name, icon)
	const duk_idx_t CLASS_FUNC_IDX = 0;

	ERR_FAIL_COND_V(!duk_is_function(ctx, CLASS_FUNC_IDX), DUK_ERR_TYPE_ERROR);

	duk_get_prop_string(ctx, CLASS_FUNC_IDX, DUK_HIDDEN_SYMBOL("cls"));
	ClassDB::ClassInfo *cls = static_cast<ClassDB::ClassInfo *>(duk_get_pointer(ctx, -1));
	duk_pop(ctx);
	ERR_FAIL_NULL_V(cls, DUK_ERR_TYPE_ERROR);

	const char *class_name = duk_get_string_default(ctx, 1, NULL);
	const char *class_icon = duk_get_string_default(ctx, 2, NULL);

	duk_push_current_function(ctx);

	if (duk_is_null_or_undefined(ctx, 1) && !class_name) {
		duk_get_prop_literal(ctx, -1, DUK_HIDDEN_SYMBOL("cls_name"));
		class_name = duk_get_string_default(ctx, -1, NULL);
		duk_pop(ctx);
		if (!class_name) {
			duk_get_prop_literal(ctx, -1, "name");
			class_name = duk_get_string_default(ctx, -1, NULL);
			duk_pop(ctx);
			ERR_FAIL_COND_V(class_name == NULL || !strlen(class_name), DUK_ERR_EVAL_ERROR);
		}
	}

	if (duk_is_null_or_undefined(ctx, 2) && !class_icon) {
		duk_get_prop_literal(ctx, -1, DUK_HIDDEN_SYMBOL("cls_icon"));
		class_icon = duk_get_string_default(ctx, -1, "");
		duk_pop(ctx);
	}

	duk_pop(ctx);

	ECMAClassInfo ecma_class;
	ecma_class.class_name = class_name;
	ecma_class.icon_path = class_icon;
	ecma_class.native_class = cls;
	ecma_class.ecma_constructor = { duk_get_heapptr(ctx, CLASS_FUNC_IDX) };

	// ecmascript methods
	duk_get_prop_literal(ctx, CLASS_FUNC_IDX, PROTOTYPE_LITERAL);
	duk_enum(ctx, -1, DUK_HINT_NONE);
	while (duk_next(ctx, -1, true)) {
		if (duk_is_ecmascript_function(ctx, -1)) {
			StringName name = duk_get_godot_string(ctx, -2);
			ECMAMethodInfo method = { duk_get_heapptr(ctx, -1) };
			ecma_class.methods.set(name, method);
		}
		duk_pop_2(ctx);
	}
	duk_pop_2(ctx);

	get_singleton()->ecma_classes.set(class_name, ecma_class);

	duk_dup(ctx, CLASS_FUNC_IDX);
	return DUK_HAS_RET_VAL;
}

#if 0
duk_ret_t DuktapeBindingHelper::decorator_register_ecma_class(duk_context *ctx) {
	// godot.GDCLASS(name, icon)

	const char *class_name = duk_get_string_default(ctx, 0, NULL);
	const char *icon_path = duk_get_string_default(ctx, 1, NULL);

	duk_push_heapptr(ctx, get_singleton()->duk_ptr_register_ecma_class);
	duk_push_string(ctx, class_name);
	duk_put_prop_literal(ctx, -2, DUK_HIDDEN_SYMBOL("cls_name"));
	duk_push_string(ctx, icon_path);
	duk_put_prop_literal(ctx, -2, DUK_HIDDEN_SYMBOL("cls_icon"));

	return DUK_HAS_RET_VAL;
}
#endif


ECMAScriptGCHandler DuktapeBindingHelper::create_ecma_instance_for_godot_object(const StringName &ecma_class_name, Object *p_object) {

	ECMAScriptGCHandler ret = { NULL };

	ECMAClassInfo *ecma_class = ecma_classes.getptr(ecma_class_name);
	ERR_FAIL_NULL_V(p_object, ret);
	ERR_FAIL_NULL_V(ecma_class, ret);

	duk_require_stack(ctx, 3);

	duk_idx_t object_idx = duk_get_top(ctx);
	duk_push_object(ctx);

	duk_push_pointer(ctx, p_object);
	duk_put_prop_literal(ctx, -2, DUK_HIDDEN_SYMBOL("ptr"));

	duk_push_heapptr(ctx, ecma_class->ecma_constructor.ecma_object);
	duk_get_prop_literal(ctx, -1, PROTOTYPE_LITERAL);
	duk_remove(ctx, -2);
	duk_put_prop_literal(ctx, -2, PROTO_LITERAL);

	set_strong_ref(p_object, duk_get_heapptr(ctx, -1));

	//	call _init method
	duk_get_prop_literal(ctx, -1, "_init");
	ERR_FAIL_COND_V(!duk_is_function(ctx, -1), ret);
	duk_dup(ctx, -2);
	duk_call_method(ctx, 0);
	duk_pop(ctx);

	ret.ecma_object = duk_get_heapptr(ctx, object_idx);

	return ret;
}

Variant DuktapeBindingHelper::call_method(const ECMAScriptGCHandler &p_object, const ECMAMethodInfo &p_method, const Variant **p_args, int p_argcount, Variant::CallError &r_error) {
	ERR_FAIL_COND_V(p_object.is_null() || p_method.is_null(), NULL);

	duk_push_heapptr(ctx, p_method.ecma_object);
	duk_push_heapptr(ctx, p_object.ecma_object);

	for (int i = 0; i < p_argcount; ++i) {
		duk_push_godot_variant(ctx, *(p_args[i]));
	}
	duk_call_method(ctx, p_argcount);
	Variant ret = duk_get_godot_variant(ctx, -1);
	duk_pop(ctx);
	return ret;
}

