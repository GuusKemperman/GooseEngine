export module test_exporters:test_schedule_exporter;

import stl;
import runtime_reflection;
import io;
import exporters;
import core_traits;
export import test_core;
import :helpers;

using namespace ge::test_core;
using namespace exporter_test_helpers;

namespace
{
	template< size_t, typename... Args >
	void dummy_system( Args... )
	{
		// Needed to prevent the function from being folded
		std::puts( __FUNCSIG__ );
	}

	template< size_t >
	struct dummy_env
	{
	};

	void expect_single_build_error( const graph_result& result, std::string_view error )
	{
		const std::list< ge::logger::entry >& logged = result.m_logger.get_logged_messages();
		expect::is_eq( logged.size(), 1ull );
		expect::is_eq( logged.front().m_logged_text, error );
		expect::is_false( result.m_graph.has_value() );
	}

	bool is_in_group( const ge::exporter::execution_group& group, std::string_view name )
	{
		return std::ranges::find( group, name, &ge::refl::func_data::m_name ) != group.end();
	}

	// True if all systems are scheduled in the same group.
	bool do_systems_run_in_parallel( const graph_result& result, std::initializer_list< std::string_view > system_names )
	{
		if( !result.m_graph.has_value() )
		{
			return false;
		}

		return std::ranges::any_of(
			*result.m_graph,
			[ system_names ]( const ge::exporter::execution_group& group )
			{
				return std::ranges::all_of(
					system_names,
					[ &group ]( std::string_view name ) { return is_in_group( group, name ); } );
			} );
	}

	// True if all systems are scheduled, each in a strictly later group than the one before it.
	bool do_systems_run_in_this_order( const graph_result& result, std::initializer_list< std::string_view > system_names )
	{
		if( !result.m_graph.has_value() )
		{
			return false;
		}

		const ge::exporter::execution_graph& graph = *result.m_graph;
		std::optional< std::ptrdiff_t > previous_group{};
		for( std::string_view name : system_names )
		{
			const auto group = std::ranges::find_if(
				graph,
				[ name ]( const ge::exporter::execution_group& candidate ) { return is_in_group( candidate, name ); } );
			if( group == graph.end() )
			{
				return false;
			}

			const std::ptrdiff_t group_index = std::ranges::distance( graph.begin(), group );
			if( previous_group.has_value() && group_index <= *previous_group )
			{
				return false;
			}
			previous_group = group_index;
		}

		return true;
	}
} // namespace

namespace ordering_tests
{
	REFL_FUNC( ge::test_core::unit_test_trait{} )
	export API void two_systems_run_in_parallel()
	{
		const graph_result result = build_test_graph(
			[]( module_builder& builder )
			{
				add_system< &dummy_system< 1 > >( builder, "system1" );
				add_system< &dummy_system< 2 > >( builder, "system2" );
			} );

		expect_no_build_errors( result );
		expect::is_true( do_systems_run_in_parallel( result, { "system1", "system2" } ) );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )
	export API void zero_systems_no_error()
	{
		const graph_result result = build_test_graph( []( module_builder& ) {} );

		expect_no_build_errors( result );
		expect::is_true( result.m_graph->empty() );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )
	export API void after_no_parallel()
	{
		const graph_result result = build_test_graph(
			[]( module_builder& builder )
			{
				add_system< &dummy_system< 1 > >( builder, "system1" );
				add_system< &dummy_system< 2 > >( builder, "system2", ge::traits::order_after< &dummy_system< 1 > >{} );
			} );

		expect_no_build_errors( result );
		expect::is_true( do_systems_run_in_this_order( result, { "system1", "system2" } ) );
		expect::is_false( do_systems_run_in_this_order( result, { "system2", "system1" } ) );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )
	export API void before_no_parallel()
	{
		const graph_result result = build_test_graph(
			[]( module_builder& builder )
			{
				add_system< &dummy_system< 1 > >( builder, "system1" );
				add_system< &dummy_system< 2 > >( builder, "system2", ge::traits::order_before< &dummy_system< 1 > >{} );
			} );

		expect_no_build_errors( result );
		expect::is_true( do_systems_run_in_this_order( result, { "system2", "system1" } ) );
		expect::is_false( do_systems_run_in_this_order( result, { "system1", "system2" } ) );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )
	export API void two_underconstrained_writers_gives_error()
	{
		const graph_result result = build_test_graph(
			[]( module_builder& builder )
			{
				add_env< dummy_env< 1 > >( builder, "env1" );
				add_system< &dummy_system< 1, dummy_env< 1 >& > >( builder, "system1" );
				add_system< &dummy_system< 2, dummy_env< 1 >& > >( builder, "system2" );
			} );

		expect_single_build_error(
			result,
			"underconstrained access to 'env1': no order specified between 'system1' and 'system2'" );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )
	export API void order_against_self_gives_error()
	{
		const graph_result result = build_test_graph(
			[]( module_builder& builder )
			{ add_system< &dummy_system< 1 > >( builder, "system1", ge::traits::order_before< &dummy_system< 1 > >{} ); } );

		expect_single_build_error( result, "invalid ordering for 'system1': ordered against itself." );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )
	export API void order_against_non_system_gives_error()
	{
		const graph_result result = build_test_graph(
			[]( module_builder& builder )
			{ add_system< &dummy_system< 1 > >( builder, "system1", ge::traits::order_before< &dummy_system< 2 > >{} ); } );

		expect_single_build_error(
			result,
			"invalid ordering for 'system1': ordered constraint against function that was not a system." );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )
	export API void infinite_loop_gives_error()
	{
		const graph_result result = build_test_graph(
			[]( module_builder& builder )
			{
				add_system< &dummy_system< 1 > >( builder, "system1", ge::traits::order_before< &dummy_system< 2 > >{} );
				add_system< &dummy_system< 2 > >( builder, "system2", ge::traits::order_before< &dummy_system< 1 > >{} );
			} );

		expect_single_build_error(
			result,
			"invalid ordering: infinite loop:\n"
			"  0 | 'system1'\n"
			"  1 | 'system2'\n"
			"  2 | 'system1'\n" );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )
	export API void two_readers_run_in_parallel()
	{
		const graph_result result = build_test_graph(
			[]( module_builder& builder )
			{
				add_env< dummy_env< 1 > >( builder, "env1" );
				add_system< &dummy_system< 1, const dummy_env< 1 >& > >( builder, "system1" );
				add_system< &dummy_system< 2, const dummy_env< 1 >& > >( builder, "system2" );
			} );

		expect_no_build_errors( result );
		expect::is_true( do_systems_run_in_parallel( result, { "system1", "system2" } ) );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )
	export API void underconstrained_reader_and_writer_gives_error()
	{
		const graph_result result = build_test_graph(
			[]( module_builder& builder )
			{
				add_env< dummy_env< 1 > >( builder, "env1" );
				add_system< &dummy_system< 1, dummy_env< 1 >& > >( builder, "system1" );
				add_system< &dummy_system< 2, const dummy_env< 1 >& > >( builder, "system2" );
			} );

		expect_single_build_error(
			result,
			"underconstrained access to 'env1': no order specified between 'system1' and 'system2'" );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )
	export API void reader_ordered_after_writer_no_error()
	{
		const graph_result result = build_test_graph(
			[]( module_builder& builder )
			{
				add_env< dummy_env< 1 > >( builder, "env1" );
				add_system< &dummy_system< 2, const dummy_env< 1 >& > >(
					builder,
					"system2",
					ge::traits::order_after< &dummy_system< 1, dummy_env< 1 >& > >{} );
				add_system< &dummy_system< 1, dummy_env< 1 >& > >( builder, "system1" );
			} );

		expect_no_build_errors( result );
		expect::is_true( do_systems_run_in_this_order( result, { "system1", "system2" } ) );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )
	export API void ordered_writers_no_error()
	{
		const graph_result result = build_test_graph(
			[]( module_builder& builder )
			{
				add_env< dummy_env< 1 > >( builder, "env1" );
				add_system< &dummy_system< 1, dummy_env< 1 >& > >( builder, "system1" );
				add_system< &dummy_system< 2, dummy_env< 1 >& > >(
					builder,
					"system2",
					ge::traits::order_before< &dummy_system< 1, dummy_env< 1 >& > >{} );
			} );

		expect_no_build_errors( result );
		expect::is_true( do_systems_run_in_this_order( result, { "system2", "system1" } ) );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )
	export API void mixed_before_and_after_chain_runs_in_order()
	{
		// Registered out of order, so registry order can't accidentally give the right result
		const graph_result result = build_test_graph(
			[]( module_builder& builder )
			{
				add_system< &dummy_system< 3 > >( builder, "system3" );
				add_system< &dummy_system< 2 > >(
					builder,
					"system2",
					ge::traits::order_after< &dummy_system< 1 > >{},
					ge::traits::order_before< &dummy_system< 3 > >{} );
				add_system< &dummy_system< 1 > >( builder, "system1" );
			} );

		expect_no_build_errors( result );
		expect::is_eq( result.m_graph->size(), 3ull );
		expect::is_true( do_systems_run_in_this_order( result, { "system1", "system2", "system3" } ) );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )
	export API void transitive_order_resolves_write_conflict()
	{
		// system1 and system3 both write env1 and are only ordered through system2
		const graph_result result = build_test_graph(
			[]( module_builder& builder )
			{
				add_env< dummy_env< 1 > >( builder, "env1" );
				add_system< &dummy_system< 1, dummy_env< 1 >& > >( builder, "system1" );
				add_system< &dummy_system< 2 > >(
					builder,
					"system2",
					ge::traits::order_after< &dummy_system< 1, dummy_env< 1 >& > >{} );
				add_system< &dummy_system< 3, dummy_env< 1 >& > >(
					builder,
					"system3",
					ge::traits::order_after< &dummy_system< 2 > >{} );
			} );

		expect_no_build_errors( result );
		expect::is_true( do_systems_run_in_this_order( result, { "system1", "system2", "system3" } ) );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )
	export API void diamond_runs_middle_in_parallel()
	{
		const graph_result result = build_test_graph(
			[]( module_builder& builder )
			{
				add_system< &dummy_system< 4 > >(
					builder,
					"system4",
					ge::traits::order_after< &dummy_system< 2 > >{},
					ge::traits::order_after< &dummy_system< 3 > >{} );
				add_system< &dummy_system< 3 > >( builder, "system3", ge::traits::order_after< &dummy_system< 1 > >{} );
				add_system< &dummy_system< 2 > >( builder, "system2", ge::traits::order_after< &dummy_system< 1 > >{} );
				add_system< &dummy_system< 1 > >( builder, "system1" );
			} );

		expect_no_build_errors( result );
		expect::is_eq( result.m_graph->size(), 3ull );
		expect::is_true( do_systems_run_in_parallel( result, { "system2", "system3" } ) );
		expect::is_true( do_systems_run_in_this_order( result, { "system1", "system2", "system4" } ) );
		expect::is_true( do_systems_run_in_this_order( result, { "system1", "system3", "system4" } ) );
	}
} // namespace ordering_tests
