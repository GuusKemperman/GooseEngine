export module test_static_reflection:test_tokeniser;


import stl;
import logger;
import static_reflection;
import test_core;

using namespace ge::test_core;
using namespace ge::test_core::assert;

UNIT_TEST(tokeniser, complex_function_no_crash)
{
    ge::token_range{
"        REFL_FUNC()\n"
"        // Hello we are reflect*/ing this\n"
"        /*\n"
"        /*Comment in comment!\n"
"        // Commentttsss\n"
"        */\n"
R"(        [[nodiscard]] inline /*haha here is another comment */int function_name(int param0_name, std::string param1_name = { "Hello; { \" })}" /*helloo*/ },)" "\n"
"            std::string<char> param2 = (R\"(Hellooo \" \" ))) } [[attribution inside string ]] )\"), int foo = 1.0f);\n"
    };
}

UNIT_TEST(tokeniser, iterators)
{
    ge::token_range tokeniser{ "1 2 3 4" };

    (void)expect_exception<std::out_of_range>(
        [&]
        {
            ge::token_iterator end = tokeniser.end();
            ++end;
        });

    ge::token_iterator it = tokeniser.begin();
    is_eq(it->m_str, "1");
    is_ne(it, tokeniser.end());
    ++it;
	is_eq(it->m_str, " ");
    is_ne(it, tokeniser.end());
    ++it;
	is_eq(it->m_str, "2");
    is_ne(it, tokeniser.end());
    ++it;
    is_eq(it->m_str, " ");
    is_ne(it, tokeniser.end());
    ++it;
    is_eq(it->m_str, "3");
    is_ne(it, tokeniser.end());
    ++it;
    is_eq(it->m_str, " ");
    is_ne(it, tokeniser.end());
    ++it;
    is_eq(it->m_str, "4");
    is_ne(it, tokeniser.end());
    ++it;
    is_eq(it, tokeniser.end());
}
