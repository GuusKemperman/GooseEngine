export module utils:memory;
export import std;

export namespace ge
{
	template<typename T>
	using shared_ptr = std::shared_ptr<T>;

	template<typename T>
	using default_delete = std::default_delete<T>;

	template<typename T, typename deleter_t = default_delete<T>>
	using unique_ptr = std::unique_ptr<T, deleter_t>;

	template<typename T, typename ptr_t>
	class ref_base
	{
	protected:
		using base_t = ref_base;

		ref_base() = delete;

		constexpr explicit ref_base(ptr_t a_ptr);

	public:
		constexpr T& operator*() const;

		constexpr T* operator->() const;

		constexpr operator T& () const;

		constexpr T& get() const;

	private:
		ptr_t m_ptr;
	};

	template<typename T>
	class shared_ref :
		public ref_base<T, shared_ptr<T>>
	{
	public:
		using ref_base<T, shared_ptr<T>>::base_t;
	};

	template<typename T, typename deleter_t = default_delete<T>>
	class unique_ref :
		public ref_base<T, unique_ptr<T, deleter_t>>
	{
	public:
		using ref_base<T, unique_ptr<T, deleter_t>>::base_t;
	};

	template<typename T, typename... args_t>
	shared_ref<T> make_shared_ref(args_t&&... args) requires (std::is_constructible_v<T, args_t...>);

	template<typename T, typename deleter_t = default_delete<T>, typename... args_t>
	unique_ref<T, deleter_t> make_unique_ref(args_t&&... args) requires (std::is_constructible_v<T, args_t...>);
}

template <typename T, typename ptr_t>
constexpr ge::ref_base<T, ptr_t>::ref_base(ptr_t a_ptr) :
	m_ptr(std::move(a_ptr))
{
	if (m_ptr == nullptr)
	{
		throw std::invalid_argument("Null pointer passed to reference constructor");
	}
}

template <typename T, typename ptr_t>
constexpr T& ge::ref_base<T, ptr_t>::operator*() const
{
	return *get();
}

template <typename T, typename ptr_t>
constexpr T* ge::ref_base<T, ptr_t>::operator->() const
{
	return get();
}

template <typename T, typename ptr_t>
constexpr ge::ref_base<T, ptr_t>::operator T&() const
{
	return *m_ptr;
}

template <typename T, typename ptr_t>
constexpr T& ge::ref_base<T, ptr_t>::get() const
{
	return *m_ptr;
}

template <typename T, typename ... args_t>
ge::shared_ref<T> ge::make_shared_ref(args_t&&... args) requires (std::is_constructible_v<T, args_t...>)
{
	return ge::shared_ref<T>{ std::make_shared<T>(std::forward<args_t>(args)...) };
}

template <typename T, typename deleter_t, typename ... args_t>
ge::unique_ref<T, deleter_t> ge::make_unique_ref(args_t&&... args) requires (std::is_constructible_v<T, args_t...>)
{
	return ge::unique_ref<T>{ std::make_unique<T>(std::forward<args_t>(args)...) };
}
