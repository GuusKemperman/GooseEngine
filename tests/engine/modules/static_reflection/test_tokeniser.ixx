export module test_static_reflection:test_tokeniser;


import stl;
import logger;
import static_reflection;
export import test_core;

using namespace ge::test_core;
using namespace ge::test_core::assert;

namespace
{
	void expect_tokens(
		std::string_view source,
		std::initializer_list< ge::token > expected,
		const std::source_location& src = std::source_location::current() )
	{
		ge::token_range range{ source };
		ge::token_iterator it = range.begin();
		size_t index = 0;

		for( const ge::token& expected_token : expected )
		{
			if( it == range.end() )
			{
				failure( std::format( "ran out of tokens at index {}", index ), src );
			}
			if( it->m_str != expected_token.m_str )
			{
				failure( std::format( "token {}: got '{}', expected '{}'", index, it->m_str, expected_token.m_str ), src );
			}
			is_eq( it->m_flag, expected_token.m_flag, src );
			++it;
			index++;
		}
		is_eq( it, range.end(), src );
	}
}

namespace tokeniser
{
	REFL_FUNC( ge::test_core::unit_test_trait{} )

	export API void complex_function_no_crash()
	{
		ge::token_range{
			"        REFL_FUNC()\n"
			"        // Hello we are reflect*/ing this\n"
			"        /*\n"
			"        /*Comment in comment!\n"
			"        // Commentttsss\n"
			"        */\n"
			R"(        [[nodiscard]] inline /*haha here is another comment */int function_name(int param0_name, std::string param1_name = { "Hello; { \" })}" /*helloo*/ },)"
			"\n"
			"            std::string<char> param2 = (R\"(Hellooo \" \" ))) } [[attribution inside string ]] )\"), int foo = 1.0f);\n"
		};
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )

	export API void iterators()
	{
		ge::token_range tokeniser{ "1 2 3 4" };

		( void )expect_exception< std::out_of_range >(
			[&]
			{
				ge::token_iterator end = tokeniser.end();
				++end;
			} );

		ge::token_iterator it = tokeniser.begin();
		is_eq( it->m_str, "1" );
		is_ne( it, tokeniser.end() );
		++it;
		is_eq( it->m_str, " " );
		is_ne( it, tokeniser.end() );
		++it;
		is_eq( it->m_str, "2" );
		is_ne( it, tokeniser.end() );
		++it;
		is_eq( it->m_str, " " );
		is_ne( it, tokeniser.end() );
		++it;
		is_eq( it->m_str, "3" );
		is_ne( it, tokeniser.end() );
		++it;
		is_eq( it->m_str, " " );
		is_ne( it, tokeniser.end() );
		++it;
		is_eq( it->m_str, "4" );
		is_ne( it, tokeniser.end() );
		++it;
		is_eq( it, tokeniser.end() );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )

	export API void empty_input()
	{
		expect_tokens( "", {} );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )

	export API void whitespace_only_collapses()
	{
		expect_tokens(
			"  \t\n  ",
			{
				{ " ", ge::token::flag::white_space },
			} );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )

	export API void whitespace_between_tokens_collapses()
	{
		expect_tokens(
			"a  \n\t b",
			{
				{ "a", ge::token::flag::valid_identifier },
				{ " ", ge::token::flag::white_space },
				{ "b", ge::token::flag::valid_identifier },
			} );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )

	export API void identifiers_with_underscores_and_digits()
	{
		expect_tokens(
			"_under score9",
			{
				{ "_under", ge::token::flag::valid_identifier },
				{ " ", ge::token::flag::white_space },
				{ "score9", ge::token::flag::valid_identifier },
			} );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )

	export API void leading_digit_terminates_before_identifier()
	{
		expect_tokens(
			"9abc",
			{
				{ "9" },
				{ "abc", ge::token::flag::valid_identifier },
			} );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )

	export API void line_comment_discards_newline()
	{
		expect_tokens(
			"a//hello world\nb",
			{
				{ "a", ge::token::flag::valid_identifier },
				{ "hello world", ge::token::flag::comment },
				{ "b", ge::token::flag::valid_identifier },
			} );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )

	export API void line_comment_at_eof_is_unreachable()
	{
		// The comment token is produced, but consuming it advances the iterator
		// one past the last character, which is exactly the end() position.
		// The comment is therefore invisible to a range-based iteration.
		ge::token_range range{ "//hi" };
		is_eq( range.begin(), range.end() );
		is_eq( range.begin()->m_str, "hi" );
		is_eq( range.begin()->m_flag, ge::token::flag::comment );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )

	export API void block_comment()
	{
		expect_tokens(
			"a/* hi */b",
			{
				{ "a", ge::token::flag::valid_identifier },
				{ " hi ", ge::token::flag::comment },
				{ "b", ge::token::flag::valid_identifier },
			} );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )

	export API void block_comments_do_not_nest()
	{
		expect_tokens(
			"/*a/*b*/c",
			{
				{ "a/*b", ge::token::flag::comment },
				{ "c", ge::token::flag::valid_identifier },
			} );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )

	export API void unterminated_block_comment_overshoots()
	{
		// Documents current behavior: the unterminated comment consumes to the end
		// of the file plus the length of the missing "*/", so the iterator is not
		// equal to end() and incrementing it again throws.
		ge::token_range range{ "/*abc" };
		ge::token_iterator it = range.begin();
		is_eq( it->m_str, "abc" );
		is_eq( it->m_flag, ge::token::flag::comment );
		is_ne( it, range.end() );

		( void )expect_exception< std::out_of_range >(
			[&]
			{
				++it;
			} );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )

	export API void attribute_token_strips_brackets()
	{
		expect_tokens(
			"[[nodiscard]]int",
			{
				{ "nodiscard", ge::token::flag::attribute },
				{ "int", ge::token::flag::valid_identifier },
			} );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )

	export API void string_literal_is_single_token()
	{
		expect_tokens(
			"a = \"hello world\";",
			{
				{ "a", ge::token::flag::valid_identifier },
				{ " ", ge::token::flag::white_space },
				{ "=" },
				{ " ", ge::token::flag::white_space },
				{ "\"hello world\"" },
				{ ";" },
			} );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )

	export API void string_with_escaped_quotes()
	{
		expect_tokens(
			R"(x = "say \"hi\"";)",
			{
				{ "x", ge::token::flag::valid_identifier },
				{ " ", ge::token::flag::white_space },
				{ "=" },
				{ " ", ge::token::flag::white_space },
				{ R"("say \"hi\"")" },
				{ ";" },
			} );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )

	export API void string_prefixes_included_in_token()
	{
		expect_tokens(
			"L\"w\" u8\"a\" u\"b\" U\"c\"",
			{
				{ "L\"w\"" },
				{ " ", ge::token::flag::white_space },
				{ "u8\"a\"" },
				{ " ", ge::token::flag::white_space },
				{ "u\"b\"" },
				{ " ", ge::token::flag::white_space },
				{ "U\"c\"" },
			} );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )

	export API void raw_string_is_single_token()
	{
		expect_tokens(
			"R\"(quote \" bracket })\" x",
			{
				{ "R\"(quote \" bracket })\"" },
				{ " ", ge::token::flag::white_space },
				{ "x", ge::token::flag::valid_identifier },
			} );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )

	export API void char_literals()
	{
		// Only plain characters and the escaped quote are supported.
		expect_tokens(
			"'a' '\\''",
			{
				{ "'a'" },
				{ " ", ge::token::flag::white_space },
				{ "'\\''" },
			} );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )

	export API void multi_char_operators_combine()
	{
		expect_tokens(
			"a::b->c&&d",
			{
				{ "a", ge::token::flag::valid_identifier },
				{ "::" },
				{ "b", ge::token::flag::valid_identifier },
				{ "->" },
				{ "c", ge::token::flag::valid_identifier },
				{ "&&" },
				{ "d", ge::token::flag::valid_identifier },
			} );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )

	export API void other_operators_do_not_combine()
	{
		// Only "::", "->" and "&&" are combined into a single token.
		expect_tokens(
			"a<<b==c",
			{
				{ "a", ge::token::flag::valid_identifier },
				{ "<" },
				{ "<" },
				{ "b", ge::token::flag::valid_identifier },
				{ "=" },
				{ "=" },
				{ "c", ge::token::flag::valid_identifier },
			} );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )

	export API void number_literals()
	{
		expect_tokens(
			"123 1.5 1'000 1e-5 0b101 0x1F",
			{
				{ "123" },
				{ " ", ge::token::flag::white_space },
				{ "1.5" },
				{ " ", ge::token::flag::white_space },
				{ "1'000" },
				{ " ", ge::token::flag::white_space },
				{ "1e-5" },
				{ " ", ge::token::flag::white_space },
				{ "0b101" },
				{ " ", ge::token::flag::white_space },
				{ "0x1F" },
			} );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )

	export API void bracket_counting()
	{
		ge::token_range range{ "(({}))" };
		ge::token_iterator it = range.begin();
		is_eq( it->m_str, "(" );
		is_eq( it.parentheses_count(), 1 );
		++it;
		is_eq( it->m_str, "(" );
		is_eq( it.parentheses_count(), 2 );
		++it;
		is_eq( it->m_str, "{" );
		is_eq( it.curly_bracket_count(), 1 );
		++it;
		is_eq( it->m_str, "}" );
		is_eq( it.curly_bracket_count(), 0 );
		is_eq( it.parentheses_count(), 2 );
		++it;
		is_eq( it->m_str, ")" );
		is_eq( it.parentheses_count(), 1 );
		++it;
		is_eq( it->m_str, ")" );
		is_eq( it.parentheses_count(), 0 );
		++it;
		is_eq( it, range.end() );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )

	export API void line_numbers()
	{
		ge::token_range range{ "a\nb\n\nc" };
		ge::token_iterator it = range.begin();
		is_eq( it->m_str, "a" );
		is_eq( it.get_source().m_line_number, 1u );
		++it; // whitespace
		++it;
		is_eq( it->m_str, "b" );
		is_eq( it.get_source().m_line_number, 2u );
		++it; // whitespace spanning two newlines
		++it;
		is_eq( it->m_str, "c" );
		is_eq( it.get_source().m_line_number, 4u );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )

	export API void postfix_increment_returns_previous()
	{
		ge::token_range range{ "a b" };
		ge::token_iterator it = range.begin();
		ge::token_iterator old = it++;
		is_eq( old->m_str, "a" );
		is_eq( it->m_str, " " );
	}
}
