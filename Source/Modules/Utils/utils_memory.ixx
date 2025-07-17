export module utils:memory;
export import std;

namespace ge
{
	export template<typename T>
	using shared_ptr = std::shared_ptr<T>;

	export template <class T>
	struct in_place_delete
	{
		constexpr in_place_delete() noexcept = default;

		template <typename other_t>
		constexpr in_place_delete(const in_place_delete<other_t>&) noexcept requires std::convertible_to<other_t, T> {}

		constexpr void operator()(T* a_ptr) const noexcept;
	};

	export template<typename T>
	using default_delete = in_place_delete<T>;

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

		// Constructs internal ptr from arguments.
		// May throw std::invalid_argument("Null pointer passed to reference constructor") if
		// the resulting ptr is null.
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

		constexpr const ptr_type& get_ptr() const &;

		constexpr ptr_type&& get_ptr() &&;

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

		long use_count() const;
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

template <class T>
constexpr void ge::in_place_delete<T>::operator()(T* a_ptr) const noexcept
{
	static_assert(0 < sizeof(T), "can't delete an incomplete type");

	if (a_ptr == nullptr)
	{
		return;
	}

	a_ptr->~T();
	delete a_ptr;
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
	return get();
}

template <typename ptr_t>
constexpr typename ge::ref_base<ptr_t>::element_type* ge::ref_base<ptr_t>::operator->() const
{
	return &get();
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
constexpr const typename ge::ref_base<ptr_t>::ptr_type& ge::ref_base<ptr_t>::get_ptr() const &
{
	return m_ptr;
}

template <typename ptr_t>
constexpr typename ge::ref_base<ptr_t>::ptr_type&& ge::ref_base<ptr_t>::get_ptr() &&
{
	return std::move(m_ptr);
}

template <typename T>
long ge::shared_ref<T>::use_count() const
{
	return this->get_ptr().use_count();
}

template <typename T, typename ... args_t>
ge::shared_ptr<T> ge::make_shared_ptr(args_t&&... args) requires (std::is_constructible_v<T, args_t...>)
{
	return std::make_shared<T>(std::forward<args_t>(args)...);
}

template <typename T, typename ... args_t>
ge::unique_ptr<T> ge::make_unique_ptr(args_t&&... args) requires (std::is_constructible_v<T, args_t...>)
{
	void* buffer = std::malloc(sizeof(T));
	return { new (buffer)T(std::forward<args_t>(args)...), in_place_delete<T>{} };
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