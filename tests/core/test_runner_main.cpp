
import std;
import modules;

import windows;

int main()
{
    ge::modules::module_manager::config config{ { "bin" } };
    ge::modules::module_manager manager{ std::make_shared<ge::windows::modules::loader>(), config };

    // 

    return 0;
}

