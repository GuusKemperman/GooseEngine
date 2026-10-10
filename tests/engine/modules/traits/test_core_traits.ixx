export module test_core_traits;

import stl;
import runtime_reflection;
import core_traits;
export import test_core;

using namespace ge::test_core;

namespace
{
	template< size_t >
	struct env
	{
		int m_value{};
	};

	struct alignas( 32 ) aligned_env
	{
		std::uint32_t m_value = 0xC0FFEE;
	};

	// Distinct bodies, so they can't be folded into one function
	template< size_t Idx >
	void dummy_system()
	{
		std::puts( __FUNCSIG__ );
	}

	void triple_a_into_b( const env< 0 >& a, env< 1 >& b )
	{
		b.m_value = a.m_value * 3;
	}

	using module_builder = ge::refl::builders::module_builder;

	std::unique_ptr< ge::refl::registry_data > build_registry( std::invocable< module_builder& > auto&& func )
	{
		ge::refl::builders::endable_registry_builder reg_builder = ge::refl::builders::begin_registry();
		auto mod = reg_builder.begin_module( "traits" );
		func( mod );
		mod.end_module();
		return std::move( reg_builder ).build();
	}

	const ge::refl::type_data& find_type( const ge::refl::registry_data& reg, std::string_view name )
	{
		auto it = std::ranges::find( reg.m_types, name, &ge::refl::type_data::m_name );
		assert::is_true( it != reg.m_types.end() );
		return *it;
	}

	const ge::refl::func_data& find_func( const ge::refl::registry_data& reg, std::string_view name )
	{
		auto it = std::ranges::find( reg.m_funcs, name, &ge::refl::func_data::m_name );
		assert::is_true( it != reg.m_funcs.end() );
		return *it;
	}

	template< typename T >
	const T& find_trait( std::span< const ge::refl::value > traits )
	{
		const T* trait = ge::refl::find_value_of_type< T >( traits );
		assert::is_not_null( trait );
		return *trait;
	}

	void add_aligned_env( module_builder& builder )
	{
		builder.begin_type< aligned_env >( "aligned_env" ).add_traits( ge::traits::environment{} ).end_type();
	}

	void add_triple_a_into_b( module_builder& builder )
	{
		builder.begin_type< env< 0 > >( "env0" ).add_traits( ge::traits::environment{} ).end_type();
		builder.begin_type< env< 1 > >( "env1" ).add_traits( ge::traits::environment{} ).end_type();
		builder.begin_func< &triple_a_into_b >( "triple_a_into_b" ).add_traits( ge::traits::system{} ).end_func();
	}

	void add_system0( module_builder& builder )
	{
		builder.begin_func< &dummy_system< 0 > >( "system0" ).add_traits( ge::traits::system{} ).end_func();
	}
} // namespace

namespace environment_trait_tests
{
	REFL_FUNC( ge::test_core::unit_test_trait{} )
	export API void stores_size_and_alignment()
	{
		std::unique_ptr< ge::refl::registry_data > reg = build_registry( add_aligned_env );

		const auto& trait = find_trait< ge::traits::environment >( find_type( *reg, "aligned_env" ).m_traits );

		expect::is_eq( trait.m_size, sizeof( aligned_env ) );
		expect::is_eq( trait.m_alignment, alignof( aligned_env ) );
		expect::is_eq( trait.m_size, 32ull );
		expect::is_eq( trait.m_alignment, 32ull );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )
	export API void construct_at_default_constructs()
	{
		std::unique_ptr< ge::refl::registry_data > reg = build_registry( add_aligned_env );

		const auto& trait = find_trait< ge::traits::environment >( find_type( *reg, "aligned_env" ).m_traits );

		alignas( aligned_env ) std::array< std::byte, sizeof( aligned_env ) > storage{};
		storage.fill( std::byte{ 0xAB } );

		trait.m_construct_at( storage.data() );

		const aligned_env& constructed = *std::launder( reinterpret_cast< const aligned_env* >( storage.data() ) );
		expect::is_eq( constructed.m_value, 0xC0FFEEu );
	}
} // namespace environment_trait_tests

namespace system_trait_tests
{
	REFL_FUNC( ge::test_core::unit_test_trait{} )
	export API void params_describe_access_and_type_in_order()
	{
		std::unique_ptr< ge::refl::registry_data > reg = build_registry( add_triple_a_into_b );

		const auto& trait = find_trait< ge::traits::system >( find_func( *reg, "triple_a_into_b" ).m_traits );

		using access = ge::traits::details::system_param::access;

		expect::is_eq( trait.m_params.size(), 2ull );
		expect::is_eq( trait.m_params[ 0 ].m_access, access::read );
		expect::is_eq( &trait.m_params[ 0 ].m_type.get(), &find_type( *reg, "env0" ) );
		expect::is_eq( trait.m_params[ 1 ].m_access, access::write );
		expect::is_eq( &trait.m_params[ 1 ].m_type.get(), &find_type( *reg, "env1" ) );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )
	export API void no_params_gives_empty_params()
	{
		std::unique_ptr< ge::refl::registry_data > reg = build_registry( add_system0 );

		const auto& trait = find_trait< ge::traits::system >( find_func( *reg, "system0" ).m_traits );

		expect::is_true( trait.m_params.empty() );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )
	export API void invoke_passes_arguments_in_order()
	{
		std::unique_ptr< ge::refl::registry_data > reg = build_registry( add_triple_a_into_b );

		const auto& trait = find_trait< ge::traits::system >( find_func( *reg, "triple_a_into_b" ).m_traits );

		env< 0 > a{ .m_value = 5 };
		env< 1 > b{ .m_value = 1 };
		std::array< void*, 2 > args{ &a, &b };

		trait.m_invoke( args.data() );

		expect::is_eq( b.m_value, 15 );
		expect::is_eq( a.m_value, 5 );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )
	export API void sequence_point_identifies_the_system()
	{
		std::unique_ptr< ge::refl::registry_data > reg = build_registry( add_system0 );

		const auto& trait = find_trait< ge::traits::system >( find_func( *reg, "system0" ).m_traits );

		expect::is_eq( trait.m_func_sequence_point, ge::traits::details::sequence_point{ &dummy_system< 0 > } );
		expect::is_ne( trait.m_func_sequence_point, ge::traits::details::sequence_point{ &dummy_system< 1 > } );
	}
} // namespace system_trait_tests

namespace sequence_point_tests
{
	REFL_FUNC( ge::test_core::unit_test_trait{} )
	export API void default_is_null()
	{
		ge::traits::details::sequence_point point{};

		expect::is_null( point.m_data );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )
	export API void same_function_is_equal_different_function_is_not()
	{
		ge::traits::details::sequence_point first{ &dummy_system< 0 > };
		ge::traits::details::sequence_point first_again{ &dummy_system< 0 > };
		ge::traits::details::sequence_point second{ &dummy_system< 1 > };

		expect::is_eq( first, first_again );
		expect::is_ne( first, second );
		expect::is_ne( first, ge::traits::details::sequence_point{} );
	}
} // namespace sequence_point_tests

namespace ordering_trait_tests
{
	REFL_FUNC( ge::test_core::unit_test_trait{} )
	export API void order_before_adds_ordering_base()
	{
		std::unique_ptr< ge::refl::registry_data > reg = build_registry(
			[]( module_builder& builder )
			{
				builder.begin_func< &dummy_system< 0 > >( "system0" )
					.add_traits( ge::traits::system{}, ge::traits::order_before< &dummy_system< 1 > >{} )
					.end_func();
			} );

		const auto& ordering = find_trait< ge::traits::details::ordering_base >( find_func( *reg, "system0" ).m_traits );

		expect::is_eq( ordering.m_type, ge::traits::details::ordering_base::before );
		expect::is_eq( ordering.m_point, ge::traits::details::sequence_point{ &dummy_system< 1 > } );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )
	export API void order_after_adds_ordering_base()
	{
		std::unique_ptr< ge::refl::registry_data > reg = build_registry(
			[]( module_builder& builder )
			{
				builder.begin_func< &dummy_system< 0 > >( "system0" )
					.add_traits( ge::traits::system{}, ge::traits::order_after< &dummy_system< 2 > >{} )
					.end_func();
			} );

		const auto& ordering = find_trait< ge::traits::details::ordering_base >( find_func( *reg, "system0" ).m_traits );

		expect::is_eq( ordering.m_type, ge::traits::details::ordering_base::after );
		expect::is_eq( ordering.m_point, ge::traits::details::sequence_point{ &dummy_system< 2 > } );
	}
} // namespace ordering_trait_tests
