export module runtime_reflection:type_id;

import stl;

namespace
{
	constexpr std::uint32_t hash(const char* ch)
	{
		std::uint32_t curr = 0xbadC0ff;

		while (*ch != 0)
		{
			curr ^= (*ch << (*ch % (sizeof(curr) * 8))) + *ch;
			ch++;
		}

		return curr;
	}
}

namespace ge::refl
{
	export struct type_id
	{
		constexpr auto operator<=>(const type_id&) const = default;

		std::uint32_t m_id{};
	};

	export template<typename T>
	concept decorated = std::is_pointer_v<T> || std::is_const_v<T> || std::is_reference_v<T> || std::is_volatile_v<T>;

	export template<typename T>
	concept undecorated = !decorated<T>;

	// TODO make recursive
	template<typename T>
	struct remove_decoration
	{
		using type = std::remove_pointer_t<std::remove_cvref_t<std::remove_pointer_t<std::remove_cvref_t<T>>>>;
	};

	export template<typename T>
	using remove_decoration_t = remove_decoration<T>::type;

	export template<undecorated T>
	consteval type_id make_type_id()
	{
		return { hash(__FUNCSIG__) };
	}

	export template<typename>
	struct func_sig
	{
		static_assert(false, "Not a function signature");
	};

	export template<typename Ret, typename... Params>
		struct func_sig<Ret(Params...)>
	{
		using type = func_sig;
	};

	export template<typename Ret, typename... Params>
	struct func_sig<Ret(*)(Params...)>
	{
		using type = func_sig<Ret(Params...)>;
	};

	export template<typename Ret, typename Class, typename... Params>
	struct func_sig<Ret(Class::*)(Params...)>
	{
		using type = func_sig<Ret(Class&, Params...)>;
	};

	export template<typename Ret, typename Class, typename... Params>
		struct func_sig<Ret(Class::*)(Params...) const>
	{
		using type = func_sig<Ret(const Class&, Params...)>;
	};

	export template<typename Ret, typename Class, typename... Params>
		struct func_sig<Ret(Class::*)(Params...)&&>
	{
		using type = func_sig<Ret(Class&&, Params...)>;
	};

	export template<typename Ret, typename... Params>
		struct func_sig<Ret(&)(Params...)>
	{
		using type = func_sig<Ret(Params...)>;
	};

	export template<typename T>
	using func_sig_t = func_sig<T>::type;

	template<auto FuncPtr>
	concept is_func = requires { typename func_sig<decltype(FuncPtr)>; };

	export template<typename>
	struct data_ptr
	{
		static_assert(false, "Not a data pointer");
	};

	export template<typename T, typename DataT>
	struct data_ptr<DataT T::*>
	{
		using data_t = DataT;
		using outer_type_t = T;
	};

	template<auto DataPtr>
	concept is_data = requires { typename data_ptr<decltype(DataPtr)>::outer_type_t; };
}