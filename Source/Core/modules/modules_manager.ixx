export module modules:modules_manager;

import :modules_base;
import :modules_platform;
import std;

namespace ge::modules
{
	export class module_manager;

	export struct module_generated_data
	{
		module_base* (*m_instance_factory)(module_manager&);
	};

	struct module_internal
	{
		std::filesystem::path m_file{};
		shared_lib_meta_data m_shared_lib_meta_data{};

		std::shared_ptr<platform_module> m_platform_module{};

		module_generated_data m_module_generated_data{};

		std::shared_ptr<module_base> m_module_instance{};
	};

	class module_manager
	{
	public:
		struct config
		{
			std::vector<std::filesystem::path> m_paths_to_check{};
		};
		module_manager(std::shared_ptr<platform_loader> a_loader, const config& a_config);

	private:
		std::shared_ptr<platform_loader> m_loader{};
		std::vector<module_internal> m_modules{};
	};
}

ge::modules::module_manager::module_manager(std::shared_ptr<platform_loader> a_loader, const config& a_config) :
	m_loader(std::move(a_loader))
{
	if (m_loader == nullptr)
	{
		throw std::invalid_argument{ "Provided null loader" };
	}

	auto add_module = 
		[&](const std::filesystem::path& path)
		{
			shared_lib_meta_data shared_lib_meta_data = m_loader->get_meta_data(path);
			std::shared_ptr module = m_loader->load_platform_module(path);

			if (module == nullptr)
			{
				return;
			}

			module_internal& internal = m_modules.emplace_back(path, shared_lib_meta_data, module);

			module_generated_data(*generated_func)(){};

			try
			{
				generated_func = reinterpret_cast<decltype(generated_func)>(
					internal.m_platform_module->get_exported_func("generated_module_func"));
			}
			catch (...)
			{
				m_modules.pop_back();
				return;
			}

			internal.m_module_generated_data = std::invoke(generated_func);

			std::println("Loaded {} from {}", internal.m_shared_lib_meta_data.m_name, internal.m_file.string());

			if (internal.m_module_generated_data.m_instance_factory != nullptr)
			{
				module_base* base = internal.m_module_generated_data.m_instance_factory(*this);
				delete base;
			}
		};

	auto check_path = 
		[&](const auto& self, const std::filesystem::path& path) -> void
		{
			if (std::filesystem::is_directory(path))
			{
				for (auto entry : std::filesystem::directory_iterator{path})
				{
					self(self, entry.path());
				}
				return;
			}

			if (m_loader->is_shared_lib(path))
			{
				add_module(path);
			}
		};

	for (const auto& path : a_config.m_paths_to_check)
	{
		check_path(check_path, path);
	}
}
