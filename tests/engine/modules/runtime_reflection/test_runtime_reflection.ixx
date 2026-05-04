export module test_runtime_reflection;

import stl;
import logger;
import runtime_reflection;
import test_core;

using namespace ge::test_core;
using namespace ge::test_core::assert;

namespace
{
	struct type_1 {};
	struct type_2 {};

	int sub(int a, int b) { return a - b; }
	void incr(int& a) { a++; }
	size_t address_of(const int* a) { return std::bit_cast<size_t>(a); }
}

UNIT_TEST( building_tests, basic )
{
	ge::refl::registry reg =
		ge::refl::begin_registry()
			.begin_module("basic")
				.begin_type<type_1>("type_1")
				.end_type()
			.end_module()
		.build();

	std::optional<ge::refl::type_handle> type_handle = reg.try_get_type("type_1");
	is_true(type_handle.has_value());

	is_eq(type_handle->get_name(), "type_1");

	ge::refl::type_id expectedInfo = ge::refl::get_type_id<type_1>();
	ge::refl::type_id actualInfo = type_handle->get_id();

	is_eq(expectedInfo, actualInfo);
}

UNIT_TEST(building_tests, basic_ranges)
{
	ge::refl::registry reg =
		ge::refl::begin_registry()
		.begin_module("basic")
		.begin_type<type_1>("type_1")
		.end_type()
		.end_module()
		.build();

	auto modules = reg.modules();
	is_eq(modules.size(), 1ull);
	ge::refl::module_handle mod = *modules.begin();

	is_eq(mod.get_name(), "basic");

}

UNIT_TEST(building_tests, try_get_missing_type)
{
	ge::refl::registry reg =
		ge::refl::begin_registry()
			.begin_module("basic")
				.begin_type<type_1>("type_1")
				.end_type()
			.end_module()
		.build();

	std::optional<ge::refl::type_handle> missing = reg.try_get_type("does_not_exist");
	is_false(missing.has_value());
}

UNIT_TEST(building_tests, multiple_types_in_module)
{
	ge::refl::registry reg =
		ge::refl::begin_registry()
			.begin_module("basic")
				.begin_type<type_1>("type_1").end_type()
				.begin_type<type_2>("type_2").end_type()
			.end_module()
		.build();

	std::optional<ge::refl::type_handle> t1 = reg.try_get_type("type_1");
	std::optional<ge::refl::type_handle> t2 = reg.try_get_type("type_2");
	is_true(t1.has_value());
	is_true(t2.has_value());
	is_eq(t1->get_name(), "type_1");
	is_eq(t2->get_name(), "type_2");

	auto modules = reg.modules();
	is_eq(modules.size(), 1ull);
	ge::refl::module_handle mod = *modules.begin();
	auto types = mod.types();
	is_eq(types.size(), 2ull);
}

UNIT_TEST(building_tests, multiple_modules)
{
	ge::refl::registry reg =
		ge::refl::begin_registry()
			.begin_module("mod_a")
				.begin_type<type_1>("a_type").end_type()
			.end_module()
			.begin_module("mod_b")
				.begin_type<type_2>("b_type").end_type()
			.end_module()
		.build();

	auto modules = reg.modules();
	is_eq(modules.size(), 2ull);

	auto it = modules.begin();
	ge::refl::module_handle m0 = *it;
	++it;
	ge::refl::module_handle m1 = *it;

	is_eq(m0.get_name(), "mod_a");
	is_eq(m1.get_name(), "mod_b");
	is_eq(m0.types().size(), 1ull);
	is_eq(m1.types().size(), 1ull);
	is_eq((*m0.types().begin()).get_name(), "a_type");
	is_eq((*m1.types().begin()).get_name(), "b_type");
}

UNIT_TEST(building_tests, registry_types_range)
{
	ge::refl::registry reg =
		ge::refl::begin_registry()
			.begin_module("mod_a")
				.begin_type<type_1>("a_type").end_type()
			.end_module()
			.begin_module("mod_b")
				.begin_type<type_2>("b_type").end_type()
			.end_module()
		.build();

	auto types = reg.types();
	is_eq(types.size(), 2ull);
}

UNIT_TEST(building_tests, empty_module)
{
	ge::refl::registry reg =
		ge::refl::begin_registry()
			.begin_module("empty")
			.end_module()
		.build();

	auto modules = reg.modules();
	is_eq(modules.size(), 1ull);
	is_eq((*modules.begin()).types().size(), 0ull);
	is_false(reg.try_get_type("anything").has_value());
}

UNIT_TEST(building_tests, basic_func)
{
	ge::refl::registry reg =
		ge::refl::begin_registry()
			.begin_module("single_func")
				.begin_func<sub>("sub")
				.end_func()
			.end_module()
		.build();

	
	auto funcs = reg.funcs();
	is_eq(funcs.size(), 1ull);
	
	ge::refl::func_handle func = *funcs.begin();

	{
		ge::refl::value result = func.invoke_unchecked(ge::refl::value::create_owning(1), ge::refl::value::create_owning(2));
		is_true(result);
		is_eq(*result.as_constant<int>(), -1);
	}

	{
		int value = 1;
		ge::refl::value result = func.invoke_unchecked(ge::refl::value::create_ref(value), ge::refl::value::create_ref(value));
		is_true(result);
		is_eq(*result.as_constant<int>(), 0);
	}

	{
		int value = 1;
		ge::refl::value result = func.invoke_unchecked(ge::refl::value::create_view(value), ge::refl::value::create_view(value));
		is_true(result);
		is_eq(*result.as_constant<int>(), 0);
	}
}

UNIT_TEST(building_tests, func_that_takes_mutable_reference)
{
	ge::refl::registry reg =
		ge::refl::begin_registry()
		.begin_module("single_func")
		.begin_func<incr>("incr")
		.end_func()
		.end_module()
		.build();


	auto funcs = reg.funcs();
	is_eq(funcs.size(), 1ull);

	ge::refl::func_handle func = *funcs.begin();

	{
		int value = 0;
		ge::refl::value result = func.invoke_unchecked(ge::refl::value::create_ref(value));
		is_false(result);
		is_eq(value, 1);
	}
}

UNIT_TEST(building_tests, func_that_takes_mutable_pointers)
{
	ge::refl::registry reg =
		ge::refl::begin_registry()
		.begin_module("single_func")
		.begin_func<address_of>("address_of")
		.end_func()
		.end_module()
		.build();


	auto funcs = reg.funcs();
	is_eq(funcs.size(), 1ull);

	ge::refl::func_handle func = *funcs.begin();

	{
		int value = 0;
		ge::refl::value result = func.invoke_unchecked(ge::refl::value::create_view(&value));
		is_true(result);
		is_eq(*result.as_constant<size_t>(), address_of(&value));
	}

	{
		int value = 0;
		int* ptr = &value;
		ge::refl::value result = func.invoke_unchecked(ge::refl::value::create_view(ptr));
		is_true(result);
		is_eq(*result.as_constant<size_t>(), address_of(ptr));
	}
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

UNIT_TEST(value_tests, default_constructed_is_null)
{
	ge::refl::value v{};
	is_null(v.const_data());
}

UNIT_TEST(value_tests, as_typed_access)
{
	int x = 42;

	ge::refl::value view = ge::refl::value::create_view(x);
	is_not_null(view.as_constant<int>());
	is_eq(*view.as_constant<int>(), 42);

	ge::refl::value ref = ge::refl::value::create_ref(x);
	is_not_null(ref.as_constant<int>());
	is_not_null(ref.as_mutable<int>());
	is_eq(*ref.as_constant<int>(), 42);
	is_eq(static_cast<void*>(ref.as_mutable<int>()), ref.mutable_data());

	ge::refl::value owning = ge::refl::value::create_owning(42);
	is_not_null(owning.as_constant<int>());
	is_not_null(owning.as_mutable<int>());
	is_eq(*owning.as_constant<int>(), 42);
}

UNIT_TEST(value_tests, owning_is_deep_copy)
{
	ge::refl::value original = ge::refl::value::create_owning(42);
	ge::refl::value copy = original;

	*copy.as_mutable<int>() = 99;

	is_eq(*original.as_constant<int>(), 42);
	is_eq(*copy.as_constant<int>(), 99);
}

UNIT_TEST(value_tests, ref_mutates_source)
{
	int x = 10;
	ge::refl::value ref = ge::refl::value::create_ref(x);
	*ref.as_mutable<int>() = 7;
	is_eq(x, 7);
}

UNIT_TEST(value_tests, move_leaves_source_empty)
{
	int x = 42;
	ge::refl::value src = ge::refl::value::create_view(x);
	is_not_null(src.const_data());

	ge::refl::value dst = std::move(src);
	is_null(src.const_data());
	is_not_null(dst.const_data());

	int y = 7;
	ge::refl::value src2 = ge::refl::value::create_view(y);
	ge::refl::value dst2{};
	dst2 = std::move(src2);
	is_null(src2.const_data());
	is_not_null(dst2.const_data());
}

UNIT_TEST(value_tests, pointer_overloads)
{
	int x = 99;

	ge::refl::value view = ge::refl::value::create_view(&x);
	is_eq(view.const_data(), static_cast<const void*>(&x));

	ge::refl::value ref = ge::refl::value::create_ref(&x);
	is_eq(ref.const_data(), static_cast<const void*>(&x));
	is_eq(ref.mutable_data(), static_cast<void*>(&x));
}

UNIT_TEST(value_tests, clear_resets_value)
{
	ge::refl::value owning = ge::refl::value::create_owning(42);
	is_not_null(owning.const_data());

	owning.clear();
	is_null(owning.const_data());

	owning.clear();
	is_null(owning.const_data());
}

UNIT_TEST(value_tests, owning_non_trivial_type)
{
	int counter = 0;
	int inside_scope;

	struct destruct_tracker
	{
		int* counter;
		destruct_tracker(int* c) : counter(c) {}
		destruct_tracker(const destruct_tracker& other) : counter(other.counter) {}
		~destruct_tracker() { ++(*counter); }
	};

	{
		ge::refl::value v = ge::refl::value::create_owning(destruct_tracker{ &counter });
		inside_scope = counter;
	}
	is_gt(counter, inside_scope);
}

UNIT_TEST(value_tests, null_pointer_view)
{
	ge::refl::value v = ge::refl::value::create_view(static_cast<const int*>(nullptr));
	is_null(v.const_data());

	ge::refl::value copy = v;
	is_null(copy.const_data());

	ge::refl::value moved = std::move(v);
	is_null(moved.const_data());
	is_null(v.const_data());
}

UNIT_TEST(value_tests, null_pointer_ref)
{
	ge::refl::value v = ge::refl::value::create_ref(static_cast<int*>(nullptr));
	is_null(v.const_data());
	is_null(v.mutable_data());

	ge::refl::value copy = v;
	is_null(copy.const_data());
}

UNIT_TEST(value_tests, pointer_and_reference_equal)
{
	int x = 7;

	ge::refl::value from_ref = ge::refl::value::create_view(x);
	ge::refl::value from_ptr = ge::refl::value::create_view(&x);
	is_eq(from_ref.const_data(), from_ptr.const_data());

	ge::refl::value ref_ref = ge::refl::value::create_ref(x);
	ge::refl::value ref_ptr = ge::refl::value::create_ref(&x);
	is_eq(ref_ref.mutable_data(), ref_ptr.mutable_data());
}

UNIT_TEST(value_tests, struct_pointer_overload)
{
	struct two_ints { int a; int b; };
	two_ints p{ 3, 4 };

	ge::refl::value view = ge::refl::value::create_view(&p);
	is_eq(view.const_data(), static_cast<const void*>(&p));
	is_eq(view.as_constant<two_ints>()->a, 3);
	is_eq(view.as_constant<two_ints>()->b, 4);

	ge::refl::value ref = ge::refl::value::create_ref(&p);
	ref.as_mutable<two_ints>()->a = 99;
	is_eq(p.a, 99);
}

UNIT_TEST(value_tests, cross_ownership_assignment)
{
	int x = 1;

	ge::refl::value slot = ge::refl::value::create_owning(42);
	slot = ge::refl::value::create_view(x);
	is_eq(slot.const_data(), static_cast<const void*>(&x));

	ge::refl::value slot2 = ge::refl::value::create_view(x);
	slot2 = ge::refl::value::create_owning(77);
	is_eq(*slot2.as_constant<int>(), 77);
	is_eq(x, 1);

	ge::refl::value owned = ge::refl::value::create_owning(5);
	ge::refl::value viewed = ge::refl::value::create_view(x);
	owned = viewed;
	is_eq(owned.const_data(), static_cast<const void*>(&x));
}

UNIT_TEST(value_tests, chain_of_moves)
{
	int x = 42;
	ge::refl::value a = ge::refl::value::create_view(x);
	ge::refl::value b = std::move(a);
	ge::refl::value c = std::move(b);
	ge::refl::value d = std::move(c);

	is_null(a.const_data());
	is_null(b.const_data());
	is_null(c.const_data());
	is_eq(d.const_data(), static_cast<const void*>(&x));
}
