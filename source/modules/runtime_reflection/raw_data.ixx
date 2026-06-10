module;
#include <assert.h>

export module runtime_reflection:raw_data;

import :typetraits;
import :value;
import :fwd;
import stl;

// TODO enforce no mixing attributes, e.g., one attribute, add data add attri to data, then another attribute to original type. This breaks contiguous span thing
// TODO enums
// TODO access (private/public)

namespace ge::refl
{
	struct cached_type_data_ref
	{
		// Will hold a type_id before the registry has completed building.
		// Will hold a type_data_type after the registry has completed building
		union
		{
			type_id type_id{};
			std::reference_wrapper<const type_data> type_data;
		};
	};

	struct data_data
	{
		using handle_t = data_handle;
		using trait_base_t = data_trait;

		using setter_t = void(*)(value target_object, const value& new_value);
		using getter_t = value(*)(const value& target_object);

		// TODO registry references should be stored in the handles, not in the _data
		std::reference_wrapper<const registry_data> m_reg;
		cached_type_data_ref m_type;
		std::reference_wrapper<const type_data> m_outer_type;
		setter_t m_set{};
		getter_t m_get{};

		std::string_view m_name{};
		std::span<const value> m_traits{};
	};

	struct type_data
	{
		using handle_t = type_handle;
		using trait_base_t = type_trait;

		std::reference_wrapper<const registry_data> m_reg;
		
		std::string_view m_name{};

		std::span<const value> m_traits{};
		std::span<const data_data> m_data{};
		std::span<const func_data> m_funcs{};
		
		type_id m_id{};
	};

	struct module_data
	{
		using handle_t = module_handle;

		std::reference_wrapper<const registry_data> m_reg;

		std::string_view m_name{};

		std::span<const type_data> m_types{};
		std::span<const func_data> m_funcs{};
		std::span<const data_data> m_datas{};
	};

	struct func_data
	{
		using handle_t = func_handle;
		using trait_base_t = func_trait;

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
				return [&]<size_t... Indices>(std::index_sequence<Indices...>)
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

		inplace_vtable<vtable> m_vtable{};

		std::span<const value> m_traits{};

		std::string_view m_name{};
		std::reference_wrapper<const registry_data> m_reg;
	};

	export template<typename T, size_t Capacity>
	struct buffer
	{
		T& push_back(T&& item)
		{
			assert(m_size < Capacity);
			T* dst = end();
			new (dst)T(std::move(item));
			m_size++;
			return *dst;
		}

		[[maybe_unused]] buffer() = default;

		buffer(const buffer&) = delete;
		buffer(buffer&&) = delete;
		
		buffer& operator=(const buffer&) = delete;
		buffer& operator=(buffer&&) = delete;

		~buffer()
		{
			std::span<T> self = *this;

			for (T& item : self)
			{
				item.~T();
			}
		}

		T* data() { return reinterpret_cast<T*>(m_data.data()); }
		const T* data() const { return reinterpret_cast<const T*>(m_data.data()); }

		T* begin() { return data(); }
		const T* begin() const { return data(); }

		T* end() { return begin() + m_size; }
		const T* end() const { return begin() + m_size; }

		std::array<std::byte, sizeof(T) * Capacity> m_data;
		size_t m_size{};
	};

	struct registry_data
	{
		buffer<module_data, 64> m_modules{};
		buffer<type_data, 1024> m_types{};
		buffer<func_data, 1024> m_funcs{};
		buffer<data_data, 1024> m_datas{};
		buffer<value, 1024> m_values{};
	};
}

