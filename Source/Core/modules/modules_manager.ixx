export module modules:modules_manager;

import :modules_base;
import :modules_platform;
import std;

namespace ge::modules
{
	export class module_manager;

	export struct module_meta_data
	{
		std::string_view m_name{};
		std::span<const std::string_view> m_dependencies{};
		module_base* (*m_instance_factory)(module_manager&);
	};

	struct module_internal
	{
		std::filesystem::path m_file{};
		std::shared_ptr<platform_module> m_platform_module{};
		std::vector<exported_func> m_exported_funcs{};

		module_meta_data m_meta_data{};

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

	static bool is_generated_module_func(const exported_func& func);
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
			std::shared_ptr module = m_loader->load_platform_module(path);

			if (module == nullptr)
			{
				return;
			}

			module_internal& internal = m_modules.emplace_back(path, module, module->get_exported_funcs());

			auto generated_func = std::ranges::find_if(internal.m_exported_funcs, 
				&is_generated_module_func);

			if (generated_func == internal.m_exported_funcs.end()
				|| generated_func->m_address == nullptr)
			{
				m_modules.pop_back();
				return;
			}

			auto func_address = reinterpret_cast<module_meta_data(*)()>(generated_func->m_address);
			internal.m_meta_data = std::invoke(func_address);
			std::println("Loaded {} from {}", internal.m_meta_data.m_name, internal.m_file.string());
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

bool ge::modules::is_generated_module_func(const exported_func& func)
{
	return func.m_name == std::string_view{ "generated_module_func" };
}