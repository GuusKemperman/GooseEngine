export module dependency_injection;

import std;

namespace ge
{
	export template<typename... T>
	class depends_on;

	export template <>
	class depends_on<>
	{
	public:
		bool operator==(const depends_on&) const { return true; }
		bool operator!=(const depends_on&) const { return false; }

		//template<typename... Args>
		//void ForEachCommandBuffer(Args&&...) {};

		//template<typename... Args>
		//void ForEachCommandBuffer(Args&&...) const {};
	};

	export template<typename this_t, typename... others_t>
	class depends_on<this_t, others_t...> : public depends_on<others_t...>
	{
		using depends_base_t = depends_on<others_t...>;

	protected:
		using depends_on_constructor = depends_on<this_t, others_t...>;

	public:
		depends_on(this_t& a_this, others_t&... a_others);

		using depends_base_t::operator==;
		using depends_base_t::operator!=;

		bool operator==(const depends_on& other) const
		{
			return &m_ref.get() == &other.m_ref.get() && depends_base_t::operator==(static_cast<const depends_base_t&>(other));
		}

		bool operator!=(const depends_on& other) const
		{
			return &m_ref.get() != &other.m_ref.get() || depends_base_t::operator!=(static_cast<const depends_base_t&>(other));
		}

		//template<typename... Args>
		//void ForEachCommandBuffer(const auto& func, Args&&... args)
		//{
		//	func(mBuffer, args...);
		//	depends_base_t::ForEachCommandBuffer(func, args...);
		//}

		//template<typename... Args>
		//void ForEachCommandBuffer(const auto& func, Args&&... args) const
		//{
		//	func(mBuffer, args...);
		//	depends_base_t::ForEachCommandBuffer(func, args...);
		//}

		template<typename target_t>
		constexpr target_t& get(this depends_on& self)
		{
			if constexpr (std::convertible_to<this_t&, target_t&>)
			{
				return self.m_ref.get();
			}
			else
			{
				return self.depends_base_t::get<target_t>();
			}
		}

	private:
		std::reference_wrapper<this_t> m_ref;
	};

}

template<typename this_t, typename ...others_t>
ge::depends_on<this_t, others_t...>::depends_on(this_t& a_this, others_t&... a_others) :
	depends_base_t(a_others...),
	m_ref(a_this)
{
}

//
//namespace
//{
//	template<typename Target, typename Curr>
//	static Target& applied_get_recursive(std::reference_wrapper<Curr> curr)
//	{
//		if constexpr (std::is_same_v<Target, Curr>)
//		{
//			return curr.get();
//		}
//		else if constexpr (requires(Curr& curr) { { curr.get<Target>() } -> std::convertible_to<Target>; })
//		{
//			return curr.get().get<Target>();
//		}
//		else
//		{
//			return curr.get();
//			//[] <bool flag = false>()
//			//{
//			//	static_assert(flag, "no match");
//			//}();
//			//static_assert(false, "Could not locate dependency");
//		}
//	}
//
//	template<typename Target, typename Curr, typename... Others>
//	static Target& applied_get_recursive(std::reference_wrapper<Curr> curr, [[maybe_unused]] std::reference_wrapper<Others>... remaining)
//	{
//		if constexpr (requires(decltype(curr) curr) { applied_get_recursive<Target>(curr); })
//		{
//			return applied_get_recursive<Target>(curr);
//		}
//		else
//		{
//			return applied_get_recursive<Target>(remaining...);
//		}
//	}
//
//	template<typename Target, typename... Args>
//	static Target& applied_get(std::reference_wrapper<Args>... args)
//	{
//		return applied_get_recursive<Target>(args...);
//	}
//}
//
//template<typename... dependencies>
//template<typename T>
//constexpr T& ge::depends_on<dependencies...>::get()
//{
//	return std::apply(&applied_get<T, dependencies...>, m_dependencies);
//}