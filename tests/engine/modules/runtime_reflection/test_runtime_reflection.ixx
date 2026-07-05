// TODO write new tests

export module test_runtime_reflection;
export import test_core;

// TODO To prevent dll from not appearing
export API void thing() {};

//
//import stl;
//import logger;
//import runtime_reflection;
//import test_core;
//
//using namespace ge::test_core;
//using namespace ge::test_core::assert;
//
//
//
//namespace
//{
//	struct type_1 {};
//	struct type_2 {};
//
//	int free_subtract(int a, int b) { return a - b; }
//	int free_square(int x) { return x * x; }
//	int free_return_42() { return 42; }
//	void free_noop() {}
//	void free_increment(int& x) { ++x; }
//	int free_double_cref(const int& x) { return x * 2; }
//	int free_combine3(int a, int b, int c) { return a * 100 + b * 10 + c; }
//
//	int g_side_effect_counter = 0;
//	void free_bump_global() { ++g_side_effect_counter; }
//
//	struct fpoint
//	{
//		int x;
//		int y;
//		bool operator==(const fpoint& other) const { return x == other.x && y == other.y; }
//	};
//
//	fpoint free_make_point(int x, int y) { return fpoint{ x, y }; }
//	int free_point_x(fpoint p) { return p.x; }
//	void free_translate(fpoint& p, int dx, int dy) { p.x += dx; p.y += dy; }
//	int free_point_combine(const fpoint& a, const fpoint& b)
//	{
//		return a.x * 1000 + a.y * 100 + b.x * 10 + b.y;
//	}
//
//	int free_mixed(int by_val, int& by_ref, const int& by_cref)
//	{
//		int result = by_val * 100 + by_ref * 10 + by_cref;
//		by_ref = result;
//		return result;
//	}
//
//	template<typename T>
//	T free_template_identity(T x) { return x; }
//
//	int g_ref_target = 0;
//	int& free_get_ref() { return g_ref_target; }
//	const int& free_get_cref() { return g_ref_target; }
//
//	fpoint g_point_target{ 0, 0 };
//	fpoint& free_get_point_ref() { return g_point_target; }
//	const fpoint& free_get_point_cref() { return g_point_target; }
//
//	int& free_select_first(int& a, int&) { return a; }
//	const int& free_select_first_cref(const int& a, const int&) { return a; }
//
//	using namespace ge::refl;
//
//	static_assert(std::is_same_v<func_sig_t<int(*)(int, int)>, func_sig<int(int, int)>>);
//	static_assert(std::is_same_v<func_sig_t<int(&)(int)>, func_sig<int(int)>>);
//	static_assert(std::is_same_v<func_sig_t<void()>, func_sig<void()>>);
//	static_assert(std::is_same_v<func_sig_t<int(int, double, char)>, func_sig<int(int, double, char)>>);
//
//	struct member_func_owner
//	{
//		[[maybe_unused]] int mut(int, double) { return 0; }
//		[[maybe_unused]] int cst() const { return 0; }
//		[[maybe_unused]] int rval()&& { return 0; }
//	};
//	static_assert(std::is_same_v<func_sig_t<int(member_func_owner::*)(int, double)>, func_sig<int(member_func_owner&, int, double)>>);
//	static_assert(std::is_same_v<func_sig_t<int(member_func_owner::*)() const>, func_sig<int(const member_func_owner&)>>);
//	static_assert(std::is_same_v<func_sig_t<int(member_func_owner::*)()&&>, func_sig<int(member_func_owner&&)>>);
//
//	static_assert(std::is_same_v<remove_decoration_t<int>, int>);
//	static_assert(std::is_same_v<remove_decoration_t<int&>, int>);
//	static_assert(std::is_same_v<remove_decoration_t<const int&>, int>);
//	static_assert(std::is_same_v<remove_decoration_t<int*>, int>);
//	static_assert(std::is_same_v<remove_decoration_t<const int>, int>);
//	static_assert(std::is_same_v<remove_decoration_t<volatile int>, int>);
//
//	static_assert(make_type_id<int>() == make_type_id<remove_decoration_t<int&>>());
//	static_assert(make_type_id<int>() == make_type_id<remove_decoration_t<const int&>>());
//	static_assert(make_type_id<int>() == make_type_id<remove_decoration_t<int*>>());
//
//	static_assert(supported_param_type<int>);
//	static_assert(supported_param_type<int&>);
//	static_assert(supported_param_type<const int&>);
//	static_assert(!supported_param_type<int*>);
//	static_assert(!supported_param_type<const int*>);
//
//	// =====================================================================
//	// Fixtures for the new traits + data API
//	// =====================================================================
//
//	struct entity
//	{
//		int hp;
//		int mp;
//		fpoint pos;
//	};
//
//	int free_get_hp_by_value(const entity& e) { return e.hp; }
//	const int& free_get_hp_cref(const entity& e) { return e.hp; }
//	int free_double_hp(const entity& e) { return e.hp * 2; }
//	void free_set_hp(entity& e, int new_hp) { e.hp = new_hp; }
//	void free_clamp_hp(entity& e, int new_hp)
//	{
//		e.hp = new_hp < 0 ? 0 : (new_hp > 1000 ? 1000 : new_hp);
//	}
//
//	struct empty_type_trait : type_trait {};
//	struct empty_func_trait : func_trait {};
//	struct empty_data_trait : data_trait {};
//
//	struct universal_trait : type_trait, func_trait, data_trait {};
//
//	struct int_type_trait : type_trait { int payload{}; };
//	struct int_func_trait : func_trait { int payload{}; };
//	struct int_data_trait : data_trait { int payload{}; };
//
//	struct point_type_trait : type_trait { fpoint payload{}; };
//	struct string_type_trait : type_trait { std::string payload{}; };
//
//	struct destruct_counting_trait : type_trait
//	{
//		int* counter{};
//		destruct_counting_trait() = default;
//		destruct_counting_trait(int* c) : counter(c) {}
//		destruct_counting_trait(const destruct_counting_trait& other) : counter(other.counter) {}
//
//		[[maybe_unused]] /* // TODO remove attribute */destruct_counting_trait& operator=(const destruct_counting_trait& other)
//		{
//			counter = other.counter;
//			return *this;
//		}
//		~destruct_counting_trait()
//		{
//			if (counter) ++*counter;
//		}
//	};
//
//	struct move_only_int_trait : type_trait
//	{
//		std::unique_ptr<int> p;
//		move_only_int_trait() = default;
//		explicit move_only_int_trait(int v) : p(std::make_unique<int>(v)) {}
//		move_only_int_trait(move_only_int_trait&&) noexcept = default;
//		move_only_int_trait& operator=(move_only_int_trait&&) noexcept = default;
//	};
//
//	struct hook_record
//	{
//		int on_apply_type{};
//		int on_apply_func{};
//		int on_apply_data{};
//		int post_build_type{};
//		int post_build_func{};
//		int post_build_data{};
//		int post_build_count_at_first_apply{ -1 };
//		int on_apply_count_at_first_post_build{ -1 };
//		std::string last_type_name{};
//		std::string last_func_name{};
//		std::string last_data_name{};
//		std::string last_data_outer_name{};
//	};
//
//	struct hooked_trait : type_trait, func_trait, data_trait
//	{
//		hook_record* rec{};
//		hooked_trait() = default;
//		hooked_trait(hook_record* r) : rec(r) {}
//
//		template<typename T>
//		void on_apply(builders::type_builder<T>&)
//		{
//			if (!rec) return;
//			if (rec->post_build_count_at_first_apply == -1)
//			{
//				rec->post_build_count_at_first_apply =
//					rec->post_build_type + rec->post_build_func + rec->post_build_data;
//			}
//			++rec->on_apply_type;
//		}
//
//		template<auto FuncPtr>
//		void on_apply(builders::func_builder<FuncPtr>&)
//		{
//			if (!rec) return;
//			if (rec->post_build_count_at_first_apply == -1)
//			{
//				rec->post_build_count_at_first_apply =
//					rec->post_build_type + rec->post_build_func + rec->post_build_data;
//			}
//			++rec->on_apply_func;
//		}
//
//		template<auto DataPtr>
//		void on_apply(builders::data_builder<DataPtr>&)
//		{
//			if (!rec) return;
//			if (rec->post_build_count_at_first_apply == -1)
//			{
//				rec->post_build_count_at_first_apply =
//					rec->post_build_type + rec->post_build_func + rec->post_build_data;
//			}
//			++rec->on_apply_data;
//		}
//
//		[[maybe_unused]] /* // TODO remove attribute */void post_build(type_handle h)
//		{
//			if (!rec) return;
//			if (rec->on_apply_count_at_first_post_build == -1)
//			{
//				rec->on_apply_count_at_first_post_build =
//					rec->on_apply_type + rec->on_apply_func + rec->on_apply_data;
//			}
//			++rec->post_build_type;
//			rec->last_type_name = std::string{ h.get_name() };
//		}
//
//		[[maybe_unused]] /* // TODO remove attribute */void post_build(func_handle h)
//		{
//			if (!rec) return;
//			if (rec->on_apply_count_at_first_post_build == -1)
//			{
//				rec->on_apply_count_at_first_post_build =
//					rec->on_apply_type + rec->on_apply_func + rec->on_apply_data;
//			}
//			++rec->post_build_func;
//			rec->last_func_name = std::string{ h.get_name() };
//		}
//
//		[[maybe_unused]] /* // TODO remove attribute */void post_build(data_handle h)
//		{
//			if (!rec) return;
//			if (rec->on_apply_count_at_first_post_build == -1)
//			{
//				rec->on_apply_count_at_first_post_build =
//					rec->on_apply_type + rec->on_apply_func + rec->on_apply_data;
//			}
//			++rec->post_build_data;
//			rec->last_data_name = std::string{ h.get_name() };
//			rec->last_data_outer_name = std::string{ h.get_outer_type().get_name() };
//		}
//	};
//
//	static_assert(std::is_base_of_v<type_trait, empty_type_trait>);
//	static_assert(std::is_base_of_v<func_trait, empty_func_trait>);
//	static_assert(std::is_base_of_v<data_trait, empty_data_trait>);
//	static_assert(std::is_base_of_v<type_trait, universal_trait>);
//	static_assert(std::is_base_of_v<func_trait, universal_trait>);
//	static_assert(std::is_base_of_v<data_trait, universal_trait>);
//}
//
//namespace building_tests
//{
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void basic_query()
//	{
//		ge::refl::registry reg =
//			ge::refl::builders::begin_registry()
//			.begin_module("basic")
//			.begin_type<type_1>("type_1")
//			.end_type()
//			.end_module()
//			.build();
//
//		ge::refl::module_handle mod = *reg.modules().begin();
//
//		struct temp : ge::refl::data_trait
//		{
//
//		};
//		ge::refl::data_query::with<universal_trait>::read<temp> query{ mod };
//
//		auto types = reg.types();
//		is_eq(types.size(), 1ull);
//		ge::refl::type_handle type_handle = *types.begin();
//
//		is_eq(type_handle.get_name(), "type_1");
//
//		ge::refl::type_id expectedInfo = ge::refl::make_type_id<type_1>();
//		ge::refl::type_id actualInfo = type_handle.get_id();
//
//		is_eq(expectedInfo, actualInfo);
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void basic()
//	{
//		ge::refl::registry reg =
//			ge::refl::builders::begin_registry()
//			.begin_module("basic")
//			.begin_type<type_1>("type_1")
//			.end_type()
//			.end_module()
//			.build();
//
//		auto types = reg.types();
//		is_eq(types.size(), 1ull);
//		ge::refl::type_handle type_handle = *types.begin();
//
//		is_eq(type_handle.get_name(), "type_1");
//
//		ge::refl::type_id expectedInfo = ge::refl::make_type_id<type_1>();
//		ge::refl::type_id actualInfo = type_handle.get_id();
//
//		is_eq(expectedInfo, actualInfo);
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void basic_ranges()
//	{
//		ge::refl::registry reg =
//			ge::refl::builders::begin_registry()
//			.begin_module("basic")
//			.begin_type<type_1>("type_1")
//			.end_type()
//			.end_module()
//			.build();
//
//		auto modules = reg.modules();
//		is_eq(modules.size(), 1ull);
//		ge::refl::module_handle mod = *modules.begin();
//
//		is_eq(mod.get_name(), "basic");
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void multiple_types_in_module()
//	{
//		ge::refl::registry reg =
//			ge::refl::builders::begin_registry()
//			.begin_module("basic")
//			.begin_type<type_1>("type_1").end_type()
//			.begin_type<type_2>("type_2").end_type()
//			.end_module()
//			.build();
//
//		auto types = reg.types();
//		is_eq(types.size(), 2ull);
//		auto it = types.begin();
//		ge::refl::type_handle t1 = *it;
//		++it;
//		ge::refl::type_handle t2 = *it;
//
//		is_eq(t1.get_name(), "type_1");
//		is_eq(t2.get_name(), "type_2");
//
//		auto modules = reg.modules();
//		is_eq(modules.size(), 1ull);
//		ge::refl::module_handle mod = *modules.begin();
//		auto module_types = mod.types();
//		is_eq(module_types.size(), 2ull);
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void multiple_modules()
//	{
//		ge::refl::registry reg =
//			ge::refl::builders::begin_registry()
//			.begin_module("mod_a")
//			.begin_type<type_1>("a_type").end_type()
//			.end_module()
//			.begin_module("mod_b")
//			.begin_type<type_2>("b_type").end_type()
//			.end_module()
//			.build();
//
//		auto modules = reg.modules();
//		is_eq(modules.size(), 2ull);
//
//		auto it = modules.begin();
//		ge::refl::module_handle m0 = *it;
//		++it;
//		ge::refl::module_handle m1 = *it;
//
//		is_eq(m0.get_name(), "mod_a");
//		is_eq(m1.get_name(), "mod_b");
//		is_eq(m0.types().size(), 1ull);
//		is_eq(m1.types().size(), 1ull);
//		is_eq((*m0.types().begin()).get_name(), "a_type");
//		is_eq((*m1.types().begin()).get_name(), "b_type");
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void registry_types_range()
//	{
//		ge::refl::registry reg =
//			ge::refl::builders::begin_registry()
//			.begin_module("mod_a")
//			.begin_type<type_1>("a_type").end_type()
//			.end_module()
//			.begin_module("mod_b")
//			.begin_type<type_2>("b_type").end_type()
//			.end_module()
//			.build();
//
//		auto types = reg.types();
//		is_eq(types.size(), 2ull);
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void empty_module()
//	{
//		ge::refl::registry reg =
//			ge::refl::builders::begin_registry()
//			.begin_module("empty")
//			.end_module()
//			.build();
//
//		auto modules = reg.modules();
//		is_eq(modules.size(), 1ull);
//		is_eq((*modules.begin()).types().size(), 0ull);
//		is_eq(reg.types().size(), 0ull);
//	}
//}
//
//static void test_big_five(ge::refl::value value_1, auto check)
//{
//	check(value_1);
//
//	ge::refl::value copy = value_1;
//	check(copy);
//
//	ge::refl::value moved = std::move(value_1);
//	check(moved);
//
//	ge::refl::value copy_assigned{};
//	copy_assigned = moved;
//	check(copy_assigned);
//
//	ge::refl::value move_assigned{};
//	move_assigned = std::move(moved);
//	check(move_assigned);
//}
//
//namespace value_tests
//{
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void view_lifetime()
//	{
//		int expected = 42;
//
//		auto check = [&expected](const ge::refl::value& v)
//			{
//				is_not_null(v.const_data());
//				is_eq(v.const_data(), &expected);
//			};
//
//		test_big_five(ge::refl::value::create_view(expected), check);
//		is_eq(expected, 42);
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void ref_lifetime()
//	{
//		int expected = 42;
//
//		auto check = [&expected](const ge::refl::value& v)
//			{
//				is_not_null(v.const_data());
//				is_eq(v.const_data(), &expected);
//			};
//
//		test_big_five(ge::refl::value::create_ref(expected), check);
//		is_eq(expected, 42);
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void owning_lifetime()
//	{
//		int expected = 42;
//
//		auto check = [&expected](const ge::refl::value& v)
//			{
//				is_not_null(v.const_data());
//				is_eq(*v.as_constant<int>(), expected);
//			};
//
//		test_big_five(ge::refl::value::create_owning(expected), check);
//		is_eq(expected, 42);
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void default_constructed_is_null()
//	{
//		ge::refl::value v{};
//		is_null(v.const_data());
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void as_typed_access()
//	{
//		int x = 42;
//
//		ge::refl::value view = ge::refl::value::create_view(x);
//		is_not_null(view.as_constant<int>());
//		is_eq(*view.as_constant<int>(), 42);
//
//		ge::refl::value ref = ge::refl::value::create_ref(x);
//		is_not_null(ref.as_constant<int>());
//		is_not_null(ref.as_mutable<int>());
//		is_eq(*ref.as_constant<int>(), 42);
//		is_eq(static_cast<void*>(ref.as_mutable<int>()), ref.mutable_data());
//
//		ge::refl::value owning = ge::refl::value::create_owning(42);
//		is_not_null(owning.as_constant<int>());
//		is_not_null(owning.as_mutable<int>());
//		is_eq(*owning.as_constant<int>(), 42);
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void owning_is_deep_copy()
//	{
//		ge::refl::value original = ge::refl::value::create_owning(42);
//		ge::refl::value copy = original;
//
//		*copy.as_mutable<int>() = 99;
//
//		is_eq(*original.as_constant<int>(), 42);
//		is_eq(*copy.as_constant<int>(), 99);
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void ref_mutates_source()
//	{
//		int x = 10;
//		ge::refl::value ref = ge::refl::value::create_ref(x);
//		*ref.as_mutable<int>() = 7;
//		is_eq(x, 7);
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void move_leaves_source_empty()
//	{
//		int x = 42;
//		ge::refl::value src = ge::refl::value::create_view(x);
//		is_not_null(src.const_data());
//
//		ge::refl::value dst = std::move(src);
//		is_null(src.const_data());
//		is_not_null(dst.const_data());
//
//		int y = 7;
//		ge::refl::value src2 = ge::refl::value::create_view(y);
//		ge::refl::value dst2{};
//		dst2 = std::move(src2);
//		is_null(src2.const_data());
//		is_not_null(dst2.const_data());
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void pointer_overloads()
//	{
//		int x = 99;
//
//		ge::refl::value view = ge::refl::value::create_view(&x);
//		is_eq(view.const_data(), static_cast<const void*>(&x));
//
//		ge::refl::value ref = ge::refl::value::create_ref(&x);
//		is_eq(ref.const_data(), static_cast<const void*>(&x));
//		is_eq(ref.mutable_data(), static_cast<void*>(&x));
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void clear_resets_value()
//	{
//		ge::refl::value owning = ge::refl::value::create_owning(42);
//		is_not_null(owning.const_data());
//
//		owning.clear();
//		is_null(owning.const_data());
//
//		owning.clear();
//		is_null(owning.const_data());
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void owning_non_trivial_type()
//	{
//		int counter = 0;
//		int inside_scope;
//
//		struct destruct_tracker
//		{
//			int* counter;
//			destruct_tracker(int* c) : counter(c) {}
//			destruct_tracker(const destruct_tracker& other) : counter(other.counter) {}
//			~destruct_tracker() { ++(*counter); }
//		};
//
//		{
//			ge::refl::value v = ge::refl::value::create_owning(destruct_tracker{ &counter });
//			inside_scope = counter;
//		}
//		is_gt(counter, inside_scope);
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void null_pointer_view()
//	{
//		ge::refl::value v = ge::refl::value::create_view(static_cast<const int*>(nullptr));
//		is_null(v.const_data());
//
//		ge::refl::value copy = v;
//		is_null(copy.const_data());
//
//		ge::refl::value moved = std::move(v);
//		is_null(moved.const_data());
//		is_null(v.const_data());
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void null_pointer_ref()
//	{
//		ge::refl::value v = ge::refl::value::create_ref(static_cast<int*>(nullptr));
//		is_null(v.const_data());
//		is_null(v.mutable_data());
//
//		ge::refl::value copy = v;
//		is_null(copy.const_data());
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void pointer_and_reference_equal()
//	{
//		int x = 7;
//
//		ge::refl::value from_ref = ge::refl::value::create_view(x);
//		ge::refl::value from_ptr = ge::refl::value::create_view(&x);
//		is_eq(from_ref.const_data(), from_ptr.const_data());
//
//		ge::refl::value ref_ref = ge::refl::value::create_ref(x);
//		ge::refl::value ref_ptr = ge::refl::value::create_ref(&x);
//		is_eq(ref_ref.mutable_data(), ref_ptr.mutable_data());
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void struct_pointer_overload()
//	{
//		struct two_ints { int a; int b; };
//		two_ints p{ 3, 4 };
//
//		ge::refl::value view = ge::refl::value::create_view(&p);
//		is_eq(view.const_data(), static_cast<const void*>(&p));
//		is_eq(view.as_constant<two_ints>()->a, 3);
//		is_eq(view.as_constant<two_ints>()->b, 4);
//
//		ge::refl::value ref = ge::refl::value::create_ref(&p);
//		ref.as_mutable<two_ints>()->a = 99;
//		is_eq(p.a, 99);
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void cross_ownership_assignment()
//	{
//		int x = 1;
//
//		ge::refl::value slot = ge::refl::value::create_owning(42);
//		slot = ge::refl::value::create_view(x);
//		is_eq(slot.const_data(), static_cast<const void*>(&x));
//
//		ge::refl::value slot2 = ge::refl::value::create_view(x);
//		slot2 = ge::refl::value::create_owning(77);
//		is_eq(*slot2.as_constant<int>(), 77);
//		is_eq(x, 1);
//
//		ge::refl::value owned = ge::refl::value::create_owning(5);
//		ge::refl::value viewed = ge::refl::value::create_view(x);
//		owned = viewed;
//		is_eq(owned.const_data(), static_cast<const void*>(&x));
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void chain_of_moves()
//	{
//		int x = 42;
//		ge::refl::value a = ge::refl::value::create_view(x);
//		ge::refl::value b = std::move(a);
//		ge::refl::value c = std::move(b);
//		ge::refl::value d = std::move(c);
//
//		is_null(a.const_data());
//		is_null(b.const_data());
//		is_null(c.const_data());
//		is_eq(d.const_data(), static_cast<const void*>(&x));
//	}
//}
//
//namespace function_tests
//{
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void building_registers_func_in_module()
//	{
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_func<&free_return_42>("ret42").end_func()
//			.end_module()
//			.build();
//
//		auto modules = reg.modules();
//		is_eq(modules.size(), 1ull);
//		auto funcs = (*modules.begin()).funcs();
//		is_eq(funcs.size(), 1ull);
//		is_eq((*funcs.begin()).get_name(), "ret42");
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void registry_funcs_includes_funcs_from_all_modules()
//	{
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m1")
//			.begin_func<&free_subtract>("sub").end_func()
//			.end_module()
//			.begin_module("m2")
//			.begin_func<&free_square>("square").end_func()
//			.end_module()
//			.build();
//
//		is_eq(reg.funcs().size(), 2ull);
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void multiple_funcs_in_module_preserve_order()
//	{
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_func<&free_subtract>("sub").end_func()
//			.begin_func<&free_square>("square").end_func()
//			.begin_func<&free_return_42>("ret42").end_func()
//			.end_module()
//			.build();
//
//		auto funcs = (*reg.modules().begin()).funcs();
//		is_eq(funcs.size(), 3ull);
//
//		std::vector<std::string_view> names;
//		for (auto f : funcs)
//		{
//			names.push_back(f.get_name());
//		}
//		is_eq(names.size(), 3ull);
//		is_eq(names[0], "sub");
//		is_eq(names[1], "square");
//		is_eq(names[2], "ret42");
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void multiple_modules_each_with_own_func_count()
//	{
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m1")
//			.begin_func<&free_subtract>("sub").end_func()
//			.begin_func<&free_square>("square").end_func()
//			.end_module()
//			.begin_module("m2")
//			.begin_func<&free_return_42>("ret42").end_func()
//			.end_module()
//			.build();
//
//		auto mods = reg.modules();
//		is_eq(mods.size(), 2ull);
//		auto it = mods.begin();
//		is_eq((*it).funcs().size(), 2ull);
//		++it;
//		is_eq((*it).funcs().size(), 1ull);
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void empty_module_has_no_funcs()
//	{
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("empty")
//			.end_module()
//			.build();
//
//		auto mods = reg.modules();
//		is_eq(mods.size(), 1ull);
//		is_eq((*mods.begin()).funcs().size(), 0ull);
//		is_eq(reg.funcs().size(), 0ull);
//	}
//}
//
//namespace invoke_tests
//{
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void no_args_returning_int()
//	{
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_func<&free_return_42>("ret42").end_func()
//			.end_module()
//			.build();
//
//		auto handle = *(*reg.modules().begin()).funcs().begin();
//		ge::refl::value result = handle.invoke_unchecked();
//		is_not_null(result.const_data());
//		is_eq(*result.as_constant<int>(), 42);
//		is_eq(result.get_type_id(), ge::refl::make_type_id<int>());
//		is_true(result.is_owning());
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void no_args_void_returns_empty_value()
//	{
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_func<&free_noop>("noop").end_func()
//			.end_module()
//			.build();
//
//		auto handle = *(*reg.modules().begin()).funcs().begin();
//		ge::refl::value result = handle.invoke_unchecked();
//		is_null(result.const_data());
//		is_false(static_cast<bool>(result));
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void single_int_arg()
//	{
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_func<&free_square>("square").end_func()
//			.end_module()
//			.build();
//
//		auto handle = *(*reg.modules().begin()).funcs().begin();
//		int input = 5;
//		ge::refl::value result = handle.invoke_unchecked(input);
//		is_eq(*result.as_constant<int>(), 25);
//
//		int negative = -7;
//		is_eq(*handle.invoke_unchecked(negative).as_constant<int>(), 49);
//
//		int zero = 0;
//		is_eq(*handle.invoke_unchecked(zero).as_constant<int>(), 0);
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void two_int_args()
//	{
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_func<&free_subtract>("sub").end_func()
//			.end_module()
//			.build();
//
//		auto handle = *(*reg.modules().begin()).funcs().begin();
//		int a = 3;
//		int b = 4;
//		is_eq(*handle.invoke_unchecked(a, b).as_constant<int>(), -1);
//
//		a = -10;
//		b = 3;
//		is_eq(*handle.invoke_unchecked(a, b).as_constant<int>(), -13);
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void two_int_args_order_matters()
//	{
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_func<&free_subtract>("sub").end_func()
//			.end_module()
//			.build();
//
//		auto handle = *(*reg.modules().begin()).funcs().begin();
//		int a = 10;
//		int b = 3;
//		is_eq(*handle.invoke_unchecked(a, b).as_constant<int>(), 7);
//		is_eq(*handle.invoke_unchecked(b, a).as_constant<int>(), -7);
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void three_int_args()
//	{
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_func<&free_combine3>("combine3").end_func()
//			.end_module()
//			.build();
//
//		auto handle = *(*reg.modules().begin()).funcs().begin();
//		int a = 1, b = 2, c = 3;
//		is_eq(*handle.invoke_unchecked(a, b, c).as_constant<int>(), 123);
//		is_eq(*handle.invoke_unchecked(c, b, a).as_constant<int>(), 321);
//		is_eq(*handle.invoke_unchecked(b, a, c).as_constant<int>(), 213);
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void mutable_ref_propagates_back()
//	{
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_func<&free_increment>("inc").end_func()
//			.end_module()
//			.build();
//
//		auto handle = *(*reg.modules().begin()).funcs().begin();
//		int x = 10;
//		ge::refl::value result = handle.invoke_unchecked(x);
//		is_null(result.const_data());
//		is_eq(x, 11);
//
//		handle.invoke_unchecked(x);
//		is_eq(x, 12);
//
//		handle.invoke_unchecked(x);
//		is_eq(x, 13);
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void const_ref_does_not_mutate_caller()
//	{
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_func<&free_double_cref>("dcr").end_func()
//			.end_module()
//			.build();
//
//		auto handle = *(*reg.modules().begin()).funcs().begin();
//		int x = 7;
//		ge::refl::value result = handle.invoke_unchecked(x);
//		is_eq(*result.as_constant<int>(), 14);
//		is_eq(x, 7);
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void by_value_does_not_mutate_caller_struct()
//	{
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_func<&free_point_x>("px").end_func()
//			.end_module()
//			.build();
//
//		auto handle = *(*reg.modules().begin()).funcs().begin();
//		fpoint p{ 3, 4 };
//		ge::refl::value result = handle.invoke_unchecked(p);
//		is_eq(*result.as_constant<int>(), 3);
//		is_eq(p.x, 3);
//		is_eq(p.y, 4);
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void struct_ref_propagates_back()
//	{
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_func<&free_translate>("translate").end_func()
//			.end_module()
//			.build();
//
//		auto handle = *(*reg.modules().begin()).funcs().begin();
//		fpoint p{ 1, 2 };
//		int dx = 10;
//		int dy = 20;
//		ge::refl::value result = handle.invoke_unchecked(p, dx, dy);
//		is_null(result.const_data());
//		is_eq(p.x, 11);
//		is_eq(p.y, 22);
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void struct_const_ref_does_not_mutate()
//	{
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_func<&free_point_combine>("combine").end_func()
//			.end_module()
//			.build();
//
//		auto handle = *(*reg.modules().begin()).funcs().begin();
//		fpoint a{ 1, 2 };
//		fpoint b{ 3, 4 };
//		ge::refl::value result = handle.invoke_unchecked(a, b);
//		is_eq(*result.as_constant<int>(), 1234);
//		ge::refl::value swapped = handle.invoke_unchecked(b, a);
//		is_eq(*swapped.as_constant<int>(), 3412);
//		is_eq(a, (fpoint{ 1, 2 }));
//		is_eq(b, (fpoint{ 3, 4 }));
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void returns_struct_by_value()
//	{
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_func<&free_make_point>("mp").end_func()
//			.end_module()
//			.build();
//
//		auto handle = *(*reg.modules().begin()).funcs().begin();
//		int x = 9;
//		int y = -2;
//		ge::refl::value result = handle.invoke_unchecked(x, y);
//		is_not_null(result.const_data());
//		is_true(result.is_owning());
//		is_eq(result.get_type_id(), ge::refl::make_type_id<fpoint>());
//		is_eq(result.as_constant<fpoint>()->x, 9);
//		is_eq(result.as_constant<fpoint>()->y, -2);
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void returned_owning_outlives_invoke_args()
//	{
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_func<&free_subtract>("sub").end_func()
//			.end_module()
//			.build();
//
//		auto handle = *(*reg.modules().begin()).funcs().begin();
//		ge::refl::value result;
//		{
//			int a = 200;
//			int b = 100;
//			result = handle.invoke_unchecked(a, b);
//		}
//		is_true(result.is_owning());
//		is_eq(*result.as_constant<int>(), 100);
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void returned_struct_is_independent_copy()
//	{
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_func<&free_make_point>("mp").end_func()
//			.end_module()
//			.build();
//
//		auto handle = *(*reg.modules().begin()).funcs().begin();
//		int x = 5, y = 6;
//		ge::refl::value v = handle.invoke_unchecked(x, y);
//		v.as_mutable<fpoint>()->x = 999;
//
//		ge::refl::value v2 = handle.invoke_unchecked(x, y);
//		is_eq(v2.as_constant<fpoint>()->x, 5);
//		is_eq(v.as_constant<fpoint>()->x, 999);
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void mixed_param_qualifiers()
//	{
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_func<&free_mixed>("mixed").end_func()
//			.end_module()
//			.build();
//
//		auto handle = *(*reg.modules().begin()).funcs().begin();
//		int v = 1;
//		int r = 2;
//		int c = 3;
//		ge::refl::value result = handle.invoke_unchecked(v, r, c);
//		is_eq(*result.as_constant<int>(), 123);
//		is_eq(v, 1);
//		is_eq(r, 123);
//		is_eq(c, 3);
//
//		int v2 = 3;
//		int r2 = 2;
//		int c2 = 1;
//		ge::refl::value result2 = handle.invoke_unchecked(v2, r2, c2);
//		is_eq(*result2.as_constant<int>(), 321);
//		is_eq(v2, 3);
//		is_eq(r2, 321);
//		is_eq(c2, 1);
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void repeated_invokes_are_independent()
//	{
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_func<&free_square>("square").end_func()
//			.end_module()
//			.build();
//
//		auto handle = *(*reg.modules().begin()).funcs().begin();
//		int x = 6;
//		is_eq(*handle.invoke_unchecked(x).as_constant<int>(), 36);
//		is_eq(*handle.invoke_unchecked(x).as_constant<int>(), 36);
//
//		int y = 4;
//		is_eq(*handle.invoke_unchecked(y).as_constant<int>(), 16);
//
//		int z = 11;
//		is_eq(*handle.invoke_unchecked(z).as_constant<int>(), 121);
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void void_function_global_side_effect()
//	{
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_func<&free_bump_global>("bump").end_func()
//			.end_module()
//			.build();
//
//		int before = g_side_effect_counter;
//		auto handle = *(*reg.modules().begin()).funcs().begin();
//		handle.invoke_unchecked();
//		is_eq(g_side_effect_counter, before + 1);
//		handle.invoke_unchecked();
//		handle.invoke_unchecked();
//		is_eq(g_side_effect_counter, before + 3);
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void accepts_value_argument_by_owning()
//	{
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_func<&free_square>("square").end_func()
//			.end_module()
//			.build();
//
//		auto handle = *(*reg.modules().begin()).funcs().begin();
//		ge::refl::value arg = ge::refl::value::create_owning(8);
//		ge::refl::value result = handle.invoke_unchecked(arg);
//		is_eq(*result.as_constant<int>(), 64);
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void accepts_value_argument_by_ref()
//	{
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_func<&free_increment>("inc").end_func()
//			.end_module()
//			.build();
//
//		auto handle = *(*reg.modules().begin()).funcs().begin();
//		int x = 50;
//		ge::refl::value arg = ge::refl::value::create_ref(x);
//		handle.invoke_unchecked(arg);
//		is_eq(x, 51);
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void chains_invocations_via_returned_value()
//	{
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_func<&free_make_point>("mp").end_func()
//			.begin_func<&free_point_x>("px").end_func()
//			.end_module()
//			.build();
//
//		auto funcs = (*reg.modules().begin()).funcs();
//		auto it = funcs.begin();
//		auto mp = *it;
//		++it;
//		auto px = *it;
//
//		int xv = 7, yv = 9;
//		ge::refl::value point_value = mp.invoke_unchecked(xv, yv);
//		is_eq(point_value.get_type_id(), ge::refl::make_type_id<fpoint>());
//
//		ge::refl::value x_value = px.invoke_unchecked(point_value);
//		is_eq(*x_value.as_constant<int>(), 7);
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void distinct_funcs_use_distinct_signatures()
//	{
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_func<&free_subtract>("sub").end_func()
//			.begin_func<&free_square>("square").end_func()
//			.begin_func<&free_return_42>("ret42").end_func()
//			.end_module()
//			.build();
//
//		auto funcs = (*reg.modules().begin()).funcs();
//		auto it = funcs.begin();
//		auto sub_handle = *it;
//		++it;
//		auto square_handle = *it;
//		++it;
//		auto ret42_handle = *it;
//
//		int a = 5, b = 7;
//		is_eq(*sub_handle.invoke_unchecked(a, b).as_constant<int>(), -2);
//		is_eq(*square_handle.invoke_unchecked(a).as_constant<int>(), 25);
//		is_eq(*ret42_handle.invoke_unchecked().as_constant<int>(), 42);
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void template_instantiation_is_distinct()
//	{
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_func<&free_template_identity<int>>("id_int").end_func()
//			.end_module()
//			.build();
//
//		auto handle = *(*reg.modules().begin()).funcs().begin();
//		int x = 123;
//		ge::refl::value result = handle.invoke_unchecked(x);
//		is_eq(*result.as_constant<int>(), 123);
//		is_eq(result.get_type_id(), ge::refl::make_type_id<int>());
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void return_value_is_owning_and_holds_correct_type_id()
//	{
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_func<&free_make_point>("mp").end_func()
//			.begin_func<&free_return_42>("ret42").end_func()
//			.end_module()
//			.build();
//
//		auto funcs = (*reg.modules().begin()).funcs();
//		auto it = funcs.begin();
//		auto mp = *it;
//		++it;
//		auto ret42 = *it;
//
//		int xv = 1, yv = 2;
//		ge::refl::value point_v = mp.invoke_unchecked(xv, yv);
//		is_true(point_v.is_owning());
//		is_eq(point_v.get_type_id(), ge::refl::make_type_id<fpoint>());
//		is_ne(point_v.get_type_id(), ge::refl::make_type_id<int>());
//
//		ge::refl::value int_v = ret42.invoke_unchecked();
//		is_true(int_v.is_owning());
//		is_eq(int_v.get_type_id(), ge::refl::make_type_id<int>());
//		is_ne(int_v.get_type_id(), ge::refl::make_type_id<fpoint>());
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void invoke_does_not_share_state_between_handles()
//	{
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_func<&free_subtract>("s1").end_func()
//			.begin_func<&free_subtract>("s2").end_func()
//			.end_module()
//			.build();
//
//		auto funcs = (*reg.modules().begin()).funcs();
//		auto it = funcs.begin();
//		auto s1 = *it;
//		++it;
//		auto s2 = *it;
//
//		is_eq(s1.get_name(), "s1");
//		is_eq(s2.get_name(), "s2");
//
//		int x = 7;
//		int y = 3;
//		is_eq(*s1.invoke_unchecked(x, y).as_constant<int>(), 4);
//		is_eq(*s2.invoke_unchecked(x, y).as_constant<int>(), 4);
//		is_eq(*s1.invoke_unchecked(y, x).as_constant<int>(), -4);
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void ref_return_is_non_owning()
//	{
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_func<&free_get_ref>("get_ref").end_func()
//			.end_module()
//			.build();
//
//		g_ref_target = 42;
//		auto handle = *(*reg.modules().begin()).funcs().begin();
//		ge::refl::value result = handle.invoke_unchecked();
//
//		is_false(result.is_owning());
//		is_eq(result.get_type_id(), ge::refl::make_type_id<int>());
//		is_eq(result.const_data(), static_cast<const void*>(&g_ref_target));
//		is_eq(*result.as_constant<int>(), 42);
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void ref_return_is_mutable_and_aliases_target()
//	{
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_func<&free_get_ref>("get_ref").end_func()
//			.end_module()
//			.build();
//
//		g_ref_target = 7;
//		auto handle = *(*reg.modules().begin()).funcs().begin();
//		ge::refl::value result = handle.invoke_unchecked();
//
//		is_true(result.is_mutable());
//		is_eq(result.mutable_data(), static_cast<void*>(&g_ref_target));
//
//		*result.as_mutable<int>() = 999;
//		is_eq(g_ref_target, 999);
//
//		g_ref_target = 17;
//		is_eq(*result.as_constant<int>(), 17);
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void const_ref_return_is_non_owning()
//	{
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_func<&free_get_cref>("get_cref").end_func()
//			.end_module()
//			.build();
//
//		g_ref_target = 55;
//		auto handle = *(*reg.modules().begin()).funcs().begin();
//		ge::refl::value result = handle.invoke_unchecked();
//
//		is_false(result.is_owning());
//		is_eq(result.get_type_id(), ge::refl::make_type_id<int>());
//		is_eq(result.const_data(), static_cast<const void*>(&g_ref_target));
//		is_eq(*result.as_constant<int>(), 55);
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void const_ref_return_is_immutable_view()
//	{
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_func<&free_get_cref>("get_cref").end_func()
//			.end_module()
//			.build();
//
//		g_ref_target = 11;
//		auto handle = *(*reg.modules().begin()).funcs().begin();
//		ge::refl::value result = handle.invoke_unchecked();
//
//		is_false(result.is_mutable());
//
//		g_ref_target = 22;
//		is_eq(*result.as_constant<int>(), 22);
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void struct_ref_return_is_non_owning()
//	{
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_func<&free_get_point_ref>("get_point_ref").end_func()
//			.end_module()
//			.build();
//
//		g_point_target = fpoint{ 5, 6 };
//		auto handle = *(*reg.modules().begin()).funcs().begin();
//		ge::refl::value result = handle.invoke_unchecked();
//
//		is_false(result.is_owning());
//		is_true(result.is_mutable());
//		is_eq(result.get_type_id(), ge::refl::make_type_id<fpoint>());
//		is_eq(result.const_data(), static_cast<const void*>(&g_point_target));
//
//		result.as_mutable<fpoint>()->x = 100;
//		is_eq(g_point_target.x, 100);
//		is_eq(g_point_target.y, 6);
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void struct_const_ref_return_is_non_owning_view()
//	{
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_func<&free_get_point_cref>("get_point_cref").end_func()
//			.end_module()
//			.build();
//
//		g_point_target = fpoint{ 8, 9 };
//		auto handle = *(*reg.modules().begin()).funcs().begin();
//		ge::refl::value result = handle.invoke_unchecked();
//
//		is_false(result.is_owning());
//		is_false(result.is_mutable());
//		is_eq(result.get_type_id(), ge::refl::make_type_id<fpoint>());
//		is_eq(result.const_data(), static_cast<const void*>(&g_point_target));
//		is_eq(result.as_constant<fpoint>()->x, 8);
//		is_eq(result.as_constant<fpoint>()->y, 9);
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void ref_return_with_args_picks_first_argument()
//	{
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_func<&free_select_first>("first").end_func()
//			.end_module()
//			.build();
//
//		auto handle = *(*reg.modules().begin()).funcs().begin();
//		int a = 1;
//		int b = 2;
//		ge::refl::value result = handle.invoke_unchecked(a, b);
//
//		is_false(result.is_owning());
//		is_true(result.is_mutable());
//		is_eq(result.const_data(), static_cast<const void*>(&a));
//
//		*result.as_mutable<int>() = 10;
//		is_eq(a, 10);
//		is_eq(b, 2);
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void const_ref_return_with_args_picks_first_argument()
//	{
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_func<&free_select_first_cref>("first_cref").end_func()
//			.end_module()
//			.build();
//
//		auto handle = *(*reg.modules().begin()).funcs().begin();
//		int a = 5;
//		int b = 6;
//		ge::refl::value result = handle.invoke_unchecked(a, b);
//
//		is_false(result.is_owning());
//		is_false(result.is_mutable());
//		is_eq(result.const_data(), static_cast<const void*>(&a));
//		is_eq(*result.as_constant<int>(), 5);
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void const_view_to_const_ref_param()
//	{
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_func<&free_double_cref>("dcr").end_func()
//			.end_module()
//			.build();
//
//		auto handle = *(*reg.modules().begin()).funcs().begin();
//		int x = 7;
//		ge::refl::value view = ge::refl::value::create_view(x);
//		ge::refl::value result = handle.invoke_unchecked(view);
//		is_eq(*result.as_constant<int>(), 14);
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void const_qualified_caller_variable()
//	{
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_func<&free_square>("square").end_func()
//			.end_module()
//			.build();
//
//		auto handle = *(*reg.modules().begin()).funcs().begin();
//		const int x = 7;
//		ge::refl::value result = handle.invoke_unchecked(x);
//		is_eq(*result.as_constant<int>(), 49);
//	}
//}
//
//// =========================================================================
//// trait_tests — building, retrieving, and lifetime of traits
//// =========================================================================
//
//namespace trait_tests
//{
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void type_trait_count_zero_when_unused()
//	{
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_type<type_1>("t").end_type()
//			.end_module()
//			.build();
//		is_eq((*reg.types().begin()).traits().size(), 0ull);
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void type_trait_one_added_count_one()
//	{
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_type<type_1>("t")
//			.add_traits(empty_type_trait{})
//			.end_type()
//			.end_module()
//			.build();
//		is_eq((*reg.types().begin()).traits().size(), 1ull);
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void type_trait_state_round_trip_int()
//	{
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_type<type_1>("t")
//			.add_traits(int_type_trait{ .payload = 99 })
//			.end_type()
//			.end_module()
//			.build();
//
//		auto traits = (*reg.types().begin()).traits();
//		const ge::refl::value& v = *traits.begin();
//		is_not_null(v.const_data());
//		is_eq(v.get_type_id(), ge::refl::make_type_id<int_type_trait>());
//		is_eq(v.as_constant<int_type_trait>()->payload, 99);
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void type_trait_state_round_trip_struct()
//	{
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_type<type_1>("t")
//			.add_traits(point_type_trait{ .payload = fpoint{ 3, 7 } })
//			.end_type()
//			.end_module()
//			.build();
//
//		auto traits = (*reg.types().begin()).traits();
//		is_eq((*traits.begin()).as_constant<point_type_trait>()->payload, (fpoint{ 3, 7 }));
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void type_trait_state_round_trip_string_heap()
//	{
//		std::string long_payload = "hello world long enough to heap allocate definitely yes definitely";
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_type<type_1>("t")
//			.add_traits(string_type_trait{ .payload = long_payload })
//			.end_type()
//			.end_module()
//			.build();
//
//		auto traits = (*reg.types().begin()).traits();
//		is_eq(traits.size(), 1ull);
//		is_eq((*traits.begin()).as_constant<string_type_trait>()->payload, long_payload);
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void type_trait_three_added_count_three()
//	{
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_type<type_1>("t")
//			.add_traits(empty_type_trait{})
//			.add_traits(int_type_trait{ .payload = 1 })
//			.add_traits(point_type_trait{ .payload = fpoint{ 5, 6 } })
//			.end_type()
//			.end_module()
//			.build();
//		is_eq((*reg.types().begin()).traits().size(), 3ull);
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void type_trait_order_preserved()
//	{
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_type<type_1>("t")
//			.add_traits(int_type_trait{ .payload = 10 })
//			.add_traits(int_type_trait{ .payload = 20 })
//			.add_traits(int_type_trait{ .payload = 30 })
//			.end_type()
//			.end_module()
//			.build();
//
//		std::vector<int> values;
//		for (const auto& v : (*reg.types().begin()).traits())
//		{
//			values.push_back(v.as_constant<int_type_trait>()->payload);
//		}
//		is_eq(values.size(), 3ull);
//		is_eq(values[0], 10);
//		is_eq(values[1], 20);
//		is_eq(values[2], 30);
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void type_trait_independent_across_types_in_same_module()
//	{
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_type<type_1>("t1")
//			.add_traits(int_type_trait{ .payload = 1 })
//			.end_type()
//			.begin_type<type_2>("t2")
//			.add_traits(int_type_trait{ .payload = 2 })
//			.add_traits(int_type_trait{ .payload = 3 })
//			.end_type()
//			.end_module()
//			.build();
//
//		auto types = reg.types();
//		auto it = types.begin();
//		auto t1 = *it;
//		++it;
//		auto t2 = *it;
//		is_eq(t1.traits().size(), 1ull);
//		is_eq(t2.traits().size(), 2ull);
//		is_eq((*t1.traits().begin()).as_constant<int_type_trait>()->payload, 1);
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void type_trait_independent_across_modules()
//	{
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("a")
//			.begin_type<type_1>("ta")
//			.add_traits(int_type_trait{ .payload = 100 })
//			.end_type()
//			.end_module()
//			.begin_module("b")
//			.begin_type<type_2>("tb")
//			.add_traits(int_type_trait{ .payload = 200 })
//			.end_type()
//			.end_module()
//			.build();
//
//		auto types = reg.types();
//		auto it = types.begin();
//		auto ta = *it;
//		++it;
//		auto tb = *it;
//		is_eq(ta.traits().size(), 1ull);
//		is_eq(tb.traits().size(), 1ull);
//		is_eq((*ta.traits().begin()).as_constant<int_type_trait>()->payload, 100);
//		is_eq((*tb.traits().begin()).as_constant<int_type_trait>()->payload, 200);
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void type_trait_value_type_id_matches_trait_type()
//	{
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_type<type_1>("t")
//			.add_traits(int_type_trait{ .payload = 5 })
//			.add_traits(point_type_trait{ .payload = fpoint{ 1, 2 } })
//			.end_type()
//			.end_module()
//			.build();
//
//		auto traits = (*reg.types().begin()).traits();
//		auto it = traits.begin();
//		is_eq((*it).get_type_id(), ge::refl::make_type_id<int_type_trait>());
//		++it;
//		is_eq((*it).get_type_id(), ge::refl::make_type_id<point_type_trait>());
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void traits_values_are_not_mutable()
//	{
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_type<type_1>("t").add_traits(int_type_trait{ .payload = 5 }).end_type()
//			.end_module()
//			.build();
//
//		const ge::refl::value& v = *(*reg.types().begin()).traits().begin();
//		is_false(v.is_mutable());
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void func_trait_count_zero_when_unused()
//	{
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_func<&free_return_42>("ret42").end_func()
//			.end_module()
//			.build();
//
//		auto func = *(*reg.modules().begin()).funcs().begin();
//		is_eq(func.traits().size(), 0ull);
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void func_trait_one_added()
//	{
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_func<&free_return_42>("ret42")
//			.add_traits(empty_func_trait{})
//			.end_func()
//			.end_module()
//			.build();
//
//		auto func = *(*reg.modules().begin()).funcs().begin();
//		is_eq(func.traits().size(), 1ull);
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void func_trait_state_round_trip()
//	{
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_func<&free_return_42>("ret42")
//			.add_traits(int_func_trait{ .payload = 77 })
//			.end_func()
//			.end_module()
//			.build();
//
//		auto func = *(*reg.modules().begin()).funcs().begin();
//		is_eq((*func.traits().begin()).as_constant<int_func_trait>()->payload, 77);
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void func_trait_multiple_order_preserved()
//	{
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_func<&free_return_42>("ret42")
//			.add_traits(int_func_trait{ .payload = 1 })
//			.add_traits(int_func_trait{ .payload = 2 })
//			.add_traits(int_func_trait{ .payload = 3 })
//			.end_func()
//			.end_module()
//			.build();
//
//		auto func = *(*reg.modules().begin()).funcs().begin();
//		std::vector<int> values;
//		for (const auto& v : func.traits())
//		{
//			values.push_back(v.as_constant<int_func_trait>()->payload);
//		}
//		is_eq(values.size(), 3ull);
//		is_eq(values[0], 1);
//		is_eq(values[1], 2);
//		is_eq(values[2], 3);
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void func_trait_independent_per_func()
//	{
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_func<&free_return_42>("a").add_traits(int_func_trait{ .payload = 10 }).end_func()
//			.begin_func<&free_noop>("b").add_traits(int_func_trait{ .payload = 20 }).add_traits(int_func_trait{ .payload = 30 }).end_func()
//			.end_module()
//			.build();
//
//		auto funcs = (*reg.modules().begin()).funcs();
//		auto it = funcs.begin();
//		auto a = *it; ++it;
//		auto b = *it;
//
//		is_eq(a.traits().size(), 1ull);
//		is_eq(b.traits().size(), 2ull);
//		is_eq((*a.traits().begin()).as_constant<int_func_trait>()->payload, 10);
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void func_with_traits_still_invokes_correctly()
//	{
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_func<&free_subtract>("sub")
//			.add_traits(int_func_trait{ .payload = 5 })
//			.add_traits(int_func_trait{ .payload = 6 })
//			.end_func()
//			.end_module()
//			.build();
//
//		auto func = *(*reg.modules().begin()).funcs().begin();
//		int a = 10, b = 3;
//		is_eq(*func.invoke_unchecked(a, b).as_constant<int>(), 7);
//		is_eq(func.traits().size(), 2ull);
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void type_and_func_trait_disjoint_in_same_type()
//	{
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_type<type_1>("t")
//			.add_traits(int_type_trait{ .payload = 1 })
//			.begin_func<&free_return_42>("ret").add_traits(int_func_trait{ .payload = 2 }).end_func()
//			.end_type()
//			.end_module()
//			.build();
//
//		auto t = *reg.types().begin();
//		is_eq(t.traits().size(), 1ull);
//		auto f = *t.funcs().begin();
//		is_eq(f.traits().size(), 1ull);
//		is_eq((*t.traits().begin()).as_constant<int_type_trait>()->payload, 1);
//		is_eq((*f.traits().begin()).as_constant<int_func_trait>()->payload, 2);
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void multi_inherit_trait_attaches_to_type()
//	{
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_type<type_1>("t").add_traits(universal_trait{}).end_type()
//			.end_module()
//			.build();
//
//		auto t = *reg.types().begin();
//		is_eq(t.traits().size(), 1ull);
//		is_eq((*t.traits().begin()).get_type_id(), ge::refl::make_type_id<universal_trait>());
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void multi_inherit_trait_attaches_to_func()
//	{
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_func<&free_noop>("f").add_traits(universal_trait{}).end_func()
//			.end_module()
//			.build();
//
//		auto f = *(*reg.modules().begin()).funcs().begin();
//		is_eq(f.traits().size(), 1ull);
//		is_eq((*f.traits().begin()).get_type_id(), ge::refl::make_type_id<universal_trait>());
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void multi_inherit_trait_distinct_storage_per_target()
//	{
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_type<type_1>("t").add_traits(universal_trait{}).end_type()
//			.begin_func<&free_noop>("f").add_traits(universal_trait{}).end_func()
//			.end_module()
//			.build();
//
//		auto t = *reg.types().begin();
//		auto f = *(*reg.modules().begin()).funcs().begin();
//		is_ne((*t.traits().begin()).const_data(), (*f.traits().begin()).const_data());
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void traits_iteration_yields_typed_values()
//	{
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_type<type_1>("t")
//			.add_traits(int_type_trait{ .payload = 1 })
//			.add_traits(int_type_trait{ .payload = 2 })
//			.end_type()
//			.end_module()
//			.build();
//
//		int sum = 0;
//		for (const auto& v : (*reg.types().begin()).traits())
//		{
//			is_not_null(v.const_data());
//			is_eq(v.get_type_id(), ge::refl::make_type_id<int_type_trait>());
//			sum += v.as_constant<int_type_trait>()->payload;
//		}
//		is_eq(sum, 3);
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void traits_outlive_builder_scope()
//	{
//		ge::refl::registry reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_type<type_1>("t").add_traits(int_type_trait{ .payload = 42 }).end_type()
//			.end_module()
//			.build();
//
//		auto t = *reg.types().begin();
//		is_eq(t.traits().size(), 1ull);
//		is_eq((*t.traits().begin()).as_constant<int_type_trait>()->payload, 42);
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void traits_pointer_stable_across_handle_copies()
//	{
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_type<type_1>("t").add_traits(int_type_trait{ .payload = 7 }).end_type()
//			.end_module()
//			.build();
//
//		ge::refl::type_handle h1 = *reg.types().begin();
//		ge::refl::type_handle h2 = h1;
//		is_eq((*h1.traits().begin()).const_data(), (*h2.traits().begin()).const_data());
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void traits_pointer_stable_after_more_targets_built()
//	{
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_type<type_1>("t1").add_traits(int_type_trait{ .payload = 11 }).end_type()
//			.begin_type<type_2>("t2").add_traits(int_type_trait{ .payload = 22 }).end_type()
//			.begin_func<&free_return_42>("f").add_traits(int_func_trait{ .payload = 33 }).end_func()
//			.end_module()
//			.build();
//
//		auto t1 = *reg.types().begin();
//		is_eq(t1.traits().size(), 1ull);
//		is_eq((*t1.traits().begin()).as_constant<int_type_trait>()->payload, 11);
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void trait_destructor_runs_when_registry_destroyed()
//	{
//		int counter = 0;
//		int snapshot_after_build = 0;
//		{
//			auto reg = ge::refl::builders::begin_registry()
//				.begin_module("m")
//				.begin_type<type_1>("t").add_traits(destruct_counting_trait{ &counter }).end_type()
//				.end_module()
//				.build();
//			snapshot_after_build = counter;
//		}
//		is_gt(counter, snapshot_after_build);
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void trait_destructor_runs_for_each_added_instance()
//	{
//		int counter = 0;
//		int snapshot_after_build = 0;
//		{
//			auto reg = ge::refl::builders::begin_registry()
//				.begin_module("m")
//				.begin_type<type_1>("t")
//				.add_traits(destruct_counting_trait{ &counter })
//				.add_traits(destruct_counting_trait{ &counter })
//				.add_traits(destruct_counting_trait{ &counter })
//				.end_type()
//				.end_module()
//				.build();
//			snapshot_after_build = counter;
//		}
//		is_eq(counter - snapshot_after_build, 3);
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void move_only_trait_supported()
//	{
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_type<type_1>("t").add_traits(move_only_int_trait{ 99 }).end_type()
//			.end_module()
//			.build();
//
//		auto traits = (*reg.types().begin()).traits();
//		is_eq(traits.size(), 1ull);
//		const move_only_int_trait* p = (*traits.begin()).as_constant<move_only_int_trait>();
//		is_not_null(p);
//		is_not_null(p->p.get());
//		is_eq(*p->p, 99);
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void registry_with_traits_only_no_funcs_no_data()
//	{
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_type<type_1>("t").add_traits(int_type_trait{ .payload = 1 }).end_type()
//			.end_module()
//			.build();
//
//		auto t = *reg.types().begin();
//		is_eq(t.funcs().size(), 0ull);
//		is_eq(t.datas().size(), 0ull);
//		is_eq(t.traits().size(), 1ull);
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void many_traits_on_single_type()
//	{
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_type<type_1>("t")
//			.add_traits(int_type_trait{ .payload = 0 }).add_traits(int_type_trait{ .payload = 1 }).add_traits(int_type_trait{ .payload = 2 })
//			.add_traits(int_type_trait{ .payload = 3 }).add_traits(int_type_trait{ .payload = 4 }).add_traits(int_type_trait{ .payload = 5 })
//			.add_traits(int_type_trait{ .payload = 6 }).add_traits(int_type_trait{ .payload = 7 }).add_traits(int_type_trait{ .payload = 8 })
//			.add_traits(int_type_trait{ .payload = 9 }).add_traits(int_type_trait{ .payload = 10 }).add_traits(int_type_trait{ .payload = 11 })
//			.add_traits(int_type_trait{ .payload = 12 }).add_traits(int_type_trait{ .payload = 13 }).add_traits(int_type_trait{ .payload = 14 })
//			.add_traits(int_type_trait{ .payload = 15 })
//			.end_type()
//			.end_module()
//			.build();
//
//		auto traits = (*reg.types().begin()).traits();
//		is_eq(traits.size(), 16ull);
//		int idx = 0;
//		for (const auto& v : traits)
//		{
//			is_eq(v.as_constant<int_type_trait>()->payload, idx);
//			++idx;
//		}
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void interleaved_type_traits_in_one_module()
//	{
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_type<type_1>("ta").add_traits(int_type_trait{ .payload = 1 }).end_type()
//			.begin_type<type_2>("tb").add_traits(int_type_trait{ .payload = 2 }).end_type()
//			.end_module()
//			.build();
//
//		auto types = reg.types();
//		auto it = types.begin();
//		auto a = *it;
//		++it;
//		auto b = *it;
//		is_eq(a.traits().size(), 1ull);
//		is_eq(b.traits().size(), 1ull);
//		is_eq((*a.traits().begin()).as_constant<int_type_trait>()->payload, 1);
//		is_eq((*b.traits().begin()).as_constant<int_type_trait>()->payload, 2);
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void traits_after_many_funcs_alloc_still_pointer_stable()
//	{
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_func<&free_subtract>("a").end_func()
//			.begin_func<&free_square>("b").end_func()
//			.begin_type<type_1>("t")
//			.add_traits(int_type_trait{ .payload = 100 })
//			.add_traits(int_type_trait{ .payload = 200 })
//			.end_type()
//			.begin_func<&free_return_42>("c").end_func()
//			.end_module()
//			.build();
//
//		auto t = *reg.types().begin();
//		is_eq(t.traits().size(), 2ull);
//		auto it = t.traits().begin();
//		is_eq((*it).as_constant<int_type_trait>()->payload, 100);
//		++it;
//		is_eq((*it).as_constant<int_type_trait>()->payload, 200);
//	}
//}
//
//// =========================================================================
//// trait_hook_tests — on_apply / post_build hook ordering and dispatch
//// =========================================================================
//
//namespace trait_hook_tests
//{
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void on_apply_called_for_type_trait()
//	{
//		hook_record rec;
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_type<type_1>("t").add_traits(hooked_trait{ &rec }).end_type()
//			.end_module()
//			.build();
//		is_eq(rec.on_apply_type, 1);
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void on_apply_called_for_func_trait()
//	{
//		hook_record rec;
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_func<&free_noop>("f").add_traits(hooked_trait{ &rec }).end_func()
//			.end_module()
//			.build();
//		is_eq(rec.on_apply_func, 1);
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void on_apply_called_for_data_trait()
//	{
//		hook_record rec;
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_type<int>("int").end_type()
//			.begin_type<fpoint>("fpoint").end_type()
//			.begin_type<entity>("entity")
//			.begin_data<&entity::hp>("hp").add_traits(hooked_trait{ &rec }).end_data()
//			.end_type()
//			.end_module()
//			.build();
//		is_eq(rec.on_apply_data, 1);
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void on_apply_runs_before_any_post_build()
//	{
//		hook_record rec;
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_type<type_1>("t1").add_traits(hooked_trait{ &rec }).end_type()
//			.begin_type<type_2>("t2").add_traits(hooked_trait{ &rec }).end_type()
//			.end_module()
//			.build();
//
//		is_eq(rec.post_build_count_at_first_apply, 0);
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void all_on_apply_run_before_any_post_build()
//	{
//		hook_record rec;
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_type<type_1>("t1").add_traits(hooked_trait{ &rec }).end_type()
//			.begin_type<type_2>("t2").add_traits(hooked_trait{ &rec }).end_type()
//			.end_module()
//			.build();
//
//		is_eq(rec.on_apply_count_at_first_post_build, 2);
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void post_build_called_once_per_trait_for_type()
//	{
//		hook_record rec;
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_type<type_1>("t")
//			.add_traits(hooked_trait{ &rec })
//			.add_traits(hooked_trait{ &rec })
//			.add_traits(hooked_trait{ &rec })
//			.end_type()
//			.end_module()
//			.build();
//		is_eq(rec.post_build_type, 3);
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void post_build_handle_for_type_has_correct_name()
//	{
//		hook_record rec;
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_type<type_1>("particular_name")
//			.add_traits(hooked_trait{ &rec })
//			.end_type()
//			.end_module()
//			.build();
//		is_eq(rec.last_type_name, std::string{ "particular_name" });
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void post_build_handle_for_func_has_correct_name()
//	{
//		hook_record rec;
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_func<&free_noop>("specific_func")
//			.add_traits(hooked_trait{ &rec })
//			.end_func()
//			.end_module()
//			.build();
//		is_eq(rec.last_func_name, std::string{ "specific_func" });
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void post_build_handle_for_data_has_correct_name_and_outer_type()
//	{
//		hook_record rec;
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_type<int>("int").end_type()
//			.begin_type<fpoint>("fpoint").end_type()
//			.begin_type<entity>("entity_type")
//			.begin_data<&entity::hp>("specific_data")
//			.add_traits(hooked_trait{ &rec })
//			.end_data()
//			.end_type()
//			.end_module()
//			.build();
//		is_eq(rec.last_data_name, std::string{ "specific_data" });
//		is_eq(rec.last_data_outer_name, std::string{ "entity_type" });
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void on_apply_dispatches_to_correct_overload_per_target()
//	{
//		hook_record rec;
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_type<type_1>("t").add_traits(hooked_trait{ &rec }).end_type()
//			.begin_func<&free_noop>("f").add_traits(hooked_trait{ &rec }).end_func()
//			.begin_type<int>("int").end_type()
//			.begin_type<fpoint>("fpoint").end_type()
//			.begin_type<entity>("e")
//			.begin_data<&entity::hp>("hp").add_traits(hooked_trait{ &rec }).end_data()
//			.end_type()
//			.end_module()
//			.build();
//		is_eq(rec.on_apply_type, 1);
//		is_eq(rec.on_apply_func, 1);
//		is_eq(rec.on_apply_data, 1);
//		is_eq(rec.post_build_type, 1);
//		is_eq(rec.post_build_func, 1);
//		is_eq(rec.post_build_data, 1);
//	}
//}
//
//// =========================================================================
//// data_tests — data builder, accessors, getter/setter, datas() ranges
//// =========================================================================
//
//namespace data_tests
//{
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void building_data_in_type_registers_one_member()
//	{
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_type<int>("int").end_type()
//			.begin_type<fpoint>("fpoint").end_type()
//			.begin_type<entity>("entity")
//			.begin_data<&entity::hp>("hp").end_data()
//			.end_type()
//			.end_module()
//			.build();
//
//		auto types = reg.types();
//		auto it = types.begin();
//		++it; // skip int
//		++it; // skip fpoint
//		auto t = *it;
//		is_eq(t.datas().size(), 1ull);
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void data_get_name()
//	{
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_type<int>("int").end_type()
//			.begin_type<fpoint>("fpoint").end_type()
//			.begin_type<entity>("entity")
//			.begin_data<&entity::hp>("hp").end_data()
//			.end_type()
//			.end_module()
//			.build();
//
//		auto types = reg.types();
//		auto it = types.begin();
//		++it; ++it;
//		auto d = *(*it).datas().begin();
//		is_eq(d.get_name(), "hp");
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void data_multiple_members_count_and_names_in_order()
//	{
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_type<int>("int").end_type()
//			.begin_type<fpoint>("fpoint").end_type()
//			.begin_type<entity>("entity")
//			.begin_data<&entity::hp>("hp").end_data()
//			.begin_data<&entity::mp>("mp").end_data()
//			.begin_data<&entity::pos>("pos").end_data()
//			.end_type()
//			.end_module()
//			.build();
//
//		auto types = reg.types();
//		auto it = types.begin();
//		++it; ++it;
//		auto datas = (*it).datas();
//		is_eq(datas.size(), 3ull);
//
//		std::vector<std::string_view> names;
//		for (auto d : datas) names.push_back(d.get_name());
//		is_eq(names[0], "hp");
//		is_eq(names[1], "mp");
//		is_eq(names[2], "pos");
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void data_get_outer_type_id_and_name()
//	{
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_type<int>("int").end_type()
//			.begin_type<fpoint>("fpoint").end_type()
//			.begin_type<entity>("entity")
//			.begin_data<&entity::hp>("hp").end_data()
//			.end_type()
//			.end_module()
//			.build();
//
//		auto types = reg.types();
//		auto it = types.begin();
//		++it; ++it;
//		auto d = *(*it).datas().begin();
//		is_eq(d.get_outer_type().get_id(), ge::refl::make_type_id<entity>());
//		is_eq(d.get_outer_type().get_name(), "entity");
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void data_get_type_int_member()
//	{
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_type<int>("int").end_type()
//			.begin_type<fpoint>("fpoint").end_type()
//			.begin_type<entity>("entity")
//			.begin_data<&entity::hp>("hp").end_data()
//			.end_type()
//			.end_module()
//			.build();
//
//		auto types = reg.types();
//		auto it = types.begin();
//		++it; ++it;
//		auto d = *(*it).datas().begin();
//		is_eq(d.get_type().get_id(), ge::refl::make_type_id<int>());
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void data_get_type_struct_member()
//	{
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_type<int>("int").end_type()
//			.begin_type<fpoint>("fpoint").end_type()
//			.begin_type<entity>("entity")
//			.begin_data<&entity::pos>("pos").end_data()
//			.end_type()
//			.end_module()
//			.build();
//
//		auto types = reg.types();
//		auto it = types.begin();
//		++it; ++it;
//		auto d = *(*it).datas().begin();
//		is_eq(d.get_type().get_id(), ge::refl::make_type_id<fpoint>());
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void type_with_no_data_has_empty_datas()
//	{
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_type<type_1>("t").end_type()
//			.end_module()
//			.build();
//
//		is_eq((*reg.types().begin()).datas().size(), 0ull);
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void data_independent_per_type()
//	{
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_type<int>("int").end_type()
//			.begin_type<fpoint>("fpoint").end_type()
//			.begin_type<entity>("entity")
//			.begin_data<&entity::hp>("hp").end_data()
//			.begin_data<&entity::mp>("mp").end_data()
//			.end_type()
//			.begin_type<type_1>("other").end_type()
//			.end_module()
//			.build();
//
//		auto types = reg.types();
//		auto it = types.begin();
//		++it; ++it;
//		auto entity_type = *it;
//		++it;
//		auto other_type = *it;
//		is_eq(entity_type.datas().size(), 2ull);
//		is_eq(other_type.datas().size(), 0ull);
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void data_handles_in_iteration_share_outer_but_have_distinct_names()
//	{
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_type<int>("int").end_type()
//			.begin_type<fpoint>("fpoint").end_type()
//			.begin_type<entity>("entity")
//			.begin_data<&entity::hp>("hp").end_data()
//			.begin_data<&entity::mp>("mp").end_data()
//			.end_type()
//			.end_module()
//			.build();
//
//		auto types = reg.types();
//		auto it = types.begin();
//		++it; ++it;
//		auto datas = (*it).datas();
//		auto dit = datas.begin();
//		auto a = *dit; ++dit;
//		auto b = *dit;
//		is_eq(a.get_name(), "hp");
//		is_eq(b.get_name(), "mp");
//		is_eq(a.get_outer_type().get_id(), b.get_outer_type().get_id());
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void default_getter_present_when_not_overridden()
//	{
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_type<int>("int").end_type()
//			.begin_type<fpoint>("fpoint").end_type()
//			.begin_type<entity>("entity")
//			.begin_data<&entity::hp>("hp").end_data()
//			.end_type()
//			.end_module()
//			.build();
//
//		auto types = reg.types();
//		auto it = types.begin();
//		++it; ++it;
//		auto d = *(*it).datas().begin();
//		is_not_null(reinterpret_cast<void*>(d.get_getter()));
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void default_setter_present_when_not_overridden()
//	{
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_type<int>("int").end_type()
//			.begin_type<fpoint>("fpoint").end_type()
//			.begin_type<entity>("entity")
//			.begin_data<&entity::hp>("hp").end_data()
//			.end_type()
//			.end_module()
//			.build();
//
//		auto types = reg.types();
//		auto it = types.begin();
//		++it; ++it;
//		auto d = *(*it).datas().begin();
//		is_not_null(reinterpret_cast<void*>(d.get_setter()));
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void default_getter_reads_int_member()
//	{
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_type<int>("int").end_type()
//			.begin_type<fpoint>("fpoint").end_type()
//			.begin_type<entity>("entity")
//			.begin_data<&entity::hp>("hp").end_data()
//			.end_type()
//			.end_module()
//			.build();
//
//		auto types = reg.types();
//		auto it = types.begin();
//		++it; ++it;
//		auto d = *(*it).datas().begin();
//		auto getter = d.get_getter();
//
//		entity e{ 42, 0, fpoint{ 0, 0 } };
//		ge::refl::value result = getter(ge::refl::value::create_view(e));
//		is_eq(result.get_type_id(), ge::refl::make_type_id<int>());
//		is_eq(*result.as_constant<int>(), 42);
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void default_setter_writes_int_member_only()
//	{
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_type<int>("int").end_type()
//			.begin_type<fpoint>("fpoint").end_type()
//			.begin_type<entity>("entity")
//			.begin_data<&entity::hp>("hp").end_data()
//			.end_type()
//			.end_module()
//			.build();
//
//		auto types = reg.types();
//		auto it = types.begin();
//		++it; ++it;
//		auto d = *(*it).datas().begin();
//		auto setter = d.get_setter();
//
//		entity e{ 1, 2, fpoint{ 3, 4 } };
//		setter(ge::refl::value::create_ref(e), ge::refl::value::create_owning(99));
//		is_eq(e.hp, 99);
//		is_eq(e.mp, 2);
//		is_eq(e.pos.x, 3);
//		is_eq(e.pos.y, 4);
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void default_getter_setter_round_trip_int()
//	{
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_type<int>("int").end_type()
//			.begin_type<fpoint>("fpoint").end_type()
//			.begin_type<entity>("entity")
//			.begin_data<&entity::mp>("mp").end_data()
//			.end_type()
//			.end_module()
//			.build();
//
//		auto types = reg.types();
//		auto it = types.begin();
//		++it; ++it;
//		auto d = *(*it).datas().begin();
//		auto getter = d.get_getter();
//		auto setter = d.get_setter();
//
//		entity e{ 0, 0, fpoint{ 0, 0 } };
//		setter(ge::refl::value::create_ref(e), ge::refl::value::create_owning(123));
//		is_eq(*getter(ge::refl::value::create_view(e)).as_constant<int>(), 123);
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void default_getter_struct_member()
//	{
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_type<int>("int").end_type()
//			.begin_type<fpoint>("fpoint").end_type()
//			.begin_type<entity>("entity")
//			.begin_data<&entity::pos>("pos").end_data()
//			.end_type()
//			.end_module()
//			.build();
//
//		auto types = reg.types();
//		auto it = types.begin();
//		++it; ++it;
//		auto d = *(*it).datas().begin();
//		auto getter = d.get_getter();
//
//		entity e{ 0, 0, fpoint{ 7, 8 } };
//		ge::refl::value result = getter(ge::refl::value::create_view(e));
//		is_eq(result.get_type_id(), ge::refl::make_type_id<fpoint>());
//		is_eq(*result.as_constant<fpoint>(), (fpoint{ 7, 8 }));
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void default_setter_struct_member()
//	{
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_type<int>("int").end_type()
//			.begin_type<fpoint>("fpoint").end_type()
//			.begin_type<entity>("entity")
//			.begin_data<&entity::pos>("pos").end_data()
//			.end_type()
//			.end_module()
//			.build();
//
//		auto types = reg.types();
//		auto it = types.begin();
//		++it; ++it;
//		auto d = *(*it).datas().begin();
//		auto setter = d.get_setter();
//
//		entity e{ 0, 0, fpoint{ 0, 0 } };
//		setter(ge::refl::value::create_ref(e), ge::refl::value::create_owning(fpoint{ 11, 22 }));
//		is_eq(e.pos.x, 11);
//		is_eq(e.pos.y, 22);
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void default_setter_then_getter_round_trip_struct()
//	{
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_type<int>("int").end_type()
//			.begin_type<fpoint>("fpoint").end_type()
//			.begin_type<entity>("entity")
//			.begin_data<&entity::pos>("pos").end_data()
//			.end_type()
//			.end_module()
//			.build();
//
//		auto types = reg.types();
//		auto it = types.begin();
//		++it; ++it;
//		auto d = *(*it).datas().begin();
//		auto setter = d.get_setter();
//		auto getter = d.get_getter();
//
//		entity e{ 0, 0, fpoint{ 0, 0 } };
//		setter(ge::refl::value::create_ref(e), ge::refl::value::create_owning(fpoint{ 5, 9 }));
//		ge::refl::value got = getter(ge::refl::value::create_view(e));
//		is_eq(*got.as_constant<fpoint>(), (fpoint{ 5, 9 }));
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void default_setter_repeated_calls_are_independent()
//	{
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_type<int>("int").end_type()
//			.begin_type<fpoint>("fpoint").end_type()
//			.begin_type<entity>("entity")
//			.begin_data<&entity::hp>("hp").end_data()
//			.end_type()
//			.end_module()
//			.build();
//
//		auto types = reg.types();
//		auto it = types.begin();
//		++it; ++it;
//		auto d = *(*it).datas().begin();
//		auto setter = d.get_setter();
//
//		entity e{ 0, 0, fpoint{ 0, 0 } };
//		for (int i = 0; i < 5; ++i)
//		{
//			setter(ge::refl::value::create_ref(e), ge::refl::value::create_owning(i * 10));
//			is_eq(e.hp, i * 10);
//		}
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void default_getter_does_aliases_member()
//	{
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_type<int>("int").end_type()
//			.begin_type<fpoint>("fpoint").end_type()
//			.begin_type<entity>("entity")
//			.begin_data<&entity::hp>("hp").end_data()
//			.end_type()
//			.end_module()
//			.build();
//
//		auto types = reg.types();
//		auto it = types.begin();
//		++it; ++it;
//		auto d = *(*it).datas().begin();
//		auto getter = d.get_getter();
//
//		entity e{ 10, 0, fpoint{ 0, 0 } };
//		ge::refl::value got = getter(ge::refl::value::create_view(e));
//		is_eq(*got.as_constant<int>(), 10);
//		e.hp = 200;
//		is_eq(*got.as_constant<int>(), 200);
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void custom_getter_replaces_default()
//	{
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_type<int>("int").end_type()
//			.begin_type<fpoint>("fpoint").end_type()
//			.begin_type<entity>("entity")
//			.begin_data<&entity::hp>("hp").getter<&free_double_hp>().end_data()
//			.end_type()
//			.end_module()
//			.build();
//
//		auto types = reg.types();
//		auto it = types.begin();
//		++it; ++it;
//		auto d = *(*it).datas().begin();
//		auto getter = d.get_getter();
//
//		entity e{ 21, 0, fpoint{ 0, 0 } };
//		ge::refl::value result = getter(ge::refl::value::create_view(e));
//		is_eq(*result.as_constant<int>(), 42);
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void custom_setter_replaces_default()
//	{
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_type<int>("int").end_type()
//			.begin_type<fpoint>("fpoint").end_type()
//			.begin_type<entity>("entity")
//			.begin_data<&entity::hp>("hp").setter<&free_clamp_hp>().end_data()
//			.end_type()
//			.end_module()
//			.build();
//
//		auto types = reg.types();
//		auto it = types.begin();
//		++it; ++it;
//		auto d = *(*it).datas().begin();
//		auto setter = d.get_setter();
//
//		entity e{ 0, 0, fpoint{ 0, 0 } };
//		setter(ge::refl::value::create_ref(e), ge::refl::value::create_owning(-5));
//		is_eq(e.hp, 0);
//
//		setter(ge::refl::value::create_ref(e), ge::refl::value::create_owning(5000));
//		is_eq(e.hp, 1000);
//
//		setter(ge::refl::value::create_ref(e), ge::refl::value::create_owning(500));
//		is_eq(e.hp, 500);
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void nullptr_getter_disables_get()
//	{
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_type<int>("int").end_type()
//			.begin_type<fpoint>("fpoint").end_type()
//			.begin_type<entity>("entity")
//			.begin_data<&entity::hp>("hp").getter<nullptr>().end_data()
//			.end_type()
//			.end_module()
//			.build();
//
//		auto types = reg.types();
//		auto it = types.begin();
//		++it; ++it;
//		auto d = *(*it).datas().begin();
//		is_null(reinterpret_cast<void*>(d.get_getter()));
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void nullptr_setter_disables_set()
//	{
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_type<int>("int").end_type()
//			.begin_type<fpoint>("fpoint").end_type()
//			.begin_type<entity>("entity")
//			.begin_data<&entity::hp>("hp").setter<nullptr>().end_data()
//			.end_type()
//			.end_module()
//			.build();
//
//		auto types = reg.types();
//		auto it = types.begin();
//		++it; ++it;
//		auto d = *(*it).datas().begin();
//		is_null(reinterpret_cast<void*>(d.get_setter()));
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void custom_getter_const_ref_return_signature()
//	{
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_type<int>("int").end_type()
//			.begin_type<fpoint>("fpoint").end_type()
//			.begin_type<entity>("entity")
//			.begin_data<&entity::hp>("hp").getter<&free_get_hp_cref>().end_data()
//			.end_type()
//			.end_module()
//			.build();
//
//		auto types = reg.types();
//		auto it = types.begin();
//		++it; ++it;
//		auto d = *(*it).datas().begin();
//		auto getter = d.get_getter();
//
//		entity e{ 7, 0, fpoint{ 0, 0 } };
//		ge::refl::value v = getter(ge::refl::value::create_view(e));
//		is_eq(v.get_type_id(), ge::refl::make_type_id<int>());
//		is_eq(*v.as_constant<int>(), 7);
//		is_false(v.is_mutable());
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void custom_getter_value_return_signature()
//	{
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_type<int>("int").end_type()
//			.begin_type<fpoint>("fpoint").end_type()
//			.begin_type<entity>("entity")
//			.begin_data<&entity::hp>("hp").getter<&free_get_hp_by_value>().end_data()
//			.end_type()
//			.end_module()
//			.build();
//
//		auto types = reg.types();
//		auto it = types.begin();
//		++it; ++it;
//		auto d = *(*it).datas().begin();
//		auto getter = d.get_getter();
//
//		entity e{ 17, 0, fpoint{ 0, 0 } };
//		ge::refl::value v = getter(ge::refl::value::create_view(e));
//		is_eq(*v.as_constant<int>(), 17);
//		is_true(v.is_owning());
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void custom_setter_then_default_getter_round_trip()
//	{
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_type<int>("int").end_type()
//			.begin_type<fpoint>("fpoint").end_type()
//			.begin_type<entity>("entity")
//			.begin_data<&entity::hp>("hp").setter<&free_set_hp>().end_data()
//			.end_type()
//			.end_module()
//			.build();
//
//		auto types = reg.types();
//		auto it = types.begin();
//		++it; ++it;
//		auto d = *(*it).datas().begin();
//		auto setter = d.get_setter();
//		auto getter = d.get_getter();
//
//		entity e{ 0, 0, fpoint{ 0, 0 } };
//		setter(ge::refl::value::create_ref(e), ge::refl::value::create_owning(73));
//		is_eq(*getter(ge::refl::value::create_view(e)).as_constant<int>(), 73);
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void data_with_one_trait_count()
//	{
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_type<int>("int").end_type()
//			.begin_type<fpoint>("fpoint").end_type()
//			.begin_type<entity>("entity")
//			.begin_data<&entity::hp>("hp")
//			.add_traits(empty_data_trait{})
//			.end_data()
//			.end_type()
//			.end_module()
//			.build();
//
//		auto types = reg.types();
//		auto it = types.begin();
//		++it; ++it;
//		auto d = *(*it).datas().begin();
//		is_eq(d.traits().size(), 1ull);
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void data_traits_state_round_trip()
//	{
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_type<int>("int").end_type()
//			.begin_type<fpoint>("fpoint").end_type()
//			.begin_type<entity>("entity")
//			.begin_data<&entity::hp>("hp")
//			.add_traits(int_data_trait{ .payload = 88 })
//			.end_data()
//			.end_type()
//			.end_module()
//			.build();
//
//		auto types = reg.types();
//		auto it = types.begin();
//		++it; ++it;
//		auto d = *(*it).datas().begin();
//		is_eq((*d.traits().begin()).as_constant<int_data_trait>()->payload, 88);
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void data_traits_multiple_order_preserved()
//	{
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_type<int>("int").end_type()
//			.begin_type<fpoint>("fpoint").end_type()
//			.begin_type<entity>("entity")
//			.begin_data<&entity::hp>("hp")
//			.add_traits(int_data_trait{ .payload = 1 })
//			.add_traits(int_data_trait{ .payload = 2 })
//			.add_traits(int_data_trait{ .payload = 3 })
//			.end_data()
//			.end_type()
//			.end_module()
//			.build();
//
//		auto types = reg.types();
//		auto it = types.begin();
//		++it; ++it;
//		auto d = *(*it).datas().begin();
//		std::vector<int> values;
//		for (const auto& v : d.traits())
//		{
//			values.push_back(v.as_constant<int_data_trait>()->payload);
//		}
//		is_eq(values.size(), 3ull);
//		is_eq(values[0], 1);
//		is_eq(values[1], 2);
//		is_eq(values[2], 3);
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void data_traits_isolated_from_outer_type_traits()
//	{
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_type<int>("int").end_type()
//			.begin_type<fpoint>("fpoint").end_type()
//			.begin_type<entity>("entity")
//			.add_traits(int_type_trait{ .payload = 100 })
//			.begin_data<&entity::hp>("hp")
//			.add_traits(int_data_trait{ .payload = 200 })
//			.end_data()
//			.end_type()
//			.end_module()
//			.build();
//
//		auto types = reg.types();
//		auto it = types.begin();
//		++it; ++it;
//		auto t = *it;
//		auto d = *t.datas().begin();
//		is_eq(t.traits().size(), 1ull);
//		is_eq(d.traits().size(), 1ull);
//		is_eq((*t.traits().begin()).as_constant<int_type_trait>()->payload, 100);
//		is_eq((*d.traits().begin()).as_constant<int_data_trait>()->payload, 200);
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void data_traits_isolated_per_data_member()
//	{
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_type<int>("int").end_type()
//			.begin_type<fpoint>("fpoint").end_type()
//			.begin_type<entity>("entity")
//			.begin_data<&entity::hp>("hp").add_traits(int_data_trait{ .payload = 1 }).end_data()
//			.begin_data<&entity::mp>("mp").add_traits(int_data_trait{ .payload = 2 }).add_traits(int_data_trait{ .payload = 3 }).end_data()
//			.end_type()
//			.end_module()
//			.build();
//
//		auto types = reg.types();
//		auto it = types.begin();
//		++it; ++it;
//		auto datas = (*it).datas();
//		auto dit = datas.begin();
//		auto hp = *dit; ++dit;
//		auto mp = *dit;
//		is_eq(hp.traits().size(), 1ull);
//		is_eq(mp.traits().size(), 2ull);
//		is_eq((*hp.traits().begin()).as_constant<int_data_trait>()->payload, 1);
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void data_with_traits_and_custom_getter_chains()
//	{
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_type<int>("int").end_type()
//			.begin_type<fpoint>("fpoint").end_type()
//			.begin_type<entity>("entity")
//			.begin_data<&entity::hp>("hp")
//			.add_traits(int_data_trait{ .payload = 1 })
//			.getter<&free_double_hp>()
//			.add_traits(int_data_trait{ .payload = 2 })
//			.end_data()
//			.end_type()
//			.end_module()
//			.build();
//
//		auto types = reg.types();
//		auto it = types.begin();
//		++it; ++it;
//		auto d = *(*it).datas().begin();
//		is_eq(d.traits().size(), 2ull);
//
//		entity e{ 5, 0, fpoint{ 0, 0 } };
//		ge::refl::value v = d.get_getter()(ge::refl::value::create_view(e));
//		is_eq(*v.as_constant<int>(), 10);
//	}
//
//	// =========================================================================
//	// data_tests — module_handle::datas() and type/module-level aggregation
//	// =========================================================================
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void module_datas_aggregates_across_types_in_module()
//	{
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_type<int>("int").end_type()
//			.begin_type<fpoint>("fpoint").end_type()
//			.begin_type<entity>("entity")
//			.begin_data<&entity::hp>("hp").end_data()
//			.begin_data<&entity::mp>("mp").end_data()
//			.end_type()
//			.begin_type<type_1>("t1").end_type()
//			.end_module()
//			.build();
//
//		auto mod = *reg.modules().begin();
//		is_eq(mod.datas().size(), 2ull);
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void module_datas_zero_when_no_data()
//	{
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_type<type_1>("t1").end_type()
//			.end_module()
//			.build();
//
//		is_eq((*reg.modules().begin()).datas().size(), 0ull);
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void module_datas_zero_for_empty_module()
//	{
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("empty").end_module()
//			.build();
//
//		is_eq((*reg.modules().begin()).datas().size(), 0ull);
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void module_datas_disjoint_across_modules()
//	{
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("a")
//			.begin_type<int>("int").end_type()
//			.begin_type<entity>("entity_a")
//			.begin_data<&entity::hp>("hp").end_data()
//			.end_type()
//			.end_module()
//			.begin_module("b")
//			.begin_type<fpoint>("fpoint")
//			.begin_data<&fpoint::x>("x").end_data()
//			.begin_data<&fpoint::y>("y").end_data()
//			.end_type()
//			.end_module()
//			.build();
//
//		auto modules = reg.modules();
//		auto it = modules.begin();
//		auto a = *it; ++it;
//		auto b = *it;
//		is_eq(a.datas().size(), 1ull);
//		is_eq(b.datas().size(), 2ull);
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void module_datas_iterates_with_correct_names_in_order()
//	{
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_type<int>("int").end_type()
//			.begin_type<fpoint>("fpoint").end_type()
//			.begin_type<entity>("entity")
//			.begin_data<&entity::hp>("hp").end_data()
//			.begin_data<&entity::mp>("mp").end_data()
//			.begin_data<&entity::pos>("pos").end_data()
//			.end_type()
//			.end_module()
//			.build();
//
//		auto mod = *reg.modules().begin();
//		std::vector<std::string_view> names;
//		for (auto d : mod.datas()) names.push_back(d.get_name());
//		is_eq(names.size(), 3ull);
//		is_eq(names[0], "hp");
//		is_eq(names[1], "mp");
//		is_eq(names[2], "pos");
//	}
//
//	REFL_FUNC(ge::test_core::unit_test_trait{})
//	export API void module_datas_outer_types_match_owning_type()
//	{
//		auto reg = ge::refl::builders::begin_registry()
//			.begin_module("m")
//			.begin_type<int>("int").end_type()
//			.begin_type<fpoint>("a")
//			.begin_data<&fpoint::x>("x").end_data()
//			.end_type()
//			.begin_type<entity>("b")
//			.begin_data<&entity::mp>("mp").end_data()
//			.end_type()
//			.end_module()
//			.build();
//
//		auto mod = *reg.modules().begin();
//		std::vector<std::string_view> outer_names;
//		for (auto d : mod.datas()) outer_names.push_back(d.get_outer_type().get_name());
//		is_eq(outer_names.size(), 2ull);
//		is_eq(outer_names[0], "a");
//		is_eq(outer_names[1], "b");
//	}
//}
//
//
