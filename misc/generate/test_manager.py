def get_test_manager_header():
    return (
        '/* THIS FILE IS GENERATED DO NOT EDIT */\n#include "test_manager.h"\nString TestManager::{} = \n{};'
    )


def get_test_files():
    return {
        "src/tests/godot.unit_test.gen.cpp": (
            "UNIT_TEST",
            "src/tests/UnitTest.js",
        ),
    }
