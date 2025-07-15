export module meta:builder;

import :type;
import std;

namespace ge::meta
{
	export template<typename T>
	class builder
	{
	public:
		builder(std::string_view a_name);

		ge::meta::type&& finish() && { return std::move(m_type); }

	private:
		ge::meta::type m_type;
	};



}
template <typename T>
ge::meta::builder<T>::builder(std::string_view a_name) :
	m_type(std::type_identity_t<T>{}, a_name)
{
}