export module runtime_reflection:query;

import :data;

namespace ge::refl
{
	template<undecorated T>
	struct with_access{	};

	template<undecorated T>
	struct read_access : with_access<T> {};

	template<undecorated T>
	struct check_access { };

	template<undecorated T>
	struct read_if_exists_access : check_access<T>{};

	template<typename T>
	struct check_result
	{
		using type_t = T;
		bool m_value{};
	};

	template<typename stored_definitives_tuple, typename maybe_tuple = std::tuple<>, typename checked_tuple = std::tuple<>, typename... definitives>
	class query_element
	{
	public:
		template<undecorated T>
		bool check() const
		{
			return std::get<check_result<T>>(m_checked_tuple).m_value;
		}

		template<undecorated T>
		const T* read_if_exists() const
		{
			return std::get<const T*>(m_maybe_tuple);
		}

		template<undecorated T>
		const T& read() const
		{
			return std::get<const T&>(m_definitive_tuple);
		}

	// TODO make private
		template<typename T>
		static constexpr bool value_matcher(const value& value)
		{
			// TODO is_a?
			return value.get_type_id() == make_type_id<remove_decoration_t<T>>();
		}

		template<typename raw_data_t>
		static bool matches(const raw_data_t& raw_data)
		{
			std::span<const value> traits = raw_data.m_traits;

			bool has_all_required = ([&traits]<typename T>()
			{
				return std::ranges::any_of(traits, &value_matcher<T>);
			}.template operator() < definitives > () && ...);

			return has_all_required;
		}

		template<typename raw_data_t>
		query_element(const raw_data_t& raw_data) :
		m_definitive_tuple([&raw_data]<typename... reads>(std::type_identity<std::tuple<const raw_data_t&, reads...>>) -> std::tuple<const raw_data_t&, reads...>
		{
			std::span<const value> traits = raw_data.m_traits;

			return std::tuple<const raw_data_t&, reads...>(
				raw_data,
				[&traits]<typename T>() -> T
			{
				const value& trait = *std::ranges::find_if(traits, &value_matcher<T>);
				return *trait.as_constant<remove_decoration_t<T>>();
			}.template operator() < reads > ()...);
		}(std::type_identity<stored_definitives_tuple>{})),
		m_maybe_tuple([&raw_data]<typename... maybes>(std::type_identity<std::tuple<maybes...>>) -> std::tuple<maybes...>
		{
			std::span<const value> traits = raw_data.m_traits;

			return std::make_tuple<maybes...>(
				[&traits]<typename T>() -> T
			{
				auto it = std::ranges::find_if(traits, &value_matcher<T>);

				if (it == traits.end())
				{
					return nullptr;
				}

				const value& trait = *it;
				return trait.as_constant<remove_decoration_t<T>>();
			}.template operator() < maybes > ()...);
		}(std::type_identity<maybe_tuple>{})),
		m_checked_tuple([&raw_data]<typename... check_results>(std::type_identity<std::tuple<check_results...>>) -> std::tuple<check_results...>
		{
			std::span<const value> traits = raw_data.m_traits;

			return std::make_tuple<check_results...>(
				[&traits]<typename check_result>() -> check_result
			{
				using type_t = check_result::type_t;

				bool exists = std::ranges::contains(traits, &value_matcher<type_t>);
				return check_result{ .m_value = exists };
			}.template operator() < check_results > ()...);
		}(std::type_identity<checked_tuple>{}))
		{
		}

		stored_definitives_tuple m_definitive_tuple;
		maybe_tuple m_maybe_tuple;
		checked_tuple m_checked_tuple;
	};

	template<undecorated query_element, typename to_append>
	struct append_to_query_element;

	template<undecorated next_trait, typename... definitives, typename... stored_definitives, typename... maybes, typename... checked>
	struct append_to_query_element<
		query_element<std::tuple<stored_definitives...>, std::tuple<maybes...>, std::tuple<checked...>, definitives...>, with_access<next_trait>>
	{
		using next = query_element<std::tuple<stored_definitives...>, std::tuple<maybes...>, std::tuple<checked...>, definitives..., next_trait>;
	};

	template<undecorated next_trait, typename... definitives, typename... stored_definitives, typename... maybes, typename... checked>
	struct append_to_query_element<
		query_element<std::tuple<stored_definitives...>, std::tuple<maybes...>, std::tuple<checked...>, definitives...>, read_access<next_trait>>
	{
		using next = query_element<std::tuple<stored_definitives..., const next_trait&>, std::tuple<maybes...>, std::tuple<checked...>, definitives..., next_trait>;
	};

	template<undecorated next_trait, typename... definitives, typename... stored_definitives, typename... maybes, typename... checked>
	struct append_to_query_element<
		query_element<std::tuple<stored_definitives...>, std::tuple<maybes...>, std::tuple<checked...>, definitives...>, check_access<next_trait>>
	{
		using next = query_element<std::tuple<definitives..., const next_trait&>, std::tuple<maybes...>, std::tuple<checked..., check_result<next_trait>>, definitives...>;
	};

	template<undecorated next_trait, typename... definitives, typename... stored_definitives, typename... maybes, typename... checked>
	struct append_to_query_element<
		query_element<std::tuple<stored_definitives...>, std::tuple<maybes...>, std::tuple<checked...>, definitives...>, read_if_exists_access<next_trait>>
	{
		using next = query_element<std::tuple<definitives..., const next_trait&>, std::tuple<maybes..., next_trait>, std::tuple<checked..., check_result<next_trait>>, definitives...>;
	};

	export template<typename element_data_type , typename element_t = query_element<std::tuple<const element_data_type&>> >
	class query;

	export using type_query = query<type_data>;
	export using data_query = query<data_data>;
	export using func_query = query<func_data>;

	template<typename T>
	struct query_handle_type;

	template<typename handle_t, typename element_t>
	struct query_handle_type<query<handle_t, element_t>>
	{
		using type = handle_t;
	};

	export template<typename T>
		concept is_type_query = std::is_same_v<typename query_handle_type<T>::type, type_data>;

	export template<typename T>
		concept is_data_query = std::is_same_v<typename query_handle_type<T>::type, data_data>;

	export template<typename T>
		concept is_func_query = std::is_same_v<typename query_handle_type<T>::type, func_data>;

	static_assert(is_type_query<type_query>);
	static_assert(is_data_query<data_query>);
	static_assert(is_func_query<func_query>);
	static_assert(!is_func_query<type_query>);

	template<typename element_data_type, typename element_t>
	class query
	{
	public:
		query(std::span<const element_data_type> source_range) : m_source_range(adapt_range(source_range)) {}

		auto begin(this auto& self) { return self.m_source_range.begin(); }
		auto end(this auto& self) { return self.m_source_range.end(); }

		template<std::derived_from<typename element_data_type::trait_base_t> T>
		using with = query<element_data_type, typename append_to_query_element<element_t, with_access<T>>::next>;
	
		template<std::derived_from<typename element_data_type::trait_base_t> T>
		using read = query<element_data_type, typename append_to_query_element<element_t, read_access<T>>::next>;

		template<std::derived_from<typename element_data_type::trait_base_t>T>
		using check = query < element_data_type, typename append_to_query_element<element_t, check_access<T>>::next>;

		template<std::derived_from<typename element_data_type::trait_base_t> T>
		using read_if_exists = query < element_data_type, typename append_to_query_element<element_t, read_if_exists_access<T>>::next>;

	private:
		static auto adapt_range(std::span<const element_data_type> source_range)
		{
			return std::views::filter(source_range, 
				[](const element_data_type& raw_data) -> bool
				{
					return element_t::matches(raw_data);
				});
		}

		decltype(adapt_range({})) m_source_range;
	};
}

template<std::size_t I, typename definitive_tuple, typename... others>
struct std::tuple_element<I, ge::refl::query_element<definitive_tuple, others...>>
{
	using type = std::tuple_element_t<I, definitive_tuple>;
};

template<typename definitive_tuple, typename... others>
struct std::tuple_size<ge::refl::query_element<definitive_tuple, others...>>
: integral_constant<std::size_t, std::tuple_size_v<definitive_tuple>> {
};

