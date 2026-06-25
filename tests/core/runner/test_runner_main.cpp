#include <crtdbg.h>
#include <stdlib.h>

import stl;
import modules;

import windows;
import test_core;
import runtime_reflection;

int main()
{
	for (auto entry : std::filesystem::directory_iterator{ "../" })
	{
		std::cout << entry.path().string() << std::endl;
	}


    ge::modules::module_manager::config config{ { "bin" } };
    ge::modules::module_manager manager{ std::make_shared<ge::windows::modules::loader>(), config };

	manager.load_all();

	std::unique_ptr<ge::refl::registry_data> reg = [&manager]
		{
			ge::refl::builders::endable_registry_builder reg_builder = ge::refl::builders::begin_registry();

			for (ge::modules::module_handle module : manager.get_modules())
			{
				using build_func_t = void(*)(ge::refl::builders::registry_builder&);
				build_func_t build_func = reinterpret_cast<build_func_t>(module.get_function_address("build_runtime_reflection"));

				if (build_func == nullptr)
				{
					continue;
				}

				build_func(reg_builder);
			}

			return std::move(reg_builder).build();
		}();

	ge::refl::func_query::with<ge::test_core::unit_test_trait > unit_tests{ reg->m_funcs };

	for (const ge::refl::func_data& func : unit_tests)
	{
		ge::refl::invoke(func);
	}
	



}

