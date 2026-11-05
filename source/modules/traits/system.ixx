module;

#include <cassert>

export module core_traits:system;

import runtime_reflection;
import stl;

namespace ge::traits
{
	namespace details
	{
		export struct system_param
		{
			enum class access : std::uint8_t
			{
				read,
				write
			} m_access{};
			std::reference_wrapper< const refl::type_data > m_type;
		};
	} // namespace details

	export struct system : refl::func_trait
	{
		refl::func_id m_id{};

		std::vector< details::system_param > m_params{};

		std::vector< details::system_param > ( *m_populate_accesses )( const refl::builders::post_build_context& );

		using invoke_t = void ( * )( void** );
		invoke_t m_invoke{};

		template< auto& Func >
		void on_apply( const refl::builders::func_builder< Func >& )
		{
			m_id = refl::make_func_id< Func >();

			m_populate_accesses = +[]( const refl::builders::post_build_context& context )
			{
				return [ & ]< typename Ret, typename... ParamsT >( refl::func_sig< Ret( ParamsT... ) > )
				{
					return [ & ]< size_t... Indices >( std::index_sequence< Indices... > )
					{
						std::vector< details::system_param > params{
							[ & ]< typename ParamT, size_t Idx >()
							{
								static_assert( std::is_lvalue_reference_v< ParamT >, "Only const T& and T& are supported" );

								using NonRef = std::remove_reference_t< ParamT >;
								using Undecorated = std::remove_const_t< NonRef >;

								auto it = std::ranges::find_if(
									context.m_reg.m_types,
									[]( const refl::type_data& type )
									{ return type.m_id == refl::make_type_id< Undecorated >(); } );

								assert( it != context.m_reg.m_types.end() && "Parameter type was either not reflected" );

								return details::system_param{ .m_access = std::is_const_v< NonRef >
																			  ? details::system_param::access::read
																			  : details::system_param::access::write,
															  .m_type = *it };
							}.template operator()< ParamsT, Indices >()...
						};

						return params;
					}( std::make_index_sequence< sizeof...( ParamsT ) >() );
				}( refl::func_sig_t< decltype( Func ) >{} );
			};

			m_invoke = +[]( void** args )
			{
				[ & ]< typename Ret, typename... ParamsT >( refl::func_sig< Ret( ParamsT... ) > )
				{
					[ & ]< size_t... Indices >( std::index_sequence< Indices... > )
					{
						std::invoke(
							Func,
							[ & ]< typename ParamT, size_t Idx >() -> ParamT
																	  {
																		  static_assert(
																			  std::is_reference_v< ParamT >,
																			  "Only references are supported" );
																		  void* arg_address = args[ Idx ];
																		  using NonRef = std::remove_reference_t< ParamT >;

																		  return *std::bit_cast< NonRef* >( arg_address );
																	  }.template operator()< ParamsT, Indices >()... );
					}( std::make_index_sequence< sizeof...( ParamsT ) >() );
				}( refl::func_sig_t< decltype( Func ) >{} );
			};
		}

		inline void post_build( const refl::builders::post_build_context& context, const refl::func_data& )
		{
			m_params = m_populate_accesses( context );
		}
	};

	namespace details
	{
		export struct ordering_base : refl::func_trait
		{
			refl::func_id m_other_func{};

			enum type : bool
			{
				before,
				after
			} m_type{};
		};
	} // namespace details

	export template< auto& OtherSystem, details::ordering_base::type Type >
		requires ge::refl::is_func< OtherSystem >
	struct ordering : refl::func_trait
	{
		template< auto& Func >
		void on_apply( refl::builders::func_builder< Func >& func )
		{
			func.add_traits( details::ordering_base{ .m_other_func = refl::make_func_id< OtherSystem >(), .m_type = Type } );
		}
	};

	export template< auto& OtherSystem >
	using order_before = ordering< OtherSystem, details::ordering_base::type::before >;

	export template< auto& OtherSystem >
	using order_after = ordering< OtherSystem, details::ordering_base::type::after >;
} // namespace ge::traits
