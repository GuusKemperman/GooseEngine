module;
#include <assert.h>

export module runtime_reflection:handles;

import stl;
import :handle_fwd;
import :raw_data;
import :value;
import :traits;

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

// TODO enforce no mixing attributes, e.g., one attribute, add data add attri to data, then another attribute to original type. This breaks contiguous span thing
// TODO enums
// TODO access (private/public)
namespace ge::refl
{
	export class data_handle
	{
	public:
		API data_handle(const data_data& data) : m_data(data) {}

		API std::string_view get_name() const { return m_data.get().m_name; }

		auto get_type(this const auto& self) { return type_handle{ self.m_data.get().m_type.type_data }; }

		// The type this member is located in
		auto get_outer_type(this const auto& self) { return type_handle{ self.m_data.get().m_outer_type }; }

		using setter_t = data_data::setter_t;
		using getter_t = data_data::getter_t;

		API setter_t get_setter() const { return m_data.get().m_set; }
		API getter_t get_getter() const { return m_data.get().m_get; }

		API auto traits() const { return m_data.get().m_traits; }

	private:
		std::reference_wrapper<const data_data> m_data;
	};

	export class func_handle
	{
	public:
		API func_handle(const func_data& data) : m_data(data) {}

		API std::string_view get_name() const { return m_data.get().m_name; }

		// The type this member is located in
		// API type_handle get_outer_type() const;

		API auto traits() const { return m_data.get().m_traits; }

		template<typename... Args>
		value invoke_unchecked(Args&&... args) const
		{
			std::array<value, sizeof...(Args)> packed{

				[] <typename Arg>(Arg && arg) -> value
				{
					if constexpr (std::is_same_v<remove_decoration_t<Arg>, value>)
					{
						return arg.is_mutable() ? value::create_ref(arg) : value::create_view(arg);
					}
					else if constexpr (std::is_const_v<std::remove_reference_t<Arg>>)
					{
						return value::create_view(arg);
					}
					else
					{
						return value::create_ref(arg);
					}
				}.operator()(std::forward<Args>(args))... };
			assert(check_invoke(packed));
			return m_data.get().m_vtable->invoke(packed.data());
		}

	private:
		API bool check_invoke(std::span<const value> args) const
		{
			std::span<const type_id> params = m_data.get().m_vtable->get_params();

			assert(params.size() == args.size());

			for (const auto& [arg, param] : std::ranges::zip_view(args, params))
			{
				assert(arg && "empty argument provided");
				// TODO proper is_a.
				assert(arg.get_type_id() == param && "param type mismatch");
			}

			return true;
		}

		std::reference_wrapper<const func_data> m_data;
	};

	export class type_handle
	{
	public:
		API type_handle(const type_data& data) : m_data(data) {}

		API type_id get_id() const { return m_data.get().m_id; }
		API std::string_view get_name() const { return m_data.get().m_name; }

		API auto datas() const { return view_as_public_handles<data_handle>(m_data.get().m_data); }
		API auto funcs() const { return view_as_public_handles<func_handle>(m_data.get().m_funcs); }
		API auto traits() const { return m_data.get().m_traits; }

	private:
		std::reference_wrapper<const type_data> m_data;
	};

}