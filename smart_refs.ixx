export module utils:memory;
export import std;

namespace ge
{
	export template<typename T>
	using shared_ptr = std::shared_ptr<T>;

	export template<typename T>
	using default_delete = std::default_delete<T>;

	export template<typename T, typename deleter_t = default_delete<T>>
	using unique_ptr = std::unique_ptr<T, deleter_t>;

	export template<typename T>
	class shared_ref;

	export template<typename T, typename deleter_t = default_delete<T>>
	class unique_ref;

	template<typename T>
	concept NonNullPtrT = !std::is_same_v<T, std::nullptr_t>;

	template<typename ptr_t>
	class ref_base
	{
	public:
		using ptr_type = ptr_t;
		using element_type = typename ptr_type::element_type;

		ref_base() = delete;

		template<NonNullPtrT... arg_t>
		constexpr ref_base(arg_t&&... a_arg) requires (std::is_constructible_v<ptr_t, arg_t...>);

		template<typename other_ptr_t>
		constexpr ref_base(const ref_base<other_ptr_t>& other) requires(std::is_constructible_v<ptr_t, const other_ptr_t&>);

		template<typename other_ptr_t>
		constexpr ref_base(ref_base<other_ptr_t>&& other) noexcept requires(std::is_constructible_v<ptr_t, other_ptr_t&&>);

		template<typename other_ptr_t>
		constexpr ref_base& operator=(const ref_base<other_ptr_t>& other) requires(std::is_assignable_v<ptr_t, const other_ptr_t&>);

		template<typename other_ptr_t>
		constexpr ref_base& operator=(ref_base<other_ptr_t>&& other) noexcept requires(std::is_assignable_v<ptr_t, other_ptr_t&&>);

		template<typename other_t>
		constexpr operator shared_ref<other_t>() const requires (std::is_convertible_v<const ptr_t&, typename shared_ref<other_t>::ptr_type>);

		constexpr element_type& operator*() const;

		constexpr element_type* operator->() const;

		constexpr operator element_type& () const;

		constexpr element_type& get() const;

		template <typename self_t>
		constexpr auto&& get_ptr(this self_t&& self);

	protected:
		using base_t = ref_base;

		ptr_type m_ptr;
	};

	export template<typename T>
	class shared_ref :
		public ref_base<shared_ptr<T>>
	{
	public:
		using ref_base<shared_ptr<T>>::base_t;
		using ref_base<shared_ptr<T>>::operator=;
	};

	export template<typename T, typename deleter_t>
	class unique_ref :
		public ref_base<unique_ptr<T, deleter_t>>
	{
	public:
		using ref_base<unique_ptr<T>>::base_t;
		using ref_base<unique_ptr<T>>::operator=;
		using deleter_type = deleter_t;
	};

	export template<typename T, typename... args_t>
	shared_ptr<T> make_shared_ptr(args_t&&... args) requires (std::is_constructible_v<T, args_t...>);

	export template<typename T, typename... args_t>
	unique_ptr<T> make_unique_ptr(args_t&&... args) requires (std::is_constructible_v<T, args_t...>);

	export template<typename T, typename... args_t>
	shared_ref<T> make_shared_ref(args_t&&... args) requires (std::is_constructible_v<T, args_t...>);

	export template<typename T, typename... args_t>
	unique_ref<T> make_unique_ref(args_t&&... args) requires (std::is_constructible_v<T, args_t...>);

	template <typename T>
	constexpr bool is_shared_ptr_v = false;

	template <typename T>
	constexpr bool is_shared_ptr_v<shared_ptr<T>> = true;

	template <typename T>
	constexpr bool is_unique_ptr_v = false;

	template <typename T, typename D>
	constexpr bool is_unique_ptr_v<unique_ptr<T, D>> = true;

	template <typename T>
	constexpr bool is_shared_ref_v = false;

	template <typename T>
	constexpr bool is_shared_ref_v<shared_ref<T>> = true;

	template <typename T>
	constexpr bool is_unique_ref_v = false;

	template <typename T, typename D>
	constexpr bool is_unique_ref_v<unique_ref<T, D>> = true;

	export template<typename T>
	concept SharedPtr = is_shared_ptr_v<std::remove_cvref_t<T>>;

	export template<typename T>
	concept UniquePtr = is_unique_ptr_v<std::remove_cvref_t<T>>;

	export template<typename T>
	concept SharedRef = is_shared_ref_v<std::remove_cvref_t<T>>;

	export template<typename T>
	concept UniqueRef = is_unique_ref_v<std::remove_cvref_t<T>>;
}

template <typename ptr_t>
template <ge::NonNullPtrT ... arg_t>
constexpr ge::ref_base<ptr_t>::ref_base(arg_t&&... a_arg) requires (std::is_constructible_v<ptr_t, arg_t...>) :
	m_ptr(std::forward<arg_t>(a_arg)...)
{
	if (m_ptr == nullptr)
	{
		throw std::invalid_argument("Null pointer passed to reference constructor");
	}
}

template <typename ptr_t>
template < typename other_ptr_t>
constexpr ge::ref_base<ptr_t>::ref_base(const ref_base<other_ptr_t>& other) requires (std::
	is_constructible_v<ptr_t, const other_ptr_t&>) :
	m_ptr(other.get_ptr())
{
}

template <typename ptr_t>
template < typename other_ptr_t>
constexpr ge::ref_base<ptr_t>::ref_base(ref_base<other_ptr_t>&& other) noexcept requires (
	std::is_constructible_v<ptr_t, other_ptr_t&&>) :
	m_ptr(std::move(other).get_ptr())
{
}

template <typename ptr_t>
template < typename other_ptr_t>
constexpr ge::ref_base<ptr_t>& ge::ref_base<ptr_t>::operator=(
	const ref_base<other_ptr_t>& other) requires (std::is_assignable_v<ptr_t, const other_ptr_t&>)
{
	m_ptr = other.get_ptr();
	return *this;
}

template <typename ptr_t>
template < typename other_ptr_t>
constexpr ge::ref_base<ptr_t>& ge::ref_base<ptr_t>::operator=(
	ref_base<other_ptr_t>&& other) noexcept requires (std::is_assignable_v<ptr_t, other_ptr_t&&>)
{
	m_ptr = std::move(other).get_ptr();
	return *this;
}

template <typename ptr_t>
template <typename other_t>
constexpr ge::ref_base<ptr_t>::operator ge::shared_ref<other_t>() const requires (std::is_convertible_v<const
	ptr_t&, typename shared_ref<other_t>::ptr_type>)
{
	return { *this };
}

template <typename ptr_t>
constexpr typename ge::ref_base<ptr_t>::element_type& ge::ref_base<ptr_t>::operator*() const
{
	return *get();
}

template <typename ptr_t>
constexpr typename ge::ref_base<ptr_t>::element_type* ge::ref_base<ptr_t>::operator->() const
{
	return get();
}

template <typename ptr_t>
constexpr ge::ref_base<ptr_t>::operator typename ptr_t::element_type&() const
{
	return *m_ptr;
}

template <typename ptr_t>
constexpr typename ge::ref_base<ptr_t>::element_type& ge::ref_base<ptr_t>::get() const
{
	return *m_ptr;
}

template <typename ptr_t>
template <typename self_t>
constexpr auto&& ge::ref_base<ptr_t>::get_ptr(this self_t&& self)
{
	return std::forward<self_t>(self).m_ptr;
}

template <typename T, typename ... args_t>
ge::shared_ptr<T> ge::make_shared_ptr(args_t&&... args) requires (std::is_constructible_v<T, args_t...>)
{
	return std::make_shared<T>(std::forward<args_t>(args)...);
}

template <typename T, typename ... args_t>
ge::unique_ptr<T> ge::make_unique_ptr(args_t&&... args) requires (std::is_constructible_v<T, args_t...>)
{
	return std::make_unique<T>(std::forward<args_t>(args)...);
}

template <typename T, typename ... args_t>
ge::shared_ref<T> ge::make_shared_ref(args_t&&... args) requires (std::is_constructible_v<T, args_t...>)
{
	return ge::shared_ref<T>{ make_shared_ptr<T>(std::forward<args_t>(args)...) };
}

template <typename T, typename ... args_t>
ge::unique_ref<T> ge::make_unique_ref(args_t&&... args) requires (std::is_constructible_v<T, args_t...>)
{
	return ge::unique_ref<T>{ make_unique_ptr<T>(std::forward<args_t>(args)...) };
}

// =================================================================
// Compile-time unit tests
// =================================================================

[[maybe_unused]] static void foo()
{
	{
		ge::unique_ref<int> p = ge::make_unique_ref<int>(5);
		ge::shared_ref<int> f{ std::move(p) };
		ge::shared_ref<const int> g{ f };
		ge::shared_ref<const int> g3 = g;
		g3 = f;
		ge::shared_ref<const int> g2 = f;
	}

	{
		ge::unique_ptr<int> p = ge::make_unique_ptr<int>(5);
		ge::shared_ptr<int> f{ std::move(p) };
		ge::shared_ptr<const int> g{ f };
		ge::shared_ptr<const int> g2 = f;
		ge::shared_ptr<const int> g3 = g;
		g2 = f;
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


static_assert(std::convertible_to<ge::shared_ref<int>, ge::shared_ref<const int>>);
