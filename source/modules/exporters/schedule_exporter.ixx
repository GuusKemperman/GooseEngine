module;

#include <assert.h>

export module exporters:schedule;

import runtime_reflection;
import stl;
import io;
import utils;
import core_traits;
import :environments;

namespace ge::exporter
{
	// Each function in a execution_group can be executed using parallel-for without any race conditions on environments/entities
	export using execution_group = std::vector< std::reference_wrapper< const refl::func_data > >;
	export using execution_graph = std::vector< execution_group >;

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
			auto sequence_point_to_func = [ &pending, &pending_systems, &logger, &num_errors ](
											  const traits::details::sequence_point point ) -> pending_system*
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

			// TODO replace with try_get_trait
			for( const refl::value& trait : pending.m_func.get().m_traits )
			{
				if( trait.get_type_id() != refl::make_type_id< traits::details::ordering_base >() )
				{
					continue;
				}

				const traits::details::ordering_base* order_request = trait.as_constant< traits::details::ordering_base >();
				assert( order_request != nullptr );

				pending_system* ordered_against = sequence_point_to_func( order_request->m_point );

				if( ordered_against == nullptr )
				{
					// sequence_point_to_func reported the error for us
					continue;
				}

				switch( order_request->m_type )
				{
				case traits::details::ordering_base::before:
				{
					if( pending_system* system_that_runs_after_us = sequence_point_to_func( order_request->m_point ) )
					{
						system_that_runs_after_us->m_execute_after.emplace_back( pending );
					}
					break;
				}
				case traits::details::ordering_base::after:
				{
					if( pending_system* system_that_runs_before_us = sequence_point_to_func( order_request->m_point ) )
					{
						pending.m_execute_after.emplace_back( *system_that_runs_before_us );
					}
					break;
				}
				default:
					std::unreachable();
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
			using access = traits::details::system_param::access;

			struct accessor
			{
				std::reference_wrapper< const pending_system > m_system;
				access m_access{};
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
				return ( first.m_access == access::write || second.m_access == access::write )
					   && !is_scheduled_in_this_order( is_scheduled_in_this_order, first.m_system, second.m_system )
					   && &first.m_system.get() != &second.m_system.get();
			};

			std::unordered_map<
				std::reference_wrapper< const refl::type_data >,
				accessors,
				decltype( []( const refl::type_data& type ) { return type.m_id.m_id; } ),
				decltype( []( const refl::type_data& lhs, const refl::type_data& rhs ) { return &lhs == &rhs; } ) >
				accessors_map{};

			for( const pending_system& system : pending_systems )
			{
				for( const traits::details::system_param& param : system.m_system_trait.get().m_params )
				{
					accessors_map[ param.m_type.get() ].push_back( accessor{ .m_system = system, .m_access = param.m_access } );
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
			graph.resize( std::max< size_t >( system.m_group_idx + 1u, graph.size() ) );
			graph[ system.m_group_idx ].push_back( system.m_func );
		}

		return graph;
	}

	// runtime
	export struct exported_scheduled_system
	{
		// TODO don't directly store a function ptr in an exported pack, they are very much not stable
		traits::system::invoke_t m_invoke{};
		rel::ptr< void* > m_arguments_buffer{};
	};

	export using exported_scheduled_group = rel::span< const exported_scheduled_system >;
	export using exported_schedule = rel::span< const exported_scheduled_group >;

	export API const exported_schedule& export_schedule(
		pack_writer& writer,
		execution_graph graph,
		const environments_map& environments_map )
	{
		std::span< exported_scheduled_group > exported_groups = writer.emplace_array< exported_scheduled_group >( graph.size() );
		exported_schedule& schedule = writer.emplace< exported_schedule >( exported_groups );

		for( auto [ intermediate_group, exported_group ] : std::views::zip( graph, exported_groups ) )
		{
			std::span< exported_scheduled_system > exported_systems
				= writer.emplace_array< exported_scheduled_system >( intermediate_group.size() );

			exported_group = exported_scheduled_group{ exported_systems };

			for( auto [ intermediate_system, exported_system ] : std::views::zip( intermediate_group, exported_systems ) )
			{
				const traits::system& system_trait
					= *refl::find_value_of_type< traits::system >( intermediate_system.get().m_traits );

				std::span< void* > arguments_buffer = writer.emplace_array< void* >( system_trait.m_params.size() );

				for( auto [ reflected_param, arg_ptr ] : std::views::zip( system_trait.m_params, arguments_buffer ) )
				{
					arg_ptr = environments_map.at( reflected_param.m_type.get().m_id );
				}

				exported_system.m_arguments_buffer = rel::ptr< void* >{ arguments_buffer.data() };
				exported_system.m_invoke = system_trait.m_invoke;
			}
		}

		return schedule;
	}

	export API void execute_schedule( const exported_schedule& schedule )
	{
		for( const exported_scheduled_group& group : schedule )
		{
			std::for_each(
				std::execution::par_unseq,
				group.begin(),
				group.end(),
				[]( const auto& system ) { system.m_invoke( system.m_arguments_buffer.get() ); } );
		}
	}
} // namespace ge::exporter
