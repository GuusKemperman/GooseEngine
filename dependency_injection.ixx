export module dependency_injection;

import std;

namespace ge
{
	template<typename target_t, typename test_t>
	static constexpr bool has_or_is_dependency();

	template<typename target_t, typename test_t>
	static constexpr target_t& get_or_is_dependency(test_t& a_test) requires (has_or_is_dependency<target_t, test_t>());

	export template<typename... T>
		class depends_on;

	export template <>
		class depends_on<>
	{
	public:
		template<typename... args_t>
		constexpr depends_on(args_t&&...) {}

		bool operator==(const depends_on&) const { return true; }
		bool operator!=(const depends_on&) const { return false; }

		template<typename target_t>
		static constexpr bool has_dependency();

		template<typename target_t>
		constexpr auto get(this auto&&) = delete;
	};

	export template<typename this_t, typename... others_t>
		class depends_on<this_t, others_t...> : public depends_on<others_t...>
	{
		using depends_base_t = depends_on<others_t...>;

	protected:
		using depends_on_constructor = depends_on;

	public:
		depends_on() = delete;

		template<typename... args_t>
		constexpr depends_on(args_t&... args) requires (has_or_is_dependency<this_t, args_t>() || ...);

		~depends_on() = default;

		using depends_base_t::operator==;
		using depends_base_t::operator!=;

		constexpr bool operator==(const depends_on& other) const;
		constexpr bool operator!=(const depends_on& other) const;

		template<typename target_t>
		static constexpr bool has_dependency();

		template<typename target_t>
		constexpr target_t& get() requires (has_dependency<target_t>());

		template<typename target_t>
		constexpr target_t& get() const requires (std::is_const_v<target_t>&& has_dependency<target_t>());

	private:
		std::reference_wrapper<this_t> m_ref;
	};
}

template <typename target_t, typename test_t>
constexpr bool ge::has_or_is_dependency()
{
	if constexpr (std::convertible_to<test_t&, target_t&>)
	{
		return true;
	}
	else if constexpr (
		(!std::is_const_v<test_t> || std::is_const_v<target_t>)
		&&
		requires
	{
		{ test_t::template has_dependency<target_t>() } -> std::same_as<bool>;
	})
	{
		if constexpr (test_t::template has_dependency<target_t>())
		{
			return true;
		}
	}
	return false;
}

template <typename target_t, typename test_t>
constexpr target_t& ge::get_or_is_dependency(test_t& a_test) requires (has_or_is_dependency<target_t, test_t>())
{
	if constexpr (std::convertible_to<test_t&, target_t&>)
	{
		return a_test;
	}
	else
	{
		return test_t::template get<target_t>();
	}
}

template <typename this_t, typename ... others_t>
template <typename ... args_t>
constexpr ge::depends_on<this_t, others_t...>::depends_on(args_t&... args) requires (has_or_is_dependency<this_t, args_t>() || ...) :
	depends_base_t(args...),
	m_ref(
		[&]() -> decltype(auto)
		{
			this_t* ptr{};

			auto lambda = [&]<typename arg_t>(arg_t& arg)
			{
				if constexpr (has_or_is_dependency<this_t, arg_t>())
				{
					ptr = &get_or_is_dependency<this_t>(arg);
				}
			};

			(lambda.template operator()<args_t>(args), ...);

			return *ptr;
		}())
{
}

template <typename this_t, typename... others_t>
constexpr bool ge::depends_on<this_t, others_t...>::operator==(const depends_on& other) const
{
	return &m_ref.get() == &other.m_ref.get()
		&& depends_base_t::operator==(static_cast<const depends_base_t&>(other));
}

template <typename this_t, typename... others_t>
constexpr bool ge::depends_on<this_t, others_t...>::operator!=(const depends_on& other) const
{
	return &m_ref.get() != &other.m_ref.get()
		|| depends_base_t::operator!=(static_cast<const depends_base_t&>(other));
}

template <typename target_t>
constexpr bool ge::depends_on<>::has_dependency()
{
	return false;
}

template<typename this_t, typename ...others_t>
template<typename target_t>
constexpr bool ge::depends_on<this_t, others_t...>::has_dependency()
{
	if (has_or_is_dependency<target_t, this_t>())
	{
		return true;
	}

	return depends_base_t::template has_dependency<target_t>();
}

template <typename this_t, typename ... others_t>
template <typename target_t>
constexpr target_t& ge::depends_on<this_t, others_t...>::get() requires (has_dependency<target_t>())
{
	if constexpr (has_or_is_dependency<target_t, this_t>())
	{
		return get_or_is_dependency<target_t>(m_ref);
	}
	else
	{
		return depends_base_t::template get<target_t>();
	}
}

template <typename this_t, typename ... others_t>
template <typename target_t>
constexpr target_t& ge::depends_on<this_t, others_t...>::get() const requires (std::is_const_v<target_t>&&
	has_dependency<target_t>())
{
	return const_cast<depends_on&>(*this).get<target_t>();
}

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




	// 
	static_assert(!std::is_constructible_v<depend_test>);
	static_assert(std::is_constructible_v<depend_test, depend_test&>);
	static_assert(std::is_constructible_v<depend_test, const depend_test&>);



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
