
#ifndef TEST_JAVASCRIPT_H
#define TEST_JAVASCRIPT_H

#include "../javascript_language.h"
#include "../src/tests/test_manager.h"

#include "tests/test_macros.h"

namespace JavaScriptTests {

TEST_CASE("[JavaScript] Test all") {
	JavaScriptLanguage::get_singleton()->init();
	String code = TestManager::UNIT_TEST;
	Error err = JavaScriptLanguage::get_singleton()->execute_file(code);
	CHECK(err == OK);
}

} // namespace JavaScriptTests

#endif // TEST_JAVASCRIPT_H
