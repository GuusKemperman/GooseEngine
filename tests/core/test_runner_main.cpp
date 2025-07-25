
import std;
import modules;

import windows;

int main(int arg_c, char** arg_v)
{
    if (arg_c < 2)
    {
        std::println(std::cerr, "Provided only {} arguments", arg_c);
        return -1;
    }

    ge::modules::module_manager::config config{ { "bin" } };
    ge::modules::module_manager manager{ std::make_shared<ge::windows::modules::loader>(), config };
    manager.load_with_dependencies(arg_v[1]);
    return 0;
}

