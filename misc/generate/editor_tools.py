def get_editor_tools_header():
    return (
        '/* THIS FILE IS GENERATED DO NOT EDIT */\n#include "editor_tools.h"\nString JavaScriptPlugin::{} = \n{};'
    )


def get_editor_tools_files():
    return {
        "editor/godot.d.ts.gen.cpp": (
            "BUILTIN_DECLARATION_TEXT",
            "misc/typescript/godot.d.ts",
        ),
        "editor/tsconfig.json.gen.cpp": (
            "TSCONFIG_CONTENT",
            "misc/typescript/tsconfig.json",
        ),
        "editor/decorators.ts.gen.cpp": (
            "TS_DECORATORS_CONTENT",
            "misc/typescript/decorators.ts",
        ),
        "editor/package.json.gen.cpp": (
            "PACKAGE_JSON_CONTENT",
            "misc/typescript/package.json",
        ),
    }
