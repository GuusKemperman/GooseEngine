export module test_utils;

import test_core;
import utils;

using namespace ge::test_core;

struct test_struct
{
	int value = 42;
	int get_value() const { return value; }
};

namespace smart_refs
{
	REFL_FUNC(unit_test{})
	export API void ref_base_constructor_nullptr_throwsInvalidArgument()
	{
		std::invalid_argument e = assert::expect_exception<std::invalid_argument>(
			[]
			{
				std::shared_ptr<test_struct> null_ptr{};
				ge::shared_ref<test_struct> ref{ null_ptr };
			});

		assert::is_eq(std::string_view{ "Null pointer passed to reference constructor" }, std::string{ e.what() });
	}

	REFL_FUNC(unit_test{})
	export API void shared_ref_partialDeduction_assignmentFromTypedInstance_deducesCorrectly()
	{
		ge::shared_ref ref = ge::make_shared_ref<test_struct>(99); // Deduction here
		assert::is_eq(ref->get_value(), 99);
	}

	REFL_FUNC(unit_test{})
	export API void shared_ref_conversionFromUniqueRef_thenConstAssigns_maintainsCorrectValue()
	{
		ge::unique_ref<int> unique = ge::make_unique_ref<int>(123);
		ge::shared_ref<int> shared{ std::move(unique) };

		ge::shared_ref<const int> const1{ shared };
		ge::shared_ref<const int> const2 = shared;
		ge::shared_ref<const int> const3 = const1;
		const3 = shared;

		assert::is_eq(*const1, 123);
		assert::is_eq(*const2, 123);
		assert::is_eq(*const3, 123);
	}

	REFL_FUNC(unit_test{})
	export API void shared_ptr_conversionFromUniquePtr_thenConstAssigns_maintainsCorrectValue()
	{
		ge::unique_ptr<int> unique = ge::make_unique_ptr<int>(456);
		ge::shared_ptr<int> shared{ std::move(unique) };

		ge::shared_ptr<const int> const1{ shared };
		ge::shared_ptr<const int> const2 = shared;
		ge::shared_ptr<const int> const3 = const1;
		const2 = shared;

		assert::is_eq(*const1, 456);
		assert::is_eq(*const2, 456);
		assert::is_eq(*const3, 456);
	}

	REFL_FUNC(unit_test{})
	export API void make_shared_ref_validArgs_constructsExpectedSharedRef()
	{
		auto ref = ge::make_shared_ref<test_struct>();
		assert::is_eq(ref->get_value(), 42);
		assert::is_eq((*ref).get_value(), 42);
		assert::is_eq(ref.get().get_value(), 42);
		assert::is_true(ref.use_count() == 1);
	}

	REFL_FUNC(unit_test{})
	export API void make_unique_ref_validArgs_constructsExpectedUniqueRef()
	{
		auto ref = ge::make_unique_ref<test_struct>();
		assert::is_eq(ref->get_value(), 42);
		assert::is_eq((*ref).get_value(), 42);
		assert::is_eq(ref.get().get_value(), 42);
	}

	REFL_FUNC(unit_test{})
	export API void shared_ref_copyConstruct_copiesAndSharesOwnership()
	{
		auto ref1 = ge::make_shared_ref<test_struct>();
		auto ref2 = ref1;
		assert::is_eq(ref1.get().get_value(), ref2.get().get_value());
		assert::is_eq(ref1.use_count(), 2L);
		assert::is_eq(ref2.use_count(), 2L);
	}

	REFL_FUNC(unit_test{})
	export API void shared_ref_moveConstruct_movesOwnership()
	{
		auto ref1 = ge::make_shared_ref<test_struct>();
		long count = ref1.use_count();
		auto ref2 = std::move(ref1);
		assert::is_eq(ref2.use_count(), count);
	}

	REFL_FUNC(unit_test{})
	export API void shared_ref_copyAssign_copiesAndSharesOwnership()
	{
		auto ref1 = ge::make_shared_ref<test_struct>();
		ge::shared_ref<test_struct> ref2 = ge::make_shared_ref<test_struct>();
		ref2 = ref1;
		assert::is_eq(ref1.use_count(), 2L);
		assert::is_eq(ref2.use_count(), 2L);
	}

	REFL_FUNC(unit_test{})
	export API void shared_ref_moveAssign_movesOwnership()
	{
		auto ref1 = ge::make_shared_ref<test_struct>();
		ge::shared_ref<test_struct> ref2 = ge::make_shared_ref<test_struct>();
		ref2 = std::move(ref1);
		assert::is_eq(ref2->get_value(), 42);
	}

	REFL_FUNC(unit_test{})
	export API void unique_ref_moveConstruct_transfersOwnership()
	{
		auto ref1 = ge::make_unique_ref<test_struct>();
		auto val = ref1->get_value();
		auto ref2 = std::move(ref1);
		assert::is_eq(ref2->get_value(), val);
	}

	REFL_FUNC(unit_test{})
	export API void unique_ref_moveAssign_transfersOwnership()
	{
		auto ref1 = ge::make_unique_ref<test_struct>();
		ge::unique_ref<test_struct> ref2 = ge::make_unique_ref<test_struct>();
		ref2 = std::move(ref1);
		assert::is_eq(ref2->get_value(), 42);
	}

	REFL_FUNC(unit_test{})
	export API void ref_base_getPtr_rvalueAndLvalue_yieldsCorrectPointer()
	{
		auto ref = ge::make_shared_ref<test_struct>();
		const auto& ptr = ref.get_ptr();
		assert::is_not_null(ptr.get());

		auto moved_ptr = std::move(ref).get_ptr();
		assert::is_not_null(moved_ptr.get());
	}

	REFL_FUNC(unit_test{})
	export API void ref_base_operatorConversionToElementRef_returnsReference()
	{
		auto ref = ge::make_shared_ref<test_struct>();
		test_struct& s = ref;
		assert::is_eq(s.get_value(), 42);
	}

	REFL_FUNC(unit_test{})
	export API void ref_base_operatorConversionToSharedRef_compatibleTypes_conversionSucceeds()
	{
		auto base = ge::make_shared_ref<test_struct>();
		ge::shared_ref<const test_struct> converted = base;
		assert::is_eq(converted->get_value(), 42);
	}
}


// Concepts
static_assert(ge::SharedPtr<std::shared_ptr<int>>);
static_assert(ge::SharedPtr<const std::shared_ptr<int>>);
static_assert(ge::SharedPtr<volatile std::shared_ptr<int>&>);
static_assert(!ge::SharedPtr<int>);
static_assert(!ge::SharedPtr<std::unique_ptr<int>>);

static_assert(ge::UniquePtr<std::unique_ptr<int>>);
static_assert(ge::UniquePtr<std::unique_ptr<int, ge::default_delete<int>>>);
static_assert(ge::UniquePtr<const std::unique_ptr<int>>);
static_assert(!ge::UniquePtr<int>);
static_assert(!ge::UniquePtr<std::shared_ptr<int>>);

static_assert(ge::SharedRef<ge::shared_ref<int>>);
static_assert(ge::SharedRef<const ge::shared_ref<int>&>);
static_assert(ge::SharedRef<volatile ge::shared_ref<int>&&>);
static_assert(!ge::SharedRef<int>);
static_assert(!ge::SharedRef<ge::unique_ref<int>>);

static_assert(ge::UniqueRef<ge::unique_ref<int>>);
static_assert(ge::UniqueRef<ge::unique_ref<int, ge::default_delete<int>>>);
static_assert(ge::UniqueRef<const ge::unique_ref<int>>);
static_assert(!ge::UniqueRef<int>);
static_assert(!ge::UniqueRef<ge::shared_ref<int>>);

// SharedRef constructibility
static_assert(std::is_constructible_v<ge::shared_ref<const int>, ge::shared_ref<int>>);
static_assert(!std::is_constructible_v<ge::shared_ref<int>, ge::shared_ref<const int>>);
static_assert(!std::is_constructible_v<ge::shared_ref<int>, std::nullptr_t>);
static_assert(std::is_constructible_v<ge::shared_ref<int>, ge::unique_ref<int>>);

// UniqueRef constructibility
static_assert(std::is_constructible_v<ge::unique_ref<const int>, ge::unique_ref<int>>);
static_assert(!std::is_constructible_v<ge::unique_ref<int>, ge::unique_ref<const int>>);
static_assert(!std::is_constructible_v<ge::unique_ref<int>, std::nullptr_t>);
static_assert(!std::is_constructible_v<ge::unique_ref<int>, ge::shared_ref<int>>);

// Default constructibility
static_assert(!std::is_default_constructible_v<ge::shared_ref<int>>);
static_assert(!std::is_default_constructible_v<ge::unique_ref<int>>);

// Copy/Move semantics
static_assert(std::is_copy_constructible_v<ge::shared_ref<int>>);
static_assert(std::is_copy_assignable_v<ge::shared_ref<int>>);
static_assert(std::is_move_constructible_v<ge::shared_ref<int>>);
static_assert(std::is_move_assignable_v<ge::shared_ref<int>>);

static_assert(!std::is_copy_constructible_v<ge::unique_ref<int>>);
static_assert(!std::is_copy_assignable_v<ge::unique_ref<int>>);
static_assert(std::is_move_constructible_v<ge::unique_ref<int>>);
static_assert(std::is_move_assignable_v<ge::unique_ref<int>>);

// Member function return types
static_assert(std::is_same_v<decltype(std::declval<ge::shared_ref<int>>().get()), int&>);
static_assert(std::is_same_v<decltype(std::declval<ge::shared_ref<const int>>().get()), const int&>);
static_assert(std::is_same_v<decltype(std::declval<ge::unique_ref<int>>().get()), int&>);
static_assert(std::is_same_v<decltype(*std::declval<ge::shared_ref<int>>()), int&>);
static_assert(std::is_same_v<decltype(std::declval<ge::unique_ref<int>>().operator->()), int*>);

// Factory function return types
static_assert(std::is_same_v<
	decltype(ge::make_shared_ref<int>(0)),
	ge::shared_ref<int>
>);
static_assert(std::is_same_v<
	decltype(ge::make_unique_ref<int>(0)),
	ge::unique_ref<int>
>);

// Convertible
static_assert(std::convertible_to<ge::shared_ref<int>, ge::shared_ref<const int>>);
