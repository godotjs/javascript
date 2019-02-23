#include "ecma_class_browser.h"
#include "../ecmascript_language.h"
#include "editor/filesystem_dock.h"

struct ECMAScriptAlphCompare {
	_FORCE_INLINE_ bool operator()(const Ref<ECMAScript> &l, const Ref<ECMAScript> &r) const {
		return String(l->get_class_name()) < String(r->get_class_name());
	}
};

void ECMAScriptPlugin::_bind_methods() {
	ClassDB::bind_method("_on_bottom_panel_toggled", &ECMAScriptPlugin::_on_bottom_panel_toggled);
}

void ECMAScriptPlugin::_on_bottom_panel_toggled(bool pressed) {
	if (pressed) {
		ecma_class_browser->update_tree();
	}
}

void ECMAClassBrower::_on_filter_changed(const String &p_text) {
	update_tree();
}

Variant ECMAClassBrower::get_drag_data_fw(const Point2 &p_point, Control *p_from) {
	if (p_from == class_tree) {
		TreeItem *item = class_tree->get_item_at_position(class_tree->get_local_mouse_position());
		if (item) {
			Ref<ECMAScript> script = item->get_metadata(0);
			Vector<String> paths;

			String class_dir = String("res://ECMAClass/");
			String path = class_dir + script->get_class_name() + ".es";
			if (!res_dir->dir_exists(class_dir)) {
				res_dir->make_dir_recursive(class_dir);
			}
			if (!res_dir->file_exists(path)) {
				ResourceSaver::save(path, script);
			}

			paths.push_back(path);
			Dictionary drag_data = EditorNode::get_singleton()->drag_files_and_dirs(paths, p_from);
			return drag_data;
		}
	}
	return NULL;
}

ECMAScriptPlugin::ECMAScriptPlugin(EditorNode *p_node) {
	ecma_class_browser = memnew(ECMAClassBrower);
	bottom_button = p_node->add_bottom_panel_item("ECMAScript", ecma_class_browser);
	bottom_button->connect("toggled", this, "_on_bottom_panel_toggled");
}

void ECMAClassBrower::_bind_methods() {
	ClassDB::bind_method("_on_filter_changed", &ECMAClassBrower::_on_filter_changed);
	ClassDB::bind_method(D_METHOD("get_drag_data_fw"), &ECMAClassBrower::get_drag_data_fw);
}

void ECMAClassBrower::update_tree() {

	String filter = filter_input->get_text();
	class_tree->clear();

	List<Ref<ECMAScript> > classes;
	ECMAScriptLanguage::get_singleton()->get_registered_classes(classes);
	classes.sort_custom<ECMAScriptAlphCompare>();

	Ref<Texture> script_icon = get_icon("Script", "EditorIcons");

	TreeItem *root = class_tree->create_item();
	for (List<Ref<ECMAScript> >::Element *E = classes.front(); E; E = E->next()) {
		String name = E->get()->get_class_name();
		String native_class_name = E->get()->get_ecma_class()->native_class->name;
		if (filter.empty() || filter.is_subsequence_ofi(name) || filter.is_subsequence_ofi(native_class_name)) {
			TreeItem *item = class_tree->create_item(root);
			item->set_metadata(0, E->get());
			item->set_metadata(1, E->get());
			item->set_text(0, name);
			item->set_text(1, native_class_name);
			item->set_icon(0, script_icon);
			item->set_text_align(0, TreeItem::ALIGN_LEFT);
			item->set_text_align(1, TreeItem::ALIGN_CENTER);
		}
	}
}

ECMAClassBrower::ECMAClassBrower() :
		res_dir(DirAccess::open("res://")) {

	set_custom_minimum_size(Size2(0, 300));

	class_tree = memnew(Tree);
	class_tree->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	class_tree->set_hide_root(true);
	class_tree->set_column_titles_visible(true);
	class_tree->set_select_mode(Tree::SELECT_ROW);
	class_tree->set_columns(2);
	class_tree->set_column_title(0, TTR("Script Class"));
	class_tree->set_column_title(1, TTR("Native Class"));
	class_tree->set_drag_forwarding(this);
	add_child(class_tree);

	HBoxContainer *hbox = memnew(HBoxContainer);
	Label *filter_text = memnew(Label);
	filter_text->set_text(TTR("Filter:"));
	filter_input = memnew(LineEdit);
	hbox->add_child(filter_text);
	hbox->add_child(filter_input);
	filter_input->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	filter_input->connect("text_changed", this, "_on_filter_changed");
	add_child(hbox);
}
