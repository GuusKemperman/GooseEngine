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
	std::string convert(std::string_view src)
	{
		std::array partitions{ ge::converter::module_partition{.m_file_name = "test_file.ixx", .m_parse_result = ge::parse(ge::token_range{ src }) } };
		ge::converter::module module{ .m_name ="test_module", .m_partitions = partitions };
		return ge::converter::convert_module(module);
	}

	std::string strip_whitespace(std::string_view str)
	{
		std::string result{};
		result.reserve(str.size());
		std::ranges::copy_if(str, std::back_inserter(result),
			[](char ch)
			{
				return !std::isspace(static_cast<unsigned char>(ch));
			});
		return result;
	}

	void contains_in_order(std::string_view output, std::initializer_list<std::string_view> fragments,
		const std::source_location& src = std::source_location::current())
	{
		const std::string haystack = strip_whitespace(output);
		size_t offset = 0;

		for (std::string_view fragment : fragments)
		{
			const size_t pos = haystack.find(strip_whitespace(fragment), offset);
			if (pos == std::string::npos)
			{
				failure(std::format("fragment '{}' not found (in order) in output:\n{}", fragment, output), src);
			}
			offset = pos + 1;
		}
	}

	void does_not_contain(std::string_view output, std::string_view fragment,
		const std::source_location& src = std::source_location::current())
	{
		if (strip_whitespace(output).find(strip_whitespace(fragment)) != std::string::npos)
		{
			failure(std::format("fragment '{}' unexpectedly found in output:\n{}", fragment, output), src);
		}
	}
}

namespace converter
{
	REFL_FUNC(ge::test_core::unit_test_trait{})
	export API void empty_module()
	{
		std::string output = convert("");

		contains_in_order(output,
			{
				"import test_module;",
				"import runtime_reflection;",
				"extern \"C\"",
				"void build_runtime_reflection(ge::refl::builders::registry_builder& builder)",
				"builder.begin_module(\"test_module\")",
				".end_module();",
			});

		does_not_contain(output, ".begin_func");
		does_not_contain(output, ".begin_data");
		does_not_contain(output, ".begin_type");
	}

	REFL_FUNC(ge::test_core::unit_test_trait{})
	export API void global_func_and_data()
	{
		std::string output = convert(
			"REFL_FUNC(ge::my_trait{})\n"
			"int foo();\n"
			"REFL_DATA()\n"
			"static int bar = 5;\n");

		// Globals are qualified with the root scope "::"; funcs are emitted before data.
		contains_in_order(output,
			{
				"builder.begin_module(\"test_module\")",
				".begin_func<&::foo>(\"foo\")",
				".add_traits(ge::my_trait{})",
				".end_func()",
				".begin_data<&::bar>(\"bar\")",
				".end_data()",
				".end_module();",
			});
	}

	REFL_FUNC(ge::test_core::unit_test_trait{})
	export API void nested_type_in_namespace()
	{
		std::string output = convert(
			"namespace outer\n"
			"{\n"
			"    REFL_TYPE()\n"
			"    struct widget\n"
			"    {\n"
			"        REFL_FUNC()\n"
			"        void method();\n"
			"        REFL_DATA()\n"
			"        int field = 0;\n"
			"    };\n"
			"\n"
			"    REFL_FUNC()\n"
			"    void helper();\n"
			"}\n");

		// begin_type takes the type itself (no '&'); members are fully qualified
		// and nested inside begin_type/end_type; types come before free funcs.
		contains_in_order(output,
			{
				".begin_type<::outer::widget>(\"widget\")",
				".begin_func<&::outer::widget::method>(\"method\")",
				".end_func()",
				".begin_data<&::outer::widget::field>(\"field\")",
				".end_data()",
				".end_type()",
				".begin_func<&::outer::helper>(\"helper\")",
			});
	}

	REFL_FUNC(ge::test_core::unit_test_trait{})
	export API void no_traits_emits_no_add_traits()
	{
		std::string output = convert(
			"REFL_FUNC()\n"
			"void plain();\n");

		contains_in_order(output, { ".begin_func<&::plain>(\"plain\")", ".end_func()" });
		does_not_contain(output, ".add_traits");
	}

	REFL_FUNC(ge::test_core::unit_test_trait{})
	export API void parse_error_emits_static_assert()
	{
		std::string output = convert(
			"REFL_DATA()\n"
			"int ? = 5;\n");

		// The error path must replace the module boilerplate, not wrap it.
		contains_in_order(output,
			{
				"#line",
				"\"test_file.ixx\"",
				"static_assert(false,",
			});
		does_not_contain(output, "begin_module");
	}

	REFL_FUNC(ge::test_core::unit_test_trait{})
	export API void multiple_partitions_share_one_module()
	{
		std::array partitions{
			ge::converter::module_partition{ .m_file_name = "first.ixx", .m_parse_result = ge::parse(ge::token_range{ "REFL_FUNC()\nvoid from_first();" }) },
			ge::converter::module_partition{ .m_file_name = "second.ixx", .m_parse_result = ge::parse(ge::token_range{ "REFL_FUNC()\nvoid from_second();" }) } };
		ge::converter::module module{ .m_name = "multi_module", .m_partitions = partitions };

		std::string output = ge::converter::convert_module(module);

		contains_in_order(output,
			{
				"builder.begin_module(\"multi_module\")",
				".begin_func<&::from_first>(\"from_first\")",
				".begin_func<&::from_second>(\"from_second\")",
				".end_module();",
			});

		const std::string stripped = strip_whitespace(output);
		const size_t first_occurrence = stripped.find("begin_module");
		is_true(first_occurrence != std::string::npos);
		is_eq(stripped.find("begin_module", first_occurrence + 1), std::string::npos);
	}
}
