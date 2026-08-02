export module test_runtime_reflection;

import stl;
import runtime_reflection;
export import test_core;

using namespace ge::test_core;
using namespace ge::test_core::assert;

namespace
{
	struct type_1
	{
	};

	struct type_2
	{
	};

	struct tag_a : ge::refl::func_trait
	{
		int m_id{};
	};

	struct tag_b : ge::refl::func_trait
	{
		std::string_view m_label{};
	};

	struct tag_unused : ge::refl::func_trait
	{
	};

	struct tag_derived_a : tag_a
	{
	};

	struct type_tag : ge::refl::type_trait
	{
		int m_id{};
	};

	struct data_tag : ge::refl::data_trait
	{
		int m_id{};
	};

	void fixture_func_0()
	{
	}

	void fixture_func_1()
	{
	}

	void fixture_func_2()
	{
	}

	int free_square( int x )
	{
		return x * x;
	}

	struct fixture_holder
	{
		int m_value{};
	};

	struct fpoint
	{
		int x;
		int y;
		bool operator==( const fpoint& ) const = default;
	};

	struct entity
	{
		int hp;
		int mp;
		fpoint pos;
	};

	const int& free_get_hp_cref( const entity& e )
	{
		return e.hp;
	}

	int free_double_hp( const entity& e )
	{
		return e.hp * 2;
	}

	void free_clamp_hp( entity& e, int new_hp )
	{
		e.hp = new_hp < 0 ? 0 : ( new_hp > 1000 ? 1000 : new_hp );
	}

	struct empty_type_trait : ge::refl::type_trait
	{
	};

	struct int_type_trait : ge::refl::type_trait
	{
		int payload{};
	};

	struct int_func_trait : ge::refl::func_trait
	{
		int payload{};
	};

	struct int_data_trait : ge::refl::data_trait
	{
		int payload{};
	};

	struct string_type_trait : ge::refl::type_trait
	{
		std::string payload{};
	};

	struct destruct_counting_trait : ge::refl::type_trait
	{
		destruct_counting_trait( int* c )
			: counter( c )
		{
		}

		destruct_counting_trait( const destruct_counting_trait& other )
			: counter( other.counter )
		{
		}

		~destruct_counting_trait()
		{
			if( counter != nullptr )
			{
				++*counter;
			}
		}

		int* counter{};
	};

	struct move_only_int_trait : ge::refl::type_trait
	{
		explicit move_only_int_trait( int v )
			: p( std::make_unique< int >( v ) )
		{
		}

		std::unique_ptr< int > p{};
	};

	struct hook_record
	{
		int on_apply_type{};
		int on_apply_func{};
		int on_apply_data{};
		int post_build_type{};
		int post_build_func{};
		int post_build_data{};
		int post_build_count_at_first_apply{ -1 };
		int on_apply_count_at_first_post_build{ -1 };
		std::string_view last_type_name{};
		std::string_view last_func_name{};
		std::string_view last_data_name{};
		std::string_view last_data_outer_name{};
	};

	struct hooked_trait : ge::refl::type_trait, ge::refl::func_trait, ge::refl::data_trait
	{
		hooked_trait( hook_record* r )
			: rec( r )
		{
		}

		void note_apply()
		{
			if( rec->post_build_count_at_first_apply == -1 )
			{
				rec->post_build_count_at_first_apply =
					rec->post_build_type + rec->post_build_func + rec->post_build_data;
			}
		}

		void note_post_build()
		{
			if( rec->on_apply_count_at_first_post_build == -1 )
			{
				rec->on_apply_count_at_first_post_build =
					rec->on_apply_type + rec->on_apply_func + rec->on_apply_data;
			}
		}

		template<typename T>
		void on_apply( const ge::refl::builders::type_builder< T >& )
		{
			note_apply();
			++rec->on_apply_type;
		}

		template<auto FuncPtr>
		void on_apply( const ge::refl::builders::func_builder< FuncPtr >& )
		{
			note_apply();
			++rec->on_apply_func;
		}

		template<auto DataPtr>
		void on_apply( const ge::refl::builders::data_builder< DataPtr >& )
		{
			note_apply();
			++rec->on_apply_data;
		}

		void post_build( const ge::refl::type_data& type )
		{
			note_post_build();
			++rec->post_build_type;
			rec->last_type_name = type.m_name;
		}

		void post_build( const ge::refl::func_data& func )
		{
			note_post_build();
			++rec->post_build_func;
			rec->last_func_name = func.m_name;
		}

		void post_build( const ge::refl::data_data& data )
		{
			note_post_build();
			++rec->post_build_data;
			rec->last_data_name = data.m_name;
			rec->last_data_outer_name = data.m_outer_type.get().m_name;
		}

		hook_record* rec{};
	};

	const ge::refl::type_data& find_type( const ge::refl::registry_data& reg, ge::refl::type_id id )
	{
		const ge::refl::type_data* found = std::ranges::find_if(
			reg.m_types,
			[id]( const ge::refl::type_data& type )
			{
				return type.m_id == id;
			} );
		is_true( found != reg.m_types.end() );
		return *found;
	}

	void test_big_five( ge::refl::value value_1, auto check )
	{
		check( value_1 );

		ge::refl::value copy = value_1;
		check( copy );

		ge::refl::value moved = std::move( value_1 );
		check( moved );

		ge::refl::value copy_assigned{};
		copy_assigned = moved;
		check( copy_assigned );

		ge::refl::value move_assigned{};
		move_assigned = std::move( moved );
		check( move_assigned );
	}
}

namespace compile_time_tests
{
	REFL_FUNC( ge::test_core::unit_test_trait{} )

	export API void type_utilities()
	{
		using namespace ge::refl;

		static_assert( std::is_same_v< func_sig_t< int( * )( int, int ) >, func_sig< int( int, int ) > > );
		static_assert( std::is_same_v< func_sig_t< int( & )( int ) >, func_sig< int( int ) > > );
		static_assert( std::is_same_v< func_sig_t< void() >, func_sig< void() > > );
		static_assert( std::is_same_v< func_sig_t< int( int, double, char ) >, func_sig< int( int, double, char ) > > );

		struct member_func_owner
		{
		};
		static_assert(
			std::is_same_v< func_sig_t< int( member_func_owner::* )( int, double ) >, func_sig< int(
				                member_func_owner&,
				                int,
				                double ) > > );
		static_assert(
			std::is_same_v< func_sig_t< int( member_func_owner::* )() const >, func_sig< int( const member_func_owner& ) > > );
		static_assert( std::is_same_v< func_sig_t< int( member_func_owner::* )() && >, func_sig< int( member_func_owner&& ) > > );

		static_assert( std::is_same_v< remove_decoration_t< int >, int > );
		static_assert( std::is_same_v< remove_decoration_t< int& >, int > );
		static_assert( std::is_same_v< remove_decoration_t< const int& >, int > );
		static_assert( std::is_same_v< remove_decoration_t< int* >, int > );
		static_assert( std::is_same_v< remove_decoration_t< const int >, int > );
		static_assert( std::is_same_v< remove_decoration_t< volatile int >, int > );

		static_assert( make_type_id< int >() == make_type_id< remove_decoration_t< int& > >() );
		static_assert( make_type_id< int >() == make_type_id< remove_decoration_t< const int& > >() );
		static_assert( make_type_id< int >() == make_type_id< remove_decoration_t< int* > >() );

		static_assert( supported_param_type< int > );
		static_assert( supported_param_type< int& > );
		static_assert( supported_param_type< const int& > );
		static_assert( !supported_param_type< int* > );
		static_assert( !supported_param_type< const int* > );
	}
}

namespace query_tests
{
	REFL_FUNC( ge::test_core::unit_test_trait{} )

	export API void empty_range_yields_nothing()
	{
		const std::span< const ge::refl::func_data > empty{};

		ge::refl::func_query plain{ empty };
		is_true( plain.begin() == plain.end() );

		ge::refl::func_query::with< tag_a > with_query{ empty };
		is_true( with_query.begin() == with_query.end() );

		ge::refl::func_query::read< tag_a > read_query{ empty };
		is_true( read_query.begin() == read_query.end() );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )

	export API void unfiltered_query_yields_all_in_order()
	{
		ge::refl::builders::endable_registry_builder builder = ge::refl::builders::begin_registry();
		builder.begin_module( "m" )
		       .begin_func< &fixture_func_0 >( "f0" ).end_func()
		       .begin_func< &fixture_func_1 >( "f1" ).end_func()
		       .begin_func< &fixture_func_2 >( "f2" ).end_func()
		       .end_module();
		const std::unique_ptr< ge::refl::registry_data > reg = std::move( builder ).build();

		ge::refl::func_query query{ reg->m_funcs };

		const std::array< std::string_view, 3 > expected_names{ "f0", "f1", "f2" };
		size_t index = 0;
		for( const auto& [ func ] : query )
		{
			is_eq( func.m_name, expected_names[ index ] );
			// The handle element is a reference straight into the source range.
			is_true( &func == reg->m_funcs.data() + index );
			index++;
		}
		is_eq( index, 3ull );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )

	export API void func_without_traits_never_matches_with()
	{
		ge::refl::builders::endable_registry_builder builder = ge::refl::builders::begin_registry();
		builder.begin_module( "m" )
		       .begin_func< &fixture_func_0 >( "f0" ).end_func()
		       .begin_func< &fixture_func_1 >( "f1" ).end_func()
		       .end_module();
		const std::unique_ptr< ge::refl::registry_data > reg = std::move( builder ).build();

		ge::refl::func_query::with< tag_a > query{ reg->m_funcs };
		is_true( query.begin() == query.end() );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )

	export API void with_filters_to_subset_preserving_order()
	{
		ge::refl::builders::endable_registry_builder builder = ge::refl::builders::begin_registry();
		builder.begin_module( "m" )
		       .begin_func< &fixture_func_0 >( "f0" ).add_traits( tag_a{ .m_id = 0 } ).end_func()
		       .begin_func< &fixture_func_1 >( "f1" ).end_func()
		       .begin_func< &fixture_func_2 >( "f2" ).add_traits( tag_a{ .m_id = 2 } ).end_func()
		       .end_module();
		const std::unique_ptr< ge::refl::registry_data > reg = std::move( builder ).build();

		ge::refl::func_query::with< tag_a > query{ reg->m_funcs };

		std::vector< std::string_view > names{};
		for( const auto& [ func ] : query )
		{
			names.push_back( func.m_name );
		}
		is_eq( names.size(), 2ull );
		is_eq( names[ 0 ], "f0" );
		is_eq( names[ 1 ], "f2" );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )

	export API void with_no_match_yields_empty()
	{
		ge::refl::builders::endable_registry_builder builder = ge::refl::builders::begin_registry();
		builder.begin_module( "m" )
		       .begin_func< &fixture_func_0 >( "f0" ).add_traits( tag_a{ .m_id = 1 } ).end_func()
		       .end_module();
		const std::unique_ptr< ge::refl::registry_data > reg = std::move( builder ).build();

		ge::refl::func_query::with< tag_unused > query{ reg->m_funcs };
		is_true( query.begin() == query.end() );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )

	export API void read_binds_reference_to_stored_trait()
	{
		ge::refl::builders::endable_registry_builder builder = ge::refl::builders::begin_registry();
		builder.begin_module( "m" )
		       .begin_func< &fixture_func_0 >( "f" ).add_traits( tag_a{ .m_id = 42 } ).end_func()
		       .end_module();
		const std::unique_ptr< ge::refl::registry_data > reg = std::move( builder ).build();

		ge::refl::func_query::read< tag_a > query{ reg->m_funcs };

		size_t count = 0;
		for( const auto& [ func, bound ] : query )
		{
			is_eq( func.m_name, "f" );
			is_eq( bound.m_id, 42 );
			// The read element aliases the value owned by the registry.
			is_true( &bound == func.m_traits.front().as_constant< tag_a >() );
			is_false( func.m_traits.front().is_mutable() );
			count++;
		}
		is_eq( count, 1ull );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )

	export API void read_no_match_yields_empty_and_does_not_bind()
	{
		ge::refl::builders::endable_registry_builder builder = ge::refl::builders::begin_registry();
		builder.begin_module( "m" )
		       .begin_func< &fixture_func_0 >( "f" ).add_traits( tag_b{ .m_label = "only b" } ).end_func()
		       .end_module();
		const std::unique_ptr< ge::refl::registry_data > reg = std::move( builder ).build();

		// matches() is evaluated before the element is constructed, so a non-matching
		// func never reaches read_element's unchecked find_if dereference.
		ge::refl::func_query::read< tag_a > query{ reg->m_funcs };
		is_true( query.begin() == query.end() );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )

	export API void read_binds_first_of_duplicate_traits()
	{
		ge::refl::builders::endable_registry_builder builder = ge::refl::builders::begin_registry();
		builder.begin_module( "m" )
		       .begin_func< &fixture_func_0 >( "f" ).add_traits( tag_a{ .m_id = 1 }, tag_a{ .m_id = 2 } ).end_func()
		       .end_module();
		const std::unique_ptr< ge::refl::registry_data > reg = std::move( builder ).build();

		ge::refl::func_query::read< tag_a > query{ reg->m_funcs };

		size_t count = 0;
		for( const auto& [ func, bound ] : query )
		{
			is_eq( func.m_traits.size(), 2ull );
			is_eq( bound.m_id, 1 );
			is_true( &bound == func.m_traits.front().as_constant< tag_a >() );
			count++;
		}
		is_eq( count, 1ull );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )

	export API void chained_with_and_read_filters_on_both()
	{
		ge::refl::builders::endable_registry_builder builder = ge::refl::builders::begin_registry();
		builder.begin_module( "m" )
		       .begin_func< &fixture_func_0 >( "f0" ).add_traits( tag_b{ .m_label = "zero" }, tag_a{ .m_id = 0 } ).end_func()
		       .begin_func< &fixture_func_1 >( "f1" ).add_traits( tag_b{ .m_label = "one" } ).end_func()
		       .begin_func< &fixture_func_2 >( "f2" ).add_traits( tag_a{ .m_id = 2 }, tag_b{ .m_label = "two" } ).end_func()
		       .end_module();
		const std::unique_ptr< ge::refl::registry_data > reg = std::move( builder ).build();

		ge::refl::func_query::with< tag_a >::read< tag_b > query{ reg->m_funcs };

		// with<> adds a filter but no binding; the arity stays [func, tag_b].
		std::vector< std::string_view > labels{};
		for( const auto& [ func, b ] : query )
		{
			is_false( func.m_name.empty() );
			labels.push_back( b.m_label );
		}
		is_eq( labels.size(), 2ull );
		is_eq( labels[ 0 ], "zero" );
		is_eq( labels[ 1 ], "two" );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )

	export API void multiple_reads_bind_in_chain_order()
	{
		ge::refl::builders::endable_registry_builder builder = ge::refl::builders::begin_registry();
		builder.begin_module( "m" )
		       .begin_func< &fixture_func_0 >( "f" ).add_traits( tag_a{ .m_id = 4 }, tag_b{ .m_label = "label" } ).end_func()
		       .end_module();
		const std::unique_ptr< ge::refl::registry_data > reg = std::move( builder ).build();

		ge::refl::func_query::read< tag_a >::read< tag_b > query{ reg->m_funcs };

		using elem_t = std::remove_cvref_t< decltype(*query.begin()) >;
		static_assert( std::tuple_size_v< elem_t > == 3 );
		static_assert( std::is_same_v< std::tuple_element_t< 0, elem_t >, const ge::refl::func_data& > );
		static_assert( std::is_same_v< std::tuple_element_t< 1, elem_t >, const tag_a& > );
		static_assert( std::is_same_v< std::tuple_element_t< 2, elem_t >, const tag_b& > );

		size_t count = 0;
		for( const auto& [ func, a, b ] : query )
		{
			is_eq( func.m_name, "f" );
			is_eq( a.m_id, 4 );
			is_eq( b.m_label, "label" );
			is_true( &a == func.m_traits[ 0 ].as_constant< tag_a >() );
			is_true( &b == func.m_traits[ 1 ].as_constant< tag_b >() );
			count++;
		}
		is_eq( count, 1ull );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )

	export API void read_and_with_require_all_traits_present()
	{
		ge::refl::builders::endable_registry_builder builder = ge::refl::builders::begin_registry();
		builder.begin_module( "m" )
		       .begin_func< &fixture_func_0 >( "only_a" ).add_traits( tag_a{ .m_id = 1 } ).end_func()
		       .begin_func< &fixture_func_1 >( "only_b" ).add_traits( tag_b{ .m_label = "b" } ).end_func()
		       .begin_func< &fixture_func_2 >( "both" ).add_traits( tag_a{ .m_id = 2 }, tag_b{ .m_label = "b" } ).end_func()
		       .end_module();
		const std::unique_ptr< ge::refl::registry_data > reg = std::move( builder ).build();

		ge::refl::func_query::read< tag_a >::read< tag_b > query{ reg->m_funcs };

		size_t count = 0;
		for( const auto& [ func, a, b ] : query )
		{
			is_eq( func.m_name, "both" );
			is_eq( a.m_id, 2 );
			is_eq( b.m_label, "b" );
			count++;
		}
		is_eq( count, 1ull );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )

	export API void query_is_reiterable()
	{
		ge::refl::builders::endable_registry_builder builder = ge::refl::builders::begin_registry();
		builder.begin_module( "m" )
		       .begin_func< &fixture_func_0 >( "f0" ).add_traits( tag_a{ .m_id = 0 } ).end_func()
		       .begin_func< &fixture_func_1 >( "f1" ).end_func()
		       .begin_func< &fixture_func_2 >( "f2" ).add_traits( tag_a{ .m_id = 2 } ).end_func()
		       .end_module();
		const std::unique_ptr< ge::refl::registry_data > reg = std::move( builder ).build();

		ge::refl::func_query::with< tag_a > query{ reg->m_funcs };

		std::vector< std::string_view > first_pass{};
		for( const auto& [ func ] : query )
		{
			first_pass.push_back( func.m_name );
		}

		std::vector< std::string_view > second_pass{};
		for( const auto& [ func ] : query )
		{
			second_pass.push_back( func.m_name );
		}

		is_eq( first_pass.size(), 2ull );
		is_true( first_pass == second_pass );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )

	export API void with_uses_exact_type_match_not_is_a()
	{
		// Codifies current exact-match behavior; query.ixx has
		// "TODO this should probably be an is_a?" - update this test when is_a matching lands.
		ge::refl::builders::endable_registry_builder builder = ge::refl::builders::begin_registry();
		builder.begin_module( "m" )
		       .begin_func< &fixture_func_0 >( "derived_only" ).add_traits( tag_derived_a{} ).end_func()
		       .end_module();
		const std::unique_ptr< ge::refl::registry_data > reg = std::move( builder ).build();

		ge::refl::func_query::with< tag_a > base_with{ reg->m_funcs };
		is_true( base_with.begin() == base_with.end() );

		ge::refl::func_query::read< tag_a > base_read{ reg->m_funcs };
		is_true( base_read.begin() == base_read.end() );

		ge::refl::func_query::with< tag_derived_a > derived_with{ reg->m_funcs };

		size_t count = 0;
		for( const auto& [ func ] : derived_with )
		{
			is_eq( func.m_name, "derived_only" );
			count++;
		}
		is_eq( count, 1ull );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )

	export API void type_query_alias_works()
	{
		ge::refl::builders::endable_registry_builder builder = ge::refl::builders::begin_registry();
		builder.begin_module( "m" )
		       .begin_type< type_1 >( "untagged" ).end_type()
		       .begin_type< fixture_holder >( "fixture_holder" ).add_traits( type_tag{ .m_id = 7 } ).end_type()
		       .end_module();
		const std::unique_ptr< ge::refl::registry_data > reg = std::move( builder ).build();

		ge::refl::type_query::read< type_tag > query{ reg->m_types };

		size_t count = 0;
		for( const auto& [ type, tag ] : query )
		{
			is_eq( type.m_name, "fixture_holder" );
			is_true( type.m_id == ge::refl::make_type_id< fixture_holder >() );
			is_eq( tag.m_id, 7 );
			count++;
		}
		is_eq( count, 1ull );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )

	export API void data_query_alias_works_over_registry()
	{
		ge::refl::builders::endable_registry_builder builder = ge::refl::builders::begin_registry();
		builder.begin_module( "m" )
		       // The member's type must be registered too: build() resolves it unchecked.
		       .begin_type< int >( "int" ).end_type()
		       .begin_type< fixture_holder >( "fixture_holder" )
		       .begin_data< &fixture_holder::m_value >( "m_value" ).add_traits( data_tag{ .m_id = 5 } ).end_data()
		       .end_type()
		       .end_module();
		const std::unique_ptr< ge::refl::registry_data > reg = std::move( builder ).build();

		ge::refl::data_query::read< data_tag > query{ reg->m_datas };

		size_t count = 0;
		for( const auto& [ data, tag ] : query )
		{
			is_eq( data.m_name, "m_value" );
			is_eq( data.m_outer_type.get().m_name, "fixture_holder" );
			is_eq( tag.m_id, 5 );
			count++;
		}
		is_eq( count, 1ull );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )

	export API void query_over_registry_built_funcs()
	{
		// Mirror of how the test runner itself discovers unit tests.
		ge::refl::builders::endable_registry_builder builder = ge::refl::builders::begin_registry();
		builder.begin_module( "m" )
		       .begin_func< &fixture_func_0 >( "tagged" ).add_traits( tag_a{ .m_id = 7 } ).end_func()
		       .begin_func< &fixture_func_1 >( "plain" ).end_func()
		       .end_module();
		const std::unique_ptr< ge::refl::registry_data > reg = std::move( builder ).build();

		ge::refl::func_query::read< tag_a > query{ reg->m_funcs };

		size_t count = 0;
		for( const auto& [ func, bound ] : query )
		{
			is_eq( func.m_name, "tagged" );
			is_eq( bound.m_id, 7 );
			count++;
		}
		is_eq( count, 1ull );
	}
}

namespace value_tests
{
	REFL_FUNC( ge::test_core::unit_test_trait{} )

	export API void view_lifetime()
	{
		int expected = 42;

		auto check = [&expected]( const ge::refl::value& v )
		{
			is_not_null( v.const_data() );
			is_eq( v.const_data(), &expected );
		};

		test_big_five( ge::refl::value::create_view( expected ), check );
		is_eq( expected, 42 );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )

	export API void ref_lifetime()
	{
		int expected = 42;

		auto check = [&expected]( const ge::refl::value& v )
		{
			is_not_null( v.const_data() );
			is_eq( v.const_data(), &expected );
		};

		test_big_five( ge::refl::value::create_ref( expected ), check );
		is_eq( expected, 42 );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )

	export API void owning_lifetime()
	{
		int expected = 42;

		auto check = [&expected]( const ge::refl::value& v )
		{
			is_not_null( v.const_data() );
			is_eq( *v.as_constant< int >(), expected );
		};

		test_big_five( ge::refl::value::create_owning( expected ), check );
		is_eq( expected, 42 );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )

	export API void default_constructed_is_null()
	{
		ge::refl::value v{};
		is_null( v.const_data() );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )

	export API void as_typed_access()
	{
		int x = 42;

		ge::refl::value view = ge::refl::value::create_view( x );
		is_not_null( view.as_constant< int >() );
		is_eq( *view.as_constant< int >(), 42 );

		ge::refl::value ref = ge::refl::value::create_ref( x );
		is_not_null( ref.as_constant< int >() );
		is_not_null( ref.as_mutable< int >() );
		is_eq( *ref.as_constant< int >(), 42 );
		is_eq( static_cast< void* >( ref.as_mutable< int >() ), ref.mutable_data() );

		ge::refl::value owning = ge::refl::value::create_owning( 42 );
		is_not_null( owning.as_constant< int >() );
		is_not_null( owning.as_mutable< int >() );
		is_eq( *owning.as_constant< int >(), 42 );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )

	export API void owning_is_deep_copy()
	{
		ge::refl::value original = ge::refl::value::create_owning( 42 );
		ge::refl::value copy = original;

		*copy.as_mutable< int >() = 99;

		is_eq( *original.as_constant< int >(), 42 );
		is_eq( *copy.as_constant< int >(), 99 );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )

	export API void ref_mutates_source()
	{
		int x = 10;
		ge::refl::value ref = ge::refl::value::create_ref( x );
		*ref.as_mutable< int >() = 7;
		is_eq( x, 7 );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )

	export API void move_leaves_source_empty()
	{
		int x = 42;
		ge::refl::value src = ge::refl::value::create_view( x );
		is_not_null( src.const_data() );

		ge::refl::value dst = std::move( src );
		is_null( src.const_data() );
		is_not_null( dst.const_data() );

		int y = 7;
		ge::refl::value src2 = ge::refl::value::create_view( y );
		ge::refl::value dst2{};
		dst2 = std::move( src2 );
		is_null( src2.const_data() );
		is_not_null( dst2.const_data() );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )

	export API void pointer_overloads()
	{
		int x = 99;

		ge::refl::value view = ge::refl::value::create_view( &x );
		is_eq( view.const_data(), static_cast< const void* >( &x ) );

		ge::refl::value ref = ge::refl::value::create_ref( &x );
		is_eq( ref.const_data(), static_cast< const void* >( &x ) );
		is_eq( ref.mutable_data(), static_cast< void* >( &x ) );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )

	export API void clear_resets_value()
	{
		ge::refl::value owning = ge::refl::value::create_owning( 42 );
		is_not_null( owning.const_data() );

		owning.clear();
		is_null( owning.const_data() );

		owning.clear();
		is_null( owning.const_data() );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )

	export API void owning_non_trivial_type()
	{
		int counter = 0;
		int inside_scope = 0;

		struct destruct_tracker
		{
			int* counter;

			destruct_tracker( int* c )
				: counter( c )
			{
			}

			destruct_tracker( const destruct_tracker& other )
				: counter( other.counter )
			{
			}

			~destruct_tracker()
			{
				++( *counter );
			}
		};

		{
			ge::refl::value v = ge::refl::value::create_owning( destruct_tracker{ &counter } );
			inside_scope = counter;
		}
		is_gt( counter, inside_scope );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )

	export API void null_pointer_view()
	{
		ge::refl::value v = ge::refl::value::create_view( static_cast< const int* >( nullptr ) );
		is_null( v.const_data() );

		ge::refl::value copy = v;
		is_null( copy.const_data() );

		ge::refl::value moved = std::move( v );
		is_null( moved.const_data() );
		is_null( v.const_data() );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )

	export API void null_pointer_ref()
	{
		ge::refl::value v = ge::refl::value::create_ref( static_cast< int* >( nullptr ) );
		is_null( v.const_data() );
		is_null( v.mutable_data() );

		ge::refl::value copy = v;
		is_null( copy.const_data() );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )

	export API void pointer_and_reference_equal()
	{
		int x = 7;

		ge::refl::value from_ref = ge::refl::value::create_view( x );
		ge::refl::value from_ptr = ge::refl::value::create_view( &x );
		is_eq( from_ref.const_data(), from_ptr.const_data() );

		ge::refl::value ref_ref = ge::refl::value::create_ref( x );
		ge::refl::value ref_ptr = ge::refl::value::create_ref( &x );
		is_eq( ref_ref.mutable_data(), ref_ptr.mutable_data() );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )

	export API void struct_pointer_overload()
	{
		struct two_ints
		{
			int a;
			int b;
		};
		two_ints p{ 3, 4 };

		ge::refl::value view = ge::refl::value::create_view( &p );
		is_eq( view.const_data(), static_cast< const void* >( &p ) );
		is_eq( view.as_constant< two_ints >()->a, 3 );
		is_eq( view.as_constant< two_ints >()->b, 4 );

		ge::refl::value ref = ge::refl::value::create_ref( &p );
		ref.as_mutable< two_ints >()->a = 99;
		is_eq( p.a, 99 );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )

	export API void cross_ownership_assignment()
	{
		int x = 1;

		ge::refl::value slot = ge::refl::value::create_owning( 42 );
		slot = ge::refl::value::create_view( x );
		is_eq( slot.const_data(), static_cast< const void* >( &x ) );

		ge::refl::value slot2 = ge::refl::value::create_view( x );
		slot2 = ge::refl::value::create_owning( 77 );
		is_eq( *slot2.as_constant< int >(), 77 );
		is_eq( x, 1 );

		ge::refl::value owned = ge::refl::value::create_owning( 5 );
		ge::refl::value viewed = ge::refl::value::create_view( x );
		owned = viewed;
		is_eq( owned.const_data(), static_cast< const void* >( &x ) );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )

	export API void chain_of_moves()
	{
		int x = 42;
		ge::refl::value a = ge::refl::value::create_view( x );
		ge::refl::value b = std::move( a );
		ge::refl::value c = std::move( b );
		ge::refl::value d = std::move( c );

		is_null( a.const_data() );
		is_null( b.const_data() );
		is_null( c.const_data() );
		is_eq( d.const_data(), static_cast< const void* >( &x ) );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )

	export API void value_type_id_round_trip()
	{
		int x = 5;
		ge::refl::value view = ge::refl::value::create_view( x );
		is_true( view.get_type_id() == ge::refl::make_type_id< int >() );
		is_false( view.get_type_id() == ge::refl::make_type_id< fpoint >() );

		ge::refl::value owning = ge::refl::value::create_owning( fpoint{ 1, 2 } );
		is_true( owning.get_type_id() == ge::refl::make_type_id< fpoint >() );
	}
}

namespace building_tests
{
	REFL_FUNC( ge::test_core::unit_test_trait{} )

	export API void single_type_registered()
	{
		ge::refl::builders::endable_registry_builder builder = ge::refl::builders::begin_registry();
		builder.begin_module( "basic" )
		       .begin_type< type_1 >( "type_1" ).end_type()
		       .end_module();
		const std::unique_ptr< ge::refl::registry_data > reg = std::move( builder ).build();

		is_eq( reg->m_types.m_size, 1ull );
		is_eq( reg->m_modules.m_size, 1ull );

		const ge::refl::type_data& type = *reg->m_types.begin();
		is_eq( type.m_name, "type_1" );
		is_true( type.m_id == ge::refl::make_type_id< type_1 >() );

		const ge::refl::module_data& mod = *reg->m_modules.begin();
		is_eq( mod.m_name, "basic" );
		is_eq( mod.m_types.size(), 1ull );
		is_true( mod.m_types.data() == reg->m_types.data() );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )

	export API void multiple_types_preserve_order()
	{
		ge::refl::builders::endable_registry_builder builder = ge::refl::builders::begin_registry();
		builder.begin_module( "basic" )
		       .begin_type< type_1 >( "type_1" ).end_type()
		       .begin_type< type_2 >( "type_2" ).end_type()
		       .end_module();
		const std::unique_ptr< ge::refl::registry_data > reg = std::move( builder ).build();

		is_eq( reg->m_types.m_size, 2ull );
		is_eq( reg->m_types.data()[ 0 ].m_name, "type_1" );
		is_eq( reg->m_types.data()[ 1 ].m_name, "type_2" );

		const ge::refl::module_data& mod = *reg->m_modules.begin();
		is_eq( mod.m_types.size(), 2ull );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )

	export API void multiple_modules_partition_spans()
	{
		ge::refl::builders::endable_registry_builder builder = ge::refl::builders::begin_registry();
		builder.begin_module( "mod_a" )
		       .begin_type< type_1 >( "a_type" ).end_type()
		       .begin_func< &fixture_func_0 >( "a_func" ).end_func()
		       .end_module()
		       .begin_module( "mod_b" )
		       .begin_type< type_2 >( "b_type" ).end_type()
		       .end_module();
		const std::unique_ptr< ge::refl::registry_data > reg = std::move( builder ).build();

		is_eq( reg->m_modules.m_size, 2ull );
		is_eq( reg->m_types.m_size, 2ull );
		is_eq( reg->m_funcs.m_size, 1ull );

		const ge::refl::module_data& mod_a = reg->m_modules.data()[ 0 ];
		const ge::refl::module_data& mod_b = reg->m_modules.data()[ 1 ];

		is_eq( mod_a.m_name, "mod_a" );
		is_eq( mod_a.m_types.size(), 1ull );
		is_eq( mod_a.m_funcs.size(), 1ull );
		is_eq( mod_a.m_types.front().m_name, "a_type" );
		is_eq( mod_a.m_funcs.front().m_name, "a_func" );

		is_eq( mod_b.m_name, "mod_b" );
		is_eq( mod_b.m_types.size(), 1ull );
		is_eq( mod_b.m_funcs.size(), 0ull );
		is_eq( mod_b.m_types.front().m_name, "b_type" );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )

	export API void empty_module_has_empty_spans()
	{
		ge::refl::builders::endable_registry_builder builder = ge::refl::builders::begin_registry();
		builder.begin_module( "empty" ).end_module();
		const std::unique_ptr< ge::refl::registry_data > reg = std::move( builder ).build();

		is_eq( reg->m_modules.m_size, 1ull );
		is_eq( reg->m_types.m_size, 0ull );
		is_eq( reg->m_funcs.m_size, 0ull );
		is_eq( reg->m_datas.m_size, 0ull );

		const ge::refl::module_data& mod = *reg->m_modules.begin();
		is_eq( mod.m_types.size(), 0ull );
		is_eq( mod.m_funcs.size(), 0ull );
		is_eq( mod.m_datas.size(), 0ull );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )

	export API void funcs_nested_in_type_fill_type_span()
	{
		ge::refl::builders::endable_registry_builder builder = ge::refl::builders::begin_registry();
		builder.begin_module( "m" )
		       .begin_type< type_1 >( "t" )
		       .begin_func< &fixture_func_0 >( "member_func" ).end_func()
		       .end_type()
		       .begin_func< &fixture_func_1 >( "free_func" ).end_func()
		       .end_module();
		const std::unique_ptr< ge::refl::registry_data > reg = std::move( builder ).build();

		const ge::refl::type_data& type = find_type( *reg, ge::refl::make_type_id< type_1 >() );
		is_eq( type.m_funcs.size(), 1ull );
		is_eq( type.m_funcs.front().m_name, "member_func" );

		// The module span covers both the nested and the free function.
		is_eq( reg->m_modules.begin()->m_funcs.size(), 2ull );
	}
}

namespace function_tests
{
	REFL_FUNC( ge::test_core::unit_test_trait{} )

	export API void func_registered_with_name_and_no_traits()
	{
		ge::refl::builders::endable_registry_builder builder = ge::refl::builders::begin_registry();
		builder.begin_module( "m" )
		       .begin_func< &fixture_func_0 >( "ret42" ).end_func()
		       .end_module();
		const std::unique_ptr< ge::refl::registry_data > reg = std::move( builder ).build();

		is_eq( reg->m_funcs.m_size, 1ull );
		is_eq( reg->m_funcs.begin()->m_name, "ret42" );
		is_true( reg->m_funcs.begin()->m_traits.empty() );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )

	export API void invocable_trait_binds_function_pointer()
	{
		ge::refl::builders::endable_registry_builder builder = ge::refl::builders::begin_registry();
		builder.begin_module( "m" )
		       .begin_func< &free_square >( "square" ).add_traits( ge::refl::invocable_trait< int( int ) >{} ).end_func()
		       .begin_func< &fixture_func_0 >( "plain" ).end_func()
		       .end_module();
		const std::unique_ptr< ge::refl::registry_data > reg = std::move( builder ).build();

		ge::refl::func_query::read< ge::refl::invocable_trait< int( int ) > > invocables{ reg->m_funcs };

		size_t count = 0;
		for( const auto& [ func, invocable ] : invocables )
		{
			is_eq( func.m_name, "square" );
			is_eq( invocable.m_invoke( 5 ), 25 );
			is_eq( invocable.m_invoke( -7 ), 49 );
			count++;
		}
		is_eq( count, 1ull );
	}
}

namespace trait_tests
{
	REFL_FUNC( ge::test_core::unit_test_trait{} )

	export API void trait_count_zero_when_unused()
	{
		ge::refl::builders::endable_registry_builder builder = ge::refl::builders::begin_registry();
		builder.begin_module( "m" )
		       .begin_type< type_1 >( "t" ).end_type()
		       .end_module();
		const std::unique_ptr< ge::refl::registry_data > reg = std::move( builder ).build();

		is_eq( reg->m_types.begin()->m_traits.size(), 0ull );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )

	export API void trait_count_one_when_added()
	{
		ge::refl::builders::endable_registry_builder builder = ge::refl::builders::begin_registry();
		builder.begin_module( "m" )
		       .begin_type< type_1 >( "t" ).add_traits( empty_type_trait{} ).end_type()
		       .end_module();
		const std::unique_ptr< ge::refl::registry_data > reg = std::move( builder ).build();

		is_eq( reg->m_types.begin()->m_traits.size(), 1ull );
		is_true( reg->m_types.begin()->m_traits.front().get_type_id() == ge::refl::make_type_id< empty_type_trait >() );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )

	export API void trait_payload_round_trip()
	{
		ge::refl::builders::endable_registry_builder builder = ge::refl::builders::begin_registry();
		builder.begin_module( "m" )
		       .begin_type< type_1 >( "t" ).add_traits( int_type_trait{ .payload = 99 } ).end_type()
		       .end_module();
		const std::unique_ptr< ge::refl::registry_data > reg = std::move( builder ).build();

		const ge::refl::value& v = reg->m_types.begin()->m_traits.front();
		is_not_null( v.const_data() );
		is_true( v.get_type_id() == ge::refl::make_type_id< int_type_trait >() );
		is_eq( v.as_constant< int_type_trait >()->payload, 99 );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )

	export API void trait_payload_round_trip_string_heap()
	{
		std::string long_payload = "hello world long enough to heap allocate definitely yes definitely";
		ge::refl::builders::endable_registry_builder builder = ge::refl::builders::begin_registry();
		builder.begin_module( "m" )
		       .begin_type< type_1 >( "t" ).add_traits( string_type_trait{ .payload = long_payload } ).end_type()
		       .end_module();
		const std::unique_ptr< ge::refl::registry_data > reg = std::move( builder ).build();

		const std::span< const ge::refl::value > traits = reg->m_types.begin()->m_traits;
		is_eq( traits.size(), 1ull );
		is_eq( traits.front().as_constant< string_type_trait >()->payload, long_payload );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )

	export API void traits_order_preserved()
	{
		ge::refl::builders::endable_registry_builder builder = ge::refl::builders::begin_registry();
		builder.begin_module( "m" )
		       .begin_type< type_1 >( "t" )
		       .add_traits( int_type_trait{ .payload = 10 } )
		       .add_traits( int_type_trait{ .payload = 20 } )
		       .add_traits( int_type_trait{ .payload = 30 } )
		       .end_type()
		       .end_module();
		const std::unique_ptr< ge::refl::registry_data > reg = std::move( builder ).build();

		std::vector< int > values{};
		for( const ge::refl::value& v : reg->m_types.begin()->m_traits )
		{
			values.push_back( v.as_constant< int_type_trait >()->payload );
		}
		is_eq( values.size(), 3ull );
		is_eq( values[ 0 ], 10 );
		is_eq( values[ 1 ], 20 );
		is_eq( values[ 2 ], 30 );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )

	export API void traits_not_mutable_after_build()
	{
		ge::refl::builders::endable_registry_builder builder = ge::refl::builders::begin_registry();
		builder.begin_module( "m" )
		       .begin_type< type_1 >( "t" ).add_traits( int_type_trait{ .payload = 5 } ).end_type()
		       .end_module();
		const std::unique_ptr< ge::refl::registry_data > reg = std::move( builder ).build();

		is_false( reg->m_types.begin()->m_traits.front().is_mutable() );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )

	export API void traits_independent_across_targets()
	{
		ge::refl::builders::endable_registry_builder builder = ge::refl::builders::begin_registry();
		builder.begin_module( "m" )
		       .begin_type< type_1 >( "t1" ).add_traits( int_type_trait{ .payload = 1 } ).end_type()
		       .begin_type< type_2 >( "t2" )
		       .add_traits( int_type_trait{ .payload = 2 } )
		       .add_traits( int_type_trait{ .payload = 3 } )
		       .end_type()
		       .begin_func< &fixture_func_0 >( "f" ).add_traits( int_func_trait{ .payload = 4 } ).end_func()
		       .end_module();
		const std::unique_ptr< ge::refl::registry_data > reg = std::move( builder ).build();

		const ge::refl::type_data& t1 = find_type( *reg, ge::refl::make_type_id< type_1 >() );
		const ge::refl::type_data& t2 = find_type( *reg, ge::refl::make_type_id< type_2 >() );

		is_eq( t1.m_traits.size(), 1ull );
		is_eq( t2.m_traits.size(), 2ull );
		is_eq( t1.m_traits.front().as_constant< int_type_trait >()->payload, 1 );
		is_eq( t2.m_traits[ 0 ].as_constant< int_type_trait >()->payload, 2 );
		is_eq( t2.m_traits[ 1 ].as_constant< int_type_trait >()->payload, 3 );

		is_eq( reg->m_funcs.begin()->m_traits.size(), 1ull );
		is_eq( reg->m_funcs.begin()->m_traits.front().as_constant< int_func_trait >()->payload, 4 );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )

	export API void trait_destructor_runs_when_registry_destroyed()
	{
		int counter = 0;
		int snapshot_after_build = 0;
		{
			ge::refl::builders::endable_registry_builder builder = ge::refl::builders::begin_registry();
			builder.begin_module( "m" )
			       .begin_type< type_1 >( "t" ).add_traits( destruct_counting_trait{ &counter } ).end_type()
			       .end_module();
			const std::unique_ptr< ge::refl::registry_data > reg = std::move( builder ).build();
			snapshot_after_build = counter;
		}
		is_gt( counter, snapshot_after_build );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )

	export API void trait_destructor_runs_for_each_added_instance()
	{
		int counter = 0;
		int snapshot_after_build = 0;
		{
			ge::refl::builders::endable_registry_builder builder = ge::refl::builders::begin_registry();
			builder.begin_module( "m" )
			       .begin_type< type_1 >( "t" )
			       .add_traits( destruct_counting_trait{ &counter } )
			       .add_traits( destruct_counting_trait{ &counter } )
			       .add_traits( destruct_counting_trait{ &counter } )
			       .end_type()
			       .end_module();
			const std::unique_ptr< ge::refl::registry_data > reg = std::move( builder ).build();
			snapshot_after_build = counter;
		}
		is_eq( counter - snapshot_after_build, 3 );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )

	export API void move_only_trait_supported()
	{
		ge::refl::builders::endable_registry_builder builder = ge::refl::builders::begin_registry();
		builder.begin_module( "m" )
		       .begin_type< type_1 >( "t" ).add_traits( move_only_int_trait{ 99 } ).end_type()
		       .end_module();
		const std::unique_ptr< ge::refl::registry_data > reg = std::move( builder ).build();

		const std::span< const ge::refl::value > traits = reg->m_types.begin()->m_traits;
		is_eq( traits.size(), 1ull );
		const move_only_int_trait* p = traits.front().as_constant< move_only_int_trait >();
		is_not_null( p );
		is_not_null( p->p.get() );
		is_eq( *p->p, 99 );
	}
}

namespace trait_hook_tests
{
	REFL_FUNC( ge::test_core::unit_test_trait{} )

	export API void on_apply_and_post_build_dispatch_per_target_kind()
	{
		hook_record rec{};
		ge::refl::builders::endable_registry_builder builder = ge::refl::builders::begin_registry();
		builder.begin_module( "m" )
		       .begin_type< type_1 >( "t" ).add_traits( hooked_trait{ &rec } ).end_type()
		       .begin_func< &fixture_func_0 >( "f" ).add_traits( hooked_trait{ &rec } ).end_func()
		       .begin_type< int >( "int" ).end_type()
		       .begin_type< entity >( "e" )
		       .begin_data< &entity::hp >( "hp" ).add_traits( hooked_trait{ &rec } ).end_data()
		       .end_type()
		       .end_module();
		const std::unique_ptr< ge::refl::registry_data > reg = std::move( builder ).build();

		is_eq( rec.on_apply_type, 1 );
		is_eq( rec.on_apply_func, 1 );
		is_eq( rec.on_apply_data, 1 );
		is_eq( rec.post_build_type, 1 );
		is_eq( rec.post_build_func, 1 );
		is_eq( rec.post_build_data, 1 );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )

	export API void all_on_apply_run_before_any_post_build()
	{
		hook_record rec{};
		ge::refl::builders::endable_registry_builder builder = ge::refl::builders::begin_registry();
		builder.begin_module( "m" )
		       .begin_type< type_1 >( "t1" ).add_traits( hooked_trait{ &rec } ).end_type()
		       .begin_type< type_2 >( "t2" ).add_traits( hooked_trait{ &rec } ).end_type()
		       .end_module();
		const std::unique_ptr< ge::refl::registry_data > reg = std::move( builder ).build();

		is_eq( rec.post_build_count_at_first_apply, 0 );
		is_eq( rec.on_apply_count_at_first_post_build, 2 );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )

	export API void post_build_receives_correct_records()
	{
		hook_record rec{};
		ge::refl::builders::endable_registry_builder builder = ge::refl::builders::begin_registry();
		builder.begin_module( "m" )
		       .begin_type< type_1 >( "particular_name" ).add_traits( hooked_trait{ &rec } ).end_type()
		       .begin_func< &fixture_func_0 >( "specific_func" ).add_traits( hooked_trait{ &rec } ).end_func()
		       .begin_type< int >( "int" ).end_type()
		       .begin_type< entity >( "entity_type" )
		       .begin_data< &entity::hp >( "specific_data" ).add_traits( hooked_trait{ &rec } ).end_data()
		       .end_type()
		       .end_module();
		const std::unique_ptr< ge::refl::registry_data > reg = std::move( builder ).build();

		is_eq( rec.last_type_name, "particular_name" );
		is_eq( rec.last_func_name, "specific_func" );
		is_eq( rec.last_data_name, "specific_data" );
		is_eq( rec.last_data_outer_name, "entity_type" );
	}
}

namespace data_tests
{
	REFL_FUNC( ge::test_core::unit_test_trait{} )

	export API void data_registered_with_name_outer_and_resolved_type()
	{
		ge::refl::builders::endable_registry_builder builder = ge::refl::builders::begin_registry();
		builder.begin_module( "m" )
		       .begin_type< int >( "int" ).end_type()
		       .begin_type< entity >( "entity" )
		       .begin_data< &entity::hp >( "hp" ).end_data()
		       .end_type()
		       .end_module();
		const std::unique_ptr< ge::refl::registry_data > reg = std::move( builder ).build();

		is_eq( reg->m_datas.m_size, 1ull );
		const ge::refl::data_data& d = *reg->m_datas.begin();

		is_eq( d.m_name, "hp" );
		is_eq( d.m_outer_type.get().m_name, "entity" );
		is_true( d.m_outer_type.get().m_id == ge::refl::make_type_id< entity >() );
		// build() resolves the member's cached type reference.
		is_true( d.m_type.type_data.get().m_id == ge::refl::make_type_id< int >() );

		const ge::refl::type_data& entity_type = find_type( *reg, ge::refl::make_type_id< entity >() );
		is_eq( entity_type.m_data.size(), 1ull );
		is_true( entity_type.m_data.data() == &d );

		is_eq( reg->m_modules.begin()->m_datas.size(), 1ull );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )

	export API void multiple_members_preserve_order()
	{
		ge::refl::builders::endable_registry_builder builder = ge::refl::builders::begin_registry();
		builder.begin_module( "m" )
		       .begin_type< int >( "int" ).end_type()
		       .begin_type< fpoint >( "fpoint" ).end_type()
		       .begin_type< entity >( "entity" )
		       .begin_data< &entity::hp >( "hp" ).end_data()
		       .begin_data< &entity::mp >( "mp" ).end_data()
		       .begin_data< &entity::pos >( "pos" ).end_data()
		       .end_type()
		       .end_module();
		const std::unique_ptr< ge::refl::registry_data > reg = std::move( builder ).build();

		const ge::refl::type_data& entity_type = find_type( *reg, ge::refl::make_type_id< entity >() );
		is_eq( entity_type.m_data.size(), 3ull );
		is_eq( entity_type.m_data[ 0 ].m_name, "hp" );
		is_eq( entity_type.m_data[ 1 ].m_name, "mp" );
		is_eq( entity_type.m_data[ 2 ].m_name, "pos" );

		is_eq( reg->m_modules.begin()->m_datas.size(), 3ull );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )

	export API void default_getter_reads_and_aliases_member()
	{
		ge::refl::builders::endable_registry_builder builder = ge::refl::builders::begin_registry();
		builder.begin_module( "m" )
		       .begin_type< int >( "int" ).end_type()
		       .begin_type< entity >( "entity" )
		       .begin_data< &entity::hp >( "hp" ).end_data()
		       .end_type()
		       .end_module();
		const std::unique_ptr< ge::refl::registry_data > reg = std::move( builder ).build();

		const ge::refl::data_data& d = *reg->m_datas.begin();
		is_not_null( d.m_get );

		entity e{ 42, 0, fpoint{ 0, 0 } };
		ge::refl::value result = d.m_get( ge::refl::value::create_view( e ) );
		is_true( result.get_type_id() == ge::refl::make_type_id< int >() );
		is_eq( *result.as_constant< int >(), 42 );
		is_false( result.is_mutable() );

		// The default getter returns a view aliasing the member.
		is_eq( result.const_data(), static_cast< const void* >( &e.hp ) );
		e.hp = 200;
		is_eq( *result.as_constant< int >(), 200 );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )

	export API void default_setter_writes_member_only()
	{
		ge::refl::builders::endable_registry_builder builder = ge::refl::builders::begin_registry();
		builder.begin_module( "m" )
		       .begin_type< int >( "int" ).end_type()
		       .begin_type< entity >( "entity" )
		       .begin_data< &entity::hp >( "hp" ).end_data()
		       .end_type()
		       .end_module();
		const std::unique_ptr< ge::refl::registry_data > reg = std::move( builder ).build();

		const ge::refl::data_data& d = *reg->m_datas.begin();
		is_not_null( d.m_set );

		entity e{ 1, 2, fpoint{ 3, 4 } };
		d.m_set( ge::refl::value::create_ref( e ), ge::refl::value::create_owning( 99 ) );
		is_eq( e.hp, 99 );
		is_eq( e.mp, 2 );
		is_eq( e.pos.x, 3 );
		is_eq( e.pos.y, 4 );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )

	export API void struct_member_getter_setter_round_trip()
	{
		ge::refl::builders::endable_registry_builder builder = ge::refl::builders::begin_registry();
		builder.begin_module( "m" )
		       .begin_type< fpoint >( "fpoint" ).end_type()
		       .begin_type< entity >( "entity" )
		       .begin_data< &entity::pos >( "pos" ).end_data()
		       .end_type()
		       .end_module();
		const std::unique_ptr< ge::refl::registry_data > reg = std::move( builder ).build();

		const ge::refl::data_data& d = *reg->m_datas.begin();

		entity e{ 0, 0, fpoint{ 0, 0 } };
		d.m_set( ge::refl::value::create_ref( e ), ge::refl::value::create_owning( fpoint{ 11, 22 } ) );
		is_eq( e.pos.x, 11 );
		is_eq( e.pos.y, 22 );

		ge::refl::value got = d.m_get( ge::refl::value::create_view( e ) );
		is_true( got.get_type_id() == ge::refl::make_type_id< fpoint >() );
		is_true( *got.as_constant< fpoint >() == fpoint{ 11, 22 } );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )

	export API void nullptr_getter_and_setter_disable_access()
	{
		ge::refl::builders::endable_registry_builder builder = ge::refl::builders::begin_registry();
		builder.begin_module( "m" )
		       .begin_type< int >( "int" ).end_type()
		       .begin_type< entity >( "entity" )
		       .begin_data< &entity::hp >( "hp" ).getter< nullptr >().setter< nullptr >().end_data()
		       .end_type()
		       .end_module();
		const std::unique_ptr< ge::refl::registry_data > reg = std::move( builder ).build();

		const ge::refl::data_data& d = *reg->m_datas.begin();
		is_null( d.m_get );
		is_null( d.m_set );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )

	export API void custom_getter_value_return_is_owning()
	{
		ge::refl::builders::endable_registry_builder builder = ge::refl::builders::begin_registry();
		builder.begin_module( "m" )
		       .begin_type< int >( "int" ).end_type()
		       .begin_type< entity >( "entity" )
		       .begin_data< &entity::hp >( "hp" ).getter< &free_double_hp >().end_data()
		       .end_type()
		       .end_module();
		const std::unique_ptr< ge::refl::registry_data > reg = std::move( builder ).build();

		const ge::refl::data_data& d = *reg->m_datas.begin();

		entity e{ 21, 0, fpoint{ 0, 0 } };
		ge::refl::value result = d.m_get( ge::refl::value::create_view( e ) );
		is_eq( *result.as_constant< int >(), 42 );
		is_true( result.is_owning() );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )

	export API void custom_getter_cref_return_is_view()
	{
		ge::refl::builders::endable_registry_builder builder = ge::refl::builders::begin_registry();
		builder.begin_module( "m" )
		       .begin_type< int >( "int" ).end_type()
		       .begin_type< entity >( "entity" )
		       .begin_data< &entity::hp >( "hp" ).getter< &free_get_hp_cref >().end_data()
		       .end_type()
		       .end_module();
		const std::unique_ptr< ge::refl::registry_data > reg = std::move( builder ).build();

		const ge::refl::data_data& d = *reg->m_datas.begin();

		entity e{ 7, 0, fpoint{ 0, 0 } };
		ge::refl::value result = d.m_get( ge::refl::value::create_view( e ) );
		is_eq( *result.as_constant< int >(), 7 );
		is_false( result.is_mutable() );
		is_false( result.is_owning() );
		is_eq( result.const_data(), static_cast< const void* >( &e.hp ) );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )

	export API void custom_setter_replaces_default()
	{
		ge::refl::builders::endable_registry_builder builder = ge::refl::builders::begin_registry();
		builder.begin_module( "m" )
		       .begin_type< int >( "int" ).end_type()
		       .begin_type< entity >( "entity" )
		       .begin_data< &entity::hp >( "hp" ).setter< &free_clamp_hp >().end_data()
		       .end_type()
		       .end_module();
		const std::unique_ptr< ge::refl::registry_data > reg = std::move( builder ).build();

		const ge::refl::data_data& d = *reg->m_datas.begin();

		entity e{ 0, 0, fpoint{ 0, 0 } };
		d.m_set( ge::refl::value::create_ref( e ), ge::refl::value::create_owning( -5 ) );
		is_eq( e.hp, 0 );

		d.m_set( ge::refl::value::create_ref( e ), ge::refl::value::create_owning( 5000 ) );
		is_eq( e.hp, 1000 );

		d.m_set( ge::refl::value::create_ref( e ), ge::refl::value::create_owning( 500 ) );
		is_eq( e.hp, 500 );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )

	export API void data_traits_isolated_per_member()
	{
		ge::refl::builders::endable_registry_builder builder = ge::refl::builders::begin_registry();
		builder.begin_module( "m" )
		       .begin_type< int >( "int" ).end_type()
		       .begin_type< entity >( "entity" )
		       .add_traits( int_type_trait{ .payload = 100 } )
		       .begin_data< &entity::hp >( "hp" ).add_traits( int_data_trait{ .payload = 1 } ).end_data()
		       .begin_data< &entity::mp >( "mp" )
		       .add_traits( int_data_trait{ .payload = 2 } )
		       .add_traits( int_data_trait{ .payload = 3 } )
		       .end_data()
		       .end_type()
		       .end_module();
		const std::unique_ptr< ge::refl::registry_data > reg = std::move( builder ).build();

		const ge::refl::type_data& entity_type = find_type( *reg, ge::refl::make_type_id< entity >() );
		is_eq( entity_type.m_traits.size(), 1ull );
		is_eq( entity_type.m_traits.front().as_constant< int_type_trait >()->payload, 100 );

		const ge::refl::data_data& hp = entity_type.m_data[ 0 ];
		const ge::refl::data_data& mp = entity_type.m_data[ 1 ];
		is_eq( hp.m_traits.size(), 1ull );
		is_eq( mp.m_traits.size(), 2ull );
		is_eq( hp.m_traits.front().as_constant< int_data_trait >()->payload, 1 );
		is_eq( mp.m_traits[ 0 ].as_constant< int_data_trait >()->payload, 2 );
		is_eq( mp.m_traits[ 1 ].as_constant< int_data_trait >()->payload, 3 );
	}
}
