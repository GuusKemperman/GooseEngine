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

		void* get_exported_func(std::string_view func_name) override;

	private:
		HMODULE m_module{};
	};

	export class loader final :
		public ge::modules::platform_loader
	{
	public:
		bool is_shared_lib(const std::filesystem::path& path) override;

		ge::modules::shared_lib_meta_data get_meta_data(const std::filesystem::path& path) override;

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

void* ge::windows::modules::module::get_exported_func(std::string_view func_name)
{
	void* address = GetProcAddress(m_module, func_name.data());

	if (address == nullptr)
	{
		throw std::runtime_error{ std::format("Could not find function {} address - {}",
			func_name,
			GetLastError()) };
	}

	return address;
}

bool ge::windows::modules::loader::is_shared_lib(const std::filesystem::path& path)
{
	return path.extension() == std::filesystem::path{ ".dll" };
}

ge::modules::shared_lib_meta_data ge::windows::modules::loader::get_meta_data(const std::filesystem::path& path)
{
	const std::string dll_content = [&]()
		{
			std::ifstream file_stream{ path, std::ifstream::binary };
			std::string ret{};
			char ch;
			while (file_stream.get(ch))
			{
				ret.push_back(ch);
			}
			return ret;
		}();

	DWORD pe_pos = *reinterpret_cast<const DWORD*>(&dll_content.at(0x3c));

	DWORD coff_pos = pe_pos + 4;

	_IMAGE_FILE_HEADER file_header = *reinterpret_cast<const _IMAGE_FILE_HEADER*>(&dll_content.at(coff_pos));

	DWORD opt_header_pos = coff_pos + sizeof(_IMAGE_FILE_HEADER);
	WORD magic = *reinterpret_cast<const WORD*>(&dll_content.at(opt_header_pos));

	std::variant<IMAGE_OPTIONAL_HEADER32, IMAGE_OPTIONAL_HEADER64> opt_header{};

	switch (magic)
	{
	case 0x10b:
	{
		opt_header = *reinterpret_cast<const IMAGE_OPTIONAL_HEADER32*>(&dll_content.at(opt_header_pos));
		break;
	}
	case 0x20b:
	{
		opt_header = *reinterpret_cast<const IMAGE_OPTIONAL_HEADER64*>(&dll_content.at(opt_header_pos));
		break;
	}
	default: throw std::runtime_error{ "Invalid optional header" };
	}

	const IMAGE_SECTION_HEADER* section_header = reinterpret_cast<const IMAGE_SECTION_HEADER*>(
		&dll_content.at(opt_header_pos + file_header.SizeOfOptionalHeader));

	IMAGE_DATA_DIRECTORY export_dir;
	IMAGE_DATA_DIRECTORY import_dir;

	std::visit(
		[&](const auto& header)
		{
			export_dir = header.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
			import_dir = header.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
		}, opt_header);


	auto rva_to_pos = [&](DWORD rva)
		{
			for (WORD i = 0; i < file_header.NumberOfSections; i++)
			{
				const IMAGE_SECTION_HEADER& section = section_header[i];
				DWORD start = section.VirtualAddress;
				DWORD end = start + max(section.Misc.VirtualSize, section.SizeOfRawData);

				if (rva >= start && rva < end)
				{
					DWORD offset_in_section = rva - start;

					if (offset_in_section < section.SizeOfRawData)
					{
						return section.PointerToRawData + offset_in_section;
					}
					break; // RVA found but not in file data (zero-padded region)
				}
			}
			return (DWORD)-1; // Invalid RVA
		};

	auto data_dir_to_strings =
		[&](const IMAGE_DATA_DIRECTORY& dir)
		{
			if (dir.VirtualAddress == 0)
			{
				throw std::runtime_error{ "No export directory found" };
			}

			IMAGE_EXPORT_DIRECTORY exportDirectory = *reinterpret_cast<const IMAGE_EXPORT_DIRECTORY*>(
				&dll_content.at(rva_to_pos(dir.VirtualAddress)));

			const DWORD* name_addresses = reinterpret_cast<const DWORD*>(
				&dll_content.at(rva_to_pos(exportDirectory.AddressOfNames)));
			DWORD count = exportDirectory.NumberOfNames;

			std::vector<std::string> ret{};

			for (DWORD i = 0; i < count; ++i)
			{
				std::string_view name = &dll_content.at(rva_to_pos(name_addresses[i]));

				ret.emplace_back(name);
			}

			return ret;
		};

	return { data_dir_to_strings(export_dir) };
}

std::shared_ptr<ge::modules::platform_module> ge::windows::modules::loader::load_platform_module(
	const std::filesystem::path& shared_lib)
{
	return std::make_shared<module>(shared_lib);
}
