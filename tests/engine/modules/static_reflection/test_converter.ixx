export module test_static_reflection:test_converter;

import stl;
import static_reflection;
export import test_core;

using namespace ge::test_core;
using namespace ge::test_core::assert;

// Assertion policy: these end-to-end tests check for semantically essential
// fragments, in order and ignoring all whitespace - never the full golden
// output. Indentation, newlines and boilerplate may change freely without
// breaking them; the builder calls, their qualification, nesting and order
// may not.
namespace
{
	struct source_file
	{
		std::string_view m_name{};
		std::string_view m_src{};
	};

	std::string convert( std::string_view module_name, std::initializer_list< source_file > files )
	{
		std::vector< ge::converter::module_partition > partitions{};
		for( const source_file& file : files )
		{
			partitions.push_back(
				{ .m_file_name = std::string{ file.m_name }, .m_parse_result = ge::parse( ge::token_range{ file.m_src } ) } );
		}

		ge::converter::module module{ .m_name = module_name, .m_partitions = partitions };
		return ge::converter::convert_module( module );
	}

	std::string convert( std::string_view src )
	{
		return convert( "test_module", { { "test_file.ixx", src } } );
	}

	std::string strip_whitespace( std::string_view str )
	{
		std::string result{};
		result.reserve( str.size() );
		std::ranges::copy_if(
			str,
			std::back_inserter( result ),
			[]( char ch ) { return !std::isspace( static_cast< unsigned char >( ch ) ); } );
		return result;
	}

	void contains_in_order(
		std::string_view output,
		std::initializer_list< std::string_view > fragments,
		const std::source_location& src = std::source_location::current() )
	{
		const std::string haystack = strip_whitespace( output );
		size_t offset = 0;

		for( std::string_view fragment : fragments )
		{
			const size_t pos = haystack.find( strip_whitespace( fragment ), offset );
			if( pos == std::string::npos )
			{
				failure( std::format( "fragment '{}' not found (in order) in output:\n{}", fragment, output ), src );
			}
			offset = pos + 1;
		}
	}

	void does_not_contain(
		std::string_view output,
		std::initializer_list< std::string_view > fragments,
		const std::source_location& src = std::source_location::current() )
	{
		const std::string haystack = strip_whitespace( output );
		for( std::string_view fragment : fragments )
		{
			if( haystack.find( strip_whitespace( fragment ) ) != std::string::npos )
			{
				failure( std::format( "fragment '{}' unexpectedly found in output:\n{}", fragment, output ), src );
			}
		}
	}

	size_t count_occurrences( std::string_view output, std::string_view fragment )
	{
		const std::string haystack = strip_whitespace( output );
		const std::string needle = strip_whitespace( fragment );

		size_t count = 0;
		for( size_t pos = haystack.find( needle ); pos != std::string::npos; pos = haystack.find( needle, pos + needle.size() ) )
		{
			count++;
		}
		return count;
	}
} // namespace

namespace converter
{
	REFL_FUNC( ge::test_core::unit_test_trait{} )
	export API void empty_module()
	{
		std::string output = convert( "" );

		contains_in_order(
			output,
			{
				"import test_module;",
				"import runtime_reflection;",
				"extern \"C\"",
				"void build_runtime_reflection(ge::refl::builders::registry_builder& builder)",
				"builder.begin_module(\"test_module\")",
				".end_module();",
			} );

		does_not_contain( output, { ".begin_func", ".begin_data", ".begin_type" } );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )
	export API void global_func_and_data()
	{
		std::string output = convert( "REFL_DATA()\n"
									  "static int bar = 5;\n"
									  "REFL_FUNC(ge::my_trait{})\n"
									  "int foo();\n" );

		// Globals are qualified with the root scope "::"; funcs are emitted before data.
		contains_in_order(
			output,
			{
				"builder.begin_module(\"test_module\")",
				".begin_func<&::foo>(\"foo\")",
				".add_traits(ge::my_trait{})",
				".end_func()",
				".begin_data<&::bar>(\"bar\")",
				".end_data()",
				".end_module();",
			} );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )
	export API void nested_type_in_namespace()
	{
		// Declared in reverse of the emit order, so the order asserts can't pass on source order alone.
		std::string output = convert( "namespace outer\n"
									  "{\n"
									  "    REFL_FUNC()\n"
									  "    void helper();\n"
									  "\n"
									  "    REFL_TYPE()\n"
									  "    struct widget\n"
									  "    {\n"
									  "        REFL_DATA()\n"
									  "        int field = 0;\n"
									  "        REFL_FUNC()\n"
									  "        void method();\n"
									  "    };\n"
									  "}\n" );

		// begin_type takes the type itself (no '&'); members are fully qualified
		// and nested inside begin_type/end_type; types come before free funcs.
		contains_in_order(
			output,
			{
				".begin_type<::outer::widget>(\"widget\")",
				".begin_func<&::outer::widget::method>(\"method\")",
				".end_func()",
				".begin_data<&::outer::widget::field>(\"field\")",
				".end_data()",
				".end_type()",
				".begin_func<&::outer::helper>(\"helper\")",
			} );

		// Empty REFL_*() traits emit no add_traits call.
		does_not_contain( output, { ".add_traits" } );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )
	export API void parse_error_emits_static_assert()
	{
		std::string output = convert( "REFL_DATA()\n"
									  "int ? = 5;\n" );

		// The error path must replace the module boilerplate, not wrap it.
		contains_in_order(
			output,
			{
				"#line",
				"\"test_file.ixx\"",
				"static_assert(false,",
			} );
		does_not_contain( output, { "begin_module" } );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )
	export API void multiple_partitions_share_one_module()
	{
		std::string output = convert(
			"multi_module",
			{
				{ "first.ixx", "REFL_FUNC()\nvoid from_first();" },
				{ "second.ixx", "REFL_FUNC()\nvoid from_second();" },
			} );

		contains_in_order(
			output,
			{
				"builder.begin_module(\"multi_module\")",
				".begin_func<&::from_first>(\"from_first\")",
				".begin_func<&::from_second>(\"from_second\")",
				".end_module();",
			} );

		is_eq( count_occurrences( output, "begin_module" ), 1ull );
	}
} // namespace converter
