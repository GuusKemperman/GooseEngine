export module test_exporters:test_schedule_exporter;

import stl;
import runtime_reflection;
import io;
import exporters;
import core_traits;
export import test_core;

using namespace ge::test_core;

namespace
{
	using module_builder = ge::refl::builders::module_builder;

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

	struct graph_result
	{
		std::unique_ptr< ge::refl::registry_data > m_reg{};
		ge::logger m_logger{};
		std::optional< ge::exporter::execution_graph > m_graph{};
	};

	graph_result build_test_graph( std::invocable< module_builder& > auto&& func )
	{
		ge::refl::builders::endable_registry_builder reg_builder = ge::refl::builders::begin_registry();
		auto mod = reg_builder.begin_module( "scheduling" );
		func( mod );
		mod.end_module();

		graph_result result{ .m_reg = std::move( reg_builder ).build() };
		result.m_graph = ge::exporter::build_graph( { result.m_reg->m_funcs }, result.m_logger );
		return result;
	}

	template< auto Func >
	void add_system( module_builder& builder, std::string_view name, auto&&... extra_traits )
	{
		builder.begin_func< Func >( name )
			.add_traits( ge::traits::system{}, std::forward< decltype( extra_traits ) >( extra_traits )... )
			.end_func();
	}

	void expect_no_build_errors( const graph_result& result )
	{
		// Assume the build graph only logs errors/warnings
		expect::is_eq( result.m_logger.get_logged_messages().size(), 0ull );
		expect::is_true( result.m_graph.has_value() );
	}

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
				builder.begin_type< dummy_env< 1 > >( "env1" ).add_traits( ge::traits::environment{} ).end_type();
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
} // namespace ordering_tests
