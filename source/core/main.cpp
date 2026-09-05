#include <cassert>

import stl;
import modules;

import windows;
import scheduling;
import runtime_reflection;

int main()
{
	// TODO Not really good to assume this
	assert( std::filesystem::current_path().string().ends_with( "bin" ) );

	ge::windows::modules::loader windows_loader{};
	std::vector< ge::modules::module > modules
		= ge::modules::load_modules_in_folder( windows_loader, std::filesystem::current_path() );

	std::unique_ptr< ge::refl::registry_data > reg = [ &modules ]
	{
		ge::refl::builders::endable_registry_builder reg_builder = ge::refl::builders::begin_registry();

		for( ge::modules::module module : modules )
		{
			using build_func_t = void ( * )( ge::refl::builders::registry_builder& );
			build_func_t build_func
				= reinterpret_cast< build_func_t >( module.m_platform_module->get_exported_func( "build_runtime_reflection" ) );

			if( build_func == nullptr )
			{
				continue;
			}

			build_func( reg_builder );
		}

		return std::move( reg_builder ).build();
	}();

	//ge::scheduling::environments_map envs
	//	= ge::scheduling::build_environment_map( ge::scheduling::environments_query{ reg->m_types } );


	//ge::scheduling::execution_graph graph = ge::scheduling::build_graph( ge::scheduling::systems_query{ reg->m_funcs }, envs );


	//while(true)
	//{
	//	for(const ge::scheduling::cached_system& sys : graph.m_systems)
	//	{
	//		sys.m_invoke( sys.m_cached_arguments );
	//	}
	//}
}
