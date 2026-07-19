export module test_static_reflection:test_converter;

import stl;
import static_reflection;
export import test_core;

using namespace ge::test_core;
using namespace ge::test_core::assert;

namespace
{
	std::string convert(std::string_view src)
	{
		std::array partitions{ ge::converter::module_partition{.m_file_name = "test_file.ixx", .m_parse_result = ge::parse(ge::token_range{ src }) } };
		ge::converter::module module{ .m_name ="test_module", .m_partitions = partitions };
		return ge::converter::convert_module(module);
	}
}

// TODO: End-to-end Tests that take C++ source code and determines if the output is as expected. Only a handful of tests for different edge cases; if we ever change the output format, we dont want to update hundreds of tests.

