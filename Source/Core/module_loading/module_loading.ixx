module;

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

export module module_loading;

export import std;

namespace ge::module_loading
{
	export class module_platform_handle;

	export bool is_shared_lib(const std::filesystem::path& path);

	export std::shared_ptr<module_platform_handle> load_platform_module(const std::filesystem::path& shared_lib);

	export struct exported_func
	{
		void* m_address;
		std::string_view m_name{};
	};
	export std::vector<exported_func> get_exported_functions(const module_platform_handle& handle);
}

module :private;

class ge::module_loading::module_platform_handle
{
public:
	module_platform_handle(const std::filesystem::path& path);

	module_platform_handle(const module_platform_handle&) = delete;
	module_platform_handle(module_platform_handle&&) = delete;

	module_platform_handle& operator=(const module_platform_handle&) = delete;
	module_platform_handle& operator=(module_platform_handle&&) = delete;

	~module_platform_handle();

	HMODULE m_module{};
	std::filesystem::path m_path{};
};

bool ge::module_loading::is_shared_lib(const std::filesystem::path& path)
{
	return path.extension() == std::filesystem::path{ ".dll" };
}

std::shared_ptr<ge::module_loading::module_platform_handle>
	ge::module_loading::load_platform_module(const std::filesystem::path& shared_lib)
{
	if (!is_shared_lib(shared_lib))
	{
		return nullptr;
	}

	std::shared_ptr ptr = std::make_shared<module_platform_handle>(shared_lib);

	if (!ptr->m_module)
	{
		throw std::runtime_error{ std::format("Failed to load DLL {}. Error: {}", shared_lib.string(), GetLastError())};
	}

	return ptr;
}

std::vector<ge::module_loading::exported_func> ge::module_loading::get_exported_functions(
	const module_platform_handle& handle)
{
	// Get DOS header
	PIMAGE_DOS_HEADER dosHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(handle.m_module);
	if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE)
	{
		throw std::runtime_error{ "Invalid DOS signature" };
	}

	// Get NT headers
	PIMAGE_NT_HEADERS ntHeaders = reinterpret_cast<PIMAGE_NT_HEADERS>(
		reinterpret_cast<BYTE*>(handle.m_module) + dosHeader->e_lfanew);
	if (ntHeaders->Signature != IMAGE_NT_SIGNATURE)
	{
		throw std::runtime_error{ "Invalid NT signature" };
	}

	// Get export directory
	IMAGE_DATA_DIRECTORY exportDir = ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
	if (exportDir.VirtualAddress == 0)
	{
		throw std::runtime_error{ "No export directory found" };
	}

	PIMAGE_EXPORT_DIRECTORY exportDirectory = reinterpret_cast<PIMAGE_EXPORT_DIRECTORY>(
		reinterpret_cast<BYTE*>(handle.m_module) + exportDir.VirtualAddress);

	DWORD* names = reinterpret_cast<DWORD*>(
		reinterpret_cast<BYTE*>(handle.m_module) + exportDirectory->AddressOfNames);
	DWORD count = exportDirectory->NumberOfNames;

	std::vector<exported_func> ret{};

	for (DWORD i = 0; i < count; ++i)
	{
		std::string_view name = reinterpret_cast<const char*>(
			reinterpret_cast<BYTE*>(handle.m_module) + names[i]);
		
		void* address = reinterpret_cast<void*>(
			GetProcAddress(handle.m_module, name.data())
			);

		if (address == nullptr)
		{
			std::println(std::cerr, "Failed to GetProcAddress for {} from {} - Error: {}",
				name,
				handle.m_path.string(),
				GetLastError()
			);
			continue;
		}

		ret.emplace_back(address, name);
	}

	return ret;
}

ge::module_loading::module_platform_handle::module_platform_handle(const std::filesystem::path& path) :
	m_module(LoadLibraryW(path.c_str())),
	m_path(path)
{
}

ge::module_loading::module_platform_handle::~module_platform_handle()
{
	if (!m_module)
	{
		return;
	}

	if (!FreeLibrary(m_module))
	{
		std::println(std::cerr, "Failed to unload DLL {}. Error: {}",
			m_path.string(),
			GetLastError()
		);
	}
}
