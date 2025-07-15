module;

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

export module windows:modules;

import modules;

namespace ge::windows::modules
{
	export class module :
		public ge::modules::platform_module
	{
	public:
		module(const std::filesystem::path& a_path);

		module(const module&) = delete;
		module(module&&) = delete;

		module& operator=(const module&) = delete;
		module& operator=(module&&) = delete;

		~module() override;

		std::vector<ge::modules::exported_func> get_exported_funcs() override;

	private:
		HMODULE m_module{};
	};

	export class loader final :
		public ge::modules::platform_loader
	{
	public:
		bool is_shared_lib(const std::filesystem::path& path) override;

		std::shared_ptr<ge::modules::platform_module> load_platform_module(const std::filesystem::path& shared_lib) override;
	};
}

ge::windows::modules::module::module(const std::filesystem::path& a_path) :
	m_module(LoadLibraryW(a_path.c_str()))
{
	if (!m_module)
	{
		throw std::runtime_error{ std::format("Failed to load DLL {}. Error: {}", a_path.string(), GetLastError()) };
	}
}

ge::windows::modules::module::~module()
{
	FreeLibrary(m_module);
}

std::vector<ge::modules::exported_func> ge::windows::modules::module::get_exported_funcs()
{
	// Get DOS header
	PIMAGE_DOS_HEADER dosHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(m_module);
	if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE)
	{
		throw std::runtime_error{ "Invalid DOS signature" };
	}

	// Get NT headers
	PIMAGE_NT_HEADERS ntHeaders = reinterpret_cast<PIMAGE_NT_HEADERS>(
		reinterpret_cast<BYTE*>(m_module) + dosHeader->e_lfanew);
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
		reinterpret_cast<BYTE*>(m_module) + exportDir.VirtualAddress);

	DWORD* names = reinterpret_cast<DWORD*>(
		reinterpret_cast<BYTE*>(m_module) + exportDirectory->AddressOfNames);
	DWORD count = exportDirectory->NumberOfNames;

	std::vector<ge::modules::exported_func> ret{};

	for (DWORD i = 0; i < count; ++i)
	{
		std::string_view name = reinterpret_cast<const char*>(
			reinterpret_cast<BYTE*>(m_module) + names[i]);

		void* address = reinterpret_cast<void*>(
			GetProcAddress(m_module, name.data())
			);

		ret.emplace_back(address, name);
	}

	return ret;
}

bool ge::windows::modules::loader::is_shared_lib(const std::filesystem::path& path)
{
	return path.extension() == std::filesystem::path{ ".dll" };
}

std::shared_ptr<ge::modules::platform_module> ge::windows::modules::loader::load_platform_module(
	const std::filesystem::path& shared_lib)
{
	return std::make_shared<module>(shared_lib);
}
