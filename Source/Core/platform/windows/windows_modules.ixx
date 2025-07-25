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
		API module(const std::filesystem::path& a_path);

		module(const module&) = delete;
		module(module&&) = delete;

		module& operator=(const module&) = delete;
		module& operator=(module&&) = delete;

		API ~module() override;

		API void* get_exported_func(std::string_view func_name) override;

	private:
		HMODULE m_module{};
	};

	export class loader final :
		public ge::modules::platform_loader
	{
	public:
		API std::filesystem::path get_platform_shared_lib_file_extension() const override;

		API ge::modules::shared_lib_meta_data get_meta_data(const std::filesystem::path& path) const override;

		API std::shared_ptr<ge::modules::platform_module> load_platform_module(const std::filesystem::path& shared_lib) const override;
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

std::filesystem::path ge::windows::modules::loader::get_platform_shared_lib_file_extension() const
{
	return { ".dll" };
}

ge::modules::shared_lib_meta_data ge::windows::modules::loader::get_meta_data(const std::filesystem::path& path) const
{
	const std::string dll_content = 
		[&]
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

	const auto get_as = 
		[&dll_content]<typename T>(DWORD file_pos) -> const T&
		{
			if (dll_content.size() <= file_pos
				|| dll_content.size() <= file_pos + sizeof(T))
			{
				throw std::out_of_range{ "invalid file position" };
			}
			return *reinterpret_cast<const T*>(dll_content.data() + file_pos);
		};

	const DWORD pe_pos = get_as.operator()<DWORD>(0x3c);
	const DWORD coff_pos = pe_pos + 4;
	const DWORD opt_header_pos = coff_pos + sizeof(_IMAGE_FILE_HEADER);

	const _IMAGE_FILE_HEADER& file_header = get_as.operator()<_IMAGE_FILE_HEADER>(coff_pos);

	const auto opt_header = 
		[&]() -> std::variant<std::reference_wrapper<const IMAGE_OPTIONAL_HEADER32>,
			std::reference_wrapper<const IMAGE_OPTIONAL_HEADER64>>
		{
			const WORD magic = get_as.operator()<WORD>(opt_header_pos);

			switch (magic)
			{
			case 0x10b:
			{
				return { std::cref(get_as.operator()<IMAGE_OPTIONAL_HEADER32>(opt_header_pos)) };
			}
			case 0x20b:
			{
				return { std::cref(get_as.operator()<IMAGE_OPTIONAL_HEADER64>(opt_header_pos)) };
			}
			default: throw std::runtime_error{ "Invalid optional header" };
			}
		}();

	IMAGE_DATA_DIRECTORY export_dir;
	IMAGE_DATA_DIRECTORY import_dir;

	std::visit(
		[&](const auto& header)
		{
			export_dir = header.get().DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
			import_dir = header.get().DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
		}, opt_header);

	const auto rva_to_pos = 
		[&](DWORD rva)
		{
			const IMAGE_SECTION_HEADER* section_header = 
				&get_as.operator()<IMAGE_SECTION_HEADER>(opt_header_pos + file_header.SizeOfOptionalHeader);

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

	std::vector<std::string> dependencies = 
		[&]
		{
			const _IMAGE_IMPORT_DESCRIPTOR* import_table = reinterpret_cast<const _IMAGE_IMPORT_DESCRIPTOR*>(
				&dll_content.at(rva_to_pos(import_dir.VirtualAddress)));

			std::vector<std::string> ret{};

			while (true)
			{
				if (import_table->Characteristics == 0)
				{
					break;
				}

				std::filesystem::path name{ &dll_content.at(rva_to_pos(import_table->Name)) };
				name.replace_extension();
				ret.emplace_back(name.string());
				import_table++;
			}

			return ret;
		}();

	std::vector<std::string> exported_names = 
		[&]
		{
			if (export_dir.VirtualAddress == 0)
			{
				throw std::runtime_error{ "No export directory found" };
			}

			IMAGE_EXPORT_DIRECTORY export_table = *reinterpret_cast<const IMAGE_EXPORT_DIRECTORY*>(
				&dll_content.at(rva_to_pos(export_dir.VirtualAddress)));

			const DWORD* name_addresses = reinterpret_cast<const DWORD*>(
				&dll_content.at(rva_to_pos(export_table.AddressOfNames)));
			DWORD count = export_table.NumberOfNames;

			std::vector<std::string> ret{};

			for (DWORD i = 0; i < count; ++i)
			{
				std::string_view name = &dll_content.at(rva_to_pos(name_addresses[i]));
				ret.emplace_back(name);
			}

			return ret;
		}();

	return { std::move(exported_names), std::move(dependencies) };
}

std::shared_ptr<ge::modules::platform_module> ge::windows::modules::loader::load_platform_module(
	const std::filesystem::path& shared_lib) const
{
	return std::make_shared<module>(shared_lib);
}
