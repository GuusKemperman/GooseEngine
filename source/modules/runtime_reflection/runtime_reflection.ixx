//module;
//#include <assert.h>

export module runtime_reflection;

export import std;

/* Requirements:

- Multiple different registries support
- Attributes visiting
- Data visiting
- Function visiting
- Copy, store, move type instead of type*
- Infinite levels of qualifiers
- Data that is not available at compile, should be stored in registry
- Should be possible to remove a reflected module (assuming nothing depends on it) without a memory leak
- Everything stored inside registry buffer is immutable
*/

namespace ge::refl
{

	export class data;
	export class func;
	export class type;
	export class value;
	
	export class registry;

	template<typename T, typename Prev>
	class type_builder;

	namespace detail
	{
		template<typename T>
		struct is_type_builder_t : std::bool_constant<false>
		{
		};

		template<typename T, typename Prev>
		struct is_type_builder_t<type_builder<T, Prev>> : std::bool_constant<true>
		{

		};

	}
	template<typename T>
	concept type_builder_type = detail::is_type_builder_t<T>::value;

	export struct on_apply_context
	{
		
	};

	export class type_trait
	{
	public:
		template<typename T>
		void on_apply([[maybe_unused]] const on_apply_context& ctx) { }
	};

	struct data_data;
	struct func_data;
	struct type_data;
	struct value_data;
	struct registry_data;

	export class data
	{
	public:

	private:
		std::reference_wrapper<const data_data> m_data;
	};

	export struct type_info
	{
		API auto operator<=>(const type_info&) const = default;

		template<typename T>
		static const type_info& get_type_info()
		{
			// TODO add name support
			// TODO add hash support
			static type_info info{ .m_debug_name = "Not implemented", .m_hash = 0, .m_size = sizeof(T) };
			return info;
		}

		std::string_view m_debug_name{};
		size_t m_hash{};
		size_t m_size{};
	};



	export class type
	{
	public:
		API type(const type_data& data) : m_data(data) {}

		API const type_info& get_info() const;
		API std::string_view get_name() const;

	private:
		std::reference_wrapper<const type_data> m_data;
	};

	export class registry
	{
	public:
		registry(std::unique_ptr<const registry_data> data) : 
			m_data(std::move(data))
		{
			
		}

		API std::optional<type> try_get_type(std::string_view name) const;

	private:
		std::unique_ptr<const registry_data> m_data;
	};

	struct data_data
	{
		std::reference_wrapper<const registry_data> m_reg;

		std::string_view m_name{};

		std::reference_wrapper<const type_data> m_type;
		
		const type_data* m_located_inside_type;
		std::span<const value_data> m_traits{};
	};

	struct type_data
	{
		std::reference_wrapper<const registry_data> m_reg;
		std::reference_wrapper<const type_info> m_info;

		std::string_view m_name{};

		std::span<const value_data> m_traits{};
		std::span<const data_data> m_data{};
		std::span<const func_data> m_funcs{};
	};

	struct value_data
	{
		template<typename T>
		value_data(const registry_data& reg, T&& ) :
			m_reg(reg)
		{
			throw "not implemented yet";
		}
		std::reference_wrapper<const registry_data> m_reg;

		const type_data* m_type{};
		void* m_value{};
	};
	
	struct func_data
	{
		std::reference_wrapper<const registry_data> m_reg;

	};

	struct registry_data
	{
		API auto* try_get_type_data(this auto&& self, std::string_view name)
		{
			auto end = self.m_types + self.m_types_size;
			auto it = std::find_if(self.m_types, end,
				[&](const type_data& type)
				{
					return type.m_name == name;
				});

			if (it == end)
			{
				return nullptr;
			}
			return it;
		}

		API auto& get_type_data(this auto&& self, std::string_view name)
		{
			auto* type_data = try_get_type_data(self, name);
			assert(type_data != nullptr);
			return *type_data;
		}

		// TODO these should be done using containers/custom allocators
		static constexpr size_t sMaxNumTypes = 1024;
		type_data* m_types = (type_data*)std::malloc(sMaxNumTypes * sizeof(type_data));
		size_t m_types_size{};

		static constexpr size_t sMaxNumValues = 1024;
		value_data* m_values = (value_data*)std::malloc(sMaxNumValues * sizeof(value_data));
		size_t m_values_size{};

		static constexpr size_t sMaxNumFuncs = 1024;
		func_data* m_funcs = (func_data*)std::malloc(sMaxNumFuncs * sizeof(func_data));
		size_t m_func_size{};

		static constexpr size_t sMaxNumData = 1024;
		data_data* m_datas = (data_data*)std::malloc(sMaxNumData * sizeof(data_data));
		size_t m_data_size{};
	};

	const type_info& type::get_info() const
	{
		return m_data.get().m_info;
	}

	std::string_view type::get_name() const
	{
		return m_data.get().m_name;
	}

	std::optional<type> registry::try_get_type(std::string_view name) const
	{
		auto end = m_data->m_types + m_data->m_types_size;
		auto it = std::find_if(m_data->m_types, end, 
			[&](const type_data& type)
			{
				return type.m_name == name;
			});

		if (it == end)
		{
			return std::nullopt;
		}
		return type{ *it };
	}

	class builder_destination
	{
	public:
		virtual ~builder_destination() = default;

		// Allocates pointer-stable object in contiguous buffer
		virtual value_data& alloc_value() = 0;
		virtual type_data& alloc_type() = 0;
		virtual data_data& alloc_data() = 0;
		virtual func_data& alloc_func() = 0;

		virtual const registry_data& get_reg() = 0;
	};

	class builder_base
	{
	protected:
		API builder_base(builder_destination& reg) :
			m_destination(reg)
		{
			
		}
		
		API builder_destination& get_destination() const
		{
			return m_destination;
		}

		API static builder_destination& get_destination(const builder_base& other)
		{
			return other.get_destination();
		}

	private:
		builder_destination& m_destination;
	};

	namespace builder_parts
	{
		template<typename Derived>
		class name_part
		{
		public:
			Derived& set_name(std::string_view name)
			{
				m_name = name;
				return static_cast<Derived&>(*this);
			}

		protected:
			std::string_view m_name{};
		};

		template<typename Derived>
		class type_part
		{
		public:
			template<typename T>
			type_builder<T, Derived> begin_type( std::string_view name )
			{
				auto builder = type_builder<T, Derived>(static_cast<Derived&>(*this));
				builder.set_name(name);
				return builder;
			}
		};

		template<auto DataPtr>
		class data_part
		{
		public:

		};

		template<typename OnApplyArg, typename TraitT>
		void trait(builder_destination& dest, std::span<const value_data>& traits, TraitT&& trait)
		{
			value_data& data = dest.alloc_value();

			if (traits.empty())
			{
				traits = { &data, 1 };
			}
			else
			{
				if (&data != traits.data() + traits.size())
				{
					// TODO should be an assert
					throw std::runtime_error{ "arr not contiguous" };
				}
				traits = { traits.data(), traits.size() + 1 };
			}

			const registry_data& reg = dest.get_reg();

			data = value_data{ reg, std::forward<TraitT>(trait) };
			TraitT& inplace = *static_cast<TraitT*>(traits.back().m_value);

			on_apply_context ctx{};

			inplace.template on_apply< OnApplyArg >(ctx);
		}
	}

	template<typename DestinationT>
	class module_builder : 
		public builder_base,
		public builder_parts::name_part<module_builder<DestinationT>>,
		public builder_parts::type_part<module_builder<DestinationT>>
	{
	public:
		module_builder(DestinationT& destination) :
			builder_base(destination)
		{
		}

		DestinationT& end_module()
		{
			return static_cast<DestinationT&>( get_destination() );
		}

	private:
		
	};

	export class registry_builder : public builder_destination
	{
	public:
		virtual ~registry_builder() = default;

		API auto begin_module()
		{
			return module_builder{ *this };
		}

		API registry build()
		{
			return registry{ std::move(m_reg) };
		}

	protected:
		std::unique_ptr<registry_data> m_reg = std::make_unique<registry_data>();

		template<size_t Max, typename T>
		T& alloc(T* buffer, size_t& count)
		{
			if (count >= Max)
			{
				// TODO should be assert
				throw std::runtime_error{ "Buffer too small" };
			}
			T& value = buffer[count++];
			return value;
		}

		API value_data& alloc_value() override
		{
			return alloc<registry_data::sMaxNumValues>(m_reg->m_values, m_reg->m_values_size);
		}

		API type_data& alloc_type() override
		{
			return alloc<registry_data::sMaxNumTypes>(m_reg->m_types, m_reg->m_types_size);
		}
		API data_data& alloc_data() override
		{
			return alloc<registry_data::sMaxNumData>(m_reg->m_datas, m_reg->m_data_size);
		}
		API func_data& alloc_func() override
		{
			return alloc<registry_data::sMaxNumFuncs>(m_reg->m_funcs, m_reg->m_func_size);
		}
		API const registry_data& get_reg() override
		{
			return *m_reg;
		}
	};

	export API registry_builder begin_registry() { return {}; }

	template<typename T, typename Prev>
	class type_builder : 
		public builder_base,
		public builder_parts::name_part<type_builder<T, Prev>>,
		public builder_parts::type_part<type_builder<T, Prev>>
	{
	public:
		using prev = Prev;
		using type = T;

		type_builder(prev& prev) :
			builder_base(builder_base::get_destination(prev)),
			m_prev(prev),
			m_target(get_destination().alloc_type())
		{

		}

		template<std::derived_from<type_trait> TraitT>
		decltype(auto) trait(TraitT&& trait)
		{
			builder_parts::trait<T>(get_destination(), m_target.m_traits, std::forward<TraitT>(trait));
			return *this;
		}

		prev& end_type()
		{
			return m_prev;
		}

	protected:
		prev& m_prev;
		type_data& m_target;
	};


	struct trait :type_trait
	{
		 
	};
	//void reflect(module_builder& builder)
	//{
	//	builder
	//	.set_name("hello")
	//	.begin_type<float>("float").trait(trait{})
	//	.end_type();
	//}

	//void make_type_dat(registry& reg)
	//{
	//	//struct player
	//	//{
	//	//	int health;
	//	//	void heal() {}
	//	//};

	//	//type_data* new_type = reg.allocate_objects<type_data>();
	//	//new_type->m_reg = reg;

	//	//new_type->m_name = "player";
	//	//
	//	//{
	//	//	static constexpr size_t amount = 1;
	//	//	data_data* ptr = reg.allocate_objects<data_data>(amount);
	//	//	new_type->m_data = { ptr, amount };

	//	//	ptr[0].m_reg = reg;
	//	//	ptr[0].m_name = "health";
	//	//	ptr[0].m_located_inside_type = new_type;
	//	//	ptr[0].m_type = reg.get_type<int>().m_data;
	//	//}

	//	// For each data/func, set 
	//}
}


