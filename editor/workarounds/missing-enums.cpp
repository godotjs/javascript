

/* All types are generated in editor_tools, but constructors are missing we need to add them manually. */

#include "../editor_tools.h"

Dictionary create_missing_enums() {
	Dictionary dict;
	dict["Vector3"] = "\t\tenum Axis {\n"
					  "\t\t\tAXIS_X = 0,\n"
					  "\t\t\tAXIS_Y = 1,\n"
					  "\t\t\tAXIS_Z = 2,\n"
					  "\t\t}\n";
	return dict;
}

Dictionary JavaScriptPlugin::DECLARATION_ENUMS = create_missing_enums();
