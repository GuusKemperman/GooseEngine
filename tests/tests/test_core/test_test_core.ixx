export module test_test_core;

import test_core;
import stl;

using namespace ge::test_core;
using namespace ge::test_core::assert;

namespace
{
	template<typename... args>
	void test_all(auto func_to_test, 
		std::tuple<args...> success, 
		std::tuple<args...> failure)
	{
		std::apply(func_to_test, std::tuple_cat(success, std::make_tuple(std::source_location::current())));
		std::source_location here;

		test_exception e = expect_exception<test_exception>(
			[&]
			{
				here = std::source_location::current();
				std::apply(func_to_test, std::tuple_cat(failure, std::make_tuple(std::source_location::current())));
			});

		auto expected_line = here.line() + 1;

		is_eq(e.m_src.line(), expected_line);
		is_eq(std::string_view{ e.m_src.file_name() }, std::string_view{ here.file_name()});
		is_eq(std::string_view{ e.m_src.function_name() }, std::string_view{ here.function_name() });
	}
}

UNIT_TEST(asserts, is_true_all)
{
	test_all(&is_true, std::make_tuple(true), std::make_tuple(false));
}

UNIT_TEST(asserts, is_false_all)
{
	test_all(&is_false, std::make_tuple(false), std::make_tuple(true));
}

UNIT_TEST(asserts, is_null_raw)
{
	int* null{};
	int dummy{};
	test_all(&is_null<int*>, std::make_tuple(null), std::make_tuple(&dummy));
}

UNIT_TEST(asserts, is_null_smart)
{
	std::shared_ptr<int> null{};
	std::shared_ptr<int> not_null = std::make_shared<int>();
	test_all(&is_null<std::shared_ptr<int>>, 
			std::make_tuple(null), 
			std::make_tuple(not_null));
}

UNIT_TEST(asserts, is_not_null_raw)
{
	int* null{};
	int dummy{};
	test_all(&is_not_null<int*>, std::make_tuple(&dummy), std::make_tuple(null));
}

UNIT_TEST(asserts, is_not_null_smart)
{
	std::shared_ptr<int> null{};
	std::shared_ptr<int> not_null = std::make_shared<int>();
	test_all(&is_not_null<std::shared_ptr<int>>,
		std::make_tuple(not_null),
		std::make_tuple(null));
}

UNIT_TEST(asserts, eq_all)
{
	test_all(&is_eq<int, int>, std::make_tuple(1, 1), std::make_tuple(1, 2));
}

UNIT_TEST(asserts, ne_all)
{
	test_all(&is_ne<int, int>, std::make_tuple(1, 2), std::make_tuple(1, 1));
}

UNIT_TEST(asserts, lt_all)
{
	test_all(&is_lt<int, int>, std::make_tuple(1, 2), std::make_tuple(1, 1));
	test_all(&is_lt<int, int>, std::make_tuple(1, 2), std::make_tuple(2, 1));
}

UNIT_TEST(asserts, gt_all)
{
	test_all(&is_gt<int, int>, std::make_tuple(2, 1), std::make_tuple(1, 1));
	test_all(&is_gt<int, int>, std::make_tuple(2, 1), std::make_tuple(1, 2));
}

UNIT_TEST(asserts, le_all)
{
	test_all(&is_le<int, int>, std::make_tuple(1, 1), std::make_tuple(2, 1));
	test_all(&is_le<int, int>, std::make_tuple(1, 2), std::make_tuple(2, 1));
}

UNIT_TEST(asserts, ge_all)
{
	test_all(&is_ge<int, int>, std::make_tuple(1, 1), std::make_tuple(1, 2));
	test_all(&is_ge<int, int>, std::make_tuple(2, 1), std::make_tuple(1, 2));
}