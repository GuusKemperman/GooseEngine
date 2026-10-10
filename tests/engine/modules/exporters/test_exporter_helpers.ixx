export module test_exporters:helpers;

import stl;
import runtime_reflection;
import io;
import exporters;
import core_traits;
import test_core;

using namespace ge::test_core;

// Shared by the test_exporters partitions. Named rather than anonymous, so other partitions can use it.
namespace exporter_test_helpers
{
	using module_builder = ge::refl::builders::module_builder;

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

	template< typename T >
	void add_env( module_builder& builder, std::string_view name )
	{
		builder.begin_type< T >( name ).add_traits( ge::traits::environment{} ).end_type();
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
} // namespace exporter_test_helpers
