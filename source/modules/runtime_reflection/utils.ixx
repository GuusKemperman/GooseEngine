module;

#include <cassert>

export module runtime_reflection:utils;

export import :value;
export import :data;
export import :type_id;

namespace
{
	API bool check_invoke(const ge::refl::func_data& func, std::span<const ge::refl::value> args)
	{
		std::span<const ge::refl::type_id> params = func.m_vtable->get_params();

		assert(params.size() == args.size());

		for (const auto& [arg, param] : std::ranges::zip_view(args, params))
		{
			assert(arg && "empty argument provided");
			// TODO proper is_a.
			assert(arg.get_type_id() == param && "param type mismatch");
		}

		return true;
	}
}

namespace ge::refl
{
	export template<typename... Args>
	value invoke(const func_data& func, Args&&... args)
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
		assert(check_invoke(func, packed));
		return func.m_vtable->invoke(packed.data());
	}


}
