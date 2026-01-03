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
        R"(        [[nodiscard]] inline /*haha here is another comment */ API  int ___function_name    (int param0_name, std::string param1_name = { "Hello; { \" })}" /*helloo*/ },)" "\n"
        "            std::string<char> param2 = (R\"(Hellooo \" \" ))) } [[attribution inside string ]] )\"), std::array<std::vector<std::pair<int, float>>, 0x401ul > foo = 1.0f);\n"
    );

    is_eq(file.m_funcs.size(), 1ull);
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
    is_eq(file.m_funcs.front().m_parameters.size(), 4ull);
}

UNIT_TEST(tokeniser, func_signature)
{
    ge::tokeniser tokeniser{ "int func_name() { return 1; }" };

    ge::token_iterator it = tokeniser.begin();
    is_eq(it->m_str, "int"); ++it;
    is_eq(it->m_flag, ge::token::flag::white_space); ++it;
    is_eq(it->m_str, "func_name"); ++it;
    is_eq(it->m_str, "("); ++it;
    is_eq(it->m_str, ")"); ++it;
    is_eq(it->m_flag, ge::token::flag::white_space); ++it;
    is_eq(it->m_str, "{"); ++it;
    is_eq(it->m_flag, ge::token::flag::white_space); ++it;
    is_eq(it->m_str, "return"); ++it;
    is_eq(it->m_flag, ge::token::flag::white_space); ++it;
    is_eq(it->m_str, "1"); ++it;
    is_eq(it->m_str, ";"); ++it;
    is_eq(it->m_flag, ge::token::flag::white_space); ++it;
    is_eq(it->m_str, "}"); ++it;
    is_eq(it, tokeniser.end());
}

UNIT_TEST(parser, complete_file)
{
    ge::parser parser{};

    std::string_view src =

        "REFL_FUNC()\n"
        "int global() { return 1; }\n"
		"REFL_DATA()\n"
		"int global_data = 5;"
        "\n"
        "namespace first\n"
        "{\n"
		"REFL_DATA()\n"
        "static inline API std::vector<std::string<char>> global_data;\n"
        "REFL_DATA()\n"
        "static inline API std::vector<std::string<char>> global_data_2 = global_data;\n"

        "REFL_DATA()\n"
        "static inline API std::vector<std::string<char>> global_data_3{global_data};\n"
        "REFL_DATA()\n"
        "static inline API std::vector<std::string<char>> global_data_4(global_data);\n"

	"REFL_FUNC()\n"
        "	int first_func() { return 70; }\n"
        "	\n"
        "    namespace second\n"
        "	{\n"
        "REFL_FUNC()\n"
        "		static int second()\n"
        "		{\n"
        "            return 2;\n"
        "		}\n"
        "	}\n"
        "\n"
        "	namespace third\n"
        "	{\n"
        "REFL_FUNC()\n"
        "        int third() { return 5; }\n"
        "	}\n"
        "}\n"
        "\n"
        "namespace\n"
        "{\n"
        "   REFL_FUNC()\n"
        "    int anon() { return 6; }\n"
        "}";
    
	ge::parsed_file file = parser.parse(src);

    is_eq(file.m_funcs.at(0).m_return_type, "int");
    is_eq(file.m_funcs.at(0).m_name, "global");
    is_eq(file.m_data.at(0).m_type, "int");
    is_eq(file.m_data.at(0).m_name, "global_data");
	
	is_eq(file.m_namespaces.at(0).get().m_data.at(0).m_type, "std::vector<std::string<char>>");
	is_eq(file.m_namespaces.at(0).get().m_data.at(0).m_name, "global_data");
	is_true(file.m_namespaces.at(0).get().m_data.at(0).m_has_inline_keyword);
	is_true(file.m_namespaces.at(0).get().m_data.at(0).m_has_static_keyword);
	
    is_eq(file.m_namespaces.at(0).get().m_data.at(1).m_type, "std::vector<std::string<char>>");
    is_eq(file.m_namespaces.at(0).get().m_data.at(1).m_name, "global_data_2");
    is_true(file.m_namespaces.at(0).get().m_data.at(1).m_has_inline_keyword);
    is_true(file.m_namespaces.at(0).get().m_data.at(1).m_has_static_keyword);

    is_eq(file.m_namespaces.at(0).get().m_data.at(2).m_type, "std::vector<std::string<char>>");
    is_eq(file.m_namespaces.at(0).get().m_data.at(2).m_name, "global_data_3");
    is_true(file.m_namespaces.at(0).get().m_data.at(2).m_has_inline_keyword);
    is_true(file.m_namespaces.at(0).get().m_data.at(2).m_has_static_keyword);

    is_eq(file.m_namespaces.at(0).get().m_data.at(3).m_type, "std::vector<std::string<char>>");
    is_eq(file.m_namespaces.at(0).get().m_data.at(3).m_name, "global_data_4");
    is_true(file.m_namespaces.at(0).get().m_data.at(3).m_has_inline_keyword);
    is_true(file.m_namespaces.at(0).get().m_data.at(3).m_has_static_keyword);

	is_eq(file.m_namespaces.at(0).get().m_name, "first");
    is_eq(file.m_namespaces.at(0).get().m_funcs.at(0).m_return_type, "int");
    is_eq(file.m_namespaces.at(0).get().m_funcs.at(0).m_name, "first_func");
    is_eq(file.m_namespaces.at(0).get().m_namespaces.at(0).get().m_name, "second");
    is_eq(file.m_namespaces.at(0).get().m_namespaces.at(0).get().m_funcs.at(0).m_return_type, "int");
    is_eq(file.m_namespaces.at(0).get().m_namespaces.at(0).get().m_funcs.at(0).m_name, "second");
    is_eq(file.m_namespaces.at(0).get().m_namespaces.at(1).get().m_name, "third");
    is_eq(file.m_namespaces.at(0).get().m_namespaces.at(1).get().m_funcs.at(0).m_return_type, "int");
    is_eq(file.m_namespaces.at(0).get().m_namespaces.at(1).get().m_funcs.at(0).m_name, "third");
    is_eq(file.m_namespaces.at(1).get().m_name, "");
    is_eq(file.m_namespaces.at(1).get().m_funcs.at(0).m_return_type, "int");
    is_eq(file.m_namespaces.at(1).get().m_funcs.at(0).m_name, "anon");
}

UNIT_TEST(parser, simple_class)
{
    ge::parser parser{};

    std::string_view src =
        "namespace my_name_spacey\n"
        "{\n"
        "    REFL_TYPE(i_am_an_attribute = 5)\n"
        "        class __myClassName :\n"
        "        public foo, private bar, protected _foobar<foo, bar>\n"
        "    {\n"
        "        REFL_DATA(me_is_data!)\n"
        "            std::vector<char> vecy{};\n"
        "\n"
        "    public:\n"
        "        REFL_FUNC(hi)\n"
        "            bool is_alpha(char al = ')') -> bool { return al == '}'; }\n"
        "    };\n"
        "}\n"
        ;

    ge::parsed_file file = parser.parse(src);
    is_eq(file.m_namespaces.at(0)->m_name, "my_name_spacey");
    is_eq(file.m_namespaces.at(0)->m_types.at(0)->m_name, "__myClassName");
    is_eq(file.m_namespaces.at(0)->m_types.at(0)->m_attributes, "i_am_an_attribute=5");
    is_eq(file.m_namespaces.at(0)->m_types.at(0)->m_base_types.at(0).m_name, "foo");
    is_eq(file.m_namespaces.at(0)->m_types.at(0)->m_base_types.at(0).m_access, ge::parsed_access_type::public_access);
    is_eq(file.m_namespaces.at(0)->m_types.at(0)->m_base_types.at(1).m_name, "bar");
    is_eq(file.m_namespaces.at(0)->m_types.at(0)->m_base_types.at(1).m_access, ge::parsed_access_type::private_access);
    is_eq(file.m_namespaces.at(0)->m_types.at(0)->m_base_types.at(2).m_name, "_foobar<foo, bar>");
    is_eq(file.m_namespaces.at(0)->m_types.at(0)->m_base_types.at(2).m_access, ge::parsed_access_type::protected_access);
    is_eq(file.m_namespaces.at(0)->m_types.at(0)->m_type, ge::parsed_type_type::class_type);
    is_eq(file.m_namespaces.at(0)->m_types.at(0)->m_data.at(0).m_name, "vecy");
    is_eq(file.m_namespaces.at(0)->m_types.at(0)->m_data.at(0).m_attributes, "me_is_data!");
    is_eq(file.m_namespaces.at(0)->m_types.at(0)->m_data.at(0).m_type, "std::vector<char>");
    is_eq(file.m_namespaces.at(0)->m_types.at(0)->m_data.at(0).m_access, ge::parsed_access_type::private_access);
    is_eq(file.m_namespaces.at(0)->m_types.at(0)->m_funcs.at(0).m_name, "is_alpha");
    is_eq(file.m_namespaces.at(0)->m_types.at(0)->m_funcs.at(0).m_attributes, "hi");
    is_eq(file.m_namespaces.at(0)->m_types.at(0)->m_funcs.at(0).m_name, "is_alpha");
    is_eq(file.m_namespaces.at(0)->m_types.at(0)->m_funcs.at(0).m_return_type, "bool");
    is_eq(file.m_namespaces.at(0)->m_types.at(0)->m_funcs.at(0).m_parameters.at(0).m_type, "char");
    is_eq(file.m_namespaces.at(0)->m_types.at(0)->m_funcs.at(0).m_parameters.at(0).m_name, "al");
    is_eq(file.m_namespaces.at(0)->m_types.at(0)->m_funcs.at(0).m_parameters.size(), 1ull);
    is_eq(file.m_namespaces.at(0)->m_types.at(0)->m_funcs.at(0).m_access, ge::parsed_access_type::public_access);
}
