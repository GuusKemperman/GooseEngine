export module modules:modules_platform;

export import std;

namespace ge::modules
{
	class module_base;
	class module_manager;

	export class platform_module
	{
	public:
		virtual ~platform_module() = default;

		virtual void* get_exported_func(std::string_view func_name) = 0;
	};

	export struct shared_lib_meta_data
	{
		std::vector<std::string> m_exported_function_names{};
		std::vector<std::string> m_dependencies{};
	};

	export class platform_loader
	{
	public:
		virtual ~platform_loader() = default;

		virtual bool is_shared_lib(const std::filesystem::path& path) = 0;

		virtual shared_lib_meta_data get_meta_data(const std::filesystem::path& path) = 0;

		virtual std::shared_ptr<platform_module> load_platform_module(const std::filesystem::path& shared_lib) = 0;
	};
}
