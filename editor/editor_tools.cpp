/**************************************************************************/
/*  editor_tools.cpp                                                      */
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

#include "editor_tools.h"

#ifdef TOOLS_ENABLED

#include "core/math/expression.h"
#include "core/os/keyboard.h"
#include "editor/doc_tools.h"
#include "editor/editor_help.h"
#include "editor/filesystem_dock.h"

#include "../javascript_language.h"

#define TS_IGNORE "//@ts-ignore\n"
static HashMap<String, HashSet<String>> ts_ignore_errors;
static HashMap<String, HashSet<String>> removed_members;
static HashMap<String, List<String>> added_apis;
typedef HashMap<String, Vector<const DocData::ConstantDoc *>> ClassEnumerations;
static HashMap<String, ClassEnumerations> class_enumerations;

struct JavaScriptAlphCompare {
	_FORCE_INLINE_ bool operator()(const Ref<JavaScript> &l, const Ref<JavaScript> &r) const {
		return String(l->get_class_name()) < String(r->get_class_name());
	}
};

static Error dump_to_file(const String &p_path, const String &p_content) {
	Ref<FileAccess> tsconfig = FileAccess::open(p_path, FileAccess::WRITE);
	if (tsconfig.is_valid() && tsconfig->is_open()) {
		tsconfig->store_string(p_content);
		return OK;
	}
	return FAILED;
}

void JavaScriptPlugin::_bind_methods() {
	ClassDB::bind_method(D_METHOD("_on_menu_item_pressed"), &JavaScriptPlugin::_on_menu_item_pressed);
	ClassDB::bind_method(D_METHOD("_export_typescript_declare_file"), &JavaScriptPlugin::_export_typescript_declare_file);
	ClassDB::bind_method(D_METHOD("_export_enumeration_binding_file"), &JavaScriptPlugin::_export_enumeration_binding_file);
}

void JavaScriptPlugin::_notification(int p_what) {
	switch (p_what) {
		case MainLoop::NOTIFICATION_APPLICATION_FOCUS_IN: {
			const HashSet<Ref<JavaScript>> &scripts = JavaScriptLanguage::get_singleton()->get_scripts();
			for (const Ref<JavaScript> &s : scripts) {
				uint64_t last_time = s->get_last_modified_time();
				uint64_t time = FileAccess::get_modified_time(s->get_script_path());
				if (last_time != time) {
					JavaScriptLanguage::get_singleton()->reload_tool_script(s, true);
				}
			}
		} break;
	}
}

void JavaScriptPlugin::_on_menu_item_pressed(int item) {
	switch (item) {
		case MenuItem::ITEM_GEN_DECLARE_FILE:
			declaration_file_dialog->popup_centered_ratio();
			break;
		case MenuItem::ITEM_GEN_TYPESCRIPT_PROJECT:
			_generate_typescript_project();
			break;
		case MenuItem::ITEM_GEN_ENUM_BINDING_SCRIPT:
			enumberation_file_dialog->popup_centered_ratio();
			break;
	}
}

JavaScriptPlugin::JavaScriptPlugin(EditorNode *p_node) {
	PopupMenu *menu = memnew(PopupMenu);
	add_tool_submenu_item(TTR("JavaScript"), menu);
	menu->add_item(TTR("Generate TypeScript Declaration File"), ITEM_GEN_DECLARE_FILE);
	menu->add_item(TTR("Generate Enumeration Binding Script"), ITEM_GEN_ENUM_BINDING_SCRIPT);
	menu->add_item(TTR("Generate TypeScript Project"), ITEM_GEN_TYPESCRIPT_PROJECT);
	menu->connect("id_pressed", callable_mp(this, &JavaScriptPlugin::_on_menu_item_pressed));

	declaration_file_dialog = memnew(EditorFileDialog);
	declaration_file_dialog->set_title(TTR("Generate TypeScript Declaration File"));
	declaration_file_dialog->set_file_mode(EditorFileDialog::FILE_MODE_SAVE_FILE);
	declaration_file_dialog->set_access(EditorFileDialog::ACCESS_FILESYSTEM);
	declaration_file_dialog->add_filter(TTR("*.d.ts;TypeScript Declaration file"));
	declaration_file_dialog->set_current_file("godot.d.ts");
	declaration_file_dialog->connect("file_selected", callable_mp(this, &JavaScriptPlugin::_export_typescript_declare_file));
	EditorNode::get_singleton()->get_gui_base()->add_child(declaration_file_dialog);

	enumberation_file_dialog = memnew(EditorFileDialog);
	enumberation_file_dialog->set_title(TTR("Generate Enumeration Binding Script"));
	enumberation_file_dialog->set_file_mode(EditorFileDialog::FILE_MODE_SAVE_FILE);
	enumberation_file_dialog->set_access(EditorFileDialog::ACCESS_FILESYSTEM);
	enumberation_file_dialog->add_filter(TTR("*.mjs;JavaScript file"));
	enumberation_file_dialog->set_current_file("enumerations.mjs");
	enumberation_file_dialog->connect("file_selected", callable_mp(this, &JavaScriptPlugin::_export_enumeration_binding_file));
	EditorNode::get_singleton()->get_gui_base()->add_child(enumberation_file_dialog);

	ts_ignore_errors.clear();
	HashSet<String> ts_ignore_error_members;
	ts_ignore_errors.insert("ArrayMesh", ts_ignore_error_members);
	ts_ignore_error_members.clear();
	ts_ignore_error_members.insert("FLAG_MAX");
	ts_ignore_errors.insert("CPUParticles", ts_ignore_error_members);
	ts_ignore_error_members.clear();
	ts_ignore_error_members.insert("joy_connection_changed");
	ts_ignore_errors.insert("Input", ts_ignore_error_members);
	ts_ignore_error_members.clear();
	ts_ignore_error_members.insert("rotate");
	ts_ignore_errors.insert("PathFollow2D", ts_ignore_error_members);
	ts_ignore_error_members.clear();
	ts_ignore_error_members.insert("FLAG_MAX");
	ts_ignore_errors.insert("SpriteBase3D", ts_ignore_error_members);
}

static String apply_pattern(const String &p_pattern, const Dictionary &p_values) {
	String ret = p_pattern;
	for (const Variant *key = p_values.next(); key; key = p_values.next(key)) {
		String p = String("${") + String(*key) + "}";
		String v = p_values.get(*key, p);
		ret = ret.replace(p, v);
	}
	return ret;
}

static String format_enum_name(const String &enum_name) {
	if (enum_name.begins_with("Variant.")) {
		return enum_name.replace(".", "");
	}
	return enum_name;
}

void JavaScriptPlugin::_export_enumeration_binding_file(const String &p_path) {
	_export_typescript_declare_file("");
	String file_content = "// Tool generated file DO NOT modify manually\n"
						  "// Add this script as first autoload to your project to bind enumerations for release build of godot engine\n"
						  "\n"
						  "if (!godot.DEBUG_ENABLED) {\n"
						  "\tfunction bind(cls, enumerations) {\n"
						  "\t\tif (cls) Object.defineProperties(cls, enumerations);\n"
						  "\t};\n"
						  "\n"
						  "${enumerations}"
						  "}\n"
						  "export default class extends godot.Node {};\n";

	String enumerations = "";
	for (const auto &cls : class_enumerations) {
		const ClassEnumerations &enumeration = cls.value;
		const String &name = cls.key;
		String class_name = name;
		if (class_name != "godot") {
			class_name = "godot." + class_name;
		}
		String class_enums = "";
		uint32_t idx = 0;
		for (const auto &E : enumeration) {
			String enum_items_text = "{";
			const Vector<const DocData::ConstantDoc *> &consts = E.value;
			for (int i = 0; i < consts.size(); ++i) {
				const DocData::ConstantDoc *c = consts[i];
				enum_items_text += c->name + ": " + c->value;
				if (i < consts.size() - 1) {
					enum_items_text += ", ";
				}
			}
			enum_items_text += " }";
			Dictionary new_dict;
			new_dict["name"] = format_enum_name(E.key);
			new_dict["values"] = enum_items_text;
			class_enums += apply_pattern("\n\t\t${name}: { value: ${values} }", new_dict);
			if (idx < enumeration.size() - 1) {
				class_enums += ", ";
			} else {
				class_enums += "\n\t";
			}
			idx++;
		}
		static String class_template = "\tbind(${class}, {${enumerations}});\n";
		Dictionary class_dict;
		class_dict["class"] = class_name;
		class_dict["enumerations"] = class_enums;
		enumerations += apply_pattern(class_template, class_dict);
	}
	Dictionary enum_dict;
	enum_dict["enumerations"] = enumerations;
	file_content = apply_pattern(file_content, enum_dict);

	dump_to_file(p_path, file_content);
}

void JavaScriptPlugin::_generate_typescript_project() {
	_export_typescript_declare_file("res://godot.d.ts");
	dump_to_file("res://tsconfig.json", TSCONFIG_CONTENT);
	dump_to_file("res://decorators.ts", TS_DECORATORS_CONTENT);
	dump_to_file("res://package.json", PACKAGE_JSON_CONTENT);
}

// The following functions are used to generate a godot.d.ts file out of the docs folder from godot
#pragma region TS declare file

static String format_doc_text(const String &p_bbcode, const String &p_indent = "\t") {
	String markdown = p_bbcode.strip_edges();

	Vector<String> lines = markdown.split("\n");
	bool in_code_block = false;
	int code_block_indent = -1;

	markdown = "";
	for (int i = 0; i < lines.size(); i++) {
		String line = lines[i];
		int block_start = line.find("[codeblock]");
		if (block_start != -1) {
			code_block_indent = block_start;
			in_code_block = true;
			line = "```gdscript";
		} else if (in_code_block) {
			line = line.substr(code_block_indent, line.length());
		}

		if (in_code_block && line.find("[/codeblock]") != -1) {
			line = "```";
			in_code_block = false;
		}

		if (!in_code_block) {
			line = line.strip_edges();
			line = line.replace("[code]", "`");
			line = line.replace("[/code]", "`");
			line = line.replace("[i]", "*");
			line = line.replace("[/i]", "*");
			line = line.replace("[b]", "**");
			line = line.replace("[/b]", "**");
			line = line.replace("[u]", "__");
			line = line.replace("[/u]", "__");
			line = line.replace("[method ", "`");
			line = line.replace("[member ", "`");
			line = line.replace("[signal ", "`");
			line = line.replace("[enum ", "`");
			line = line.replace("[constant ", "`");
			line = line.replace("[", "`");
			line = line.replace("]", "`");
			line = line.replace("*/", "* /"); // To solve issues with accidental multiline comment ends
		}

		if (!in_code_block && i < lines.size() - 1) {
			line += "\n\n";
		} else if (i < lines.size() - 1) {
			line += "\n";
		}
		markdown += (i > 0 ? p_indent : "") + line;
	}
	return markdown;
}

static String format_identifier(const String &p_ident) {
	List<String> reserved_words;
	JavaScriptLanguage::get_singleton()->get_reserved_words(&reserved_words);
	if (reserved_words.find(p_ident)) {
		return String("p_") + p_ident;
	}
	return p_ident;
}

static String format_property_name(const String &p_ident) {
	List<String> reserved_words;
	JavaScriptLanguage::get_singleton()->get_reserved_words(&reserved_words);
	if (reserved_words.find(p_ident) || p_ident.find("/") != -1) {
		return String("'") + p_ident + "'";
	}
	return p_ident;
}

static String get_type_name(const String &p_type) {
	if (p_type.is_empty() || p_type == "void")
		return "void";

	if (p_type.ends_with("[]")) {
		String base_type = p_type.substr(0, p_type.length() - 2);
		return "Array<" + get_type_name(base_type) + ">";
	}

	if (p_type == "int" || p_type == "float")
		return "number";
	if (p_type == "bool")
		return "boolean";
	if (p_type == "String" || p_type == "NodePath")
		return "string";
	if (p_type == "Array")
		return "any[]";
	if (p_type == "Dictionary")
		return "object";
	if (p_type == "Variant" || p_type.contains("*"))
		return "any";
	if (p_type == "StringName")
		return "StringName | string";
	return p_type;
}

String _export_method(const DocData::MethodDoc &p_method, bool is_function = false, bool is_static = false, bool ts_ignore_error = false) {
	Dictionary dict;
	dict["description"] = format_doc_text(p_method.description, "\t\t ");
	dict["name"] = format_property_name(p_method.name);
	dict["static"] = is_static ? "static " : "";
	dict["return_type"] = get_type_name(p_method.return_type);
	String params = "";
	bool arg_default_value_started = false;
	for (int i = 0; i < p_method.arguments.size(); i++) {
		const DocData::ArgumentDoc &arg = p_method.arguments[i];
		if (!arg_default_value_started && !arg.default_value.is_empty()) {
			arg_default_value_started = true;
		}
		String arg_type = get_type_name(arg.type);
		if (!arg.enumeration.is_empty()) {
			arg_type = format_enum_name(arg.enumeration);
		}

		String default_value;
		if (arg_default_value_started) {
			default_value += " = ";
			if (arg.default_value.is_empty()) {
				if (arg.type == "string") {
					default_value += "''";
				} else {
					default_value += "new " + arg.type + "()";
				}
			} else {
				default_value += arg.default_value;
				// we don't want to have pointers or addresses in TS
				default_value = default_value
										.replace("&", "")
										.replace("*", "")
										.replace("**", "");
			}
		}

		String arg_str = format_identifier(arg.name) + ": " + arg_type + default_value;
		if (i < p_method.arguments.size() - 1) {
			arg_str += ", ";
		}
		params += arg_str;
	}

	if (p_method.qualifiers.find("vararg") != -1) {
		params += params.is_empty() ? "...args" : ", ...args";
	}
	dict["params"] = params;

	ts_ignore_error = ts_ignore_error || arg_default_value_started;
	String method_template = "\n"
							 "\t\t/** ${description} */\n"
							 "${TS_IGNORE}"
							 "\t\t${static}${name}(${params}) : ${return_type};\n";
	if (is_function) {
		method_template = "\n"
						  "\t/** ${description} */\n"
						  "${TS_IGNORE}"
						  "\tfunction ${name}(${params}) : ${return_type};\n";
		if (ts_ignore_error) {
			method_template = method_template.replace("${TS_IGNORE}", "\t" TS_IGNORE);
		} else {
			method_template = method_template.replace("${TS_IGNORE}", "");
		}
	} else {
		if (ts_ignore_error) {
			method_template = method_template.replace("${TS_IGNORE}", "\t\t" TS_IGNORE);
		} else {
			method_template = method_template.replace("${TS_IGNORE}", "");
		}
	}
	return apply_pattern(method_template, dict);
}

/* This function generates all classes for godot.d.ts based on DocData::ClassDoc */
String _export_class(const DocData::ClassDoc &class_doc,
		Dictionary missing_constructors,
		Dictionary missing_enums,
		Array ignore_methods) {
	String class_template = "\n"
							"\t/** ${brief_description}\n"
							"\t ${description} */\n"
							"${TS_IGNORE}"
							"\tclass ${name}${extends}${inherits} {\n"
							"${missingConstructors}"
							"${properties}"
							"${methods}"
							"${extrals}"
							"\t}\n"
							"\tnamespace ${name} {\n"
							"${missingEnums}"
							"${signals}"
							"${enumerations}"
							"${constants}"
							"\t}\n";
	class_template = class_template.replace("${TS_IGNORE}", ts_ignore_errors.has(class_doc.name) ? "\t" TS_IGNORE : "");
	Dictionary dict;
	dict["name"] = class_doc.name;
	dict["inherits"] = class_doc.inherits.is_empty() ? "" : get_type_name(class_doc.inherits);
	dict["extends"] = class_doc.inherits.is_empty() ? "" : " extends ";
	String brief_description = format_doc_text(class_doc.brief_description, "\t ");
	dict["brief_description"] = brief_description;
	String description = format_doc_text(class_doc.description, "\t ");
	if (description.length() == brief_description.length() && description == brief_description) {
		dict["description"] = "";
	} else {
		dict["description"] = description;
	}

	dict["missingConstructors"] = missing_constructors.get(class_doc.name, "");
	dict["missingEnums"] = missing_enums.get(class_doc.name, "");

	HashSet<String> ignore_members;
	if (removed_members.has(class_doc.name)) {
		ignore_members = removed_members[class_doc.name];
	}

	String constants = "";
	HashMap<String, Vector<const DocData::ConstantDoc *>> enumerations;
	for (const DocData::ConstantDoc &const_doc : class_doc.constants) {
		if (ignore_members.has(const_doc.name)) {
			continue;
		}

		Dictionary new_dict;
		new_dict["description"] = format_doc_text(const_doc.description, "\t\t ");
		new_dict["name"] = format_property_name(const_doc.name);
		new_dict["value"] = const_doc.value;
		String type = "number";
		if (!const_doc.enumeration.is_empty()) {
			type = const_doc.enumeration + "." + const_doc.name;
		} else if (const_doc.value.find("(") != -1) {
			type = const_doc.value.split("(")[0];
		} else if (const_doc.value.is_valid_int()) {
			type = new_dict["value"];
		}
		new_dict["type"] = type;

		String new_const_str = "\n"
							   "\t\t/** ${description} */\n"
							   "${TS_IGNORE}"
							   "\t\tconst ${name}: ${type};\n";
		if (ts_ignore_errors.has(class_doc.name) && ts_ignore_errors[class_doc.name].has(const_doc.name)) {
			new_const_str = new_const_str.replace("${TS_IGNORE}", "\t\t" TS_IGNORE);
		} else {
			new_const_str = new_const_str.replace("${TS_IGNORE}", "");
		}
		constants += apply_pattern(new_const_str, new_dict);

		if (!const_doc.enumeration.is_empty()) {
			if (!enumerations.has(const_doc.enumeration)) {
				Vector<const DocData::ConstantDoc *> e;
				e.push_back(&const_doc);
				enumerations.insert(const_doc.enumeration, e);
			} else {
				enumerations[const_doc.enumeration].push_back(&const_doc);
			}
		}
	}
	class_enumerations.insert(class_doc.name, enumerations);

	dict["constants"] = constants;
	String enumerations_str = "";
	for (const KeyValue<String, Vector<const DocData::ConstantDoc *>> &E : enumerations) {
		if (ignore_members.has(E.key)) {
			continue;
		}
		String enum_str = "\t\tenum " + E.key + " {\n";
		const Vector<const DocData::ConstantDoc *> &enums = E.value;
		for (int i = 0; i < enums.size(); i++) {
			String const_str = "\t\t\t/** ${description} */\n"
							   "\t\t\t${name} = ${value},\n";
			Dictionary new_dict;
			new_dict["description"] = format_doc_text(enums[i]->description, "\t\t\t ");
			new_dict["name"] = format_property_name(enums[i]->name);
			new_dict["value"] = enums[i]->value;
			enum_str += apply_pattern(const_str, new_dict);
		}
		enum_str += "\t\t}\n";
		enumerations_str += enum_str;
	}
	dict["enumerations"] = enumerations_str;

	Vector<DocData::MethodDoc> method_list = class_doc.methods;
	String properties = "";
	for (const DocData::PropertyDoc &prop_doc : class_doc.properties) {
		if (ignore_members.has(prop_doc.name))
			continue;

		String prop_str = "\n"
						  "\t\t/** ${description} */\n"
						  "${TS_IGNORE}"
						  "\t\t${static}${name}: ${type};\n";
		if (ts_ignore_errors.has(class_doc.name) && ts_ignore_errors[class_doc.name].has(prop_doc.name)) {
			prop_str = prop_str.replace("${TS_IGNORE}", "\t\t" TS_IGNORE);
		} else {
			prop_str = prop_str.replace("${TS_IGNORE}", "");
		}

		Dictionary new_dict;
		new_dict["description"] = format_doc_text(prop_doc.description, "\t\t ");
		new_dict["name"] = format_property_name(prop_doc.name);
		new_dict["type"] = get_type_name(prop_doc.type);
		new_dict["static"] = Engine::get_singleton()->has_singleton(class_doc.name) ? "static " : "";
		properties += apply_pattern(prop_str, new_dict);

		if (!prop_doc.getter.is_empty() && !ignore_methods.has(prop_doc.getter)) {
			DocData::MethodDoc md;
			md.name = prop_doc.getter;
			md.return_type = get_type_name(prop_doc.type);
			md.description = String("Getter of `") + prop_doc.name + "` property";
			method_list.push_back(md);
		}

		if (!prop_doc.setter.is_empty() && !ignore_methods.has(prop_doc.setter)) {
			DocData::MethodDoc md;
			md.name = prop_doc.setter;
			DocData::ArgumentDoc arg;
			arg.name = "p_value";
			arg.type = get_type_name(prop_doc.type);
			md.arguments.push_back(arg);
			md.return_type = "void";
			md.description = String("Setter of `") + prop_doc.name + "` property";
			method_list.push_back(md);
		}
	}
	dict["properties"] = properties;

	String signals = "";
	for (int i = 0; i < class_doc.signals.size(); ++i) {
		const DocData::MethodDoc &signal = class_doc.signals[i];
		if (ignore_members.has(signal.name))
			continue;

		String signal_str = "\n"
							"\t\t/** ${description} */\n"
							"${TS_IGNORE}"
							"\t\tconst ${name}: '${name}';\n";
		if (ts_ignore_errors.has(class_doc.name) && ts_ignore_errors[class_doc.name].has(signal.name)) {
			signal_str = signal_str.replace("${TS_IGNORE}", "\t\t" TS_IGNORE);
		} else {
			signal_str = signal_str.replace("${TS_IGNORE}", "");
		}
		Dictionary new_dict;
		new_dict["description"] = format_doc_text(signal.description, "\t\t\t ");
		new_dict["name"] = signal.name;
		signals += apply_pattern(signal_str, new_dict);
	}
	dict["signals"] = signals;

	// TODO: Theme properties
	String methods = "";
	for (int i = 0; i < method_list.size(); i++) {
		const DocData::MethodDoc &method_doc = method_list[i];
		if (ignore_members.has(method_doc.name))
			continue;

		if (method_doc.name == class_doc.name) {
			continue;
		}
		methods += _export_method(method_doc, false, Engine::get_singleton()->has_singleton(class_doc.name), ts_ignore_errors.has(class_doc.name) && ts_ignore_errors[class_doc.name].has(method_doc.name));
	}
	dict["methods"] = methods;

	String extrals;
	if (added_apis.has(class_doc.name)) {
		const List<String> &lines = added_apis[class_doc.name];
		for (const List<String>::Element *E = lines.front(); E; E = E->next()) {
			extrals += "\t\t" + E->get() + "\n";
		}
	}
	dict["extrals"] = extrals;

	return apply_pattern(class_template, dict);
}

void JavaScriptPlugin::_export_typescript_declare_file(const String &p_path) {
	modified_api = &JavaScriptLanguage::get_singleton()->get_main_binder()->get_modified_api();

	removed_members.clear();
	if (modified_api->has("removed")) {
		Dictionary removed = modified_api->operator[]("removed");
		const Variant *class_key = removed.next();
		while (class_key) {
			String class_name = *class_key;
			Array arr = removed[*class_key];
			HashSet<String> members;
			for (int i = 0; i < arr.size(); i++) {
				members.insert(arr[i]);
			}
			removed_members.insert(class_name, members);
			class_key = removed.next(class_key);
		}
	}
	added_apis.clear();
	if (modified_api->has("added")) {
		Dictionary added = modified_api->operator[]("added");
		const Variant *class_key = added.next();
		while (class_key) {
			String class_name = *class_key;
			Array arr = added[*class_key];
			List<String> lines;
			for (int i = 0; i < arr.size(); i++) {
				lines.push_back(arr[i]);
			}
			added_apis.insert(class_name, lines);
			class_key = added.next(class_key);
		}
	}

	const DocTools *doc = EditorHelp::get_doc_data();
	Dictionary dict;

	const String godot_module = "// This file is generated by godot editor\n"
								"${builtins}\n"
								"\n"
								"declare module " GODOT_OBJECT_NAME " {\n"
								"${constants}\n"
								"${enumerations}\n"
								"${functions}\n"
								"${classes}\n"
								"}\n"
								"\n";

	HashSet<String> ignored_classes;
	ignored_classes.insert("int");
	ignored_classes.insert("float");
	ignored_classes.insert("bool");
	ignored_classes.insert("String");
	ignored_classes.insert("Nil");
	ignored_classes.insert("Variant");
	ignored_classes.insert("Array");
	ignored_classes.insert("Dictionary");
	ignored_classes.insert("Semaphore");
	ignored_classes.insert("Thread");
	ignored_classes.insert("Mutex");

	String classes = "";
	String constants = "";
	String enumerations_str = "";
	String functions = "";

	for (const KeyValue<String, DocData::ClassDoc> &E : doc->class_list) {
		DocData::ClassDoc class_doc = E.value;
		if (ignored_classes.has(class_doc.name)) {
			continue;
		}

		if (class_doc.name != "StringName" && class_doc.name != "NodePath") {
			class_doc.name = get_type_name(class_doc.name);
		}

		if (class_doc.name.begins_with("@")) {
			HashMap<String, Vector<const DocData::ConstantDoc *>> enumerations;
			if (class_doc.name == "@GlobalScope" || class_doc.name == "@GDScript") {
				String const_str = "\n"
								   "\t/** ${description} */\n"
								   "\tconst ${name}: ${type};\n";
				for (int i = 0; i < class_doc.constants.size(); i++) {
					const DocData::ConstantDoc &const_doc = class_doc.constants[i];
					Dictionary new_dict;
					new_dict["description"] = format_doc_text(const_doc.description, "\t ");
					new_dict["name"] = format_property_name(const_doc.name);
					new_dict["value"] = const_doc.value;
					if (const_doc.name == "NAN" || const_doc.name == "INF") {
						new_dict["type"] = "number";
					} else if (!const_doc.enumeration.is_empty()) {
						new_dict["type"] = format_enum_name(const_doc.enumeration) + "." + const_doc.name;
					} else {
						new_dict["type"] = new_dict["value"];
					}
					constants += apply_pattern(const_str, new_dict);

					if (!const_doc.enumeration.is_empty()) {
						if (!enumerations.has(const_doc.enumeration)) {
							Vector<const DocData::ConstantDoc *> e;
							e.push_back(&const_doc);
							enumerations.insert(const_doc.enumeration, e);
						} else {
							enumerations[const_doc.enumeration].push_back(&const_doc);
						}
					}
				}
				class_enumerations.insert(GODOT_OBJECT_NAME, enumerations);

				for (auto F : enumerations) {
					String enum_str = "\tenum " + format_enum_name(F.key) + " {\n";
					const Vector<const DocData::ConstantDoc *> &enums = F.value;
					for (int i = 0; i < enums.size(); i++) {
						String new_const_str = "\t\t/** ${description} */\n"
											   "\t\t${name} = ${value},\n";
						Dictionary new_dict;
						new_dict["description"] = format_doc_text(enums[i]->description, "\t\t\t ");
						new_dict["name"] = format_property_name(enums[i]->name);
						new_dict["value"] = enums[i]->value;

						enum_str += apply_pattern(new_const_str, new_dict);
					}
					enum_str += "\t}\n";
					enumerations_str += enum_str;
				}
#if 0
				for (int i = 0; i < class_doc.methods.size(); i++) {
					const DocData::MethodDoc &method_doc = class_doc.methods[i];
					if (Expression::find_function(method_doc.name) == Expression::FUNC_MAX) {
						continue;
					}
					if (format_property_name(method_doc.name) != method_doc.name) {
						continue;
					}
					functions += _export_method(method_doc, true, false, ts_ignore_errors.has(class_doc.name) && ts_ignore_errors[class_doc.name].has(method_doc.name));
				}
#endif
			}
			continue;
		}
		classes += _export_class(
				class_doc,
				DECLARATION_CONSTRUCTORS,
				DECLARATION_ENUMS,
				IGNORE_METHODS);
	}
	dict["classes"] = classes;
	dict["constants"] = constants;
	dict["enumerations"] = enumerations_str;
	dict["functions"] = functions;
	dict["builtins"] = BUILTIN_DECLARATION_TEXT;

	String text = apply_pattern(godot_module, dict);
	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::WRITE);
	if (f.is_valid() && f->is_open()) {
		f->store_string(text);
	}
}

#pragma endregion TS declare file

#endif // TOOLS_ENABLED
