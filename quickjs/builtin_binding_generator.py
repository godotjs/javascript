#!/usr/bin/env python
import json, os

DIR = os.path.abspath( os.path.dirname(__file__) )
OUTPUT_FILE = os.path.join(DIR, "quickjs_builtin_binder.gen.cpp")
API = json.load(open(os.path.join(DIR, '..', 'buitin_api.gen.json'), 'r'))

VariantTypes = {
	"boolean": "Variant::BOOL",
	"number": "Variant::REAL",
	"string": "Variant::STRING",
	"Vector2": "Variant::VECTOR2",
	"Vector3": "Variant::VECTOR3",
	"Basis": "Variant::BASIS",
	"Quat": "Variant::QUAT",
	"Color": "Variant::COLOR",
	"Rect2": "Variant::RECT2",
	"RID": "Variant::_RID",
	"Transform2D": "Variant::TRANSFORM2D",
	"Plane": "Variant::PLANE",
	"AABB": "Variant::AABB",
	"Transform": "Variant::TRANSFORM",
	"PoolByteArray": "Variant::POOL_BYTE_ARRAY",
	"PoolIntArray": "Variant::POOL_INT_ARRAY",
	"PoolRealArray": "Variant::POOL_REAL_ARRAY",
	"PoolStringArray": "Variant::POOL_STRING_ARRAY",
	"PoolVector2Array": "Variant::POOL_VECTOR2_ARRAY",
	"PoolVector3Array": "Variant::POOL_VECTOR3_ARRAY",
	"PoolColorArray": "Variant::POOL_COLOR_ARRAY",
}

def apply_parttern(template, values):
	for key in values:
		template = template.replace( '${' + key + '}', values[key])
	return template


def generate_constructor(cls):
	TemplateConstructorName = '${class}_constructor'
	TemplateConstructorDeclare = 'static JSValue ${class}_constructor(JSContext *ctx, JSValueConst new_target, int argc, JSValueConst *argv);\n'
	TemplateConstructor = '''
static JSValue ${func}(JSContext *ctx, JSValueConst new_target, int argc, JSValueConst *argv) {
	${class} *ptr = memnew(${class});
	${initializer}
	return QuickJSBuiltinBinder::bind_builtin_object(ctx, ${type}, ptr);
}
'''
	ConstructorInitializers = {
	"Vector2": '''
	if (argc >= 2) {
		double x, y;
		JS_ToFloat64(ctx, &x, argv[0]);
		JS_ToFloat64(ctx, &y, argv[1]);
		ptr->x = x;
		ptr->y = y;
	}
	''',
	"Rect2": '''
	if (argc >= 4) {
		double x, y, w, h;
		JS_ToFloat64(ctx, &x, argv[0]);
		JS_ToFloat64(ctx, &y, argv[1]);
		JS_ToFloat64(ctx, &w, argv[2]);
		JS_ToFloat64(ctx, &h, argv[3]);
		ptr->position.x = x;
		ptr->position.y = y;
		ptr->size.x = w;
		ptr->size.y = h;
	} else if (argc >= 2) {
		ECMAScriptGCHandler *param0 = BINDING_DATA_FROM_JS(ctx, argv[0]);
		ECMAScriptGCHandler *param1 = BINDING_DATA_FROM_JS(ctx, argv[1]);
		if (!param0 || !param1) return JS_EXCEPTION;
		if (param0->type != Variant::VECTOR2 || param1->type != Variant::VECTOR2) return JS_EXCEPTION;
		ptr->position = *(static_cast<Vector2 *>(param0->godot_builtin_object_ptr));
		ptr->size = *(static_cast<Vector2 *>(param1->godot_builtin_object_ptr));
	}
	'''
}
	class_name = cls['name']
	constructor_name = apply_parttern(TemplateConstructorName, {"class": class_name})
	constructor_declare = apply_parttern(TemplateConstructorDeclare, {"class": class_name})
	
	initializer = ''
	if class_name in ConstructorInitializers:
		initializer = ConstructorInitializers[class_name]
	consturctor = apply_parttern(TemplateConstructor, {
		'class': class_name,
		'type': VariantTypes[class_name],
		'func': constructor_name,
		'initializer': initializer
	})
	return constructor_name, constructor_declare, consturctor

def generate_property_bindings(cls):
	class_name = cls['name']
	TemplateDeclar = 'static void bind_${class}_properties(JSContext *ctx);\n'
	TemplateBind = '\tbind_${class}_properties(ctx);\n'
	def generate_members(cls):
		Template = '''
	JSCFunctionMagic *getter = [](JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv, int magic) -> JSValue {
		ECMAScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, this_val);
		const ${class} *ptr = static_cast<${class} *>(bind->godot_builtin_object_ptr);
		switch (magic) {\
${getters}
		}
		return JS_UNDEFINED;
	};
	
	JSCFunctionMagic *setter = [](JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv, int magic) -> JSValue {
		ECMAScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, this_val);
		${class} *ptr = static_cast<${class} *>(bind->godot_builtin_object_ptr);\
${validation}
		switch (magic) {\
${setters}
		}
		return argv[0];
	};
${bindings}
		'''
		TemplateGetterItem = '''
			case ${index}:
				return QuickJSBinder::variant_to_var(ctx, ptr->${name});'''
		TemplateSetterItem = '''
			case ${index}:
#ifdef DEBUG_METHODS_ENABLED
				ERR_FAIL_COND_V(!QuickJSBinder::validate_type(ctx, ${type}, argv[0]), (JS_ThrowTypeError(ctx, "${type_name} expected for ${class}.${name}")));
#endif
				ptr->${name} = QuickJSBinder::var_to_variant(ctx, argv[0]);
				break;'''
		TemplateItemBinding = '\tbinder->get_builtin_binder().register_property(${type}, "${name}", getter, setter, ${index});\n'
		getters = ''
		setters = ''
		bindings = ''
		for i in range(len(cls['properties'])):
			p = cls['properties'][i]
			name = p['name']
			getters += apply_parttern(TemplateGetterItem, {'index': str(i), 'name': name})
			setters += apply_parttern(TemplateSetterItem, {
				'index': str(i),
				'name': name,
				'type': VariantTypes[p['type']],
				'type_name': p['type'],
				'class': class_name,
			})
			bindings += apply_parttern(TemplateItemBinding, {'index': str(i), 'name': name, 'type': VariantTypes[class_name]})
		return apply_parttern(Template, {
			'class': class_name,
			'getters': getters,
			'setters': setters,
			'bindings': bindings,
			'validation': ''
		})
	
	def generate_methods(cls):
		TemplateMethod = '''
	binder->get_builtin_binder().register_method(
		${type},
		"${name}",
		[](JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
			ECMAScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, this_val);
			${class} *ptr = static_cast<${class} *>(bind->godot_builtin_object_ptr);\
${arg_declars}
			${call}
			return ${return};
		},
		${argc});'''
		TemplateArgDeclear = '''
#ifdef DEBUG_METHODS_ENABLED
			ERR_FAIL_COND_V(!QuickJSBinder::validate_type(ctx, ${type}, argv[${index}]), (JS_ThrowTypeError(ctx, "${type_name} expected for argument ${index} of ${class}.${name}")));
#endif
			Variant arg${index} = QuickJSBinder::var_to_variant(ctx, argv[${index}]);
'''
		bindings = ''
		for m in cls['methods']:
			args = ''
			arg_declars = ''
			for i in range(len(m['arguments'])):
				arg = m['arguments'][i]
				arg_declars += apply_parttern(TemplateArgDeclear, {
					'index': str(i),
					'type': VariantTypes[arg['type']],
					'type_name': arg['type'],
					'class': class_name,
					'name': m['name']
				})
				if i > 0: args += ', '
				args += 'arg' + str(i)
			CallTemplate = ('' if m['return'] == 'void' else 'Variant ret = ') + 'ptr->${native_method}(${args});'
			call = apply_parttern(CallTemplate, {'native_method': m['native_method'], 'args': args})
			bindings += apply_parttern(TemplateMethod, {
				"class": class_name,
				"type": VariantTypes[class_name],
				"name": m['name'],
				"call": call,
				"arg_declars": arg_declars,
				"argc": str(len(m['arguments'])),
				"return": 'JS_UNDEFINED' if m['return'] == 'void' else 'QuickJSBinder::variant_to_var(ctx, ret)'
			})
		return bindings
		
	def generate_constants(cls):
		ConstTemplate = '\tbinder->get_builtin_binder().register_constant(${type}, "${name}", ${value});\n'
		bindings = ''
		for c in cls['constants']:
			bindings += apply_parttern(ConstTemplate, {
				"name": c['name'],
				"type": VariantTypes[class_name],
				"value": c['value']
			})
		return bindings
	
	def genertate_operators(cls):
		OperatorMap = {
			'operator+': 'QuickJSBinder::JS_ATOM_Symbol_operatorAdd',
			'operator-': 'QuickJSBinder::JS_ATOM_Symbol_operatorSub',
			'operator*': 'QuickJSBinder::JS_ATOM_Symbol_operatorMul',
			'operator/': 'QuickJSBinder::JS_ATOM_Symbol_operatorDiv',
		}
		TargetDeclearTemplate = '''
			ECMAScriptGCHandler *bind1 = BINDING_DATA_FROM_JS(ctx, argv[1]);
			${target_class} *target = static_cast<${target_class} *>(bind1->godot_builtin_object_ptr);\
'''
		OperatorTemplate = '''
	binder->get_builtin_binder().register_operator(
		${type},
		${js_op},
		[](JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
			ECMAScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, argv[0]);
			${class} *ptr = static_cast<${class} *>(bind->godot_builtin_object_ptr);\
${target_declear}
			${call}
			return ${return};
		},
		${argc});
'''
		bindings = ''
		for o in cls['operators']:
			op = o['native_method']
			if op in OperatorMap:
				argc = len(o['arguments']) + 1
				js_op = OperatorMap[op]
				if argc <= 1:
					if op in ['operator-']:
						js_op = 'QuickJSBinder::JS_ATOM_Symbol_operatorNeg'
					
				args = ''
				target_declear = ''
				if argc > 1:
					target_declear = apply_parttern(TargetDeclearTemplate, {'target_class': o['arguments'][0]['type']})
					args = '*target'
				CallTemplate = ('' if o['return'] == 'void' else 'Variant ret = ') + 'ptr->${op}(${args});'
				call = apply_parttern(CallTemplate, {'op': op, 'args': args})
				bindings += apply_parttern(OperatorTemplate, {
					'type': VariantTypes[class_name],
					'class': class_name,
					'js_op': js_op,
					'call': call,
					'target_declear': target_declear,
					"return": 'JS_UNDEFINED' if o['return'] == 'void' else 'QuickJSBinder::variant_to_var(ctx, ret)',
					'argc': str(argc)
				})
		return bindings
	
	TemplateBindDefine = '''
static void bind_${class}_properties(JSContext *ctx) {
	QuickJSBinder *binder = QuickJSBinder::get_context_binder(ctx);
${constants}
${members}
${methods}
${operators}
}
'''
	class_name = cls['name']
	property_declare = apply_parttern(TemplateDeclar, {"class": class_name})
	property_defines = apply_parttern(TemplateBindDefine, {
		"class": class_name,
		"members": generate_members(cls) if len(cls['properties']) else '',
		"methods": generate_methods(cls),
		"constants": generate_constants(cls),
		"operators": genertate_operators(cls),
	})
	property_bind = apply_parttern(TemplateBind, {"class": class_name})
	return property_declare, property_defines, property_bind


def generate_class_bind_action(cls, constructor):
	Template = '\tregister_builtin_class(${type}, "${class}", ${constructor}, ${argc});\n'
	return apply_parttern(Template, {
		'class': cls['name'],
		'constructor': constructor,
		'type': VariantTypes[cls['name']],
		'argc': str(cls['constructor_argc'])
	})



def generate_builtin_bindings():
	Template = '''\
#include "core/variant.h"
#include "quickjs_binder.h"
#include "quickjs_builtin_binder.h"

#ifndef inf
#define inf INFINITY
#endif

${declarations}

void QuickJSBuiltinBinder::bind_builtin_classes_gen() {

${bindings}
}

${definitions}
'''
	
	declarations = ''
	definitions = ''
	bindings = ''
	for cls in API:
		constructor_name, constructor_declare, consturctor = generate_constructor(cls)
		declarations += constructor_declare
		definitions += consturctor
		bindings += generate_class_bind_action(cls, constructor_name)
		
		property_declare, property_defines, property_bind= generate_property_bindings(cls)
		declarations += property_declare
		definitions += property_defines
		bindings += property_bind
	
	output = apply_parttern(Template, {
		'declarations': declarations,
		'bindings': bindings,
		'definitions': definitions,
	})
	file = open(OUTPUT_FILE, 'w')
	file.write(output)

if __name__ == "__main__":
	generate_builtin_bindings()