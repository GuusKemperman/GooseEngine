export module test_static_reflection:test_converter;

import stl;
import logger;
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

REFL_FUNC(ge::test_core::unit_test_trait{})
export API void simple_type()
{
	std::string_view src =
		R"(
namespace thing
{
	REFL_FUNC(ge::test_core::unit_test_trait{})
		export API void dummy()
	{
		ge::test_core::assert::is_false(true);
	}

}
)";
	std::cout << convert(src);
}


namespace converter
{
	REFL_FUNC(ge::test_core::unit_test_trait{})
	export API void simple_type()
	{
		std::string_view src =
			R"(
REFL_TYPE()
class MyType
{
public:
REFL_DATA()
int my_member = 10;

// This is not legal cus strings are not attributes but whatever
REFL_FUNC(std::string_view{"hello"})
int my_func(int, float);
};

REFL_FUNC()
int my_global_func(int, float);

)";
		std::cout << convert(src);
	}
}
