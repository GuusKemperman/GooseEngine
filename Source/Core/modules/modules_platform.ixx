export module modules:modules_platform;

export import std;

namespace ge::modules
{
	export class platform_module
	{
	public:
		API virtual ~platform_module() = default;

		API virtual void* get_exported_func(std::string_view func_name) = 0;
	};

	export struct shared_lib_meta_data
	{
		std::string m_name{};
		std::vector<std::string> m_exported_function_names{};
		std::vector<std::string> m_dependencies{};
	};

	export class platform_loader
	{
	public:
		API virtual ~platform_loader() = default;

		API virtual bool is_shared_lib(const std::filesystem::path& path) = 0;

		API virtual shared_lib_meta_data get_meta_data(const std::filesystem::path& path) = 0;

		API virtual std::shared_ptr<platform_module> load_platform_module(const std::filesystem::path& shared_lib) = 0;
	};
}
