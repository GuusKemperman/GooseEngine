export module test_scheduling;

import stl;
import runtime_reflection;
import scheduling;
import io;
export import test_core;

using namespace ge::test_core;

namespace
{
	template< size_t >
	void dummy_system()
	{
	}

	struct result
	{
		ge::logger m_logger{};
		std::optional< ge::scheduling::execution_graph > m_graph{};
	};

	// Dont judge me
	result build_test_graph( auto std::invocable< ge::refl::builders::module_builder& >&& func )
	{
		auto module_builder = ge::refl::builders::begin_registry().begin_module( "scheduling" );

		func( static_cast< ge::refl::builders::module_builder& >( module_builder ) );

		std::unique_ptr< ge::refl::registry_data > reg = std::move( module_builder.end_module() ).build();

		result result{};
		result.m_graph = ge::scheduling::build_graph( { reg->m_funcs }, result.m_logger );
		return std::move( result );
	}

	size_t get_build_error_count( const result& result )
	{
		// Assume the build graph only logs errors/warnings
		return result.m_logger.get_logged_messages().size();
	}

	bool do_systems_run_in_parallel( const result& result, std::vector< std::string_view > system_names )
	{
		if( !result.m_graph.has_value() )
		{
			return false;
		}

		for(const ge::scheduling::execution_graph::group& group : result.m_graph->m_groups)
		{
			std::int64_t num_in_group = std::ranges::count_if(
				group.m_nodes,
				[ &system_names ]( const ge::scheduling::execution_graph::group::system_node& node )
				{
					return std::ranges::find( system_names, node.m_name ) != system_names.end();
				} );

			if( num_in_group > 0 )
			{
				return num_in_group == std::ssize(system_names);
			}
		}

		return false;
	}

	bool do_systems_in_this_order( const result& result, std::vector< std::string_view > b )
	{
		if( !result.m_graph.has_value() )
		{
			return false;
		}

		for(const ge::scheduling::execution_graph::group& group : result.m_graph->m_groups)
		{
			if (b.empty())
			{
				return true;
			}

			bool is_first_in_this_group = std::ranges::find_if(group.m_nodes, 
				[&b]( const ge::scheduling::execution_graph::group::system_node& node )
				{
					return node.m_name == b.front();
				} );

			std::int64_t num_in_group = std::ranges::count_if(
				group.m_nodes,
				[ &system_names ]( const ge::scheduling::execution_graph::group::system_node& node )
				{
					return std::ranges::find( system_names, node.m_name ) != system_names.end();
				} );

			if (num_in_group == 0)
			{
				continue;
			}

			if (is_first_in_this_group && num_in_group == 1)
			{
				b.erase(b.begin());
				continue;
			}

			if (is_first_in_this_group && num_in_group > 1)
			{
				return false;
			}
		}

		return true;
	}

	void expect_build_error( const result& result, std::string_view error )
	{
		
	}

} // namespace

namespace ordering_tests
{
	REFL_FUNC( ge::test_core::unit_test_trait{} )
	export API void two_systems_run_in_parallel()
	{
		result result = ::build_test_graph( []( ge::refl::builders::module_builder& builder )
			{ 
				builder.begin_func< ::dummy_system< 1 > >( "system_1" ).add_traits( ge::scheduling::traits::system{}  );
				builder.begin_func< ::dummy_system< 2 > >( "system_2" ).add_traits( ge::scheduling::traits::system{}  );
			}
		 );

		expect::is_eq( get_build_error_count( result ), 0 );
		expect::is_true( do_systems_run_in_parallel( result, { "system1", "system2" } ) );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )
	export API void zero_systems_no_error()
	{
		result result = ::build_test_graph( []( ge::refl::builders::module_builder& builder )
			{ 
				(void)builder;
			}
		 );

		expect::is_eq( get_build_error_count( result ), 0 );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )
	export API void after_no_parallel()
	{
		result result = ::build_test_graph( []( ge::refl::builders::module_builder& builder )
			{ 
				builder.begin_func< ::dummy_system< 1 > >( "system_1" ).add_traits( ge::scheduling::traits::system{}  );
				builder.begin_func< ::dummy_system< 2 > >( "system_2" ).add_traits( ge::scheduling::traits::system{}, ge::scheduling::traits::order_after<&::dummy_system<1>>{} );
			}
		 );

		expect::is_eq( get_build_error_count( result ), 0 );
		expect::is_false( do_systems_run_in_parallel( result, { "system1", "system2" } ) );
		expect::is_true( do_systems_in_this_order( result, { "system1", "system2" } ) );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )
	export API void before_no_parallel()
	{
		result result = ::build_test_graph( []( ge::refl::builders::module_builder& builder )
			{ 
				builder.begin_func< ::dummy_system< 1 > >( "system_1" ).add_traits( ge::scheduling::traits::system{}  );
				builder.begin_func< ::dummy_system< 2 > >( "system_2" ).add_traits( ge::scheduling::traits::system{}, ge::scheduling::traits::order_before<&::dummy_system<1>>{} );
			}
		 );

		expect::is_eq( get_build_error_count( result ), 0 );
		expect::is_false( do_systems_run_in_parallel( result, { "system1", "system2" } ) );
		expect::is_true( do_systems_in_this_order( result, { "system2", "system1" } ) );
	}


	REFL_FUNC( ge::test_core::unit_test_trait{} )
	export API void order_against_self_gives_error()
	{
		result result = ::build_test_graph( []( ge::refl::builders::module_builder& builder )
			{ 
				builder.begin_func< ::dummy_system< 1 > >( "system_1" ).add_traits( ge::scheduling::traits::system{},  ge::scheduling::traits::order_before<&::dummy_system<1>>{}  );
			}
		 );

		expect::is_eq( get_build_error_count( result ), 1 );
		expect_build_error( result, "invalid ordering for system system_1: ordered against itself");
	}
} // namespace ordering_tests
