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
		API module_manager(std::shared_ptr<platform_loader> a_loader, const config& a_config);

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

	struct pending_module
	{
		std::filesystem::path m_path{};
		shared_lib_meta_data m_lib_meta_data{};
		bool m_has_loading_been_tried{};
	};
	std::vector<pending_module> pending_modules{};

	auto check_path = 
		[&](const auto& self, const std::filesystem::path& a_path) -> void
		{
			if (std::filesystem::is_directory(a_path))
			{
				for (auto entry : std::filesystem::directory_iterator{a_path})
				{
					self(self, entry.path());
				}
				return;
			}

			if (m_loader->is_shared_lib(a_path))
			{
				try
				{
					pending_modules.emplace_back(a_path, m_loader->get_meta_data(a_path));
				}
				catch (const std::runtime_error& e)
				{
					std::println(std::cerr, "Failed to register module from {} - {}",
						a_path.string(),
						e.what());
				}
			}
		};

	for (const auto& path : a_config.m_paths_to_check)
	{
		check_path(check_path, path);
	}

	auto load_module =
		[&](const auto& self, pending_module& a_pending_module) -> void
		{
			if (a_pending_module.m_has_loading_been_tried)
			{
				return;
			}

			for (const auto& dependency : a_pending_module.m_lib_meta_data.m_dependencies)
			{
				for (pending_module& other : pending_modules)
				{
					if (&other == &a_pending_module)
					{
						continue;
					}

					if (other.m_lib_meta_data.m_name == dependency)
					{
						self(self, other);
					}
				}
			}

			std::shared_ptr module = m_loader->load_platform_module(a_pending_module.m_path);
			a_pending_module.m_has_loading_been_tried = true;

			if (module == nullptr)
			{
				throw std::runtime_error{ "module was nullptr" };
			}

			module_internal& internal = m_modules.emplace_back(std::move(a_pending_module.m_path), 
				std::move(a_pending_module.m_lib_meta_data), 
				module);

			module_generated_data(*generated_func)() {};

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

			std::println("Loaded {} from {}", 
				internal.m_shared_lib_meta_data.m_name, 
				internal.m_file.string());

			if (internal.m_module_generated_data.m_instance_factory != nullptr)
			{
				module_base* base = internal.m_module_generated_data.m_instance_factory(*this);
				delete base;
			}
		};

	for (pending_module& pending : pending_modules)
	{
		load_module(load_module, pending);
	}
}
