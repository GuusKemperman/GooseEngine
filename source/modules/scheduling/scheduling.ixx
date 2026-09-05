module;

#include <assert.h>

export module scheduling;

import runtime_reflection;
import stl;
import io;

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

	export struct sequence_point
	{
		[[maybe_unused]] sequence_point() = default;

		template< typename... Params >
		constexpr sequence_point( void ( *other_system )( Params... ) )
			: m_data( std::bit_cast< void* >( other_system ) )
		{
			static_assert( sizeof( void ( * )() ) == sizeof( void* ) );
		}

		constexpr auto operator<=>( const sequence_point& ) const = default;

		void* m_data{};
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

			sequence_point m_func_sequence_point{};

			template< auto Func >
			void on_apply( const refl::builders::func_builder< Func >& )
			{
				m_func_sequence_point = sequence_point{ Func };

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

		template<size_t>
		struct ordering_type_erased : refl::func_trait
		{
			sequence_point m_point{};
		};

		export template< auto OtherSystem, typename impl_t >
		struct ordering : refl::func_trait
		{
			template< auto Func >
			void on_apply( refl::builders::func_builder< Func >& func )
			{
				func.add_traits( impl_t{ .m_point = sequence_point{ OtherSystem } } );
			}
		};

		using type_erased_order_before = ordering_type_erased<0>;
		using type_erased_order_after = ordering_type_erased<1>;

		export template< auto OtherSystem>
			requires ge::refl::is_func< OtherSystem >
		using order_before = ordering<OtherSystem, type_erased_order_before>;

		export template< auto OtherSystem>
			requires ge::refl::is_func< OtherSystem >
		using order_after = ordering<OtherSystem, type_erased_order_after>;
	} // namespace traits

	export struct execution_graph
	{
		struct group
		{
			struct system_node
			{
				std::string_view m_name;

				// TODO store resources here (e.g., environment writes)
			};

			// Each system_node in a group can be executed using parallel-for without any race conditions on environments/entities
			std::vector< system_node > m_nodes{};
		};

		std::vector< group > m_groups{};
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

	export API std::optional<execution_graph> build_graph( systems_query systems, logger& logger )
	{
		// Phase 1: place system_node only based on user-specified ordering.
		// Phase 2: for each group, check if there are system_nodes that have conflicts, e.g., multiple writes or readers + writers. If so, warn as underconstrained.
		// Phase 3: 
		
		static constexpr std::uint16_t s_unassigned_group = std::numeric_limits< std::uint16_t >::max();

		struct pending_system
		{
			std::reference_wrapper< const ge::refl::func_data > m_func;
			std::reference_wrapper< const traits::system > m_system_trait;

			std::vector< std::uint16_t > m_execute_after{}; 

			std::uint16_t m_group_idx = s_unassigned_group; 
		};

		size_t num_errors = 0;

		std::vector< pending_system > pending_systems = systems
												| std::views::transform(
													[]( const systems_query::element& element ) -> pending_system
													{
														auto [ func, system ] = element;
														return { .m_func = func, .m_system_trait = system };
													} )
												| std::ranges::to< std::vector< pending_system > >();

		for( pending_system& pending : pending_systems )
		{
			auto sequence_point_to_func_idx = [ &pending, &pending_systems, &logger, &num_errors](const sequence_point point) -> std::optional<std::uint16_t>
			{
				auto it = std::ranges::find_if(
					pending_systems,
					[ & ]( const pending_system& other ) -> bool
					{ return other.m_system_trait.get().m_func_sequence_point == point; } );

				if( it == pending_systems.end() )
				{
					logger.log(
						error,
						"invalid ordering for system {}: ordered constraint against function that was not a system.",
						pending.m_func.get().m_name );
					num_errors++;
					return std::nullopt;
				}

				if( &*it == &pending )
				{
					logger.log( error, "invalid ordering for system {}: ordered against itself.", pending.m_func.get().m_name );
					num_errors++;
					return std::nullopt;
				}

				return static_cast< std::uint16_t >( it - pending_systems.begin() );
			};

			//for( const refl::value& trait : pending.m_func.get().m_traits )
			//{
			//	switch(trait.get_type_id().m_id)
			//	{
			//	//case ge::refl::make_type_id< traits::type_erased_order_before >().m_id:
			//	//{
			//	//	if( std::optional< std::uint16_t > idx_of_system_that_runs_after_us = sequence_point_to_func_idx( before ) )
			//	//	{
			//	//		pending_system& system_that_runs_after_us = pending_systems[ *idx_of_system_that_runs_after_us ];
			//	//		// This system ^ will run after us
			//	//		system_that_runs_after_us.m_execute_after.emplace_back( *idx_of_system_that_runs_after_us );
			//	//	}
			//	//	break;
			//	//}
			//	//case ge::refl::make_type_id< traits::type_erased_order_after >().m_id:
			//	//{
			//	//	//traits::type_erased_order_after* trait = trait.as_const< traits::type_erased_order_after >();
			//	//	//if( std::optional< std::uint16_t > idx_of_system_that_runs_before_us = sequence_point_to_func_idx(after) )
			//	//	//{
			//	//	//	// We execute after this system
			//	//	//	pending.m_execute_after.emplace_back( *idx_of_system_that_runs_before_us );
			//	//	//}
			//	//	//break;
			//	//}

			//	default:
			//	}
			//}
		}

		// TODO Loop detection
		for (pending_system& system : pending_systems)
		{
			if (system.m_group_idx != s_unassigned_group)
			{
				continue;
			}

			auto assign_group = [ &pending_systems ]( const auto& self, pending_system& current ) -> std::uint16_t
			{
				// Already processed
				if(current.m_group_idx != s_unassigned_group)
				{
					return current.m_group_idx + 1u;
				}

				if( current.m_execute_after.empty() )
				{
					current.m_group_idx = 0;
					return 1u;
				}

				std::uint16_t highest_group_index{};
				for(std::uint16_t idx_of_system_that_runs_before_us : current.m_execute_after)
				{
					pending_system& system_before_us = pending_systems[ idx_of_system_that_runs_before_us ];
					highest_group_index = std::max( self( self, system_before_us ), highest_group_index );
				}

				current.m_group_idx = highest_group_index;
				return current.m_group_idx + 1u;
			};

			assign_group( assign_group, system );
		}

		// TODO sort + validate environments here?

		if(num_errors > 0)
		{
			return std::nullopt;
		}

		execution_graph graph{};

		for(const pending_system system : pending_systems)
		{
			graph.m_groups.resize( system.m_group_idx + 1u );
			graph.m_groups[ system.m_group_idx ].m_nodes.push_back(
				execution_graph::group::system_node{ .m_name = system.m_func.get().m_name } );
		}

		return graph;
	}

	//export API void cache_system_arguments( execution_graph& graph, environments_map& environments )
	//{
	//	size_t total_num_arguments = std::ranges::fold_left(
	//		graph.m_systems,
	//		size_t{},
	//		[]( size_t count, const cached_system& system )
	//		{
	//			count += system.m_num_arguments_to_cache;
	//			return count;
	//		} );

	//	graph.m_arguments_storage = std::make_unique< refl::value[] >( total_num_arguments );

	//	refl::value* cached_arguments = graph.m_arguments_storage.get();
	//	for( cached_system& system : graph.m_systems )
	//	{
	//		size_t num_arguments = system.m_num_arguments_to_cache;
	//		system.m_cached_arguments = cached_arguments;
	//		cached_arguments += num_arguments;

	//		system_trait.m_initialize_system( system, argument_factory_context );
	//	}

	//}
} // namespace ge::scheduling
