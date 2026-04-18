module;
#include <assert.h>

export module runtime_reflection;

export import stl;

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

// TODO enforce no mixing attributes, e.g., one attribute, add data add attri to data, then another attribute to original type. This breaks contiguous span thing

namespace ge::refl
{
	export struct type_id
	{
		auto operator<=>(const type_id&) const = default;

		std::uint32_t m_id{};
	};

	export enum class value_ownership
	{
		viewing, // const-ptr, const-ref
		referencing, // ptr, ref
		owning, // value, r-value
	};

	export class type_trait
	{
	public:
		template<typename T>
		void on_apply() { }
	};

	export class data_trait
	{
	public:
		template<auto PtrToMember>
		void on_apply() {}
	};

	export struct type_info
	{
		API auto operator<=>(const type_info&) const = default;

		template<typename T>
		static const type_info& get_type_info()
		{
			// TODO add name support
			// TODO add hash support
			static type_info info{ .m_debug_name = "Not implemented", .m_id = 0, .m_size = sizeof(T) };
			return info;
		}

		std::string_view m_debug_name{};
		type_id m_id{};
		size_t m_size{};
	};

	export class value
	{
		struct vtable
		{
			virtual ~vtable() = default;
			virtual const type_info& get_type_info() const = 0;
			virtual bool can_copy() const = 0;
			virtual void copy_construct(void* dst, const void* src) const = 0;
			virtual bool can_move() const = 0;
			virtual void move_construct(void* dst, void* src) const = 0;
			virtual void destruct(void* addr) const = 0;
		};

		template<typename T>
		struct vtable_impl final : vtable
		{
			const type_info& get_type_info() const override { return type_info::get_type_info<T>(); }
			bool can_copy() const override { return std::is_copy_constructible_v<T>; };
			void copy_construct(void* dst, const void* src) const override { new (dst)T(*static_cast<const T*>(src)); };
			bool can_move() const override { return std::is_move_constructible_v<T>; };
			void move_construct(void* dst, void* src) const override { new (dst)T(std::move(*static_cast<T*>(src))); };
			void destruct(void* addr) const override
			{
				T& obj = *static_cast<T*>(addr);
				obj.~T();
				delete& obj;
			};
		};

		using vtable_storage = std::array<std::byte, sizeof(vtable)>;

		template<typename T>
		static vtable_storage create_vtable()
		{
			alignas(vtable) vtable_storage dst {};
			new (dst.data())vtable_impl<std::remove_reference_t<T>>();
			return dst;
		}

		const vtable& get_vtable() const
		{
			return *reinterpret_cast<const vtable*>(m_vtable.data());
		}

	public:
		template<typename T>
		static value create_view(const T* obj)
		{
			return value{ value_ownership::viewing, create_vtable<T>(), const_cast<T*>(obj) };
		}

		template<typename T>
		static value create_view(const T& obj)
		{
			return create_view(&obj);
		}

		template<typename T>
		static value create_ref(T* obj)
		{
			return value{ value_ownership::referencing, create_vtable<T>(), obj };
		}

		template<typename T>
		static value create_ref(T& value)
		{
			return create_ref(&value);
		}

		template<typename T>
		static value create_owning(T&& args)
		{
			void* buffer = std::malloc(sizeof(T));
			new (buffer)std::remove_reference_t<T>(std::forward<T>(args));
			return value{ value_ownership::owning, create_vtable<T>(), buffer };
		}

		API value() = default;

		API value(value_ownership ownership, vtable_storage vtable, void* value) :
			m_vtable(vtable),
			m_ownership(ownership),
			m_value(value)
		{
		}

		API value(const value& other) :
			m_vtable(other.m_vtable),
			m_ownership(other.m_ownership),
			m_value(other.m_value)
		{
			if (m_ownership == value_ownership::owning && m_value != nullptr)
			{
				assert(get_vtable().can_copy());
				m_value = std::malloc(get_vtable().get_type_info().m_size);
				get_vtable().copy_construct(m_value, other.m_value);
			}
		}

		API value(value&& other) noexcept :
			m_vtable(std::exchange(other.m_vtable, {})),
			m_ownership(other.m_ownership),
			m_value(std::exchange(other.m_value, nullptr))
		{
		}

		API value& operator=(const value& other)
		{
			if (this == &other)
			{
				return *this;
			}

			clear();

			m_vtable = other.m_vtable;
			m_ownership = other.m_ownership;
			m_value = other.m_value;

			if (this != &other && m_ownership == value_ownership::owning && m_value != nullptr)
			{
				assert(get_vtable().can_copy());
				m_value = std::malloc(get_vtable().get_type_info().m_size);
				get_vtable().copy_construct(m_value, other.m_value);
			}

			return *this;
		}

		API value& operator=(value&& other) noexcept
		{
			if (this == &other)
			{
				return *this;
			}

			clear();

			m_vtable = std::exchange(other.m_vtable, {});
			m_ownership = other.m_ownership;
			m_value = std::exchange(other.m_value, nullptr);

			return *this;
		}

		API ~value()
		{
			clear();
		}

		API void clear()
		{
			if (m_ownership == value_ownership::owning && m_value != nullptr)
			{
				get_vtable().destruct(m_value);
			}
			m_vtable = {};
			m_value = nullptr;
		}

		API const void* const_data() const { return m_value; }

		API void* mutable_data()
		{
			assert(m_ownership != value_ownership::viewing);
			return m_value;
		}


		template<typename T>
		const T* as_constant() const
		{
			return static_cast<const T*>(const_data());
		}

		template<typename T>
		T* as_mutable()
		{
			return static_cast<T*>(mutable_data());
		}

	private:
		alignas(vtable) vtable_storage m_vtable {};
		value_ownership m_ownership{};
		void* m_value{};
	};

	class mutable_copyable_value;
	class mutable_move_only_value;
	class constant_copyable_value;
	class constant_move_only_value;
}

namespace ge::refl::detail
{
	struct data_data;
	struct func_data;
	struct type_data;
	struct registry_data;

	struct data_data
	{
		std::reference_wrapper<const registry_data> m_reg;
		std::reference_wrapper<const type_data> m_type;
		std::string_view m_name{};
		std::span<const value> m_traits{};
	};

	struct type_data
	{
		std::reference_wrapper<const registry_data> m_reg;
		std::reference_wrapper<const type_info> m_info;

		std::string_view m_name{};

		std::span<const value> m_traits{};
		std::span<const data_data> m_data{};
		std::span<const func_data> m_funcs{};
	};

	struct module_data
	{
		std::reference_wrapper<const registry_data> m_reg;

		std::string_view m_name{};

		std::span<const type_data> m_types{};
		std::span<const func_data> m_funcs{};
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


		API detail::module_data& alloc_module()
		{
			return alloc<sMaxNumModules>(m_modules, m_modules_size);
		}

		API value& alloc_value()
		{
			return alloc<sMaxNumValues>(m_values, m_values_size);
		}

		API detail::type_data& alloc_type()
		{
			return alloc<sMaxNumTypes>(m_types, m_types_size);
		}
		API detail::data_data& alloc_data()
		{
			return alloc<sMaxNumData>(m_datas, m_data_size);
		}
		API detail::func_data& alloc_func()
		{
			return alloc<sMaxNumFuncs>(m_funcs, m_func_size);
		}

		auto get_modules() const { return std::span< const module_data>{ m_modules, m_modules_size }; }
		auto get_types() const { return std::span< const type_data>{ m_types, m_types_size }; }
		auto get_values() const { return std::span< const value>{ m_values, m_values_size }; }
		auto get_funcs() const { return std::span< const func_data>{ m_funcs, m_data_size }; }

		// TODO these should be done using containers/custom allocators

		static constexpr size_t sMaxNumModules = 64;
		module_data* m_modules = (module_data*)std::malloc(sMaxNumModules * sizeof(module_data));
		size_t m_modules_size{};

		static constexpr size_t sMaxNumTypes = 1024;
		type_data* m_types = (type_data*)std::malloc(sMaxNumTypes * sizeof(type_data));
		size_t m_types_size{};

		static constexpr size_t sMaxNumValues = 1024;
		value* m_values = (value*)std::malloc(sMaxNumValues * sizeof(value));
		size_t m_values_size{};

		static constexpr size_t sMaxNumFuncs = 1024;
		func_data* m_funcs = (func_data*)std::malloc(sMaxNumFuncs * sizeof(func_data));
		size_t m_func_size{};

		static constexpr size_t sMaxNumData = 1024;
		data_data* m_datas = (data_data*)std::malloc(sMaxNumData * sizeof(data_data));
		size_t m_data_size{};
	};

	template<typename To, typename From >
	auto convert_from_to(const From& from)
	{
		return To(from);
	}

	template<typename To, typename From>
	auto view_as_public_handles(std::span< const From > data) requires std::constructible_from<To, const From&>
	{
		return data | std::ranges::views::transform(convert_from_to<To, From>);
	}
}

namespace ge::refl
{
	export class data_handle
	{
	public:
		API data_handle(const detail::data_data& data) : m_data(data) {}

	private:
		std::reference_wrapper<const detail::data_data> m_data;
	};

	export class func_handle
	{
	public:
		API func_handle(const detail::func_data& data) : m_data(data) {}

	private:
		std::reference_wrapper<const detail::func_data> m_data;
	};

	export class type_handle
	{
	public:
		API type_handle(const detail::type_data& data) : m_data(data) {}

		API const type_info& get_info() const { return m_data.get().m_info; }
		API std::string_view get_name() const { return m_data.get().m_name; }

		API auto types() const { return detail::view_as_public_handles<data_handle>(m_data.get().m_data); }
		API auto funcs() const { return detail::view_as_public_handles<func_handle>(m_data.get().m_funcs); }

	private:
		std::reference_wrapper<const detail::type_data> m_data;
	};

	export class module_handle
	{
	public:
		API module_handle(const detail::module_data& data) : m_data(data) {}

		API std::string_view get_name() const { return m_data.get().m_name; }

		API auto types() const { return detail::view_as_public_handles<type_handle>(m_data.get().m_types); }
		API auto funcs() const { return detail::view_as_public_handles<func_handle>(m_data.get().m_funcs); }

	private:
		std::reference_wrapper<const detail::module_data> m_data;
	};

	export class registry
	{
	public:
		registry(std::unique_ptr<const detail::registry_data> data) :
			m_data(std::move(data))
		{
		}

		API std::optional<type_handle> try_get_type(std::string_view name) const
		{
			auto end = m_data->m_types + m_data->m_types_size;
			auto it = std::find_if(m_data->m_types, end,
				[&](const detail::type_data& type)
				{
					return type.m_name == name;
				});

			if (it == end)
			{
				return std::nullopt;
			}
			return type_handle{ *it };
		}

		API auto modules() const { return detail::view_as_public_handles<module_handle>(m_data->get_modules()); }
		API auto types() const { return detail::view_as_public_handles<type_handle>(m_data->get_types()); }
		API auto funcs() const { return detail::view_as_public_handles<func_handle>(m_data->get_funcs()); }

	private:
		std::unique_ptr<const detail::registry_data> m_data;
	};

	namespace builder
	{
		template<typename T, typename Prev>
		class type_builder;

		class builder_base
		{
		protected:
			API builder_base(detail::registry_data& reg) :
				m_registry(reg)
			{

			}

			API detail::registry_data& get_registry() const
			{
				return m_registry;
			}

			API static detail::registry_data& get_registry(const builder_base& other)
			{
				return other.get_registry();
			}

		private:
			detail::registry_data& m_registry;
		};

		template<typename Derived>
		class type_part
		{
		public:
			template<typename T>
			type_builder<T, Derived> begin_type(std::string_view name)
			{
				return type_builder<T, Derived>(static_cast<Derived&>(*this), name);
			}
		};

		template<auto DataPtr>
		class data_part
		{
		public:

		};

		template<typename TraitT>
		TraitT& trait(detail::registry_data& dest, std::span<const value>& traits, TraitT&& trait)
		{
			value& data = dest.alloc_value();

			if (traits.empty())
			{
				traits = { &data, 1 };
			}
			else
			{
				assert(&data != traits.data() + traits.size());
				traits = { traits.data(), traits.size() + 1 };
			}

			data = value{ dest, std::forward<TraitT>(trait) };
			return *static_cast<TraitT*>( data.mutable_data() );
		}

		export class registry_builder;
	
		class module_builder :
			public builder_base,
			public type_part<module_builder>
		{
		public:
			module_builder(registry_builder& prev, detail::registry_data& destination, std::string_view name) :
				builder_base(destination),
				m_prev(prev),
				m_target( destination.alloc_module() )
			{
				m_target.m_name = name;
				m_target.m_funcs = { destination.m_funcs + destination.m_func_size, 0ull };
				m_target.m_types = { destination.m_types + destination.m_types_size, 0ull };
			}

			registry_builder& end_module()
			{
				m_target.m_funcs = { m_target.m_funcs.data(), static_cast<size_t>(get_registry().m_funcs - m_target.m_funcs.data()) };
				m_target.m_types = { m_target.m_types.data(), static_cast<size_t>(get_registry().m_types - m_target.m_types.data()) };
				return m_prev;
			}

		private:
			registry_builder& m_prev;
			detail::module_data& m_target;
		};

		export class registry_builder
		{
		public:
			virtual ~registry_builder() = default;

			API auto begin_module(std::string_view name)
			{
				return module_builder{ *this, *m_reg, name };
			}

			API registry build()
			{
				return registry{ std::move(m_reg) };
			}

		protected:
			std::unique_ptr<detail::registry_data> m_reg = std::make_unique<detail::registry_data>();
		};

		template<typename T, typename Prev>
		class type_builder :
			public builder_base,
			public type_part<type_builder<T, Prev>>
		{
		public:
			using prev = Prev;
			using type = T;

			type_builder(prev& prev, std::string_view name) :
				builder_base(builder_base::get_registry(prev)),
				m_prev(prev),
				m_target(get_registry().alloc_type())
			{
				m_target = detail::type_data{ .m_reg = get_registry(), .m_info = type_info::get_type_info<T>(), .m_name = name };
			}

			template<std::derived_from<type_trait> TraitT>
			decltype(auto) trait(TraitT&& trait)
			{
				builder::trait<T>(get_registry(), m_target.m_traits, std::forward<TraitT>(trait));
				return *this;
			}

			prev& end_type()
			{
				return m_prev;
			}

		protected:
			prev& m_prev;
			detail::type_data& m_target;
		};

		template<auto PtrToMember, typename Prev>
		class data_builder :
			public builder_base
		{
		public:
			using prev = Prev;

			// TODO add vaildation that its actually a member of prev::type
			// TODO maybe add support for global variables. Would probably have to be a new builderl

			data_builder(prev& prev, std::string_view name) :
				builder_base(builder_base::get_registry(prev)),
				m_prev(prev),
				m_target(get_registry().alloc_data())
			{
				m_target = detail::data_data{ .m_reg = get_registry().get_reg(), .m_name = name };
			}

			template<std::derived_from<data_trait> TraitT>
			decltype(auto) trait(TraitT&& trait)
			{
				data_trait& trait = builder::trait(get_registry(), m_target.m_traits, std::forward<TraitT>(trait));
				return *this;
			}

			prev& end_type()
			{
				return m_prev;
			}

		protected:
			prev& m_prev;
			detail::data_data& m_target;
		};
	}

	

	export API builder::registry_builder begin_registry() { return {}; }
}