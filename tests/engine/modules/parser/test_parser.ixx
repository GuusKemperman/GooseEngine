export module test_parser;

import std;
import logger;
import parser;
import test_core;

using namespace ge::test_core;
using namespace ge::test_core::assert;

namespace
{
	void print_tokens(std::string_view file)
	{
        ge::tokeniser parser{ file };
        ge::logger logger{};

        logger.log(ge::verbose, "Parsing {}", file);

        while (!parser.m_remaining_file.empty())
        {
            ge::tokeniser::token token = parser.consume_token();
            logger.log(ge::verbose, "{} - {}", static_cast<int>(token.m_flag), token.m_str);
        }
	}
}

UNIT_TEST(tokeniser, complex_function_no_crash)
{
    print_tokens(
"        REFL_FUNC()\n"
"        // Hello we are reflect*/ing this\n"
"        /*\n"
"        /*Comment in comment!\n"
"        // Commentttsss\n"
"        */\n"
R"(        [[nodiscard]] inline /*haha here is another comment */int function_name(int param0_name, std::string param1_name = { "Hello; { \" })}" /*helloo*/ },)" "\n"
"            std::string<char> param2 = (R\"(Hellooo \" \" ))) } [[attribution inside string ]] )\"), int foo = 1.0f);\n"
);
}