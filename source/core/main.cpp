#include <cassert>

import stl;
import modules;

import windows;
import exporters;
import export_core;
import runtime_reflection;
import io;

int main()
{
	// TODO Not really good to assume this
	assert( std::filesystem::current_path().string().ends_with( "bin" ) );

	ge::logger logger{};
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

	std::optional intermediate_graph = ge::exporter::build_graph( { reg->m_funcs }, logger );

	if( !intermediate_graph )
	{
		return 1;
	}

	// TODO no hard code max size
	static constexpr size_t pack_capacity = 1024 * 1024;
	std::unique_ptr< std::byte[] > pack_buffer = std::make_unique< std::byte[] >( pack_capacity );
	ge::exporter::pack_writer pack_writer{ { pack_buffer.get(), pack_capacity } };

	ge::exporter::environments_map env_map = ge::exporter::export_environments( { reg->m_types }, pack_writer );

	const ge::exporter::exported_schedule& graph = ge::exporter::export_schedule( pack_writer, *intermediate_graph, env_map );

	while( true )
	{
		ge::exporter::execute_schedule( graph );
	}
}
