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

        for (ge::token token : ge::tokeniser{ file })
        {
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

UNIT_TEST(parser, complex_function_no_crash)
{
    ge::parser parser{};

    [[maybe_unused]] ge::parsed_file file = parser.parse(
        "        REFL_FUNC(DisplayName(\"Hello)()}\"), DisplayName(\"Hello)()}\"),IsScriptable)\n"
        "        // Hello we are reflect*/ing this\n"
        "        /*\n"
        "        /*Comment in comment!\n"
        "        // Commentttsss\n"
        "        */\n"
        R"(        [[nodiscard]] inline /*haha here is another comment */int ___function_name    (int param0_name, std::string param1_name = { "Hello; { \" })}" /*helloo*/ },)" "\n"
        "            std::string<char> param2 = (R\"(Hellooo \" \" ))) } [[attribution inside string ]] )\"), std::array<std::vector<std::pair<int, float>>, 0x401ul > foo = 1.0f);\n"
    );

    is_eq(file.m_funcs.size(), 1);
    is_eq(file.m_funcs.front().m_return_type, "int");
    is_eq(file.m_funcs.front().m_name, "___function_name");
    is_eq(file.m_funcs.front().m_attributes, "DisplayName(\"Hello)()}\"),DisplayName(\"Hello)()}\"),IsScriptable");
    is_eq(file.m_funcs.front().m_parameters.at(0).m_type, "int");
    is_eq(file.m_funcs.front().m_parameters.at(0).m_name, "param0_name");
    is_eq(file.m_funcs.front().m_parameters.at(1).m_type, "std::string");
    is_eq(file.m_funcs.front().m_parameters.at(1).m_name, "param1_name");
    is_eq(file.m_funcs.front().m_parameters.at(2).m_type, "std::string<char>");
    is_eq(file.m_funcs.front().m_parameters.at(2).m_name, "param2");
    is_eq(file.m_funcs.front().m_parameters.at(3).m_type, "std::array<std::vector<std::pair<int,float>>,0x401ul>");
    is_eq(file.m_funcs.front().m_parameters.at(3).m_name, "foo");
    is_eq(file.m_funcs.front().m_parameters.size(), 4);
}