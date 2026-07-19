export module runtime_reflection:query;

import :data;

namespace
{
	template<typename T>
	constexpr bool is_value_of_type(const ge::refl::value& value)
	{
		// TODO this should probably be an is_a?
		return value.get_type_id() == ge::refl::make_type_id<ge::refl::remove_decoration_t<T>>();
	}
}

namespace ge::refl
{
	struct query_element
	{
		query_element(const auto&) {}

		static constexpr size_t num_elements = 0;

		static constexpr bool matches(std::span<const value>)
		{
			return true;
		}

		template<std::size_t Index>
		decltype(auto) get() const = delete;
	};

	template<typename raw_data_t>
	struct handle_element : query_element
	{
		handle_element(const raw_data_t& data) : query_element(data), m_data(data) {}

		static constexpr size_t num_elements = query_element::num_elements + 1;

		static constexpr bool matches(std::span<const value> traits)
		{
			return query_element::matches(traits);
		}

		template<std::size_t Index>
		decltype(auto) get() const
		{
			if constexpr (Index == num_elements - 1)
			{
				return m_data;
			}
			else
			{
				return query_element::get<Index>();
			}

		}

		const raw_data_t& m_data;
	};

	template<typename read, typename base>
	struct read_element : base
	{
		read_element(const auto& data) :
		base(data),
		m_item(*std::ranges::find_if(data.m_traits, &is_value_of_type<read>)->as_constant<read>())
		{
			
		}

		static constexpr size_t num_elements = base::num_elements + 1;

		static constexpr bool matches(std::span<const value> traits)
		{
			return base::matches(traits) && std::ranges::any_of(traits, &is_value_of_type<read>);
		}

		template<std::size_t Index>
		decltype(auto) get() const
		{
			if constexpr (Index == num_elements - 1)
			{
				return m_item;
			}
			else
			{
				return base::template get<Index>();
			}
		}

		const read& m_item;
	};

	template<typename with, typename base>
	struct with_element : base
	{
		with_element(const auto& data) : base(data) {}

		static constexpr size_t num_elements = base::num_elements;

		static constexpr bool matches(std::span<const value> traits)
		{
			return base::matches(traits) && std::ranges::any_of(traits, &is_value_of_type<with>);
		}

		template<std::size_t Index>
		decltype(auto) get() const { return base::template get<Index>(); }
	};

	export template<typename element_data_type, typename element_t = handle_element<element_data_type>>
	class query
	{
	public:
		query(std::span<const element_data_type> source_range) : m_source_range(adapt_range(source_range)) {}

		auto begin(this auto& self) { return self.m_source_range.begin(); }
		auto end(this auto& self) { return self.m_source_range.end(); }

		template<std::derived_from<typename element_data_type::trait_base_t> T>
		using with = query<element_data_type, with_element<T, element_t>>;
	
		template<std::derived_from<typename element_data_type::trait_base_t> T>
		using read = query<element_data_type, read_element<T, element_t>>;

	private:
		static auto adapt_range(std::span<const element_data_type> source_range)
		{
			return std::views::filter(source_range, 
				[](const element_data_type& raw_data) -> bool
				{
					return element_t::matches(raw_data.m_traits);
				}) | std::views::transform([](const element_data_type& raw_data)
				{
					return element_t{ raw_data };
				});
		}

		decltype(adapt_range({})) m_source_range;
	};

	export using type_query = query<type_data>;
	export using data_query = query<data_data>;
	export using func_query = query<func_data>;
}

namespace std
{
	template<std::derived_from<ge::refl::query_element> element>
	struct tuple_size<element> : std::integral_constant<std::size_t, element::num_elements> {};

	template<size_t Index, std::derived_from<ge::refl::query_element> element>
	struct tuple_element<Index, element>
	{
		using type = decltype(std::declval<element>().template get<Index>());
	};
}
