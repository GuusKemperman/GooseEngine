export module test_runtime_reflection;

import stl;
import logger;
import runtime_reflection;
import test_core;

using namespace ge::test_core;
using namespace ge::test_core::assert;

namespace
{
	struct type_1
	{
		
	};
}

UNIT_TEST( building_tests, basic )
{
	ge::refl::registry reg =
		ge::refl::begin_registry()
			.begin_module()
				.begin_type<type_1>("type_1")
				.end_type()
			.end_module()
		.build();

	std::optional<ge::refl::type> type = reg.try_get_type("type_1");
	is_true(type.has_value());

	is_eq(type->get_name(), "type_1");

	const ge::refl::type_info& expectedInfo = ge::refl::type_info::get_type_info<type_1>();
	const ge::refl::type_info& actualInfo = type->get_info();

	is_eq(expectedInfo, actualInfo);
}

static void test_big_five( ge::refl::value value_1, auto check )
{
	check(value_1);

	ge::refl::value copy = value_1;
	check(copy);

	ge::refl::value moved = std::move(value_1);
	check(moved);

	ge::refl::value copy_assigned{};
	copy_assigned = moved;
	check(copy_assigned);

	ge::refl::value move_assigned{};
	move_assigned = std::move(moved);
	check(move_assigned);
}

UNIT_TEST( value_tests, view_lifetime )
{
	int expected = 42;

	auto check = [&expected](const ge::refl::value& v)
		{
			is_not_null(v.const_data());
			is_eq(v.const_data(), &expected);
		};

	test_big_five(ge::refl::value::create_view(expected), check);
	is_eq(expected, 42);
}

UNIT_TEST(value_tests, ref_lifetime)
{
	int expected = 42;

	auto check = [&expected](const ge::refl::value& v)
		{
			is_not_null(v.const_data());
			is_eq(v.const_data(), &expected);
		};

	test_big_five(ge::refl::value::create_ref(expected), check);
	is_eq(expected, 42);
}

UNIT_TEST(value_tests, owning_lifetime)
{
	int expected = 42;

	auto check = [&expected](const ge::refl::value& v)
		{
			is_not_null(v.const_data());
			is_eq(*v.as_constant<int>(), expected);
		};

	test_big_five(ge::refl::value::create_owning(expected), check);
	is_eq(expected, 42);
}


//
//UNIT_TST(dependency, circular_impl)
//{
//	refl_data data{};
//	is_eq(data.do_thing(), 5);
//
//	ge::logger l{};
//	l.log_raw(ge::verbose, data.get_name<int>());
//}