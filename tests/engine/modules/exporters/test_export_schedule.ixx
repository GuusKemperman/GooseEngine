export module test_exporters:test_export_schedule;

import stl;
import runtime_reflection;
import io;
import exporters;
import export_core;
import core_traits;
export import test_core;
import :helpers;

using namespace ge::test_core;
using namespace exporter_test_helpers;

namespace
{
	struct counter_env
	{
		int m_count{};
	};

	struct env_a
	{
		int m_value{};
	};

	struct env_b
	{
		int m_value{};
	};

	struct env_with_default
	{
		std::uint32_t m_value = 0xC0FFEE;
	};

	struct alignas( 64 ) aligned_env
	{
		int m_value = 3;
	};

	struct not_an_env
	{
		int m_value{};
	};

	void increment( counter_env& counter )
	{
		counter.m_count++;
	}

	void set_a( env_a& a )
	{
		a.m_value = 7;
	}

	void copy_a_to_b( const env_a& a, env_b& b )
	{
		b.m_value = a.m_value * 10;
	}

	template< size_t Idx >
	void set_to_idx( env_a& a )
	{
		a.m_value = static_cast< int >( Idx );
	}

	void add_three_to_b( env_b& b )
	{
		b.m_value += 3;
	}

	struct alignas( 64 ) test_pack_buffer
	{
		test_pack_buffer()
		{
			// Non-zero, so default construction is visible
			m_bytes.fill( std::byte{ 0xAB } );
		}

		bool contains( const void* address, size_t size ) const
		{
			const std::byte* begin = m_bytes.data();
			const std::byte* end = m_bytes.data() + m_bytes.size();
			const std::byte* first = static_cast< const std::byte* >( address );
			return std::less_equal<>{}( begin, first ) && std::less_equal<>{}( first + size, end );
		}

		size_t offset_of( const void* address ) const
		{
			return static_cast< size_t >( static_cast< const std::byte* >( address ) - m_bytes.data() );
		}

		template< typename T >
		T& at_offset( size_t offset )
		{
			return *std::launder( reinterpret_cast< T* >( m_bytes.data() + offset ) );
		}

		std::array< std::byte, 4096 > m_bytes;
	};

	// Owns everything needed for a registry, graph and exported pack.
	struct exported_result
	{
		explicit exported_result( std::invocable< module_builder& > auto&& func )
			: m_graph( build_test_graph( func ) )
		{
		}

		template< typename T >
		T& env()
		{
			return *static_cast< T* >( m_env_map.at( ge::refl::make_type_id< T >() ) );
		}

		graph_result m_graph;
		test_pack_buffer m_buffer{};
		ge::exporter::pack_writer m_writer{ m_buffer.m_bytes };
		ge::exporter::environments_map m_env_map{};
		const ge::exporter::exported_schedule* m_schedule{};
	};

	void export_environments( exported_result& result )
	{
		result.m_env_map = ge::exporter::export_environments( { result.m_graph.m_reg->m_types }, result.m_writer );
	}

	// Builds the registry and graph, then exports only the environments into the result's pack.
	std::unique_ptr< exported_result > export_environments_pack( std::invocable< module_builder& > auto&& func )
	{
		auto result = std::make_unique< exported_result >( func );
		export_environments( *result );
		return result;
	}

	// Checks the graph built without errors, then exports the environments and the schedule into the result's pack.
	std::unique_ptr< exported_result > export_test_pack( std::invocable< module_builder& > auto&& func )
	{
		auto result = std::make_unique< exported_result >( func );
		expect_no_build_errors( result->m_graph );

		export_environments( *result );
		result->m_schedule = &ge::exporter::export_schedule( result->m_writer, *result->m_graph.m_graph, result->m_env_map );
		return result;
	}
} // namespace

namespace export_environments_tests
{
	REFL_FUNC( ge::test_core::unit_test_trait{} )
	export API void exports_one_default_constructed_instance_per_environment()
	{
		std::unique_ptr< exported_result > result = export_environments_pack(
			[]( module_builder& builder )
			{
				add_env< env_with_default >( builder, "env_with_default" );
				add_env< aligned_env >( builder, "aligned_env" );
				builder.begin_type< not_an_env >( "not_an_env" ).end_type();
			} );

		expect::is_eq( result->m_env_map.size(), 2ull );
		expect::is_false( result->m_env_map.contains( ge::refl::make_type_id< not_an_env >() ) );

		const env_with_default& with_default = result->env< env_with_default >();
		const aligned_env& aligned = result->env< aligned_env >();

		expect::is_eq( with_default.m_value, 0xC0FFEEu );
		expect::is_eq( aligned.m_value, 3 );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )
	export API void exported_environments_are_aligned_inside_pack_and_disjoint()
	{
		std::unique_ptr< exported_result > result = export_environments_pack(
			[]( module_builder& builder )
			{
				add_env< env_with_default >( builder, "env_with_default" );
				add_env< aligned_env >( builder, "aligned_env" );
			} );

		const void* with_default = &result->env< env_with_default >();
		const void* aligned = &result->env< aligned_env >();

		expect::is_true( result->m_buffer.contains( with_default, sizeof( env_with_default ) ) );
		expect::is_true( result->m_buffer.contains( aligned, sizeof( aligned_env ) ) );
		expect::is_eq( reinterpret_cast< std::uintptr_t >( with_default ) % alignof( env_with_default ), 0ull );
		expect::is_eq( reinterpret_cast< std::uintptr_t >( aligned ) % alignof( aligned_env ), 0ull );

		const size_t with_default_offset = result->m_buffer.offset_of( with_default );
		const size_t aligned_offset = result->m_buffer.offset_of( aligned );

		// Both lie within the written part of the pack
		expect::is_le( with_default_offset + sizeof( env_with_default ), result->m_writer.size() );
		expect::is_le( aligned_offset + sizeof( aligned_env ), result->m_writer.size() );

		// And don't overlap
		expect::is_true(
			with_default_offset + sizeof( env_with_default ) <= aligned_offset
			|| aligned_offset + sizeof( aligned_env ) <= with_default_offset );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )
	export API void no_environments_gives_empty_map()
	{
		std::unique_ptr< exported_result > result = export_environments_pack(
			[]( module_builder& builder ) { builder.begin_type< not_an_env >( "not_an_env" ).end_type(); } );

		expect::is_true( result->m_env_map.empty() );
		expect::is_eq( result->m_writer.size(), 0ull );
	}
} // namespace export_environments_tests

namespace export_schedule_tests
{
	REFL_FUNC( ge::test_core::unit_test_trait{} )
	export API void exported_groups_match_graph()
	{
		std::unique_ptr< exported_result > result = export_test_pack(
			[]( module_builder& builder )
			{
				add_env< env_a >( builder, "env_a" );
				add_env< env_b >( builder, "env_b" );
				add_env< counter_env >( builder, "counter_env" );
				add_system< &copy_a_to_b >( builder, "copy_a_to_b", ge::traits::order_after< &set_a >{} );
				add_system< &set_a >( builder, "set_a" );
				add_system< &increment >( builder, "increment" );
			} );

		const ge::exporter::exported_schedule& schedule = *result->m_schedule;
		const ge::exporter::execution_graph& graph = *result->m_graph.m_graph;

		expect::is_true( result->m_buffer.contains( &schedule, sizeof( schedule ) ) );
		expect::is_eq( graph.size(), 2ull );
		expect::is_eq( schedule.size(), graph.size() );

		for( const auto& [ exported_group, group ] : std::views::zip( schedule, graph ) )
		{
			expect::is_eq( exported_group.size(), group.size() );

			for( const auto& [ exported_system, func ] : std::views::zip( exported_group, group ) )
			{
				const ge::traits::system* trait = ge::refl::find_value_of_type< ge::traits::system >( func.get().m_traits );
				assert::is_not_null( trait );
				expect::is_eq( exported_system.m_invoke, trait->m_invoke );
			}
		}
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )
	export API void exported_arguments_point_to_exported_environments()
	{
		std::unique_ptr< exported_result > result = export_test_pack(
			[]( module_builder& builder )
			{
				add_env< env_a >( builder, "env_a" );
				add_env< env_b >( builder, "env_b" );
				add_system< &copy_a_to_b >( builder, "copy_a_to_b" );
			} );

		const ge::exporter::exported_schedule& schedule = *result->m_schedule;

		expect::is_eq( schedule.size(), 1ull );
		expect::is_eq( schedule.begin()->size(), 1ull );

		void* const* arguments = schedule.begin()->begin()->m_arguments_buffer.get();
		expect::is_true( result->m_buffer.contains( arguments, 2 * sizeof( void* ) ) );
		expect::is_eq( arguments[ 0 ], &result->env< env_a >() );
		expect::is_eq( arguments[ 1 ], &result->env< env_b >() );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )
	export API void no_systems_gives_empty_schedule()
	{
		std::unique_ptr< exported_result > result
			= export_test_pack( []( module_builder& builder ) { add_env< counter_env >( builder, "counter_env" ); } );

		expect::is_true( result->m_schedule->empty() );

		ge::exporter::execute_schedule( *result->m_schedule );
		expect::is_eq( result->env< counter_env >().m_count, 0 );
	}
} // namespace export_schedule_tests

namespace execute_schedule_tests
{
	REFL_FUNC( ge::test_core::unit_test_trait{} )
	export API void executes_each_system_once_per_execution()
	{
		std::unique_ptr< exported_result > result = export_test_pack(
			[]( module_builder& builder )
			{
				add_env< counter_env >( builder, "counter_env" );
				add_system< &increment >( builder, "increment" );
			} );

		ge::exporter::execute_schedule( *result->m_schedule );
		expect::is_eq( result->env< counter_env >().m_count, 1 );

		ge::exporter::execute_schedule( *result->m_schedule );
		expect::is_eq( result->env< counter_env >().m_count, 2 );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )
	export API void executes_all_systems_in_a_group()
	{
		std::unique_ptr< exported_result > result = export_test_pack(
			[]( module_builder& builder )
			{
				add_env< env_a >( builder, "env_a" );
				add_env< env_b >( builder, "env_b" );
				add_env< counter_env >( builder, "counter_env" );
				add_system< &set_a >( builder, "set_a" );
				add_system< &add_three_to_b >( builder, "add_three_to_b" );
				add_system< &increment >( builder, "increment" );
			} );

		expect::is_eq( result->m_schedule->size(), 1ull );

		ge::exporter::execute_schedule( *result->m_schedule );

		expect::is_eq( result->env< env_a >().m_value, 7 );
		expect::is_eq( result->env< env_b >().m_value, 3 );
		expect::is_eq( result->env< counter_env >().m_count, 1 );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )
	export API void executes_groups_in_order()
	{
		// Registered in reverse, so registry order can't accidentally give the right result
		std::unique_ptr< exported_result > result = export_test_pack(
			[]( module_builder& builder )
			{
				add_env< env_a >( builder, "env_a" );
				add_env< env_b >( builder, "env_b" );
				add_system< &copy_a_to_b >( builder, "copy_a_to_b", ge::traits::order_after< &set_a >{} );
				add_system< &set_a >( builder, "set_a" );
			} );

		ge::exporter::execute_schedule( *result->m_schedule );

		// Running copy_a_to_b first would give 0
		expect::is_eq( result->env< env_a >().m_value, 7 );
		expect::is_eq( result->env< env_b >().m_value, 70 );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )
	export API void executes_long_chain_in_order()
	{
		// Each system overwrites env_a; only the last one in the chain should be visible
		std::unique_ptr< exported_result > result = export_test_pack(
			[]( module_builder& builder )
			{
				add_env< env_a >( builder, "env_a" );
				add_system< &set_to_idx< 3 > >( builder, "set_to_3", ge::traits::order_after< &set_to_idx< 2 > >{} );
				add_system< &set_to_idx< 1 > >( builder, "set_to_1" );
				add_system< &set_to_idx< 2 > >( builder, "set_to_2", ge::traits::order_after< &set_to_idx< 1 > >{} );
			} );

		expect::is_eq( result->m_schedule->size(), 3ull );

		ge::exporter::execute_schedule( *result->m_schedule );

		expect::is_eq( result->env< env_a >().m_value, 3 );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )
	export API void memcpy_relocated_pack_executes_on_relocated_environments()
	{
		std::unique_ptr< exported_result > result = export_test_pack(
			[]( module_builder& builder )
			{
				add_env< env_a >( builder, "env_a" );
				add_env< env_b >( builder, "env_b" );
				add_env< counter_env >( builder, "counter_env" );
				add_system< &copy_a_to_b >( builder, "copy_a_to_b", ge::traits::order_after< &set_a >{} );
				add_system< &set_a >( builder, "set_a" );
				add_system< &increment >( builder, "increment" );
			} );

		const size_t schedule_offset = result->m_buffer.offset_of( result->m_schedule );
		const size_t env_a_offset = result->m_buffer.offset_of( &result->env< env_a >() );
		const size_t env_b_offset = result->m_buffer.offset_of( &result->env< env_b >() );
		const size_t counter_offset = result->m_buffer.offset_of( &result->env< counter_env >() );

		auto relocated = std::make_unique< test_pack_buffer >();
		std::memcpy( relocated->m_bytes.data(), result->m_buffer.m_bytes.data(), result->m_writer.size() );

		ge::exporter::execute_schedule( relocated->at_offset< const ge::exporter::exported_schedule >( schedule_offset ) );

		// The relocated pack acted on its own environments
		expect::is_eq( relocated->at_offset< env_a >( env_a_offset ).m_value, 7 );
		expect::is_eq( relocated->at_offset< env_b >( env_b_offset ).m_value, 70 );
		expect::is_eq( relocated->at_offset< counter_env >( counter_offset ).m_count, 1 );

		// And left the original pack untouched
		expect::is_eq( result->env< env_a >().m_value, 0 );
		expect::is_eq( result->env< env_b >().m_value, 0 );
		expect::is_eq( result->env< counter_env >().m_count, 0 );
	}
} // namespace execute_schedule_tests
