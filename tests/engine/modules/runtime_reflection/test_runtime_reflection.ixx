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

	int free_subtract(int a, int b) { return a - b; }
	int free_square(int x) { return x * x; }
	int free_return_42() { return 42; }
	void free_noop() {}
	void free_increment(int& x) { ++x; }
	int free_double_cref(const int& x) { return x * 2; }
	int free_combine3(int a, int b, int c) { return a * 100 + b * 10 + c; }

	int g_side_effect_counter = 0;
	void free_bump_global() { ++g_side_effect_counter; }

	struct fpoint
	{
		int x;
		int y;
		bool operator==(const fpoint& other) const { return x == other.x && y == other.y; }
	};

	fpoint free_make_point(int x, int y) { return fpoint{ x, y }; }
	int free_point_x(fpoint p) { return p.x; }
	void free_translate(fpoint& p, int dx, int dy) { p.x += dx; p.y += dy; }
	int free_point_combine(const fpoint& a, const fpoint& b)
	{
		return a.x * 1000 + a.y * 100 + b.x * 10 + b.y;
	}

	int free_mixed(int by_val, int& by_ref, const int& by_cref)
	{
		int result = by_val * 100 + by_ref * 10 + by_cref;
		by_ref = result;
		return result;
	}

	template<typename T>
	T free_template_identity(T x) { return x; }

	int g_ref_target = 0;
	int& free_get_ref() { return g_ref_target; }
	const int& free_get_cref() { return g_ref_target; }

	fpoint g_point_target{ 0, 0 };
	fpoint& free_get_point_ref() { return g_point_target; }
	const fpoint& free_get_point_cref() { return g_point_target; }

	int& free_select_first(int& a, int&) { return a; }
	const int& free_select_first_cref(const int& a, const int&) { return a; }

	using namespace ge::refl;

	static_assert(std::is_same_v<func_sig_t<int(*)(int, int)>, func_sig<int(int, int)>>);
	static_assert(std::is_same_v<func_sig_t<int(&)(int)>, func_sig<int(int)>>);
	static_assert(std::is_same_v<func_sig_t<void()>, func_sig<void()>>);
	static_assert(std::is_same_v<func_sig_t<int(int, double, char)>, func_sig<int(int, double, char)>>);

	struct member_func_owner
	{
		int mut(int, double) { return 0; }
		int cst() const { return 0; }
		int rval() && { return 0; }
	};
	static_assert(std::is_same_v<func_sig_t<int(member_func_owner::*)(int, double)>, func_sig<int(member_func_owner&, int, double)>>);
	static_assert(std::is_same_v<func_sig_t<int(member_func_owner::*)() const>, func_sig<int(const member_func_owner&)>>);
	static_assert(std::is_same_v<func_sig_t<int(member_func_owner::*)() &&>, func_sig<int(member_func_owner&&)>>);

	static_assert(std::is_same_v<remove_decoration_t<int>, int>);
	static_assert(std::is_same_v<remove_decoration_t<int&>, int>);
	static_assert(std::is_same_v<remove_decoration_t<const int&>, int>);
	static_assert(std::is_same_v<remove_decoration_t<int*>, int>);
	static_assert(std::is_same_v<remove_decoration_t<const int>, int>);
	static_assert(std::is_same_v<remove_decoration_t<volatile int>, int>);

	static_assert(make_type_id<int>() == make_type_id<remove_decoration_t<int&>>());
	static_assert(make_type_id<int>() == make_type_id<remove_decoration_t<const int&>>());
	static_assert(make_type_id<int>() == make_type_id<remove_decoration_t<int*>>());

	static_assert(detail::supported_param_type<int>);
	static_assert(detail::supported_param_type<int&>);
	static_assert(detail::supported_param_type<const int&>);
	static_assert(!detail::supported_param_type<int*>);
	static_assert(!detail::supported_param_type<const int*>);
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

	ge::refl::type_id expectedInfo = ge::refl::make_type_id<type_1>();
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

UNIT_TEST(function_tests, building_registers_func_in_module)
{
	auto reg = ge::refl::begin_registry()
		.begin_module("m")
			.begin_func<&free_return_42>("ret42").end_func()
		.end_module()
		.build();

	auto modules = reg.modules();
	is_eq(modules.size(), 1ull);
	auto funcs = (*modules.begin()).funcs();
	is_eq(funcs.size(), 1ull);
	is_eq((*funcs.begin()).get_name(), "ret42");
}

UNIT_TEST(function_tests, registry_funcs_includes_funcs_from_all_modules)
{
	auto reg = ge::refl::begin_registry()
		.begin_module("m1")
			.begin_func<&free_subtract>("sub").end_func()
		.end_module()
		.begin_module("m2")
			.begin_func<&free_square>("square").end_func()
		.end_module()
		.build();

	is_eq(reg.funcs().size(), 2ull);
}

UNIT_TEST(function_tests, multiple_funcs_in_module_preserve_order)
{
	auto reg = ge::refl::begin_registry()
		.begin_module("m")
			.begin_func<&free_subtract>("sub").end_func()
			.begin_func<&free_square>("square").end_func()
			.begin_func<&free_return_42>("ret42").end_func()
		.end_module()
		.build();

	auto funcs = (*reg.modules().begin()).funcs();
	is_eq(funcs.size(), 3ull);

	std::vector<std::string_view> names;
	for (auto f : funcs)
	{
		names.push_back(f.get_name());
	}
	is_eq(names.size(), 3ull);
	is_eq(names[0], "sub");
	is_eq(names[1], "square");
	is_eq(names[2], "ret42");
}

UNIT_TEST(function_tests, multiple_modules_each_with_own_func_count)
{
	auto reg = ge::refl::begin_registry()
		.begin_module("m1")
			.begin_func<&free_subtract>("sub").end_func()
			.begin_func<&free_square>("square").end_func()
		.end_module()
		.begin_module("m2")
			.begin_func<&free_return_42>("ret42").end_func()
		.end_module()
		.build();

	auto mods = reg.modules();
	is_eq(mods.size(), 2ull);
	auto it = mods.begin();
	is_eq((*it).funcs().size(), 2ull);
	++it;
	is_eq((*it).funcs().size(), 1ull);
}

UNIT_TEST(function_tests, empty_module_has_no_funcs)
{
	auto reg = ge::refl::begin_registry()
		.begin_module("empty")
		.end_module()
		.build();

	auto mods = reg.modules();
	is_eq(mods.size(), 1ull);
	is_eq((*mods.begin()).funcs().size(), 0ull);
	is_eq(reg.funcs().size(), 0ull);
}

UNIT_TEST(invoke_tests, no_args_returning_int)
{
	auto reg = ge::refl::begin_registry()
		.begin_module("m")
			.begin_func<&free_return_42>("ret42").end_func()
		.end_module()
		.build();

	auto handle = *(*reg.modules().begin()).funcs().begin();
	ge::refl::value result = handle.invoke_unchecked();
	is_not_null(result.const_data());
	is_eq(*result.as_constant<int>(), 42);
	is_eq(result.get_type_id(), ge::refl::make_type_id<int>());
	is_true(result.is_owning());
}

UNIT_TEST(invoke_tests, no_args_void_returns_empty_value)
{
	auto reg = ge::refl::begin_registry()
		.begin_module("m")
			.begin_func<&free_noop>("noop").end_func()
		.end_module()
		.build();

	auto handle = *(*reg.modules().begin()).funcs().begin();
	ge::refl::value result = handle.invoke_unchecked();
	is_null(result.const_data());
	is_false(static_cast<bool>(result));
}

UNIT_TEST(invoke_tests, single_int_arg)
{
	auto reg = ge::refl::begin_registry()
		.begin_module("m")
			.begin_func<&free_square>("square").end_func()
		.end_module()
		.build();

	auto handle = *(*reg.modules().begin()).funcs().begin();
	int input = 5;
	ge::refl::value result = handle.invoke_unchecked(input);
	is_eq(*result.as_constant<int>(), 25);

	int negative = -7;
	is_eq(*handle.invoke_unchecked(negative).as_constant<int>(), 49);

	int zero = 0;
	is_eq(*handle.invoke_unchecked(zero).as_constant<int>(), 0);
}

UNIT_TEST(invoke_tests, two_int_args)
{
	auto reg = ge::refl::begin_registry()
		.begin_module("m")
			.begin_func<&free_subtract>("sub").end_func()
		.end_module()
		.build();

	auto handle = *(*reg.modules().begin()).funcs().begin();
	int a = 3;
	int b = 4;
	is_eq(*handle.invoke_unchecked(a, b).as_constant<int>(), -1);

	a = -10;
	b = 3;
	is_eq(*handle.invoke_unchecked(a, b).as_constant<int>(), -13);
}

UNIT_TEST(invoke_tests, two_int_args_order_matters)
{
	auto reg = ge::refl::begin_registry()
		.begin_module("m")
			.begin_func<&free_subtract>("sub").end_func()
		.end_module()
		.build();

	auto handle = *(*reg.modules().begin()).funcs().begin();
	int a = 10;
	int b = 3;
	is_eq(*handle.invoke_unchecked(a, b).as_constant<int>(), 7);
	is_eq(*handle.invoke_unchecked(b, a).as_constant<int>(), -7);
}

UNIT_TEST(invoke_tests, three_int_args)
{
	auto reg = ge::refl::begin_registry()
		.begin_module("m")
			.begin_func<&free_combine3>("combine3").end_func()
		.end_module()
		.build();

	auto handle = *(*reg.modules().begin()).funcs().begin();
	int a = 1, b = 2, c = 3;
	is_eq(*handle.invoke_unchecked(a, b, c).as_constant<int>(), 123);
	is_eq(*handle.invoke_unchecked(c, b, a).as_constant<int>(), 321);
	is_eq(*handle.invoke_unchecked(b, a, c).as_constant<int>(), 213);
}

UNIT_TEST(invoke_tests, mutable_ref_propagates_back)
{
	auto reg = ge::refl::begin_registry()
		.begin_module("m")
			.begin_func<&free_increment>("inc").end_func()
		.end_module()
		.build();

	auto handle = *(*reg.modules().begin()).funcs().begin();
	int x = 10;
	ge::refl::value result = handle.invoke_unchecked(x);
	is_null(result.const_data());
	is_eq(x, 11);

	handle.invoke_unchecked(x);
	is_eq(x, 12);

	handle.invoke_unchecked(x);
	is_eq(x, 13);
}

UNIT_TEST(invoke_tests, const_ref_does_not_mutate_caller)
{
	auto reg = ge::refl::begin_registry()
		.begin_module("m")
			.begin_func<&free_double_cref>("dcr").end_func()
		.end_module()
		.build();

	auto handle = *(*reg.modules().begin()).funcs().begin();
	int x = 7;
	ge::refl::value result = handle.invoke_unchecked(x);
	is_eq(*result.as_constant<int>(), 14);
	is_eq(x, 7);
}

UNIT_TEST(invoke_tests, by_value_does_not_mutate_caller_struct)
{
	auto reg = ge::refl::begin_registry()
		.begin_module("m")
			.begin_func<&free_point_x>("px").end_func()
		.end_module()
		.build();

	auto handle = *(*reg.modules().begin()).funcs().begin();
	fpoint p{ 3, 4 };
	ge::refl::value result = handle.invoke_unchecked(p);
	is_eq(*result.as_constant<int>(), 3);
	is_eq(p.x, 3);
	is_eq(p.y, 4);
}

UNIT_TEST(invoke_tests, struct_ref_propagates_back)
{
	auto reg = ge::refl::begin_registry()
		.begin_module("m")
			.begin_func<&free_translate>("translate").end_func()
		.end_module()
		.build();

	auto handle = *(*reg.modules().begin()).funcs().begin();
	fpoint p{ 1, 2 };
	int dx = 10;
	int dy = 20;
	ge::refl::value result = handle.invoke_unchecked(p, dx, dy);
	is_null(result.const_data());
	is_eq(p.x, 11);
	is_eq(p.y, 22);
}

UNIT_TEST(invoke_tests, struct_const_ref_does_not_mutate)
{
	auto reg = ge::refl::begin_registry()
		.begin_module("m")
			.begin_func<&free_point_combine>("combine").end_func()
		.end_module()
		.build();

	auto handle = *(*reg.modules().begin()).funcs().begin();
	fpoint a{ 1, 2 };
	fpoint b{ 3, 4 };
	ge::refl::value result = handle.invoke_unchecked(a, b);
	is_eq(*result.as_constant<int>(), 1234);
	ge::refl::value swapped = handle.invoke_unchecked(b, a);
	is_eq(*swapped.as_constant<int>(), 3412);
	is_eq(a, (fpoint{ 1, 2 }));
	is_eq(b, (fpoint{ 3, 4 }));
}

UNIT_TEST(invoke_tests, returns_struct_by_value)
{
	auto reg = ge::refl::begin_registry()
		.begin_module("m")
			.begin_func<&free_make_point>("mp").end_func()
		.end_module()
		.build();

	auto handle = *(*reg.modules().begin()).funcs().begin();
	int x = 9;
	int y = -2;
	ge::refl::value result = handle.invoke_unchecked(x, y);
	is_not_null(result.const_data());
	is_true(result.is_owning());
	is_eq(result.get_type_id(), ge::refl::make_type_id<fpoint>());
	is_eq(result.as_constant<fpoint>()->x, 9);
	is_eq(result.as_constant<fpoint>()->y, -2);
}

UNIT_TEST(invoke_tests, returned_owning_outlives_invoke_args)
{
	auto reg = ge::refl::begin_registry()
		.begin_module("m")
			.begin_func<&free_subtract>("sub").end_func()
		.end_module()
		.build();

	auto handle = *(*reg.modules().begin()).funcs().begin();
	ge::refl::value result;
	{
		int a = 200;
		int b = 100;
		result = handle.invoke_unchecked(a, b);
	}
	is_true(result.is_owning());
	is_eq(*result.as_constant<int>(), 100);
}

UNIT_TEST(invoke_tests, returned_struct_is_independent_copy)
{
	auto reg = ge::refl::begin_registry()
		.begin_module("m")
			.begin_func<&free_make_point>("mp").end_func()
		.end_module()
		.build();

	auto handle = *(*reg.modules().begin()).funcs().begin();
	int x = 5, y = 6;
	ge::refl::value v = handle.invoke_unchecked(x, y);
	v.as_mutable<fpoint>()->x = 999;

	ge::refl::value v2 = handle.invoke_unchecked(x, y);
	is_eq(v2.as_constant<fpoint>()->x, 5);
	is_eq(v.as_constant<fpoint>()->x, 999);
}

UNIT_TEST(invoke_tests, mixed_param_qualifiers)
{
	auto reg = ge::refl::begin_registry()
		.begin_module("m")
			.begin_func<&free_mixed>("mixed").end_func()
		.end_module()
		.build();

	auto handle = *(*reg.modules().begin()).funcs().begin();
	int v = 1;
	int r = 2;
	int c = 3;
	ge::refl::value result = handle.invoke_unchecked(v, r, c);
	is_eq(*result.as_constant<int>(), 123);
	is_eq(v, 1);
	is_eq(r, 123);
	is_eq(c, 3);

	int v2 = 3;
	int r2 = 2;
	int c2 = 1;
	ge::refl::value result2 = handle.invoke_unchecked(v2, r2, c2);
	is_eq(*result2.as_constant<int>(), 321);
	is_eq(v2, 3);
	is_eq(r2, 321);
	is_eq(c2, 1);
}

UNIT_TEST(invoke_tests, repeated_invokes_are_independent)
{
	auto reg = ge::refl::begin_registry()
		.begin_module("m")
			.begin_func<&free_square>("square").end_func()
		.end_module()
		.build();

	auto handle = *(*reg.modules().begin()).funcs().begin();
	int x = 6;
	is_eq(*handle.invoke_unchecked(x).as_constant<int>(), 36);
	is_eq(*handle.invoke_unchecked(x).as_constant<int>(), 36);

	int y = 4;
	is_eq(*handle.invoke_unchecked(y).as_constant<int>(), 16);

	int z = 11;
	is_eq(*handle.invoke_unchecked(z).as_constant<int>(), 121);
}

UNIT_TEST(invoke_tests, void_function_global_side_effect)
{
	auto reg = ge::refl::begin_registry()
		.begin_module("m")
			.begin_func<&free_bump_global>("bump").end_func()
		.end_module()
		.build();

	int before = g_side_effect_counter;
	auto handle = *(*reg.modules().begin()).funcs().begin();
	handle.invoke_unchecked();
	is_eq(g_side_effect_counter, before + 1);
	handle.invoke_unchecked();
	handle.invoke_unchecked();
	is_eq(g_side_effect_counter, before + 3);
}

UNIT_TEST(invoke_tests, accepts_value_argument_by_owning)
{
	auto reg = ge::refl::begin_registry()
		.begin_module("m")
			.begin_func<&free_square>("square").end_func()
		.end_module()
		.build();

	auto handle = *(*reg.modules().begin()).funcs().begin();
	ge::refl::value arg = ge::refl::value::create_owning(8);
	ge::refl::value result = handle.invoke_unchecked(arg);
	is_eq(*result.as_constant<int>(), 64);
}

UNIT_TEST(invoke_tests, accepts_value_argument_by_ref)
{
	auto reg = ge::refl::begin_registry()
		.begin_module("m")
			.begin_func<&free_increment>("inc").end_func()
		.end_module()
		.build();

	auto handle = *(*reg.modules().begin()).funcs().begin();
	int x = 50;
	ge::refl::value arg = ge::refl::value::create_ref(x);
	handle.invoke_unchecked(arg);
	is_eq(x, 51);
}

UNIT_TEST(invoke_tests, chains_invocations_via_returned_value)
{
	auto reg = ge::refl::begin_registry()
		.begin_module("m")
			.begin_func<&free_make_point>("mp").end_func()
			.begin_func<&free_point_x>("px").end_func()
		.end_module()
		.build();

	auto funcs = (*reg.modules().begin()).funcs();
	auto it = funcs.begin();
	auto mp = *it;
	++it;
	auto px = *it;

	int xv = 7, yv = 9;
	ge::refl::value point_value = mp.invoke_unchecked(xv, yv);
	is_eq(point_value.get_type_id(), ge::refl::make_type_id<fpoint>());

	ge::refl::value x_value = px.invoke_unchecked(point_value);
	is_eq(*x_value.as_constant<int>(), 7);
}

UNIT_TEST(invoke_tests, distinct_funcs_use_distinct_signatures)
{
	auto reg = ge::refl::begin_registry()
		.begin_module("m")
			.begin_func<&free_subtract>("sub").end_func()
			.begin_func<&free_square>("square").end_func()
			.begin_func<&free_return_42>("ret42").end_func()
		.end_module()
		.build();

	auto funcs = (*reg.modules().begin()).funcs();
	auto it = funcs.begin();
	auto sub_handle = *it;
	++it;
	auto square_handle = *it;
	++it;
	auto ret42_handle = *it;

	int a = 5, b = 7;
	is_eq(*sub_handle.invoke_unchecked(a, b).as_constant<int>(), -2);
	is_eq(*square_handle.invoke_unchecked(a).as_constant<int>(), 25);
	is_eq(*ret42_handle.invoke_unchecked().as_constant<int>(), 42);
}

UNIT_TEST(invoke_tests, template_instantiation_is_distinct)
{
	auto reg = ge::refl::begin_registry()
		.begin_module("m")
			.begin_func<&free_template_identity<int>>("id_int").end_func()
		.end_module()
		.build();

	auto handle = *(*reg.modules().begin()).funcs().begin();
	int x = 123;
	ge::refl::value result = handle.invoke_unchecked(x);
	is_eq(*result.as_constant<int>(), 123);
	is_eq(result.get_type_id(), ge::refl::make_type_id<int>());
}

UNIT_TEST(invoke_tests, return_value_is_owning_and_holds_correct_type_id)
{
	auto reg = ge::refl::begin_registry()
		.begin_module("m")
			.begin_func<&free_make_point>("mp").end_func()
			.begin_func<&free_return_42>("ret42").end_func()
		.end_module()
		.build();

	auto funcs = (*reg.modules().begin()).funcs();
	auto it = funcs.begin();
	auto mp = *it;
	++it;
	auto ret42 = *it;

	int xv = 1, yv = 2;
	ge::refl::value point_v = mp.invoke_unchecked(xv, yv);
	is_true(point_v.is_owning());
	is_eq(point_v.get_type_id(), ge::refl::make_type_id<fpoint>());
	is_ne(point_v.get_type_id(), ge::refl::make_type_id<int>());

	ge::refl::value int_v = ret42.invoke_unchecked();
	is_true(int_v.is_owning());
	is_eq(int_v.get_type_id(), ge::refl::make_type_id<int>());
	is_ne(int_v.get_type_id(), ge::refl::make_type_id<fpoint>());
}

UNIT_TEST(invoke_tests, invoke_does_not_share_state_between_handles)
{
	auto reg = ge::refl::begin_registry()
		.begin_module("m")
			.begin_func<&free_subtract>("s1").end_func()
			.begin_func<&free_subtract>("s2").end_func()
		.end_module()
		.build();

	auto funcs = (*reg.modules().begin()).funcs();
	auto it = funcs.begin();
	auto s1 = *it;
	++it;
	auto s2 = *it;

	is_eq(s1.get_name(), "s1");
	is_eq(s2.get_name(), "s2");

	int x = 7;
	int y = 3;
	is_eq(*s1.invoke_unchecked(x, y).as_constant<int>(), 4);
	is_eq(*s2.invoke_unchecked(x, y).as_constant<int>(), 4);
	is_eq(*s1.invoke_unchecked(y, x).as_constant<int>(), -4);
}

UNIT_TEST(invoke_tests, ref_return_is_non_owning)
{
	auto reg = ge::refl::begin_registry()
		.begin_module("m")
			.begin_func<&free_get_ref>("get_ref").end_func()
		.end_module()
		.build();

	g_ref_target = 42;
	auto handle = *(*reg.modules().begin()).funcs().begin();
	ge::refl::value result = handle.invoke_unchecked();

	is_false(result.is_owning());
	is_eq(result.get_type_id(), ge::refl::make_type_id<int>());
	is_eq(result.const_data(), static_cast<const void*>(&g_ref_target));
	is_eq(*result.as_constant<int>(), 42);
}

UNIT_TEST(invoke_tests, ref_return_is_mutable_and_aliases_target)
{
	auto reg = ge::refl::begin_registry()
		.begin_module("m")
			.begin_func<&free_get_ref>("get_ref").end_func()
		.end_module()
		.build();

	g_ref_target = 7;
	auto handle = *(*reg.modules().begin()).funcs().begin();
	ge::refl::value result = handle.invoke_unchecked();

	is_true(result.is_mutable());
	is_eq(result.mutable_data(), static_cast<void*>(&g_ref_target));

	*result.as_mutable<int>() = 999;
	is_eq(g_ref_target, 999);

	g_ref_target = 17;
	is_eq(*result.as_constant<int>(), 17);
}

UNIT_TEST(invoke_tests, const_ref_return_is_non_owning)
{
	auto reg = ge::refl::begin_registry()
		.begin_module("m")
			.begin_func<&free_get_cref>("get_cref").end_func()
		.end_module()
		.build();

	g_ref_target = 55;
	auto handle = *(*reg.modules().begin()).funcs().begin();
	ge::refl::value result = handle.invoke_unchecked();

	is_false(result.is_owning());
	is_eq(result.get_type_id(), ge::refl::make_type_id<int>());
	is_eq(result.const_data(), static_cast<const void*>(&g_ref_target));
	is_eq(*result.as_constant<int>(), 55);
}

UNIT_TEST(invoke_tests, const_ref_return_is_immutable_view)
{
	auto reg = ge::refl::begin_registry()
		.begin_module("m")
			.begin_func<&free_get_cref>("get_cref").end_func()
		.end_module()
		.build();

	g_ref_target = 11;
	auto handle = *(*reg.modules().begin()).funcs().begin();
	ge::refl::value result = handle.invoke_unchecked();

	is_false(result.is_mutable());

	g_ref_target = 22;
	is_eq(*result.as_constant<int>(), 22);
}

UNIT_TEST(invoke_tests, struct_ref_return_is_non_owning)
{
	auto reg = ge::refl::begin_registry()
		.begin_module("m")
			.begin_func<&free_get_point_ref>("get_point_ref").end_func()
		.end_module()
		.build();

	g_point_target = fpoint{ 5, 6 };
	auto handle = *(*reg.modules().begin()).funcs().begin();
	ge::refl::value result = handle.invoke_unchecked();

	is_false(result.is_owning());
	is_true(result.is_mutable());
	is_eq(result.get_type_id(), ge::refl::make_type_id<fpoint>());
	is_eq(result.const_data(), static_cast<const void*>(&g_point_target));

	result.as_mutable<fpoint>()->x = 100;
	is_eq(g_point_target.x, 100);
	is_eq(g_point_target.y, 6);
}

UNIT_TEST(invoke_tests, struct_const_ref_return_is_non_owning_view)
{
	auto reg = ge::refl::begin_registry()
		.begin_module("m")
			.begin_func<&free_get_point_cref>("get_point_cref").end_func()
		.end_module()
		.build();

	g_point_target = fpoint{ 8, 9 };
	auto handle = *(*reg.modules().begin()).funcs().begin();
	ge::refl::value result = handle.invoke_unchecked();

	is_false(result.is_owning());
	is_false(result.is_mutable());
	is_eq(result.get_type_id(), ge::refl::make_type_id<fpoint>());
	is_eq(result.const_data(), static_cast<const void*>(&g_point_target));
	is_eq(result.as_constant<fpoint>()->x, 8);
	is_eq(result.as_constant<fpoint>()->y, 9);
}

UNIT_TEST(invoke_tests, ref_return_with_args_picks_first_argument)
{
	auto reg = ge::refl::begin_registry()
		.begin_module("m")
			.begin_func<&free_select_first>("first").end_func()
		.end_module()
		.build();

	auto handle = *(*reg.modules().begin()).funcs().begin();
	int a = 1;
	int b = 2;
	ge::refl::value result = handle.invoke_unchecked(a, b);

	is_false(result.is_owning());
	is_true(result.is_mutable());
	is_eq(result.const_data(), static_cast<const void*>(&a));

	*result.as_mutable<int>() = 10;
	is_eq(a, 10);
	is_eq(b, 2);
}

UNIT_TEST(invoke_tests, const_ref_return_with_args_picks_first_argument)
{
	auto reg = ge::refl::begin_registry()
		.begin_module("m")
			.begin_func<&free_select_first_cref>("first_cref").end_func()
		.end_module()
		.build();

	auto handle = *(*reg.modules().begin()).funcs().begin();
	int a = 5;
	int b = 6;
	ge::refl::value result = handle.invoke_unchecked(a, b);

	is_false(result.is_owning());
	is_false(result.is_mutable());
	is_eq(result.const_data(), static_cast<const void*>(&a));
	is_eq(*result.as_constant<int>(), 5);
}

UNIT_TEST(invoke_tests, const_view_to_const_ref_param)
{
	auto reg = ge::refl::begin_registry()
		.begin_module("m")
			.begin_func<&free_double_cref>("dcr").end_func()
		.end_module()
		.build();

	auto handle = *(*reg.modules().begin()).funcs().begin();
	int x = 7;
	ge::refl::value view = ge::refl::value::create_view(x);
	ge::refl::value result = handle.invoke_unchecked(view);
	is_eq(*result.as_constant<int>(), 14);
}

UNIT_TEST(invoke_tests, const_qualified_caller_variable)
{
	auto reg = ge::refl::begin_registry()
		.begin_module("m")
			.begin_func<&free_square>("square").end_func()
		.end_module()
		.build();

	auto handle = *(*reg.modules().begin()).funcs().begin();
	const int x = 7;
	ge::refl::value result = handle.invoke_unchecked(x);
	is_eq(*result.as_constant<int>(), 49);
}
