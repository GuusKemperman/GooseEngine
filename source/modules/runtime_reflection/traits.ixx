module;

#include <cassert>

export module runtime_reflection:traits;

import :value;
import :type_id;
import :builders;
import stl;

namespace ge::refl
{
	export template<typename FuncSigT>
	struct invocable_trait;

	export template<typename Ret, typename... Params>
	struct invocable_trait<Ret(Params...)> : func_trait
	{
		Ret(*m_invoke)(Params...);

		template<auto Func>
		void on_apply(const builders::func_builder<Func>&)
		{
			m_invoke = Func;
		}
	};

	struct dynamically_invocable_trait
	{
		struct vtable
		{
			virtual ~vtable() = default;
			// TODO we should store the non-cached params somewhere too, to get the full type-handle.
			virtual std::span<const type_id> get_params() const = 0;
			virtual type_id get_returns() const = 0;
			virtual value invoke(value* args) const = 0;
		};

		template<auto, typename>
		struct vtable_impl {};

		template<auto FuncPtr, supported_param_type Ret, supported_param_type... Params>
		struct vtable_impl<FuncPtr, func_sig<Ret(Params...)>> final : vtable
		{
			std::span<const type_id> get_params() const override
			{
				static constexpr size_t size = sizeof...(Params);
				using Arr = std::array<type_id, size>;
				static constexpr Arr params =
					[]() -> Arr
					{
						Arr param_arr{};

						[&param_arr] <size_t... Indices>(std::index_sequence<Indices...>)
						{
							([&param_arr]<typename ParamT, size_t Index>()
							{
								param_arr[Index] = make_type_id<remove_decoration_t<ParamT>>();
							}.template operator() < Params, Indices > (), ...);
						}(std::make_index_sequence<size>());

						return param_arr;
					}();

				return { params.data(), size };
			}

			type_id get_returns() const override
			{
				return make_type_id<remove_decoration_t<Ret>>();
			}

			value invoke(value* args) const override
			{
				return[&]<size_t... Indices>(std::index_sequence<Indices...>)
				{
					auto invoke = [args]() -> Ret
						{
							return std::invoke(FuncPtr,
								[args]<typename ParamT, size_t Idx>() -> ParamT
							{
								if constexpr (std::is_const_v<std::remove_reference_t<ParamT>> || undecorated<ParamT>)
								{
									return *static_cast<std::remove_reference_t<std::add_const_t<ParamT>>*>(args[Idx].const_data());
								}
								else
								{
									return *static_cast<std::remove_reference_t<ParamT>*>(args[Idx].mutable_data());
								}
							}.template operator() < Params, Indices > ()...);
						};

					// TODO clean this up
					if constexpr (std::is_same_v<Ret, void>)
					{
						invoke();
						return value{};
					}
					else if constexpr (std::is_reference_v<Ret>)
					{
						if constexpr (std::is_const_v<std::remove_reference_t<Ret>>)
						{
							return value::create_view(invoke());
						}
						else
						{
							return value::create_ref(invoke());
						}
					}
					else
					{
						return value::create_owning(invoke());
					}
				}(std::make_index_sequence<sizeof...(Params)>());
			}
		};

		template<auto Func>
		void on_apply(const builders::func_builder<Func>&)
		{
			m_vtable.set<vtable_impl<Func, func_sig_t<decltype(Func)>>>();
		}

		API bool check_invoke(std::span<const value> args)
		{
			std::span<const type_id> params = m_vtable->get_params();

			assert(params.size() == args.size());

			for (const auto& [arg, param] : std::ranges::zip_view(args, params))
			{
				assert(arg && "empty argument provided");
				// TODO proper is_a.
				assert(arg.get_type_id() == param && "param type mismatch");
			}

			return true;
		}

		template<typename... Args>
		value invoke(Args&&... args)
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
			return m_vtable->invoke(packed.data());
		}
		inplace_vtable<vtable> m_vtable{};
	};
}
