export module test_core;

import stl;
import runtime_reflection;

namespace ge::test_core
{
	export struct unit_test_trait : refl::func_trait
	{
		template<auto Func>
		static void on_apply( const refl::builders::func_builder< Func >& builder )
		{
			builder.add_traits( ge::refl::invocable_trait< void() >{} );
		}
	};

	export class test_exception
		: public std::exception
	{
	public:
		API test_exception( const std::source_location& a_src = std::source_location::current() )
			: m_src( a_src ),
			  m_what(
				  std::format(
					  "{}({})",
					  m_src.file_name(),
					  m_src.line() ) )
		{
		}

		API test_exception( std::string_view a_what, const std::source_location& a_src = std::source_location::current() )
			: test_exception( std::move( a_src ) )
		{
			m_what = std::format( "{} - {}", m_what, a_what );
		}

		API const char* what() const override
		{
			return m_what.c_str();
		}

		std::source_location m_src{};
		std::string m_what{};
	};

	namespace assert
	{
		export [[noreturn]] API void failure(
			const std::source_location& src = std::source_location::current() )
		{
			throw test_exception{ src };
		}

		export [[noreturn]] API void failure(
			std::string_view msg,
			const std::source_location& src = std::source_location::current() )
		{
			throw test_exception{ msg, src };
		}

		export API void is_true(
			bool cond,
			const std::source_location& src = std::source_location::current() )
		{
			if( !cond )
			{
				failure( src );
			}
		}

		export API void is_false(
			bool cond,
			const std::source_location& src = std::source_location::current() )
		{
			return is_true( !cond, src );
		}

		export template<typename exception_t, typename func_t> requires std::invocable< func_t >
		exception_t expect_exception(
			func_t&& a_func,
			const std::source_location& src = std::source_location::current() )
		{
			try
			{
				( void )std::invoke( a_func );
				failure( src );
			}
			catch( const exception_t& e )
			{
				return e;
			}
			catch( ... )
			{
				throw test_exception{ "different exception type", src };
			}
		}

		export void is_null(
			const auto& ptr,
			const std::source_location& src = std::source_location::current() ) requires requires
		{
			{ ptr == nullptr } -> std::same_as< bool >;
		}
		{
			return is_true( ptr == nullptr, src );
		}

		export void is_not_null(
			const auto& ptr,
			const std::source_location& src = std::source_location::current() ) requires requires
		{
			{ ptr != nullptr } -> std::same_as< bool >;
		}
		{
			return is_true( ptr != nullptr, src );
		}

		template<typename t1, typename t2>
		void assert_template(
			std::string_view operator_as_txt,
			bool failed,
			const t1& lhs,
			const t2& rhs,
			const std::source_location& src )
		{
			if( failed )
			{
				return;
			}

			if constexpr( std::formattable< t1, char > && std::formattable< t2, char > )
			{
				failure( std::format( "assert failed: {} {} {}", lhs, operator_as_txt, rhs ), src );
			}
			else
			{
				failure( src );
			}
		}

		export template<typename t1, typename t2>
		void is_eq(
			const t1& lhs,
			const t2& rhs,
			const std::source_location& src = std::source_location::current() ) requires requires
		{
			{ lhs == rhs } -> std::same_as< bool >;
		}
		{
			assert_template( "==", lhs == rhs, lhs, rhs, src );
		}

		export void is_ne(
			const auto& lhs,
			const auto& rhs,
			const std::source_location& src = std::source_location::current() ) requires requires
		{
			{ lhs != rhs } -> std::same_as< bool >;
		}
		{
			assert_template( "!=", lhs != rhs, lhs, rhs, src );
		}

		export void is_lt(
			const auto& lhs,
			const auto& rhs,
			const std::source_location& src = std::source_location::current() ) requires requires
		{
			{ lhs < rhs } -> std::same_as< bool >;
		}
		{
			assert_template( "<", lhs < rhs, lhs, rhs, src );
		}

		export void is_gt(
			const auto& lhs,
			const auto& rhs,
			const std::source_location& src = std::source_location::current() ) requires requires
		{
			{ lhs > rhs } -> std::same_as< bool >;
		}
		{
			assert_template( ">", lhs > rhs, lhs, rhs, src );
		}

		export void is_le(
			const auto& lhs,
			const auto& rhs,
			const std::source_location& src = std::source_location::current() ) requires requires
		{
			{ lhs <= rhs } -> std::same_as< bool >;
		}
		{
			assert_template( "<=", lhs <= rhs, lhs, rhs, src );
		}

		export void is_ge(
			const auto& lhs,
			const auto& rhs,
			const std::source_location& src = std::source_location::current() ) requires requires
		{
			{ lhs >= rhs } -> std::same_as< bool >;
		}
		{
			assert_template( ">=", lhs >= rhs, lhs, rhs, src );
		}
	}

	// TODO make equivalents that continue execution after a failed assertion 
	export namespace expect = assert;
}
