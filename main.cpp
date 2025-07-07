#include <cassert>
import engine;
import file_io;
import logger;
import dependency_injection;

namespace // static tests
{
	struct dependency1
	{

	};

	struct dependency2 : ge::depends_on<dependency1>
	{

	};

	struct dependency3
	{

	};

	struct depend_test : ge::depends_on<const dependency2, dependency3>
	{
		using depends_on_constructor::depends_on_constructor;
	};

	// Any const object will not return any of its mutable dependencies.
	static_assert(requires (const depend_test& d)
	{
		{ d.get<const dependency3>() } -> std::same_as<const dependency3&>;
	});
	static_assert(requires (depend_test& d)
	{
		{ d.get<const dependency3>() } -> std::same_as<const dependency3&>;
	});
	// Does not compile -> no valid candidates for get.
	//static_assert(!requires (const depend_test& d)
	//{
	//	{ d.get<dependency3>() } -> std::same_as<dependency3&>;
	//});
	static_assert(requires (depend_test& d)
	{
		{ d.get<dependency3>() } -> std::same_as<dependency3&>;
	});

	// Any const dependency can only be accessed through a const reference
	static_assert(!depend_test::has_dependency<dependency2>());
	static_assert(depend_test::has_dependency<const dependency2>());
	static_assert(!depend_test::has_dependency<dependency1>());
	static_assert(depend_test::has_dependency<const dependency1>());

	// Constructors can look up dependencies
	static_assert(std::is_constructible_v<dependency2, dependency1&>);
	static_assert(!std::is_constructible_v<depend_test>);
	static_assert(std::is_constructible_v<depend_test, dependency2&, dependency3&>);
	static_assert(std::is_constructible_v<depend_test, const dependency2&, dependency3&>);
	static_assert(std::is_constructible_v<depend_test, const dependency2&, dependency3&>);
	static_assert(!std::is_constructible_v<ge::depends_on<dependency1>, const depend_test&>);
	static_assert(std::is_constructible_v<ge::depends_on<const dependency1>, const depend_test&>);
	static_assert(!std::is_constructible_v<ge::depends_on<dependency1>, const depend_test&>);
	static_assert(!std::is_constructible_v<ge::depends_on<dependency1>, const dependency2&>);

	static_assert(depend_test::has_dependency<const dependency1>());
	static_assert(dependency2::has_dependency<dependency1>());
	static_assert(dependency2::has_dependency<const dependency1>());

	static_assert(depend_test::has_dependency<const dependency2>());
	static_assert(depend_test::has_dependency<dependency3>());
	static_assert(!dependency2::has_dependency<dependency3>());

	static_assert(std::is_same_v<decltype(std::declval<dependency2>().get<dependency1>()), dependency1&>);
	static_assert(std::is_same_v<decltype(std::declval<depend_test>().get<const dependency1>()), const dependency1&>);
	static_assert(std::is_same_v<decltype(std::declval<depend_test>().get<const dependency2>()), const dependency2&>);
	static_assert(std::is_same_v<decltype(std::declval<depend_test>().get<dependency3>()), dependency3&>);
}

int main()
{
	dependency1 d1{};
	dependency2 d2{ d1 };
	dependency3 d3{};

	depend_test t{ d2, d3 };

	//assert(&t.get<dependency1>() == &d1);
	assert(&t.get<const dependency2>() == &d2);

	t.get<dependency3>();

	//ge::engine engine{ std::make_shared<ge::file_io>(), std::make_shared<test_logger>() };
	//engine.run();
}

