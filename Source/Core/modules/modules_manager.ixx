module;
#include <assert.h>

export module modules:modules_manager;

import :modules_platform;
import stl;

namespace ge::refl::builders
{
	class registry_builder;
}

namespace ge::modules
{
	export class module_manager;

	struct module_internal
	{
		std::filesystem::path m_file{};

		std::string m_name{};

		struct loaded_data
		{
			shared_lib_meta_data m_shared_lib_meta_data{};

			std::shared_ptr<platform_module> m_platform_module{};
		};
		std::optional<loaded_data> m_data_avail_when_loaded{};
	};

	export class module_handle
	{
	public:
		module_handle(const module_internal& a_module_internal);

		API const std::filesystem::path& get_file() const;

		API std::string_view get_name() const;

		API auto get_function_address(std::string_view func_name) const -> void(*)();

		API auto get_dependencies_names() const;

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

		API auto get_modules()
		{
			return m_modules | std::ranges::views::transform([](const module_internal& module_internal)
			{
					return module_handle{ module_internal };
			});
		}

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

auto ge::modules::module_handle::get_function_address(std::string_view func_name) const -> void(*)()
{
	return m_module.get().m_data_avail_when_loaded->m_platform_module->get_exported_func(func_name);
}

auto ge::modules::module_handle::get_dependencies_names() const
{
	const std::vector<std::string>& names = m_module.get().m_data_avail_when_loaded->m_shared_lib_meta_data.m_dependencies;
	return names | std::ranges::views::transform([](const std::string& str) { return std::string_view{ str }; });
}

ge::modules::module_manager::module_manager(std::shared_ptr<platform_loader> a_loader, 
                                            config a_config) :
	m_config(std::move(a_config)),
	m_loader(std::move(a_loader))
{
	assert(m_loader != nullptr);
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
				load_with_dependencies(m_loader->get_module_name(a_path));
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

				assert(false);
				std::unreachable();
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
		load_with_dependencies(dependency);
	}

	{
		std::shared_ptr module = m_loader->load_platform_module(existing_module->m_file);
		assert(module != nullptr);
		existing_module->m_data_avail_when_loaded.emplace(
			std::move(lib_meta_data),
			std::move(module));
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

	assert(it != m_modules.end());
	return { *it };
}

