def get_editor_tools_header():
    return (
        '/* THIS FILE IS GENERATED DO NOT EDIT */\n'
        '#include "editor_tools.h"\n'
        'String JavaScriptPlugin::{} = \n'
        '{};'
    )


def get_editor_tools_files():
    return {
        "editor/godot.d.ts.gen.cpp": (
            "BUILTIN_DECLARATION_TEXT",
            "misc/typescript/godot.d.ts",
        )
    }
