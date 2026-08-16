module;

#include <assert.h>

export module scheduling;

import runtime_reflection;
import stl;

namespace ge::scheduling
{
	export struct environments_map
	{
		std::vector< refl::value > m_environments{};
	};

	export using arguments_storage = std::unique_ptr< refl::value[] >;

	export struct cached_system
	{
		void ( *m_invoke )( refl::value* args ){};

		// Stored somewhere in arguments_storage, consecutive view of sizeof...(Params) refl::value.
		refl::value* m_cached_arguments{};
	};

	export struct argument_factory_context
	{
		environments_map& m_environments;
		arguments_storage& m_arguments;
	};

	export template< typename T >
	struct argument_factory;

	// For write access to environments
	template< typename T >
	struct argument_factory< T& >
	{
		static refl::value construct( const argument_factory_context& context )
		{
			auto it = std::ranges::find_if(
				context.m_environments.m_environments,
				[]( const refl::value& value ) { return value.get_type_id() == refl::make_type_id< T >(); } );

			assert( it != context.m_environments.m_environments.end() && "Parameter was not an environment" );

			return refl::value::create_ref( *it );
		}
	};

	// For read access to environments
	template< typename T >
	struct argument_factory< const T& >
	{
		static refl::value construct( const argument_factory_context& context )
		{
			return refl::value::create_view( argument_factory< T& >::construct( context ) );
		}
	};

	// TODO specialization for entity-component queries

	// Supported arguments:
	// - Globals
	// - World-environments
	// - entities/component-queries (assumes trivially destructible const refs)

	// Future, maybe?
	// - refl queries?

	// Get all traits with argument_factory_base trait
	// Get all types with those traits

	namespace traits
	{
		export struct environment : refl::type_trait
		{
			void ( *m_insert_into_map )( environments_map& );

			template< typename T >
			void on_apply( const refl::builders::type_builder< T >& )
			{
				m_insert_into_map = +[]( environments_map& map )
				{
					map.m_environments.emplace_back( refl::value::create_owning( T{} ) );
				};
			}
		};

		export struct system : refl::func_trait
		{
			void ( *m_initialize_system )( cached_system&, const argument_factory_context& );
			size_t m_num_params{};

			template< auto Func >
			void on_apply( const refl::builders::func_builder< Func >& )
			{
				m_initialize_system = +[]( cached_system& system, const argument_factory_context& context ) -> void
				{
					[ & ]< typename Ret, typename... ParamsT >( refl::func_sig< Ret( ParamsT... ) > ) -> void
					{
						static_assert(
							std::is_same_v< Ret, void >,
							"Systems cannot have return values. Return values of systems are ignored" );

						// Cache the arguments once, so we don't have to look them up everytime we run the system
						[ & ]< size_t... Indices >( std::index_sequence< Indices... > ) -> void
						{
							(
								[ & ]< typename ParamT, size_t Idx >()
								{
									using factory = argument_factory< ParamT >;
									system.m_cached_arguments[ Idx ] = factory::construct( context );
								}.template operator()< ParamsT, Indices >(),
								... );
						}( std::make_index_sequence< sizeof...( ParamsT ) >() );

						system.m_invoke = +[]( refl::value* args ) -> void
						{
							[ & ]< size_t... Indices >( std::index_sequence< Indices... > ) -> void
							{
								std::invoke(
									Func,
									[ args ]< typename ParamT, size_t Idx >()
										-> ParamT
										   {
											   if constexpr(
												   std::is_reference_v< ParamT >
												   && !std::is_const_v< std::remove_reference_t< ParamT > > )
											   {
												   return *static_cast< std::remove_reference_t< ParamT >* >(
													   args[ Idx ].mutable_data() );
											   }
											   else
											   {
												   return *static_cast< std::remove_reference_t< std::add_const_t< ParamT > >* >(
													   args[ Idx ].const_data() );
											   }
										   }.template operator()< ParamsT, Indices >()... );
							}( std::make_index_sequence< sizeof...( ParamsT ) >() );
						};
					}( refl::func_sig_t< decltype( Func ) >{} );
				};

				m_num_params = [ & ]< typename Ret, typename... ParamsT >( refl::func_sig< Ret( ParamsT... ) > ) -> size_t
				{
					return sizeof...( ParamsT );
				}( refl::func_sig_t< decltype( Func ) >{} );
			}
		};
	} // namespace traits

	export struct execution_graph
	{
		// TODO make graph that supports multi-threading
		std::vector< cached_system > m_systems{};
		arguments_storage m_arguments_storage{};
	};

	export using environments_query = refl::type_query::read<traits::environment>;

	export API environments_map build_environment_map(environments_query types)
	{
		environments_map map{};

		for( auto [_, env_trait] : types )
		{
			env_trait.m_insert_into_map( map );
		}

		return map;
	}

	export using systems_query = refl::func_query::read< traits::system >;

	export API execution_graph build_graph( systems_query systems, environments_map& environments )
	{
		struct count
		{
			size_t num_systems{};
			size_t num_arguments{};
		};

		auto [ num_systems, num_arguments ] = std::ranges::fold_left(
			systems,
			count{},
			[]( count count, const auto& element )
			{
				auto [ _, system ] = element;
				count.num_arguments += system.m_num_params;
				count.num_systems++;
				return count;
			} );

		execution_graph graph{
			.m_systems = std::vector< cached_system >( num_systems ),
			.m_arguments_storage = std::make_unique< refl::value[] >( num_arguments ),
		};

		argument_factory_context argument_factory_context{ environments, graph.m_arguments_storage };

		count count{};

		for( auto [ _, system_trait ] : systems )
		{
			cached_system& system = graph.m_systems[ count.num_systems ];
			system.m_cached_arguments = &graph.m_arguments_storage[ count.num_arguments ];

			system_trait.m_initialize_system( graph.m_systems[ count.num_systems ], argument_factory_context );

			count.num_arguments += system_trait.m_num_params;
			count.num_systems++;
		}

		return graph;
	}
} // namespace ge::scheduling
