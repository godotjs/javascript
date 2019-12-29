#include "editor_tools.h"
#include "../ecmascript_language.h"
#include "core/math/expression.h"
#include "editor/filesystem_dock.h"

#define TS_IGNORE "//@ts-ignore\n"
static Map<String, Set<String> > ts_ignore_errors;

struct ECMAScriptAlphCompare {
	_FORCE_INLINE_ bool operator()(const Ref<ECMAScript> &l, const Ref<ECMAScript> &r) const {
		return String(l->get_class_name()) < String(r->get_class_name());
	}
};

void ECMAScriptPlugin::_bind_methods() {
	ClassDB::bind_method(D_METHOD("_on_menu_item_pressed"), &ECMAScriptPlugin::_on_menu_item_pressed);
	ClassDB::bind_method(D_METHOD("_export_typescript_declare_file"), &ECMAScriptPlugin::_export_typescript_declare_file);
}

void ECMAScriptPlugin::_on_menu_item_pressed(int item) {
	switch (item) {
		case MenuItem::ITEM_GEN_DECLAR_FILE:
			declaration_file_dialog->popup_centered_ratio();
			break;
	}
}

ECMAScriptPlugin::ECMAScriptPlugin(EditorNode *p_node) {

	PopupMenu *menu = memnew(PopupMenu);
	add_tool_submenu_item(TTR("ECMAScript"), menu);
	menu->add_item(TTR("Generate TypeScript Declaration File"), ITEM_GEN_DECLAR_FILE);
	menu->connect("id_pressed", this, "_on_menu_item_pressed");

	declaration_file_dialog = memnew(EditorFileDialog);
	declaration_file_dialog->set_title(TTR("Generate TypeScript Declaration File"));
	declaration_file_dialog->set_mode(EditorFileDialog::MODE_SAVE_FILE);
	declaration_file_dialog->set_access(EditorFileDialog::ACCESS_FILESYSTEM);
	declaration_file_dialog->add_filter(TTR("*.d.ts;TypeScript Declaration file"));
	declaration_file_dialog->set_current_file("godot.d.ts");
	declaration_file_dialog->connect("file_selected", this, "_export_typescript_declare_file");
	EditorNode::get_singleton()->get_gui_base()->add_child(declaration_file_dialog);

	ts_ignore_errors.clear();
	Set<String> ts_ignore_error_members;
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

static String applay_partern(const String &p_partern, const Dictionary &p_values) {
	String ret = p_partern;
	for (const Variant *key = p_values.next(); key; key = p_values.next(key)) {
		String p = String("${") + String(*key) + "}";
		String v = p_values.get(*key, p);
		ret = ret.replace(p, v);
	}
	return ret;
}

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
	ECMAScriptLanguage::get_singleton()->get_reserved_words(&reserved_words);
	if (reserved_words.find(p_ident)) {
		return String("p_") + p_ident;
	}
	return p_ident;
}

static String format_property_name(const String &p_ident) {
	List<String> reserved_words;
	ECMAScriptLanguage::get_singleton()->get_reserved_words(&reserved_words);
	if (reserved_words.find(p_ident) || p_ident.find("/") != -1) {
		return String("'") + p_ident + "'";
	}
	return p_ident;
}

static String get_type_name(const String &p_type) {
	if (p_type.empty())
		return "void";
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
	if (p_type == "Variant")
		return "any";
	return p_type;
}

String _export_method(const DocData::MethodDoc &p_method, bool is_function = false, bool is_static = false, bool ts_ignore_error = false) {

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

	Dictionary dict;
	dict["description"] = format_doc_text(p_method.description, "\t\t ");
	dict["name"] = format_property_name(p_method.name);
	dict["static"] = is_static ? "static " : "";
	dict["return_type"] = get_type_name(p_method.return_type);
	String params = "";
	bool arg_default_value_started = false;
	for (int i = 0; i < p_method.arguments.size(); i++) {
		const DocData::ArgumentDoc &arg = p_method.arguments[i];
		if (!arg_default_value_started && !arg.default_value.empty()) {
			arg_default_value_started = true;
		}
		String arg_str = format_identifier(arg.name) + (arg_default_value_started ? "?: " : ": ") + get_type_name(arg.type);
		if (i < p_method.arguments.size() - 1) {
			arg_str += ", ";
		}
		params += arg_str;
	}
	if (p_method.qualifiers.find("vararg") != -1) {
		params += params.empty() ? "...args" : ", ...args";
	}
	dict["params"] = params;
	return applay_partern(method_template, dict);
}

String _export_class(const DocData::ClassDoc &class_doc) {
	String class_template = "\n"
							"\t/** ${brief_description}\n"
							"\t ${description} */\n"
							"${TS_IGNORE}"
							"\tclass ${name}${extends}${inherits} {\n"
							"${signals}\n"
							"${constants}\n"
							"${enumerations}\n"
							"${properties}\n"
							"${methods}\n"
							"\t}\n";
	class_template = class_template.replace("${TS_IGNORE}", ts_ignore_errors.has(class_doc.name) ? "\t" TS_IGNORE : "");
	Dictionary dict;
	dict["name"] = class_doc.name;
	dict["inherits"] = class_doc.inherits.empty() ? "" : get_type_name(class_doc.inherits);
	dict["extends"] = class_doc.inherits.empty() ? "" : " extends ";
	String brief_description = format_doc_text(class_doc.brief_description, "\t ");
	dict["brief_description"] = brief_description;
	String description = format_doc_text(class_doc.description, "\t ");
	if (description.length() == brief_description.length() && description == brief_description) {
		dict["description"] = "";
	} else {
		dict["description"] = description;
	}

	String constants = "";
	HashMap<String, Vector<const DocData::ConstantDoc *> > enumerations;
	for (int i = 0; i < class_doc.constants.size(); i++) {
		const DocData::ConstantDoc &const_doc = class_doc.constants[i];
		Dictionary dict;
		dict["description"] = format_doc_text(const_doc.description, "\t\t ");
		dict["name"] = format_property_name(const_doc.name);
		dict["value"] = const_doc.value;
		String type = "number";
		if (const_doc.value.find("(") != -1) {
			type = const_doc.value.split("(")[0];
		}
		dict["type"] = type;
		if (const_doc.value.is_valid_integer()) {
			String const_str = "\n"
							   "\t\t/** ${description} */\n"
							   "${TS_IGNORE}"
							   "\t\tstatic readonly ${name}: ${value};\n";
			if (ts_ignore_errors.has(class_doc.name) && ts_ignore_errors[class_doc.name].has(const_doc.name)) {
				const_str = const_str.replace("${TS_IGNORE}", "\t\t" TS_IGNORE);
			} else {
				const_str = const_str.replace("${TS_IGNORE}", "");
			}
			constants += applay_partern(const_str, dict);
		} else {
			String const_str = "\n"
							   "\t\t/** ${description} \n"
							   "\t\t * @value `${value}` */\n"
							   "\t\tstatic readonly ${name}: ${type};\n";
			dict["type"] = const_doc.value.split("(")[0];
			constants += applay_partern(const_str, dict);
		}

		if (!const_doc.enumeration.empty()) {
			if (!enumerations.has(const_doc.enumeration)) {
				Vector<const DocData::ConstantDoc *> e;
				e.push_back(&const_doc);
				enumerations.set(const_doc.enumeration, e);
			} else {
				enumerations.get(const_doc.enumeration).push_back(&const_doc);
			}
		}
	}
	dict["constants"] = constants;

	String enumerations_str = "";
	const String *enum_name = enumerations.next(NULL);
	while (enum_name) {
		String enum_str = "\t\tstatic readonly " + *enum_name + " : {\n";
		const Vector<const DocData::ConstantDoc *> enums = enumerations.get(*enum_name);
		for (int i = 0; i < enums.size(); i++) {
			String const_str = "\t\t\t/** ${description} */\n"
							   "\t\t\t${name}: ${value},\n";
			Dictionary dict;
			dict["description"] = format_doc_text(enums[i]->description, "\t\t\t ");
			dict["name"] = format_property_name(enums[i]->name);
			dict["value"] = enums[i]->value;
			enum_str += applay_partern(const_str, dict);
		}
		enum_str += "\t\t}\n";
		enumerations_str += enum_str;
		enum_name = enumerations.next(enum_name);
	}
	dict["enumerations"] = enumerations_str;

	Vector<DocData::MethodDoc> method_list = class_doc.methods;
	String properties = "";
	for (int i = 0; i < class_doc.properties.size(); i++) {
		const DocData::PropertyDoc &prop_doc = class_doc.properties[i];

		String prop_str = "\n"
						  "\t\t/** ${description} */\n"
						  "${TS_IGNORE}"
						  "\t\t${static}${name}: ${type};\n";
		if (ts_ignore_errors.has(class_doc.name) && ts_ignore_errors[class_doc.name].has(prop_doc.name)) {
			prop_str = prop_str.replace("${TS_IGNORE}", "\t\t" TS_IGNORE);
		} else {
			prop_str = prop_str.replace("${TS_IGNORE}", "");
		}

		Dictionary dict;
		dict["description"] = format_doc_text(prop_doc.description, "\t\t ");
		dict["name"] = format_property_name(prop_doc.name);
		dict["type"] = get_type_name(prop_doc.type);
		dict["static"] = Engine::get_singleton()->has_singleton(class_doc.name) ? "static " : "";
		properties += applay_partern(prop_str, dict);

		if (!prop_doc.getter.empty()) {
			DocData::MethodDoc md;
			md.name = prop_doc.getter;
			md.return_type = get_type_name(prop_doc.type);
			md.description = String("Getter of `") + prop_doc.name + "` property";
			method_list.push_back(md);
		}

		if (!prop_doc.setter.empty()) {
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
		String signal_str = "\n"
							"\t\t/** ${description} */\n"
							"${TS_IGNORE}"
							"\t\tstatic ${name}: '${name}';\n";
		if (ts_ignore_errors.has(class_doc.name) && ts_ignore_errors[class_doc.name].has(signal.name)) {
			signal_str = signal_str.replace("${TS_IGNORE}", "\t\t" TS_IGNORE);
		} else {
			signal_str = signal_str.replace("${TS_IGNORE}", "");
		}
		Dictionary dict;
		dict["description"] = format_doc_text(signal.description, "\t\t\t ");
		dict["name"] = signal.name;
		signals += applay_partern(signal_str, dict);
	}
	dict["signals"] = signals;

	// TODO: Theme properties
	String methods = "";
	for (int i = 0; i < method_list.size(); i++) {
		const DocData::MethodDoc &method_doc = method_list[i];
		if (method_doc.name == class_doc.name) {
			continue;
		}
		methods += _export_method(method_doc, false, Engine::get_singleton()->has_singleton(class_doc.name), ts_ignore_errors.has(class_doc.name) && ts_ignore_errors[class_doc.name].has(method_doc.name));
	}
	dict["methods"] = methods;

	return applay_partern(class_template, dict);
}

void ECMAScriptPlugin::_export_typescript_declare_file(const String &p_path) {
	DocData *doc = EditorHelp::get_doc_data();
	Dictionary dict;

	const String godot_module = "// This file is generated by godot editor\n"
								"${buitins}\n"
								"\n"
								"declare module " GODOT_OBJECT_NAME " {\n"
								"${constants}\n"
								"${enumerations}\n"
								"${functions}\n"
								"${classes}\n"
								"}\n"
								"\n";

	Set<String> ignored_classes;
	ignored_classes.insert("int");
	ignored_classes.insert("float");
	ignored_classes.insert("bool");
	ignored_classes.insert("String");
	ignored_classes.insert("Nil");
	ignored_classes.insert("Variant");
	ignored_classes.insert("Array");
	ignored_classes.insert("Dictionary");
#if 1
	ignored_classes.insert("Vector2");
	ignored_classes.insert("Vector3");
	ignored_classes.insert("Color");
	ignored_classes.insert("Rect2");
	ignored_classes.insert("RID");
	ignored_classes.insert("NodePath");
	ignored_classes.insert("Transform2D");
	ignored_classes.insert("Transform");
	ignored_classes.insert("Basis");
	ignored_classes.insert("Quat");
	ignored_classes.insert("Plane");
	ignored_classes.insert("AABB");
	ignored_classes.insert("PoolByteArray");
	ignored_classes.insert("PoolIntArray");
	ignored_classes.insert("PoolRealArray");
	ignored_classes.insert("PoolStringArray");
	ignored_classes.insert("PoolVector2Array");
	ignored_classes.insert("PoolVector3Array");
	ignored_classes.insert("PoolColorArray");
#endif
	ignored_classes.insert("Semaphore");
	ignored_classes.insert("Thread");
	ignored_classes.insert("Mutex");

	String classes = "";
	String constants = "";
	String enumerations_str = "";
	String functions = "";

	for (Map<String, DocData::ClassDoc>::Element *E = doc->class_list.front(); E; E = E->next()) {
		DocData::ClassDoc class_doc = E->get();
		if (ignored_classes.has(class_doc.name)) {
			continue;
		}
		class_doc.name = get_type_name(class_doc.name);
		if (class_doc.name.begins_with("@")) {
			HashMap<String, Vector<const DocData::ConstantDoc *> > enumerations;
			if (class_doc.name == "@GlobalScope" || class_doc.name == "@GDScript") {
				String const_str = "\n"
								   "\t/** ${description} */\n"
								   "\tconst ${name}: ${value};\n";
				for (int i = 0; i < class_doc.constants.size(); i++) {
					const DocData::ConstantDoc &const_doc = class_doc.constants[i];
					Dictionary dict;
					dict["description"] = format_doc_text(const_doc.description, "\t ");
					dict["name"] = format_property_name(const_doc.name);
					dict["value"] = const_doc.value;
					if (const_doc.value == "nan" || const_doc.value == "inf") {
						dict["value"] = "number";
					}
					constants += applay_partern(const_str, dict);

					if (!const_doc.enumeration.empty()) {
						if (!enumerations.has(const_doc.enumeration)) {
							Vector<const DocData::ConstantDoc *> e;
							e.push_back(&const_doc);
							enumerations.set(const_doc.enumeration, e);
						} else {
							enumerations.get(const_doc.enumeration).push_back(&const_doc);
						}
					}
				}

				const String *enum_name = enumerations.next(NULL);
				while (enum_name) {
					String enum_str = "\tconst " + (*enum_name).replace(".", "") + " : {\n";
					const Vector<const DocData::ConstantDoc *> enums = enumerations.get(*enum_name);
					for (int i = 0; i < enums.size(); i++) {
						String const_str = "\t\t/** ${description} */\n"
										   "\t\t${name}: ${value},\n";
						Dictionary dict;
						dict["description"] = format_doc_text(enums[i]->description, "\t\t\t ");
						dict["name"] = format_property_name(enums[i]->name);
						dict["value"] = enums[i]->value;
						enum_str += applay_partern(const_str, dict);
					}
					enum_str += "\t}\n";
					enumerations_str += enum_str;
					enum_name = enumerations.next(enum_name);
				}

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
			}
			continue;
		}
		classes += _export_class(class_doc);
	}
	dict["classes"] = classes;
	dict["constants"] = constants;
	dict["enumerations"] = enumerations_str;
	dict["functions"] = functions;
	dict["buitins"] = BUILTIN_DECLEARATION_TEXT;

	String text = applay_partern(godot_module, dict);
	FileAccessRef f = FileAccess::open(p_path, FileAccess::WRITE);
	if (f.f && f->is_open()) {
		f->store_string(text);
		f->close();
	}
}
