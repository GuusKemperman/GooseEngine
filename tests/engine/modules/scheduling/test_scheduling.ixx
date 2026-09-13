export module test_scheduling;

import stl;
import runtime_reflection;
import scheduling;
import io;
export import test_core;

using namespace ge::test_core;

namespace
{
	template< size_t, typename... Args >
	void dummy_system( Args... )
	{
	}

	template< size_t >
	struct dummy_env
	{
	};

	struct result
	{
		ge::logger m_logger{};
		std::optional< ge::scheduling::execution_graph > m_graph{};
	};

	result build_test_graph( std::invocable< ge::refl::builders::module_builder& > auto&& func )
	{
		ge::refl::builders::endable_registry_builder reg_builder = ge::refl::builders::begin_registry();
		ge::refl::builders::endable_module_builder< ge::refl::builders::endable_registry_builder > module_builder
			= reg_builder.begin_module( "scheduling" );

		func( static_cast< ge::refl::builders::module_builder& >( module_builder ) );

		module_builder.end_module();

		std::unique_ptr< ge::refl::registry_data > reg = std::move( reg_builder ).build();

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

		for( const ge::scheduling::execution_graph::group& group : result.m_graph->m_groups )
		{
			std::int64_t num_in_group = std::ranges::count_if(
				group.m_nodes,
				[ &system_names ]( const ge::scheduling::execution_graph::group::system_node& node )
				{ return std::ranges::find( system_names, node.m_name ) != system_names.end(); } );

			if( num_in_group > 0 )
			{
				return num_in_group == std::ssize( system_names );
			}
		}

		return false;
	}

	bool do_systems_in_this_order( const result& result, std::vector< std::string_view > system_names )
	{
		if( !result.m_graph.has_value() )
		{
			return false;
		}

		for( const ge::scheduling::execution_graph::group& group : result.m_graph->m_groups )
		{
			if( system_names.empty() )
			{
				return true;
			}

			bool is_in_this_group = std::ranges::find(
										group.m_nodes,
										system_names.front(),
										&ge::scheduling::execution_graph::group::system_node::m_name )
									!= group.m_nodes.end();

			std::int64_t num_in_group = std::ranges::count_if(
				group.m_nodes,
				[ &system_names ]( const ge::scheduling::execution_graph::group::system_node& node )
				{ return std::ranges::find( system_names, node.m_name ) != system_names.end(); } );

			if( num_in_group == 0 )
			{
				continue;
			}

			if( is_in_this_group && num_in_group == 1 )
			{
				system_names.erase( system_names.begin() );
				continue;
			}

			if( is_in_this_group && num_in_group > 1 )
			{
				return false;
			}
		}

		return true;
	}

	void expect_build_error( const result& result, std::string_view error )
	{
		const std::list< ge::logger::entry >& logged = result.m_logger.get_logged_messages();
		expect::is_ne( std::ranges::find( logged, error, &ge::logger::entry::m_logged_text ), logged.end() );
	}

} // namespace

namespace ordering_tests
{
	REFL_FUNC( ge::test_core::unit_test_trait{} )
	export API void two_systems_run_in_parallel()
	{
		result result = ::build_test_graph(
			[]( ge::refl::builders::module_builder& builder )
			{
				builder.begin_func< ::dummy_system< 1 > >( "system1" ).add_traits( ge::scheduling::traits::system{} ).end_func();
				builder.begin_func< ::dummy_system< 2 > >( "system2" ).add_traits( ge::scheduling::traits::system{} ).end_func();
			} );

		expect::is_eq( get_build_error_count( result ), 0ull );

		expect::is_true( do_systems_run_in_parallel( result, { "system1", "system2" } ) );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )
	export API void zero_systems_no_error()
	{
		result result = ::build_test_graph( []( ge::refl::builders::module_builder& builder ) { ( void )builder; } );

		expect::is_eq( get_build_error_count( result ), 0ull );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )
	export API void after_no_parallel()
	{
		result result = ::build_test_graph(
			[]( ge::refl::builders::module_builder& builder )
			{
				builder.begin_func< ::dummy_system< 1 > >( "system1" ).add_traits( ge::scheduling::traits::system{} ).end_func();
				builder.begin_func< ::dummy_system< 2 > >( "system2" )
					.add_traits( ge::scheduling::traits::system{}, ge::scheduling::traits::order_after< &::dummy_system< 1 > >{} )
					.end_func();
			} );

		expect::is_eq( get_build_error_count( result ), 0ull );
		expect::is_false( do_systems_run_in_parallel( result, { "system1", "system2" } ) );
		expect::is_true( do_systems_in_this_order( result, { "system1", "system2" } ) );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )
	export API void before_no_parallel()
	{
		result result = ::build_test_graph(
			[]( ge::refl::builders::module_builder& builder )
			{
				builder.begin_func< ::dummy_system< 1 > >( "system1" ).add_traits( ge::scheduling::traits::system{} ).end_func();
				builder.begin_func< ::dummy_system< 2 > >( "system2" )
					.add_traits(
						ge::scheduling::traits::system{},
						ge::scheduling::traits::order_before< &::dummy_system< 1 > >{} )
					.end_func();
			} );

		expect::is_eq( get_build_error_count( result ), 0ull );
		expect::is_false( do_systems_run_in_parallel( result, { "system1", "system2" } ) );
		expect::is_true( do_systems_in_this_order( result, { "system2", "system1" } ) );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )
	export API void two_underconstrained_writers_gives_error()
	{
		result result = ::build_test_graph(
			[]( ge::refl::builders::module_builder& builder )
			{
				builder.begin_type< ::dummy_env< 1 > >( "env1" ).add_traits( ge::scheduling::traits::environment{} ).end_type();
				builder.begin_func< ::dummy_system< 1, ::dummy_env< 1 >& > >( "system1" )
					.add_traits( ge::scheduling::traits::system{} )
					.end_func();
				builder.begin_func< ::dummy_system< 2, ::dummy_env< 1 >& > >( "system2" )
					.add_traits( ge::scheduling::traits::system{} )
					.end_func();
			} );

		expect::is_eq( get_build_error_count( result ), 1ull );
		expect_build_error( result, "underconstrained access to 'env1': no order specified between 'system1' and 'system2'" );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )
	export API void order_against_self_gives_error()
	{
		result result = ::build_test_graph(
			[]( ge::refl::builders::module_builder& builder )
			{
				builder.begin_func< ::dummy_system< 1 > >( "system1" )
					.add_traits(
						ge::scheduling::traits::system{},
						ge::scheduling::traits::order_before< &::dummy_system< 1 > >{} )
					.end_func();
			} );

		expect::is_eq( get_build_error_count( result ), 1ull );
		expect_build_error( result, "invalid ordering for 'system1': ordered against itself." );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )
	export API void order_against_non_system_gives_error()
	{
		result result = ::build_test_graph(
			[]( ge::refl::builders::module_builder& builder )
			{
				builder.begin_func< ::dummy_system< 1 > >( "system1" )
					.add_traits(
						ge::scheduling::traits::system{},
						ge::scheduling::traits::order_before< &::dummy_system< 2 > >{} )
					.end_func();
			} );

		expect::is_eq( get_build_error_count( result ), 1ull );
		expect_build_error(
			result,
			"invalid ordering for 'system1': ordered constraint against function that was not a system." );
	}

} // namespace ordering_tests
