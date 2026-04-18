export module modules:modules_manager;

import :modules_base;
import :modules_platform;
import stl;

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

	export class module_handle
	{
	public:
		module_handle(const module_internal& a_module_internal);

		API const std::filesystem::path& get_file() const;

		API std::string_view get_name() const;

		API auto get_function_address(std::string_view func_name) const -> void(&)();

		API auto get_function_names() const;

		API auto get_dependencies_names() const;

		API const std::shared_ptr<module_base>& get_instance() const;

	private:
		std::reference_wrapper<const module_internal> m_module;
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

		API module_handle load_with_dependencies(std::string_view module_name);

		API module_handle get_handle(std::string_view module_name) const;

		template<typename module_t>
		module_t& get_instance() const;

	private:
		config m_config{};
		std::shared_ptr<platform_loader> m_loader{};
		std::list<module_internal> m_modules{};
	};
}

ge::modules::module_handle::module_handle(const module_internal& a_module_internal) :
	m_module(a_module_internal)
{
}

const std::filesystem::path& ge::modules::module_handle::get_file() const
{
	return m_module.get().m_file;
}

std::string_view ge::modules::module_handle::get_name() const
{
	return m_module.get().m_name;
}

auto ge::modules::module_handle::get_function_address(std::string_view func_name) const -> void(&)()
{
	return m_module.get().m_data_avail_when_loaded->m_platform_module->get_exported_func(func_name);
}

auto ge::modules::module_handle::get_function_names() const
{
	const std::vector<std::string>& names = m_module.get().m_data_avail_when_loaded->m_shared_lib_meta_data.m_exported_function_names;
	return names | std::ranges::views::transform([](const std::string& str) { return std::string_view{ str }; });
}

auto ge::modules::module_handle::get_dependencies_names() const
{
	const std::vector<std::string>& names = m_module.get().m_data_avail_when_loaded->m_shared_lib_meta_data.m_dependencies;
	return names | std::ranges::views::transform([](const std::string& str) { return std::string_view{ str }; });
}

const std::shared_ptr<ge::modules::module_base>& ge::modules::module_handle::get_instance() const
{
	return m_module.get().m_data_avail_when_loaded->m_module_instance;
}

//template <std::derived_from<ge::modules::module_base> module_t>
//std::shared_ptr<module_t> ge::modules::module_handle::get_instance() const
//{
//	return std::dynamic_pointer_cast<module_t>(get_instance());
//}

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

ge::modules::module_handle ge::modules::module_manager::load_with_dependencies(std::string_view module_name)
{
	auto existing_module = std::ranges::find_if(m_modules,
		[&](const module_internal& mod)
		{
			return mod.m_name == module_name;
		});

	if (existing_module == m_modules.end())
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
		existing_module = --m_modules.end();
	}

	if (existing_module->m_data_avail_when_loaded.has_value())
	{
		return { *existing_module };
	}

	shared_lib_meta_data lib_meta_data = m_loader->get_meta_data(existing_module->m_file);

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
		std::shared_ptr module = m_loader->load_platform_module(existing_module->m_file);

		if (module == nullptr)
		{
			throw std::runtime_error{ "module was nullptr" };
		}

		existing_module->m_data_avail_when_loaded.emplace(
			std::move(lib_meta_data),
			std::move(module));
	}

	
	module_generated_data(&generated_func)() =
		[&]() -> decltype(auto)
		{
			void(&func)() = existing_module->m_data_avail_when_loaded->m_platform_module->get_exported_func("generated_module_func");
			return reinterpret_cast<decltype(generated_func)>(func);
		}();
;
	existing_module->m_data_avail_when_loaded->m_module_generated_data = 
		std::invoke(generated_func);

	std::println("Loaded {} from {}",
		existing_module->m_name,
		existing_module->m_file.string());

	if (auto* instance_factory = existing_module->m_data_avail_when_loaded->
		m_module_generated_data.m_instance_factory)
	{
		module_base* base = instance_factory(*this);
		existing_module->m_data_avail_when_loaded->m_module_instance = std::shared_ptr<module_base>{ base };
	}

	return { *existing_module };
}

ge::modules::module_handle ge::modules::module_manager::get_handle(std::string_view module_name) const
{
	auto it = std::ranges::find_if(m_modules,
		[&](const module_internal& mod)
		{
			return mod.m_name == module_name;
		});

	if (it == m_modules.end())
	{
		throw std::out_of_range{ std::format("no module called {}", module_name) };
	}
	return { *it };
}

template <typename module_t>
module_t& ge::modules::module_manager::get_instance() const
{
	// Forces linker to consider this module a dependency,
	// which informs US that the module instance needs
	// to be constructed before the module from which
	// function is called.
	(void)module_t::get_derived_type_info();

	for (const module_internal& internal : m_modules)
	{
		module_t* instance = dynamic_cast<module_t*>(
			internal.m_data_avail_when_loaded->m_module_instance.get());

		if (instance != nullptr)
		{
			return *instance;
		}
	}

	throw std::out_of_range{ "could not find module instance" };
}