export module meta:type;

import std;
import utils;

namespace ge::meta
{
	template<typename T>
	class builder;

	class type_impl_base
	{
	public:
		virtual ~type_impl_base() = default;

		virtual ge::shared_ptr<void> default_construct() const = 0;
	};

	template<typename T>
	class type_impl final :
		public type_impl_base
	{
	protected:
		ge::shared_ptr<void> default_construct() const override;
	};

	static_assert(sizeof(type_impl<int>) == sizeof(type_impl_base));

	export class type
	{
		template<typename T>
		friend class builder;

		template<typename T>
		type(std::type_identity_t<T> a_type, std::string_view a_name);

		type_impl_base& get_impl() { return m_impl; }
		const type_impl_base& get_impl() const { return m_impl; }

	public:
		ge::shared_ptr<void> default_construct() const { return get_impl().default_construct(); };

	private:
		ge::unique_ref<type_impl_base> m_impl;
		std::string_view m_name{};
	};
}

template<typename T>
ge::meta::type::type(std::type_identity_t<T> a_type, std::string_view a_name) :
	m_impl(make_unique_ptr<type_impl<T>>()),
	m_name(a_name)
{
}

template <typename T>
ge::shared_ptr<void> ge::meta::type_impl<T>::default_construct() const
{
	if constexpr (std::is_default_constructible_v<T>)
	{
		return ge::make_shared_ptr<T>();
	}
	else
	{
		return nullptr;
	}
}