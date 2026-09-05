module;

#define WIN32_LEAN_AND_MEAN
#include <cassert>
#include <windows.h>

export module windows:modules;

import stl;
import modules;

namespace ge::windows::modules
{
	export class module : public ge::modules::platform_module
	{
	public:
		API module( const std::filesystem::path& a_path );

		module( const module& ) = delete;
		module( module&& ) = delete;

		module& operator=( const module& ) = delete;
		module& operator=( module&& ) = delete;

		API ~module() override;

		API auto get_exported_func( std::string_view func_name ) const -> void ( * )() override;

	private:
		HMODULE m_module{};
	};

	export class loader final : public ge::modules::platform_loader
	{
	public:
		API std::filesystem::path get_platform_shared_lib_file_extension() const override;

		API std::shared_ptr< ge::modules::platform_module > load_platform_module(
			const std::filesystem::path& shared_lib ) const override;
	};
} // namespace ge::windows::modules

ge::windows::modules::module::module( const std::filesystem::path& a_path )
	: m_module( LoadLibraryW( a_path.c_str() ) )
{
	if( m_module == nullptr )
	{
		std::cerr << std::format( "Failed to load DLL {}. Error: {}", a_path.string(), GetLastError() ) << std::endl;
		assert( false );
	}
}

ge::windows::modules::module::~module()
{
	FreeLibrary( m_module );
}

auto ge::windows::modules::module::get_exported_func( std::string_view func_name ) const -> void ( * )()
{
	return reinterpret_cast< void ( * )() >( GetProcAddress( m_module, func_name.data() ) );
}

std::filesystem::path ge::windows::modules::loader::get_platform_shared_lib_file_extension() const
{
	return { ".dll" };
}

std::shared_ptr< ge::modules::platform_module > ge::windows::modules::loader::load_platform_module(
	const std::filesystem::path& shared_lib ) const
{
	return std::make_shared< module >( shared_lib );
}
