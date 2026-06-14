export module modules:modules_platform;

export import stl;

namespace ge::modules
{
	export class platform_module
	{
	public:
		API virtual ~platform_module() = default;

		API virtual auto get_exported_func(std::string_view func_name) -> void(*)() = 0;
	};

	export struct shared_lib_meta_data
	{
		std::vector<std::string> m_exported_function_names{};
		std::vector<std::string> m_dependencies{};
	};

	export class platform_loader
	{
	public:
		API virtual ~platform_loader() = default;

		API virtual std::filesystem::path get_platform_shared_lib_file_extension() const = 0;

		API virtual std::string get_module_name(const std::filesystem::path& path) const;

		API virtual bool is_shared_lib(const std::filesystem::path& path) const;

		API virtual shared_lib_meta_data get_meta_data(const std::filesystem::path& path) const = 0;

		API virtual std::shared_ptr<platform_module> load_platform_module(const std::filesystem::path& shared_lib) const = 0;
	};
}

std::string ge::modules::platform_loader::get_module_name(const std::filesystem::path& path) const
{
	return path.filename().replace_extension().string();
}

bool ge::modules::platform_loader::is_shared_lib(const std::filesystem::path& path) const
{
	return path.extension() == get_platform_shared_lib_file_extension();
}

