export module test_static_reflection:test_parser;

import stl;
import logger;
import static_reflection;
import test_core;

// TODO these tests are out of date since the return type changed, but the tests still have an std::expcted

using namespace ge::test_core;
using namespace ge::test_core::assert;

static ge::parsed_file parse_file(std::string_view file)
{
    std::expected<ge::parsed_file, std::string> result = ge::parse(file);

	if (result.has_value())
	{
        return *std::move(result);
	}
    failure(result.error());
}

template<typename T>
static const T& list_at(const std::list<T>& list, size_t index)
{
    auto it = list.begin();
    std::advance(it, index);
    return *it;
}

UNIT_TEST(parser, invalid_file)
{
    std::string_view src =
        "REFL_DATA()\n"
        "int ? = 5;\n"
        "\n"
        "REFL_DATA()\n"
        "int valid = 69;\n";

    ge::logger logger{};
    std::expected<ge::parsed_file, std::string> result = ge::parse(src);

    is_false(result.has_value());
    is_false(result.error().empty());
    logger.log_raw(ge::severity::message, result.error());
}

UNIT_TEST(parser, complex_function_no_crash)
{
    ge::parsed_file file = parse_file(
        "        REFL_FUNC(DisplayName(\"Hello)()}\"), DisplayName(\"Hello)()}\"),IsScriptable)\n"
        "        // Hello we are reflect*/ing this\n"
        "        /*\n"
        "        /*Comment in comment!\n"
        "        // Commentttsss\n"
        "        */\n"
        R"(        [[nodiscard]] inline /*haha here is another comment */ API  int ___function_name    (const int*& param0_name, std::string param1_name = { "Hello; { \" })}" /*helloo*/ },)" "\n"
        "            std::string<char> param2 = (R\"(Hellooo \" \" ))) } [[attribution inside string ]] )\"), std::array<std::vector<std::pair<int, float>>, 0x401ul > foo = 1.0f);\n"
    );

    is_eq(file.m_funcs.size(), 1ull);
    is_eq(file.m_funcs.front().m_return_type, "int");
    is_eq(file.m_funcs.front().m_name, "___function_name");
    is_eq(file.m_funcs.front().m_traits, "DisplayName(\"Hello)()}\"), DisplayName(\"Hello)()}\"),IsScriptable");
    is_eq(file.m_funcs.front().m_parameters.at(0).m_type, "const int*&");
    is_eq(file.m_funcs.front().m_parameters.at(0).m_name, "param0_name");
    is_eq(file.m_funcs.front().m_parameters.at(1).m_type, "std::string");
    is_eq(file.m_funcs.front().m_parameters.at(1).m_name, "param1_name");
    is_eq(file.m_funcs.front().m_parameters.at(2).m_type, "std::string<char>");
    is_eq(file.m_funcs.front().m_parameters.at(2).m_name, "param2");
    is_eq(file.m_funcs.front().m_parameters.at(3).m_type, "std::array<std::vector<std::pair<int, float>>, 0x401ul >");
    is_eq(file.m_funcs.front().m_parameters.at(3).m_name, "foo");
    is_eq(file.m_funcs.front().m_parameters.size(), 4ull);
}

UNIT_TEST(tokeniser, simple_func)
{
    ge::token_range tokeniser{ "int func_name() { return 1; }" };

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

UNIT_TEST(parser, simple_namespace)
{
    std::string_view src =
        "namespace hello\n"
        "{\n"
        "    namespace         world {}\n"
        "}\n"
        "namespace {}\n";

    ge::parsed_file file = parse_file(src);

    is_eq(list_at(file.m_namespaces, 0).m_name, "hello");
    is_eq(list_at(list_at(file.m_namespaces, 0).m_namespaces, 0).m_name, "world");
    is_eq(list_at(file.m_namespaces, 1).m_name, "");
    is_eq(file.m_namespaces.size(), 2ull);
}

UNIT_TEST(parser, simple_data)
{
    std::string_view src =
        "REFL_DATA(attry!, attry2!)\n"
        "static int global_data = 5;";

    ge::parsed_file file = parse_file(src);

    is_eq(file.m_data.at(0).m_traits, "attry!, attry2!");
    is_eq(file.m_data.at(0).m_type, "int");
    is_eq(file.m_data.at(0).m_name, "global_data");
    is_eq(file.m_data.at(0).m_keywords, ge::parsed_keywords::static_keyword);
}

UNIT_TEST(parser, no_param_func)
{
    std::string_view src =
        "REFL_FUNC()\n"
        "void do_thing( /*with comment*/);\n"
        "REFL_FUNC()\n"
        "void do_thing_again();";

    ge::parsed_file file = parse_file(src);

    is_eq(file.m_funcs.at(0).m_traits, "");
    is_eq(file.m_funcs.at(0).m_return_type, "void");
    is_eq(file.m_funcs.at(0).m_name, "do_thing");
    is_eq(file.m_funcs.at(0).m_parameters.size(), 0ull);

    is_eq(file.m_funcs.at(1).m_traits, "");
    is_eq(file.m_funcs.at(1).m_return_type, "void");
    is_eq(file.m_funcs.at(1).m_name, "do_thing_again");
    is_eq(file.m_funcs.at(1).m_parameters.size(), 0ull);
}

UNIT_TEST(parser, simple_func)
{


    std::string_view src =
        "REFL_FUNC(attry!, attry2!)\n"
        "static std::vector<int> global_func(std::array<int, 2> p1 = { 1, 2 }, std::array<int, 3> p2 = std::array<int, 3>(1, 2, 3));";

    ge::parsed_file file = parse_file(src);

    is_eq(file.m_funcs.at(0).m_traits, "attry!, attry2!");
    is_eq(file.m_funcs.at(0).m_return_type, "std::vector<int>");
    is_eq(file.m_funcs.at(0).m_name, "global_func");
    is_eq(file.m_funcs.at(0).m_keywords, ge::parsed_keywords::static_keyword);
    is_eq(file.m_funcs.at(0).m_parameters.at(0).m_name, "p1");
    is_eq(file.m_funcs.at(0).m_parameters.at(0).m_type, "std::array<int, 2>");

    is_eq(file.m_funcs.at(0).m_parameters.at(1).m_name, "p2");
    is_eq(file.m_funcs.at(0).m_parameters.at(1).m_type, "std::array<int, 3>");
}

UNIT_TEST(parser, complete_file)
{
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

    ge::parsed_file file = parse_file(src);

    is_eq(file.m_funcs.at(0).m_return_type, "int");
    is_eq(file.m_funcs.at(0).m_name, "global");
    is_eq(file.m_data.at(0).m_type, "int");
    is_eq(file.m_data.at(0).m_name, "global_data");
	
	is_eq(list_at(file.m_namespaces, 0).m_data.at(0).m_type, "std::vector<std::string<char>>");
	is_eq(list_at(file.m_namespaces, 0).m_data.at(0).m_name, "global_data");
    is_eq(list_at(file.m_namespaces, 0).m_data.at(0).m_keywords, ge::parsed_keywords::static_keyword | ge::parsed_keywords::inline_keyword);

    is_eq(list_at(file.m_namespaces, 0).m_data.at(1).m_type, "std::vector<std::string<char>>");
    is_eq(list_at(file.m_namespaces, 0).m_data.at(1).m_name, "global_data_2");
    is_eq(list_at(file.m_namespaces, 0).m_data.at(1).m_keywords, ge::parsed_keywords::static_keyword | ge::parsed_keywords::inline_keyword);

    is_eq(list_at(file.m_namespaces, 0).m_data.at(2).m_type, "std::vector<std::string<char>>");
    is_eq(list_at(file.m_namespaces, 0).m_data.at(2).m_keywords, ge::parsed_keywords::static_keyword | ge::parsed_keywords::inline_keyword);
    is_eq(list_at(file.m_namespaces, 0).m_data.at(2).m_name, "global_data_3");

    is_eq(list_at(file.m_namespaces, 0).m_data.at(3).m_type, "std::vector<std::string<char>>");
    is_eq(list_at(file.m_namespaces, 0).m_data.at(3).m_name, "global_data_4");
    is_eq(list_at(file.m_namespaces, 0).m_data.at(3).m_keywords, ge::parsed_keywords::static_keyword | ge::parsed_keywords::inline_keyword);

	is_eq(list_at(file.m_namespaces, 0).m_name, "first");
    is_eq(list_at(file.m_namespaces, 0).m_funcs.at(0).m_return_type, "int");
    is_eq(list_at(file.m_namespaces, 0).m_funcs.at(0).m_name, "first_func");
    is_eq(list_at(list_at(file.m_namespaces, 0).m_namespaces, 0).m_name, "second");
    is_eq(list_at(list_at(file.m_namespaces, 0).m_namespaces, 0).m_funcs.at(0).m_return_type, "int");
    is_eq(list_at(list_at(file.m_namespaces, 0).m_namespaces, 0).m_funcs.at(0).m_name, "second");
    is_eq(list_at(list_at(file.m_namespaces, 0).m_namespaces, 1).m_name, "third");
    is_eq(list_at(list_at(file.m_namespaces, 0).m_namespaces, 1).m_funcs.at(0).m_return_type, "int");
    is_eq(list_at(list_at(file.m_namespaces, 0).m_namespaces, 1).m_funcs.at(0).m_name, "third");
    is_eq(list_at(file.m_namespaces, 1).m_name, "");
    is_eq(list_at(file.m_namespaces, 1).m_funcs.at(0).m_return_type, "int");
    is_eq(list_at(file.m_namespaces, 1).m_funcs.at(0).m_name, "anon");
}

UNIT_TEST(parser, simple_class)
{
    std::string_view src =
        "namespace my_name_spacey\n"
        "{\n"
        "    REFL_TYPE(i_am_an_trait = 5)\n"
        "        class __myClassName :\n"
        "        public foo, private bar, protected _foobar<foo, bar>, barfoo\n"
        "    {\n"
        "        REFL_DATA(me_is_data!)\n"
        "            std::vector<char> vecy{};\n"
        "\n"
        "    public:\n"
        "        REFL_FUNC(hi)\n"
        "            bool is_alpha(char al = ')', char ot = '\\'') const & -> bool { return al == '}'; }\n"
        "    };\n"
        "}\n"
        ;

    ge::parsed_file file = parse_file(src);
    is_eq(list_at(file.m_namespaces, 0).m_name, "my_name_spacey");
    is_eq(list_at(list_at(file.m_namespaces, 0).m_types, 0).m_name, "__myClassName");
    is_eq(list_at(list_at(file.m_namespaces, 0).m_types, 0).m_traits, "i_am_an_trait = 5");
    is_eq(list_at(list_at(file.m_namespaces, 0).m_types, 0).m_base_types.at(0).m_name, "foo");
    is_eq(list_at(list_at(file.m_namespaces, 0).m_types, 0).m_base_types.at(0).m_access, ge::parsed_access_specifier::public_access);
    is_eq(list_at(list_at(file.m_namespaces, 0).m_types, 0).m_base_types.at(1).m_name, "bar");
    is_eq(list_at(list_at(file.m_namespaces, 0).m_types, 0).m_base_types.at(1).m_access, ge::parsed_access_specifier::private_access);
    is_eq(list_at(list_at(file.m_namespaces, 0).m_types, 0).m_base_types.at(2).m_name, "_foobar<foo, bar>");
    is_eq(list_at(list_at(file.m_namespaces, 0).m_types, 0).m_base_types.at(2).m_access, ge::parsed_access_specifier::protected_access);
    is_eq(list_at(list_at(file.m_namespaces, 0).m_types, 0).m_base_types.at(3).m_name, "barfoo");
    is_eq(list_at(list_at(file.m_namespaces, 0).m_types, 0).m_base_types.at(3).m_access, std::nullopt);
    is_eq(list_at(list_at(file.m_namespaces, 0).m_types, 0).m_key, ge::parsed_type_key::class_type);
    is_eq(list_at(list_at(file.m_namespaces, 0).m_types, 0).m_data.at(0).m_name, "vecy");
    is_eq(list_at(list_at(file.m_namespaces, 0).m_types, 0).m_data.at(0).m_name, "vecy");
    is_eq(list_at(list_at(file.m_namespaces, 0).m_types, 0).m_data.at(0).m_traits, "me_is_data!");
    is_eq(list_at(list_at(file.m_namespaces, 0).m_types, 0).m_data.at(0).m_type, "std::vector<char>");
    is_eq(list_at(list_at(file.m_namespaces, 0).m_types, 0).m_data.at(0).m_access, ge::parsed_access_specifier::private_access);
    is_eq(list_at(list_at(file.m_namespaces, 0).m_types, 0).m_funcs.at(0).m_name, "is_alpha");
    is_eq(list_at(list_at(file.m_namespaces, 0).m_types, 0).m_funcs.at(0).m_traits, "hi");
    is_eq(list_at(list_at(file.m_namespaces, 0).m_types, 0).m_funcs.at(0).m_name, "is_alpha");
    is_eq(list_at(list_at(file.m_namespaces, 0).m_types, 0).m_funcs.at(0).m_return_type, "bool");
    is_eq(list_at(list_at(file.m_namespaces, 0).m_types, 0).m_funcs.at(0).m_trailing_qualifiers, "const &");
    is_eq(list_at(list_at(file.m_namespaces, 0).m_types, 0).m_funcs.at(0).m_parameters.at(0).m_type, "char");
    is_eq(list_at(list_at(file.m_namespaces, 0).m_types, 0).m_funcs.at(0).m_parameters.at(0).m_name, "al");
    is_eq(list_at(list_at(file.m_namespaces, 0).m_types, 0).m_funcs.at(0).m_parameters.at(1).m_type, "char");
    is_eq(list_at(list_at(file.m_namespaces, 0).m_types, 0).m_funcs.at(0).m_parameters.at(1).m_name, "ot");
    is_eq(list_at(list_at(file.m_namespaces, 0).m_types, 0).m_funcs.at(0).m_parameters.size(), 2ull);
    is_eq(list_at(list_at(file.m_namespaces, 0).m_types, 0).m_funcs.at(0).m_access, ge::parsed_access_specifier::public_access);
}

UNIT_TEST(parser, enums)
{
    std::string_view src =
		"REFL_ENUM(attry!)\n"
        "        enum empty\n"
        "	    {\n"
        "	    };\n"
        "\n"
		"REFL_ENUM(attry!)\n"
        "	    enum class empty_class {};\n"
        "\n"
		"REFL_ENUM(attry!)\n"
        "	    enum simple_entries\n"
        "	    {\n"
        "	        hello,\n"
        "	        world\n"
        "	    };\n"
        "\n"
		"REFL_ENUM(attry!)\n"
        "	    enum class complex_entries : std::uint64_t\n"
        "	    {\n"
        "	        hello = 1,\n"
        "	        world = hello, /*,*/\n"
        "	        goodnight = some_struct<simple_entries, hello>::size<1, 2>(int test = { hello }),\n"
        "	        darling\n"
        "	    };\n";

    ge::parsed_file file = parse_file(src);

    is_eq(file.m_enums.at(0).m_name, "empty");
    is_eq(file.m_enums.at(0).m_traits, "attry!");
    is_eq(file.m_enums.at(0).m_entries.size(), 0ull);

    is_eq(file.m_enums.at(1).m_name, "empty_class");
    is_eq(file.m_enums.at(1).m_traits, "attry!");
    is_eq(file.m_enums.at(1).m_entries.size(), 0ull);

    is_eq(file.m_enums.at(2).m_name, "simple_entries");
    is_eq(file.m_enums.at(2).m_traits, "attry!");
    is_eq(file.m_enums.at(2).m_entries.at(0), "hello");
    is_eq(file.m_enums.at(2).m_entries.at(1), "world");
    is_eq(file.m_enums.at(2).m_entries.size(), 2ull);

    is_eq(file.m_enums.at(3).m_name, "complex_entries");
    is_eq(file.m_enums.at(3).m_traits, "attry!");
    is_eq(file.m_enums.at(3).m_entries.at(0), "hello");
    is_eq(file.m_enums.at(3).m_entries.at(1), "world");
    is_eq(file.m_enums.at(3).m_entries.at(2), "goodnight");
    is_eq(file.m_enums.at(3).m_entries.at(3), "darling");
    is_eq(file.m_enums.at(3).m_entries.size(), 4ull);
}
