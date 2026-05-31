#include <crtdbg.h>
#include <stdlib.h>

import stl;
import modules;

import windows;
import test_core;

struct test
{
    int foo{ };
};

int test::* pr = &test::foo;
static_assert(std::is_same_v<int test::*, decltype(&test::foo)>);

void tester()
{
    test instance{};
    instance.*pr = 10;
}

int main(int arg_c, char** arg_v)
{
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE | _CRTDBG_MODE_DEBUG);
    _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);

    // 2) (Optional) Suppress the “abort() has been called” dialog as well
    _set_abort_behavior(0, _WRITE_ABORT_MSG);    _set_error_mode(_OUT_TO_STDERR);

    if (arg_c < 4)
    {
        std::println(std::cerr, "Provided only {} arguments", arg_c);
        return -1;
    }

    const std::string_view module_name = arg_v[1];
    const std::string_view category_name = arg_v[2];
    const std::string_view test_name = arg_v[3];

    ge::modules::module_manager::config config{ { "bin" } };
    ge::modules::module_manager manager{ std::make_shared<ge::windows::modules::loader>(), config };

    ge::modules::module_handle module_to_test = manager.load_with_dependencies(module_name);

    std::string get_unit_test_name = std::format("get_unit_test_{}_{}", category_name, test_name);

    using test_t = void(&)(ge::test_core::context&);
    using getter_t = test_t(&)();
    getter_t unit_test_getter = reinterpret_cast<getter_t>(module_to_test.get_function_address(get_unit_test_name));
    test_t unit_test = unit_test_getter();

    ge::test_core::context context{  };

    try
    {
        unit_test(context);
        return 0;
    }
    catch (const std::exception& e)
    {
        std::println(std::cerr, "Test failed - {}", e.what());
        return 1;
    }
    catch (...)
    {
        std::println(std::cerr, "Unknown exception");
        return 1;
    }
}




