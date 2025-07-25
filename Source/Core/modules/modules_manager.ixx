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

		std::string m_name{};

		struct loaded_data
		{
			shared_lib_meta_data m_shared_lib_meta_data{};

			std::shared_ptr<platform_module> m_platform_module{};

			module_generated_data m_module_generated_data{};

			std::shared_ptr<module_base> m_module_instance{};
		};
		std::optional<loaded_data> m_data_avail_when_loaded{};
	};

	class module_manager
	{
	public:
		struct config
		{
			std::vector<std::filesystem::path> m_binary_directories{};
		};
		API module_manager(std::shared_ptr<platform_loader> a_loader, 
			config a_config);

		API void load_all();

		API void load_with_dependencies(std::string_view module_name);

	private:
		config m_config{};
		std::shared_ptr<platform_loader> m_loader{};
		std::vector<module_internal> m_modules{};
	};
}

ge::modules::module_manager::module_manager(std::shared_ptr<platform_loader> a_loader, 
	config a_config) :
	m_config(std::move(a_config)),
	m_loader(std::move(a_loader))
{
	if (m_loader == nullptr)
	{
		throw std::invalid_argument{ "Provided null loader" };
	}
}

void ge::modules::module_manager::load_all()
{
	auto check_path =
		[&](const auto& self, const std::filesystem::path& a_path) -> void
		{
			if (std::filesystem::is_directory(a_path))
			{
				for (auto entry : std::filesystem::directory_iterator{ a_path })
				{
					self(self, entry.path());
				}
				return;
			}

			if (m_loader->is_shared_lib(a_path))
			{
				try
				{
					load_with_dependencies(m_loader->get_module_name(a_path));
				}
				catch (const std::runtime_error& e)
				{
					std::println(std::cerr, "Failed to register module from {} - {}",
						a_path.string(),
						e.what());
				}
			}
		};

	for (const auto& path : m_config.m_binary_directories)
	{
		check_path(check_path, path);
	}
}

void ge::modules::module_manager::load_with_dependencies(std::string_view module_name)
{
	size_t index_of_existing_module = 
		[&]
		{
			auto existing_module = std::ranges::find_if(m_modules,
				[&](const module_internal& mod)
				{
					return mod.m_name == module_name;
				});

			return std::distance(m_modules.begin(), existing_module);
		}();

	if (index_of_existing_module == m_modules.size())
	{
		const std::filesystem::path module_path =
			[&]() -> std::filesystem::path
			{
				std::filesystem::path module_file_name{ module_name };
				module_file_name.replace_extension(m_loader->get_platform_shared_lib_file_extension());

				for (const std::filesystem::path& binary_dir : m_config.m_binary_directories)
				{
					std::filesystem::path path = binary_dir / module_file_name;
					if (std::filesystem::exists(path)
						&& m_loader->is_shared_lib(path))
					{
						return path;
					}
				}
				throw std::runtime_error{ std::format("Could not load module {}, file not found", module_file_name.string()) };
			}();

		m_modules.emplace_back(module_path, std::string{ module_name });
	}

	auto existing_module = 
		[&]() -> decltype(auto)
		{
			return m_modules[index_of_existing_module];
		};

	if (existing_module().m_data_avail_when_loaded.has_value())
	{
		return;
	}

	shared_lib_meta_data lib_meta_data = m_loader->get_meta_data(existing_module().m_file);

	for (const std::string_view dependency :
		lib_meta_data.m_dependencies)
	{
		try
		{
			load_with_dependencies(dependency);
		}
		catch (...)
		{
			// TODO verbose logging here
		}
	}

	{
		std::shared_ptr module = m_loader->load_platform_module(existing_module().m_file);

		if (module == nullptr)
		{
			throw std::runtime_error{ "module was nullptr" };
		}

		existing_module().m_data_avail_when_loaded.emplace(
			std::move(lib_meta_data),
			std::move(module));
	}

	module_generated_data(*generated_func)() = reinterpret_cast<decltype(generated_func)>(
		existing_module().m_data_avail_when_loaded->
		m_platform_module->get_exported_func("generated_module_func"));

	existing_module().m_data_avail_when_loaded->m_module_generated_data = 
		std::invoke(generated_func);

	std::println("Loaded {} from {}",
		existing_module().m_name,
		existing_module().m_file.string());

	if (auto* instance_factory = existing_module().m_data_avail_when_loaded->
		m_module_generated_data.m_instance_factory)
	{
		module_base* base = instance_factory(*this);
		delete base;
	}
}
