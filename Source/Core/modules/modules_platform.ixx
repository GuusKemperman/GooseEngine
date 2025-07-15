export module modules:modules_platform;

export import std;

namespace ge::modules
{
	class module_base;
	class module_manager;

	export struct exported_func
	{
		void* m_address;
		std::string_view m_name{};
	};

	export class platform_module
	{
	public:
		virtual ~platform_module() = default;

		virtual std::vector<exported_func> get_exported_funcs() = 0;
	};

	export class platform_loader
	{
	public:
		virtual ~platform_loader() = default;

		virtual bool is_shared_lib(const std::filesystem::path& path) = 0;

		virtual std::shared_ptr<platform_module> load_platform_module(const std::filesystem::path& shared_lib) = 0;
	};
}
