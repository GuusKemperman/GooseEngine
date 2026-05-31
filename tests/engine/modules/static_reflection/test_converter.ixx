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
        ge::converter conv{};
        std::ostringstream output{};

        conv.begin_module(output, "test_module");
        conv.convert_to_builder(output, src);
        conv.end_module(output);

        return output.str();
    }
}


UNIT_TEST(converter, simple_type)
{
    std::string_view src =
        R"(
REFL_TYPE()
class MyType
{

};

)";
    std::cout << convert(src);


}
