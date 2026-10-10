export module test_export_core:test_rel_ptr;

import stl;
import export_core;
export import test_core;

using namespace ge::test_core;

namespace
{
	struct with_member
	{
		int m_value{};
	};

	struct self_referencing
	{
		int m_value{};
		rel::ptr< int > m_ptr{};
	};

	struct self_referencing_span
	{
		std::array< int, 3 > m_values{};
		rel::span< int > m_span{};
	};

	// Copies the object representation of T into dst, like relocating an exported pack would
	template< typename T >
	T& memcpy_to( std::span< std::byte > dst, const T& src )
	{
		std::memcpy( dst.data(), &src, sizeof( T ) );
		return *std::launder( reinterpret_cast< T* >( dst.data() ) );
	}

#ifdef NDEBUG
	// TODO Compilation failure
	rel::ptr< int > pass_through( rel::ptr< int > ptr )
	{
		return ptr;
	}
#endif
} // namespace

namespace rel_ptr_tests
{
	REFL_FUNC( ge::test_core::unit_test_trait{} )
	export API void default_constructed_is_null()
	{
		rel::ptr< int > ptr{};

		expect::is_null( ptr );
		expect::is_null( ptr.get() );
		expect::is_false( static_cast< bool >( ptr ) );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )
	export API void constructed_from_pointer_points_to_target()
	{
		with_member target{ .m_value = 42 };
		rel::ptr< with_member > ptr{ &target };

		expect::is_eq( ptr.get(), &target );
		expect::is_not_null( ptr );
		expect::is_true( static_cast< bool >( ptr ) );
		expect::is_eq( ( *ptr ).m_value, 42 );
		expect::is_eq( ptr->m_value, 42 );

		ptr->m_value = 7;
		expect::is_eq( target.m_value, 7 );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )
	export API void reset_makes_null()
	{
		int target = 1;
		rel::ptr< int > ptr{ &target };

		ptr.reset();

		expect::is_null( ptr );
		expect::is_null( ptr.get() );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )
	export API void copy_construct_points_to_same_target()
	{
#ifdef NDEBUG
		// TODO Compilation failure
		int target = 1;
		rel::ptr< int > original{ &target };
		rel::ptr< int > copy{ original };

		expect::is_eq( copy.get(), &target );
		expect::is_eq( original.get(), &target );
#endif
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )
	export API void copy_assign_points_to_same_target()
	{
#ifdef NDEBUG
		// TODO Compilation failure
		int target = 1;
		int other = 2;
		rel::ptr< int > original{ &target };
		rel::ptr< int > copy{ &other };

		copy = original;

		expect::is_eq( copy.get(), &target );
		expect::is_eq( original.get(), &target );
#endif
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )
	export API void pass_and_return_by_value_points_to_same_target()
	{
#ifdef NDEBUG
		// TODO Compilation failure
		int target = 1;
		rel::ptr< int > original{ &target };

		rel::ptr< int > result = pass_through( original );

		expect::is_eq( result.get(), &target );
#endif
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )
	export API void move_construct_takes_target_and_resets_source()
	{
		int target = 1;
		rel::ptr< int > original{ &target };
		rel::ptr< int > moved{ std::move( original ) };

		expect::is_eq( moved.get(), &target );
		expect::is_null( original );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )
	export API void move_assign_takes_target_and_resets_source()
	{
		int target = 1;
		int other = 2;
		rel::ptr< int > original{ &target };
		rel::ptr< int > moved{ &other };

		moved = std::move( original );

		expect::is_eq( moved.get(), &target );
		expect::is_null( original );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )
	export API void converting_construct_to_const_points_to_same_target()
	{
		int target = 1;
		rel::ptr< int > original{ &target };
		rel::ptr< const int > as_const{ original };

		expect::is_eq( as_const.get(), &target );
		expect::is_eq( *as_const, 1 );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )
	export API void converting_assign_to_const_points_to_same_target()
	{
#ifdef NDEBUG
		// TODO Compilation failure
		int target = 1;
		rel::ptr< int > original{ &target };
		rel::ptr< const int > as_const{};

		as_const = original;

		expect::is_eq( as_const.get(), &target );
#endif
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )
	export API void comparison_orders_by_address()
	{
		std::array< int, 2 > values{ 1, 2 };
		rel::ptr< int > first{ &values[ 0 ] };
		rel::ptr< int > second{ &values[ 1 ] };
		rel::ptr< int > also_first{ &values[ 0 ] };

		expect::is_true( first < second );
		expect::is_true( second > first );
		expect::is_true( ( first <=> also_first ) == 0 );
		expect::is_true( ( first <=> second ) < 0 );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )
	export API void plus_advances_by_elements()
	{
		std::array< int, 8 > values{ 10, 11, 12, 13, 14, 15, 16, 17 };
		rel::ptr< int > ptr{ &values[ 0 ] };

		rel::ptr< int > advanced = ptr + 2;

		expect::is_eq( advanced.get(), &values[ 2 ] );
		expect::is_eq( ptr.get(), &values[ 0 ] );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )
	export API void minus_moves_back_by_elements()
	{
#ifdef NDEBUG
		// TODO Compilation failure
		std::array< int, 8 > values{ 10, 11, 12, 13, 14, 15, 16, 17 };
		rel::ptr< int > ptr{ &values[ 3 ] };

		rel::ptr< int > moved_back = ptr - 2;

		expect::is_eq( moved_back.get(), &values[ 1 ] );
		expect::is_eq( ptr.get(), &values[ 3 ] );
#endif
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )
	export API void compound_assignment_moves_by_elements()
	{
		std::array< int, 8 > values{ 10, 11, 12, 13, 14, 15, 16, 17 };
		rel::ptr< int > ptr{ &values[ 0 ] };

		ptr += 3;
		expect::is_eq( ptr.get(), &values[ 3 ] );

		ptr -= 2;
		expect::is_eq( ptr.get(), &values[ 1 ] );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )
	export API void prefix_increment_and_decrement_move_by_one_element()
	{
		std::array< int, 4 > values{ 10, 11, 12, 13 };
		rel::ptr< int > ptr{ &values[ 1 ] };

		rel::ptr< int >& incremented = ++ptr;
		expect::is_eq( &incremented, &ptr );
		expect::is_eq( ptr.get(), &values[ 2 ] );

		rel::ptr< int >& decremented = --ptr;
		expect::is_eq( &decremented, &ptr );
		expect::is_eq( ptr.get(), &values[ 1 ] );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )
	export API void postfix_increment_returns_old_and_advances()
	{
#ifdef NDEBUG
		// TODO Compilation failure
		std::array< int, 4 > values{ 10, 11, 12, 13 };
		rel::ptr< int > ptr{ &values[ 1 ] };

		rel::ptr< int > old = ptr++;

		expect::is_eq( old.get(), &values[ 1 ] );
		expect::is_eq( ptr.get(), &values[ 2 ] );
#endif
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )
	export API void postfix_decrement_returns_old_and_moves_back()
	{
#ifdef NDEBUG
		// TODO Compilation failure
		std::array< int, 4 > values{ 10, 11, 12, 13 };
		rel::ptr< int > ptr{ &values[ 1 ] };

		rel::ptr< int > old = ptr--;

		expect::is_eq( old.get(), &values[ 1 ] );
		expect::is_eq( ptr.get(), &values[ 0 ] );
#endif
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )
	export API void memcpy_relocated_ptr_points_into_relocated_object()
	{
		self_referencing original{ .m_value = 5 };
		original.m_ptr = rel::ptr< int >{ &original.m_value };

		alignas( self_referencing ) std::array< std::byte, sizeof( self_referencing ) > relocated_storage{};
		self_referencing& relocated = memcpy_to( relocated_storage, original );

		expect::is_eq( relocated.m_ptr.get(), &relocated.m_value );

		relocated.m_value = 9;
		expect::is_eq( *relocated.m_ptr, 9 );
		expect::is_eq( *original.m_ptr, 5 );
	}
} // namespace rel_ptr_tests

namespace rel_span_tests
{
	REFL_FUNC( ge::test_core::unit_test_trait{} )
	export API void default_constructed_is_empty()
	{
		rel::span< int > span{};

		expect::is_true( span.empty() );
		expect::is_eq( span.size(), 0ull );
		expect::is_eq( span.ssize(), 0ll );
		expect::is_null( span.data() );
		expect::is_eq( span.begin(), span.end() );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )
	export API void constructed_from_pointer_and_size_views_elements()
	{
		std::array< int, 4 > values{ 10, 11, 12, 13 };
		rel::span< int > span{ values.data(), values.size() };

		expect::is_false( span.empty() );
		expect::is_eq( span.size(), 4ull );
		expect::is_eq( span.ssize(), 4ll );
		expect::is_eq( span.data(), values.data() );
		expect::is_eq( span.begin(), values.data() );
		expect::is_eq( span.end(), values.data() + 4 );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )
	export API void constructed_from_std_span_views_elements()
	{
		std::array< int, 3 > values{ 10, 11, 12 };
		rel::span< int > span{ std::span< int >{ values } };

		expect::is_eq( span.size(), 3ull );
		expect::is_eq( span.data(), values.data() );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )
	export API void index_returns_element()
	{
#ifdef NDEBUG
		// TODO Compilation failure
		std::array< int, 4 > values{ 10, 11, 12, 13 };
		rel::span< int > span{ values.data(), values.size() };

		for( size_t i = 0; i < values.size(); i++ )
		{
			expect::is_eq( &span[ i ], &values[ i ] );
		}

		span[ 2 ] = 99;
		expect::is_eq( values[ 2 ], 99 );
#endif
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )
	export API void range_for_visits_all_elements_in_order()
	{
		std::array< int, 4 > values{ 10, 11, 12, 13 };
		rel::span< int > span{ values.data(), values.size() };

		std::vector< int > visited{};
		for( int value : span )
		{
			visited.push_back( value );
		}

		expect::is_true( std::ranges::equal( visited, values ) );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )
	export API void copy_views_same_elements()
	{
#ifdef NDEBUG
		// TODO Compilation failure
		std::array< int, 4 > values{ 10, 11, 12, 13 };
		rel::span< int > original{ values.data(), values.size() };
		rel::span< int > copy = original;

		expect::is_eq( copy.size(), 4ull );
		expect::is_eq( copy.data(), values.data() );
#endif
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )
	export API void sub_span_views_part_of_elements()
	{
#ifdef NDEBUG
		// TODO Compilation failure
		std::array< int, 6 > values{ 10, 11, 12, 13, 14, 15 };
		rel::span< int > span{ values.data(), values.size() };

		rel::span< int > sub = span.sub_span( 1, 3 );

		expect::is_eq( sub.size(), 3ull );
		expect::is_eq( sub.data(), &values[ 1 ] );
		expect::is_eq( sub[ 2 ], 13 );
#endif
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )
	export API void memcpy_relocated_span_views_relocated_elements()
	{
		self_referencing_span original{ .m_values = { 10, 11, 12 } };
		original.m_span = rel::span< int >{ original.m_values.data(), original.m_values.size() };

		alignas( self_referencing_span ) std::array< std::byte, sizeof( self_referencing_span ) > relocated_storage{};
		self_referencing_span& relocated = memcpy_to( relocated_storage, original );

		expect::is_eq( relocated.m_span.size(), 3ull );
		expect::is_eq( relocated.m_span.data(), relocated.m_values.data() );

		relocated.m_values[ 1 ] = 99;
		expect::is_true( std::ranges::equal( relocated.m_span, std::array< int, 3 >{ { 10, 99, 12 } } ) );
		expect::is_true( std::ranges::equal( original.m_span, std::array< int, 3 >{ { 10, 11, 12 } } ) );
	}
} // namespace rel_span_tests
