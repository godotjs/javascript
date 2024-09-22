def get_bindings_header():
    return (
        '/* THIS FILE IS GENERATED DO NOT EDIT */\n#include "../javascript_binder.h"\nString JavaScriptBinder::{} = \n{};'
    )


def get_binding_files():
    return {
        "misc/godot.binding_script.gen.cpp": (
            "BINDING_SCRIPT_CONTENT",
            "misc/binding_script.js",
        ),
    }
