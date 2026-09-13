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
	} // namespace traits

	export struct cached_system
	{
		void ( *m_invoke )( refl::value* args ){};

		// Stored somewhere in arguments_storage, consecutive view of sizeof...(Params) refl::value.
		refl::value* m_cached_arguments{};
	};

	export struct argument_factory_context
	{
		const refl::registry_data& m_registry;
	};

	export struct access
	{
		std::vector< std::reference_wrapper< const refl::type_data > > m_reads{};
		std::vector< std::reference_wrapper< const refl::type_data > > m_writes{};
	};

	API void fold_access( access& access )
	{
		static constexpr auto dedup = []( std::vector< std::reference_wrapper< const refl::type_data > >& vec )
		{
			std::ranges::sort(
				vec,
				[]( const refl::type_data& lhs, const refl::type_data& rhs ) { return lhs.m_name < rhs.m_name; } );
			vec.erase(
				std::unique(
					vec.begin(),
					vec.end(),
					[]( const refl::type_data& lhs, const refl::type_data& rhs ) { return &lhs == &rhs; } ),
				vec.end() );
		};

		dedup( access.m_reads );
		dedup( access.m_writes );

		std::erase_if(
			access.m_reads,
			[ &access ]( const refl::type_data& readable )
			{
				return std::ranges::find_if(
						   access.m_writes,
						   [ & ]( const refl::type_data& writeable ) { return &readable == &writeable; } )
					   != access.m_writes.end();
			} );
	}

	API void declare_env_access(
		const argument_factory_context& context,
		refl::type_id type_id,
		std::vector< std::reference_wrapper< const refl::type_data > >& dest )
	{
		refl::type_query::with< traits::environment > query{ context.m_registry.m_types };

		auto it = std::ranges::find_if(
			query,
			[ &type_id ]( const auto& element ) { return element.m_handle.get().m_id == type_id; } );

		assert( it != query.end() && "Parameter was either not reflected, or did not have the 'environment' trait" );

		auto [ func ] = *it;
		dest.push_back( std::cref( func ) );
	}

	export template< typename T >
	struct argument_factory;

	// For write access to environments
	template< typename T >
	struct argument_factory< T& >
	{
		static access declare_access( const argument_factory_context& context )
		{
			access access{};
			declare_env_access( context, refl::make_type_id< T >(), access.m_writes );
			return access;
		}
	};

	// For read access to environments
	template< typename T >
	struct argument_factory< const T& >
	{
		static access declare_access( const argument_factory_context& context )
		{
			access access{};
			declare_env_access( context, refl::make_type_id< T >(), access.m_reads );
			return access;
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

	namespace traits
	{
		export struct system : refl::func_trait
		{
			void ( *m_initialize_system )( cached_system&, const argument_factory_context& );

			sequence_point m_func_sequence_point{};

			access m_access{};
			access ( *m_populate_accesses )( const refl::builders::post_build_context& );

			template< auto Func >
			void on_apply( const refl::builders::func_builder< Func >& )
			{
				m_func_sequence_point = sequence_point{ Func };

				m_populate_accesses = +[]( const refl::builders::post_build_context& context )
				{
					argument_factory_context factory_context{ .m_registry = context.m_reg };

					return [ & ]< typename Ret, typename... ParamsT >( refl::func_sig< Ret( ParamsT... ) > )
					{
						static_assert(
							std::is_same_v< Ret, void >,
							"Systems cannot have return values. Return values of systems are ignored" );

						access combined{};

						// Cache the arguments once, so we don't have to look them up everytime we run the system
						[ & ]< size_t... Indices >( std::index_sequence< Indices... > ) -> void
						{
							(
								[ & ]< typename ParamT, size_t Idx >()
								{
									using factory = argument_factory< ParamT >;
									access fromParam = factory::declare_access( factory_context );

									combined.m_writes.append_range( fromParam.m_writes );
									combined.m_reads.append_range( fromParam.m_reads );
								}.template operator()< ParamsT, Indices >(),
								... );
						}( std::make_index_sequence< sizeof...( ParamsT ) >() );

						fold_access( combined );

						return combined;
					}( refl::func_sig_t< decltype( Func ) >{} );
				};
			}

			API void post_build( const refl::builders::post_build_context& context, const refl::func_data& )
			{
				m_access = m_populate_accesses( context );
			}
		};

		template< size_t >
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

		using type_erased_order_before = ordering_type_erased< 0 >;
		using type_erased_order_after = ordering_type_erased< 1 >;

		export template< auto OtherSystem >
			requires ge::refl::is_func< OtherSystem >
		using order_before = ordering< OtherSystem, type_erased_order_before >;

		export template< auto OtherSystem >
			requires ge::refl::is_func< OtherSystem >
		using order_after = ordering< OtherSystem, type_erased_order_after >;
	} // namespace traits

	export struct execution_graph
	{
		struct group
		{
			struct system_node
			{
				std::string_view m_name;

				access m_access{};
			};

			// Each system_node in a group can be executed using parallel-for without any race conditions on environments/entities
			std::vector< system_node > m_nodes{};
		};

		std::vector< group > m_groups{};
	};

	export using environments_query = refl::type_query::read< traits::environment >;

	export API environments_map build_environment_map( environments_query types )
	{
		environments_map map{};

		for( auto [ _, env_trait ] : types )
		{
			env_trait.m_insert_into_map( map );
		}

		return map;
	}

	export using systems_query = refl::func_query::read< traits::system >;

	export API std::optional< execution_graph > build_graph( systems_query systems, logger& logger )
	{
		static constexpr std::uint16_t s_unassigned_group = std::numeric_limits< std::uint16_t >::max();

		struct pending_system
		{
			std::reference_wrapper< const refl::func_data > m_func;
			std::reference_wrapper< const traits::system > m_system_trait;

			std::vector< std::reference_wrapper< pending_system > > m_execute_after{};

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
			auto sequence_point_to_func
				= [ &pending, &pending_systems, &logger, &num_errors ]( const sequence_point point ) -> pending_system*
			{
				auto it = std::ranges::find_if(
					pending_systems,
					[ & ]( const pending_system& other ) -> bool
					{ return other.m_system_trait.get().m_func_sequence_point == point; } );

				if( it == pending_systems.end() )
				{
					logger.log(
						error,
						"invalid ordering for '{}': ordered constraint against function that was not a system.",
						pending.m_func.get().m_name );
					num_errors++;
					return nullptr;
				}

				if( &*it == &pending )
				{
					logger.log( error, "invalid ordering for '{}': ordered against itself.", pending.m_func.get().m_name );
					num_errors++;
					return nullptr;
				}

				return &*it;
			};

			for( const refl::value& trait : pending.m_func.get().m_traits )
			{
				switch( trait.get_type_id().m_id )
				{
				case ge::refl::make_type_id< traits::type_erased_order_before >().m_id:
				{
					const traits::type_erased_order_before* order_before
						= trait.as_constant< traits::type_erased_order_before >();

					assert( order_before != nullptr );

					if( pending_system* system_that_runs_after_us = sequence_point_to_func( order_before->m_point ) )
					{
						// This system ^ will run after us
						system_that_runs_after_us->m_execute_after.emplace_back( pending );
					}
					break;
				}
				case ge::refl::make_type_id< traits::type_erased_order_after >().m_id:
				{
					const traits::type_erased_order_after* order_after = trait.as_constant< traits::type_erased_order_after >();

					assert( order_after != nullptr );

					if( pending_system* system_that_runs_before_us = sequence_point_to_func( order_after->m_point ) )
					{
						// We execute after this system
						pending.m_execute_after.emplace_back( *system_that_runs_before_us );
					}
					break;
				}
				default:;
				}
			}
		}

		for( pending_system& system : pending_systems )
		{
			if( system.m_group_idx != s_unassigned_group )
			{
				continue;
			}

			auto assign_group
				= [ &logger, &num_errors ](
					  const auto& self,
					  pending_system& current,
					  std::vector< std::reference_wrapper< const pending_system > > systems_in_loop ) -> std::uint16_t
			{
				// Already processed
				if( current.m_group_idx != s_unassigned_group )
				{
					return current.m_group_idx + 1u;
				}

				systems_in_loop.emplace_back( current );

				if( std::int64_t loop_start_idx
					= std::ranges::find_if(
						  systems_in_loop,
						  [ &current ]( const pending_system& existing ) { return &current == &existing; } )
					  - systems_in_loop.begin();
					loop_start_idx + 1 != std::ssize( systems_in_loop ) )
				{
					std::string error = "invalid ordering: infinite loop:\n";

					for( auto [ idx, system_in_loop ] :
						 systems_in_loop | std::views::drop( loop_start_idx ) | std::views::enumerate )
					{
						error += std::format( "{:3} | '{}'\n", idx, system_in_loop.get().m_func.get().m_name );
					}

					logger.log_raw( severity::error, error );
					num_errors++;
					return 1u;
				}

				if( current.m_execute_after.empty() )
				{
					current.m_group_idx = 0;
					return 1u;
				}

				std::uint16_t highest_group_index{};
				for( pending_system& system_before_us : current.m_execute_after )
				{
					highest_group_index = std::max( self( self, system_before_us, systems_in_loop ), highest_group_index );
				}

				current.m_group_idx = highest_group_index;
				return current.m_group_idx + 1u;
			};

			assign_group( assign_group, system, {} );
		}

		{
			struct accessor
			{
				std::reference_wrapper< const pending_system > m_system;
				enum
				{
					read,
					write
				} m_type;
			};
			using accessors = std::vector< accessor >;

			static constexpr auto is_scheduled_in_this_order = []( const auto& self,
																   const pending_system& potentially_scheduled_before,
																   const pending_system& potentially_scheduled_after ) -> bool
			{
				for( const pending_system& before : potentially_scheduled_after.m_execute_after )
				{
					if( &before == &potentially_scheduled_before || self( self, before, potentially_scheduled_before ) )
					{
						return true;
					}
				}
				return false;
			};

			static constexpr auto is_conflict = []( const accessor& first, const accessor& second )
			{
				return ( first.m_type == accessor::write || second.m_type == accessor::write )
					   && !is_scheduled_in_this_order( is_scheduled_in_this_order, first.m_system, second.m_system );
			};

			std::unordered_map<
				std::reference_wrapper< const refl::type_data >,
				accessors,
				decltype( []( const refl::type_data& type ) { return type.m_id.m_id; } ),
				decltype( []( const refl::type_data& lhs, const refl::type_data& rhs ) { return &lhs == &rhs; } ) >
				accessors_map{};

			for( const pending_system& system : pending_systems )
			{
				for( const refl::type_data& write : system.m_system_trait.get().m_access.m_writes )
				{
					accessors_map[ write ].push_back( accessor{ .m_system = system, .m_type = accessor::write } );
				}

				for( const refl::type_data& read : system.m_system_trait.get().m_access.m_reads )
				{
					accessors_map[ read ].push_back( accessor{ .m_system = system, .m_type = accessor::read } );
				}
			}

			for( auto [ type, accessors ] : accessors_map )
			{
				std::ranges::sort(
					accessors,
					[]( const accessor& lhs, const accessor& rhs )
					{ return lhs.m_system.get().m_group_idx < rhs.m_system.get().m_group_idx; } );

				for( auto [ idx, first ] : accessors | std::views::enumerate )
				{
					for( const accessor& second : accessors | std::views::drop( idx + 1 ) )
					{
						if( is_conflict( first, second ) )
						{
							logger.log(
								error,
								"underconstrained access to '{}': no order specified between '{}' and '{}'",
								type.get().m_name,
								first.m_system.get().m_func.get().m_name,
								second.m_system.get().m_func.get().m_name );
							num_errors++;
						}
					}
				}
			}
		}

		if( num_errors > 0 )
		{
			return std::nullopt;
		}

		execution_graph graph{};

		for( const pending_system system : pending_systems )
		{
			graph.m_groups.resize( system.m_group_idx + 1u );
			graph.m_groups[ system.m_group_idx ].m_nodes.push_back(
				execution_graph::group::system_node{ .m_name = system.m_func.get().m_name } );
		}

		return graph;
	}
} // namespace ge::scheduling
