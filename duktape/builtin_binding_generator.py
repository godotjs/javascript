#!/usr/bin/env python
import json, os

DUKTAPE_DIR = os.path.abspath( os.path.dirname(__file__) )
API_FILE = os.path.join(DUKTAPE_DIR, "..", "buitin_api.gen.json");
OUTPUT_FILE = os.path.join(DUKTAPE_DIR, "duktape_builtin_bindings.gen.cpp");

variant_types = {
	"boolean": "Variant::BOOL",
	"number": "Variant::REAL",
	"string": "Variant::STRING",
	"Vector2": "Variant::VECTOR2",
	"Vector3": "Variant::VECTOR3",
	"Basis": "Variant::BASIS",
	"Color": "Variant::COLOR",
	"Rect2": "Variant::RECT2",
	"RID": "Variant::_RID",
	"Transform2D": "Variant::TRANSFORM2D",
}

def apply_parttern(template, values):
	for key in values:
		template = template.replace( '${' + key + '}', values[key])
	return template

def process_constant(cls, const):
	template = \
'''
	duk_push_literal(ctx, "${name}");
	duk_push_variant(ctx, ${value});
	duk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_VALUE|DUK_DEFPROP_ENUMERABLE);
'''
	return apply_parttern(template, const);

def process_property(cls, prop):
	prop_type_map = {
		'number': {
			"push": 'duk_push_number(ctx, ptr->${name});',
			"get": 'ptr->${name} = duk_get_number_default(ctx, 0, DUK_DOUBLE_NAN);',
		},
		'Vector2': {
			"push": 'duk_push_variant(ctx, ptr->${name});',
			"get": 'ptr->${name} = duk_get_variant(ctx, 0);',
		}
	}
	
	template = \
'''
	duk_c_function property_${name}_c_func = [](duk_context *ctx)->duk_ret_t{

		duk_idx_t argc = duk_get_top(ctx);

		duk_push_this(ctx);
		duk_get_prop_string(ctx, -1, DUK_HIDDEN_SYMBOL("ptr"));
		${class} *ptr = static_cast<${class} *>(duk_get_pointer(ctx, -1));
		ERR_FAIL_NULL_V(ptr, DUK_ERR_TYPE_ERROR);

		if (argc) {
			${get}
		}
		${push}

		return DUK_HAS_RET_VAL;
	};
	duk_push_literal(ctx, "${name}");
	duk_push_c_function(ctx, property_${name}_c_func, 0);
	duk_push_c_function(ctx, property_${name}_c_func, 1);
	duk_def_prop(ctx, -4, DUK_DEFPROP_HAVE_GETTER | DUK_DEFPROP_HAVE_SETTER);
'''
	template = apply_parttern(template, prop_type_map[prop['type']])
	values = {
		'name': prop['name'],
		'class': cls['name']
	}
	template = apply_parttern(template, values)
	
	return template
	
def process_method(cls, method):
	sub_tempate = {
		"return": {
			"void": "return DUK_NO_RET_VAL;",
			"this": 'duk_push_this(ctx);',
			"Variant": 'duk_push_variant(ctx, ret);',
		}
	}
	
	
	get_arg_template = '''
		Variant arg${index} = duk_get_variant(ctx, ${index});
		ERR_FAIL_COND_V(arg${index}.get_type() != ${type}, DUK_ERR_TYPE_ERROR);
'''

	template = \
'''
	duk_push_c_function(ctx, [](duk_context *ctx)->duk_ret_t{
		
		duk_push_this(ctx);
		${class} * ptr = duk_get_builtin_ptr<${class}>(ctx, -1);
		ERR_FAIL_NULL_V(ptr, DUK_ERR_TYPE_ERROR);
		${get_args}
		${native_call}
		${return}
	}, ${argc});
	duk_put_prop_literal(ctx, -2, "${name}");
'''

	return_content = sub_tempate['return']['Variant']
	if method['return'] in sub_tempate['return']:
		return_content = sub_tempate['return'][method['return']];
	if method['return'] != 'void':
		return_content += '\n		return DUK_HAS_RET_VAL;'
	
	get_args = ''
	args = ''
	index = 0
	for arg in method['arguments']:
		arg_conf = {
			'index': str(index),
			'type': variant_types[arg['type']]
		}
		get_args += apply_parttern(get_arg_template, arg_conf)
		args += ',arg' + str(index);
		index += 1
	if len(args) > 0:
		args = args[1:]
		
	native_call = '';
	if method['return'] in ['void', 'this']:
		pass
	else:
		native_call += 'Variant ret = ';
	native_call += 'ptr->${native_method}(${args});'
	native_call = apply_parttern(native_call, {
		'native_method': method['native_method'],
		'args': args,
	})

	values = {
		'name': method['name'],
		'class': cls['name'],
		'argc': str(len(method['arguments'])),
		'return': return_content,
		'get_args': get_args,
		'native_call': native_call
	}
	template = apply_parttern(template, values)
	return template

def process_class(cls):
	template = \
'''
static void register_properties_${name}(duk_context *ctx) {
	
	duk_push_heapptr(ctx, class_constructors->get(${variant_type}));
	${constants}
	duk_pop(ctx);

	duk_push_heapptr(ctx, class_prototypes->get(${variant_type}));
	${properties}
	${methods}
	duk_pop(ctx);
}
'''

	constants = ''
	for const in cls['constants']:
		constants += process_constant(cls, const)
	
	properties = ''
	for prop in cls['properties']:
		properties += process_property(cls, prop)
	
	methods = ''
	for m in cls['methods']:
		methods += process_method(cls, m)
	
	values = {
		"name": cls['name'],
		"variant_type": variant_types[cls['name']],
		"constants": constants,
		"properties": properties,
		"methods": methods,
	}
	return apply_parttern(template, values)


def generate_binding_code():
	
	file_content = '''#include "duktape_builtin_bindings.h"
${class_properties_funcs}

void register_builtin_class_properties_gen(duk_context *ctx) {
${class_properties_func_calls}
}
'''
	classes = json.load(
		open(API_FILE, 'r')
	)
	class_properties_funcs = ""
	class_properties_func_calls = ""
	for cls in classes:
		class_properties_funcs += process_class(cls)
		call_template = '\tregister_properties_${name}(ctx);\n'
		call_template = apply_parttern(call_template, {'name': cls['name']})
		class_properties_func_calls += call_template
	file_content = apply_parttern(file_content, {
		'class_properties_funcs': class_properties_funcs,
		'class_properties_func_calls': class_properties_func_calls
	})
	
	return file_content

def generate_duktape_builtin_bindings():
	file = open(OUTPUT_FILE, 'w')
	file.write(generate_binding_code())

if __name__ == "__main__":
	generate_duktape_builtin_bindings()
