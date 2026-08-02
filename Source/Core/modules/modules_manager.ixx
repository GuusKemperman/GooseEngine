module;
#include <assert.h>

// TODO rename this partition
export module modules:modules_manager;

import :modules_platform;
import stl;

namespace ge::modules
{
	export struct module
	{
		std::shared_ptr< const platform_module > m_platform_module{};

		std::string m_name{};
	};

	export API std::vector< module > load_modules_in_folder( const platform_loader& loader, const std::filesystem::path& folder )
	{
		assert( std::filesystem::is_directory(folder) );

		std::vector< module > loaded_modules{};

		for( auto entry : std::filesystem::directory_iterator{ folder } )
		{
			if( !loader.is_shared_lib( entry.path() ) )
			{
				continue;
			}

			std::shared_ptr< const platform_module > platform_module = loader.load_platform_module( entry.path() );

			if( platform_module == nullptr )
			{
				continue;
			}

			loaded_modules.push_back(
				module{ .m_platform_module = std::move( platform_module ), .m_name = loader.get_module_name( entry.path() ) } );
		}

		return loaded_modules;
	}
}