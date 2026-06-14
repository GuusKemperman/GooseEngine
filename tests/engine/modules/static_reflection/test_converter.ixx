export module test_static_reflection:test_converter;

import stl;
import logger;
import static_reflection;
import test_core;

using namespace ge::test_core;
using namespace ge::test_core::assert;

namespace
{
	std::string convert(std::string_view src)
	{
		ge::parsed_file parsed_file = ge::parse(ge::token_range{ src });

		std::string output = ge::begin_generated_file("test_module");
		output += ge::convert_source_file(parsed_file);
		output += ge::end_generated_file();
		return output;
	}
}


namespace converter
{
	REFL_FUNC(unit_test{})
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
