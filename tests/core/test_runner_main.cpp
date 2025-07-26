
import std;
import modules;

import windows;

int main(int arg_c, char** arg_v)
{
    if (arg_c < 3)
    {
        std::println(std::cerr, "Provided only {} arguments", arg_c);
        return -1;
    }

    ge::modules::module_manager::config config{ { "bin" } };
    ge::modules::module_manager manager{ std::make_shared<ge::windows::modules::loader>(), config };

    ge::modules::module_handle module_to_test = manager.load_with_dependencies(arg_v[1]);
    void(&unit_test)() = module_to_test.get_function_address(arg_v[2]);
    return reinterpret_cast<int(&)()>(unit_test)();
}

