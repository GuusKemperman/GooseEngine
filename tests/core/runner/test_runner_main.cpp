
import std;
import modules;

import windows;

int main(int arg_c, char** arg_v)
{
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

    using test_t = void(&)();
    using getter_t = test_t(&)();
    getter_t unit_test_getter = reinterpret_cast<getter_t>(module_to_test.get_function_address(get_unit_test_name));
    test_t unit_test = unit_test_getter();

    try
    {
        unit_test();
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




