export module runtime_reflection:module_handle;

import stl;
import :handle_fwd;
import :raw_data;
import :value;
import :handles;
import :query;

namespace
{
	template<typename To, typename From >
	auto convert_from_to(const From& from)
	{
		return To(from);
	}

	template<typename To>
	auto view_as_public_handles(const auto& inputRange)
	{
		using FromT = decltype(*inputRange.begin());
		return inputRange | std::ranges::views::transform(convert_from_to<To, FromT>);
	}
}

namespace ge::refl
{
	export class module_handle
	{
	public:
		API module_handle(const module_data& data) : m_data(data) {}

		API std::string_view get_name() const { return m_data.get().m_name; }

		template<is_data_query query>
		query query() const { return query{ m_data.get().m_datas }; }

		API auto datas() const { return view_as_public_handles<data_handle>(m_data.get().m_datas); }
		API auto types() const { return view_as_public_handles<type_handle>(m_data.get().m_types); }
		API auto funcs() const { return view_as_public_handles<func_handle>(m_data.get().m_funcs); }

	private:
		std::reference_wrapper<const module_data> m_data;
	};

	export class registry
	{
	public:
		registry(std::unique_ptr<const registry_data> data) :
			m_data(std::move(data))
		{
		}

		API auto modules() const { return view_as_public_handles<module_handle>(m_data->m_modules); }
		API auto types() const { return view_as_public_handles<type_handle>(m_data->m_types); }
		API auto funcs() const { return view_as_public_handles<func_handle>(m_data->m_funcs); }

	private:
		std::unique_ptr<const registry_data> m_data;
	};
}
