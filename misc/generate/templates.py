def get_templates_header():
    return (
        '/* THIS FILE IS GENERATED DO NOT EDIT */\n'
        '#include "javascript_template_manager.h"\n'
        'ScriptLanguage::ScriptTemplate JavaScriptLanguageManager::{} = \n'
        '{{\n'
        '"{}",\n'
        '"{}",\n'
        '"{}",\n'
        '{}\n'
        '}};'
    )


def get_templates_files():
    return {
        "src/language/templates/default_template.gen.cpp": (
            "DEFAULT_TEMPLATE",  # Name of the C++ variable
            "Node",  # Inherits from Godot Object
            "Default",  # Name of the template
            "Base template for Node with default Godot cycle methods",  # Description of the template
            "misc/template-contents/default.js",  # Path of the template
        ),
        "src/language/templates/empty_template.gen.cpp": (
            "EMPTY_TEMPLATE",  # Name of the C++ variable
            "Object",  # Inherits from Godot Object
            "Empty",  # Name of the template
            "Empty template suitable for all Objects",  # Description of the template
            "misc/template-contents/empty.js",  # Path of the template
        ),
    }
