export module dependency_injection;

import std;

namespace ge
{
	export template<typename... T>
	class depends_on;

	export template <>
	class depends_on<>
	{
	public:
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
		depends_on(this_t& a_this, others_t&... a_others);

		using depends_base_t::operator==;
		using depends_base_t::operator!=;

		bool operator==(const depends_on& other) const;
		bool operator!=(const depends_on& other) const;

		template<typename target_t>
		static constexpr bool has_dependency();

		template<typename target_t> 
		constexpr target_t& get() requires (has_dependency<target_t>());

	private:
		std::reference_wrapper<this_t> m_ref;
	};
}

template<typename this_t, typename... others_t>
ge::depends_on<this_t, others_t...>::depends_on(this_t& a_this, others_t&... a_others) :
	depends_base_t(a_others...),
	m_ref(a_this)
{
}

template <typename this_t, typename... others_t>
bool ge::depends_on<this_t, others_t...>::operator==(const depends_on& other) const
{
	return &m_ref.get() == &other.m_ref.get()
		&& depends_base_t::operator==(static_cast<const depends_base_t&>(other));
}

template <typename this_t, typename... others_t>
bool ge::depends_on<this_t, others_t...>::operator!=(const depends_on& other) const
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
	if constexpr (std::convertible_to<this_t&, target_t&>)
	{
		return true;
	}
	else if constexpr (
		(!std::is_const_v<this_t> || std::is_const_v<target_t>)
		&&
		requires
	{
		{ this_t::template has_dependency<target_t>() } -> std::same_as<bool>;
	})
	{
		if constexpr (this_t::template has_dependency<target_t>())
		{
			return true;
		}
	}
		


	return depends_base_t::template has_dependency<target_t>();
}

template <typename this_t, typename ... others_t>
template <typename target_t>
constexpr target_t& ge::depends_on<this_t, others_t...>::get() requires (has_dependency<target_t>())
{
	if constexpr (std::convertible_to<this_t&, target_t&>)
	{
		return m_ref.get();
	}
	else
	{
		if constexpr (depends_base_t::template has_dependency<target_t>())
		{
			return depends_base_t::template get<target_t>();
		}
		
		if constexpr (requires
		{
			{ this_t::template has_dependency<target_t>() } -> std::same_as<bool>;
		})
		{
			if constexpr (this_t::template has_dependency<target_t>())
			{
				return this_t::template get<target_t>();
			}
		}
	}
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

	// Any const dependency can only be accessed through a const reference
	static_assert(!depend_test::has_dependency<dependency2>());
	static_assert(depend_test::has_dependency<const dependency2>());
	static_assert(!depend_test::has_dependency<dependency1>());
	static_assert(depend_test::has_dependency<const dependency1>());



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