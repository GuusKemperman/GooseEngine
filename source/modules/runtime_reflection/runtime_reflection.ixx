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

	// TODO error handling for unsupported types, such as T**

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
	
	namespace detail
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

	export template<undecorated T>
	consteval type_id make_type_id()
	{
		return { detail::hash(__FUNCTION__) };
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

	template<typename Base> requires (sizeof(Base) == sizeof(size_t))
	class inplace_vtable
	{
		size_t m_vtable{};

	public:
		const Base* operator->() const { return std::bit_cast<const Base*>(&m_vtable);  }

		template<std::derived_from<Base> Derived> requires (sizeof(Derived) == sizeof(Base))
		void set()
		{
			void* dst = &m_vtable;
			new (dst)Derived();
		}
	};

	export class value
	{
		struct vtable
		{
			virtual ~vtable() = default;
			virtual type_id get_type_id() const = 0;
			virtual size_t get_size() const = 0;
			virtual bool can_copy() const = 0;
			virtual void copy_construct(void* dst, const void* src) const = 0;
			virtual bool can_move() const = 0;
			virtual void move_construct(void* dst, void* src) const = 0;
			virtual void destruct(void* addr) const = 0;
		};

		template<undecorated T>
		struct vtable_impl final : vtable
		{
			type_id get_type_id() const override { return make_type_id<T>(); }
			size_t get_size() const override { return sizeof(T); }
			bool can_copy() const override { return std::is_copy_constructible_v<T>; };
			void copy_construct(void* dst, const void* src) const override { new (dst)T(*static_cast<const T*>(src)); };
			bool can_move() const override { return std::is_move_constructible_v<T>; };
			void move_construct(void* dst, void* src) const override { new (dst)T(std::move(*static_cast<T*>(src))); };
			void destruct(void* addr) const override
			{
				T& obj = *static_cast<T*>(addr);
				obj.~T();
			};
		};

		template<typename T>
		static inplace_vtable<vtable> create_vtable()
		{
			inplace_vtable<vtable> dst{};
			dst.set<vtable_impl<T>>();
			return dst;
		}

		API value(inplace_vtable<vtable> vtable, void* value, bool is_mutable, bool is_owning) : 
			m_vtable(vtable),
			m_value(value),
			m_is_mutable(is_mutable),
			m_is_owning(is_owning)
		{
		}

	public:
		template<undecorated T>
		static value create_view(const T* obj) requires !std::is_same_v<remove_decoration_t<T>, value>
		{
			return value{ create_vtable<T>(), const_cast<T*>(obj), false, false };
		}

		template<undecorated T>
		static value create_view(const T& obj) requires !std::is_same_v<remove_decoration_t<T>, value>
		{
			return create_view(&obj);
		}

		API static value create_view(const value& obj)
		{
			return value{ obj.m_vtable, obj.m_value, false, false };
		}

		template<undecorated T>
		static value create_ref(T* obj) requires !std::is_same_v<remove_decoration_t<T>, value>
		{
			return value{ create_vtable<T>(), obj, true, false };
		}

		template<undecorated T>
		static value create_ref(T& obj) requires !std::is_same_v<remove_decoration_t<T>, value>
		{
			return create_ref(&obj);
		}

		API static value create_ref(value& obj)
		{
			assert(obj.m_is_mutable);
			return value{ obj.m_vtable, obj.m_value, true, false };
		}

		template<typename T>
		static value create_owning(T&& args)
		{
			// TODO error checking for unsupported types
			using undecorated = remove_decoration_t<T>;
			void* buffer = std::malloc(sizeof(undecorated));
			new (buffer)undecorated(std::forward<T>(args));
			return value{ create_vtable<undecorated>(), buffer, true, true };
		}

		API value() = default;

		API value(const value& other) :
			m_vtable(other.m_vtable),
			m_value(other.m_value),
			m_is_mutable(other.m_is_mutable),
			m_is_owning(other.m_is_owning)
		{
			if (m_is_owning && m_value != nullptr)
			{
				assert(m_vtable->can_copy());
				m_value = std::malloc(m_vtable->get_size());
				m_vtable->copy_construct(m_value, other.m_value);
			}
		}

		API value(value&& other) noexcept :
			m_vtable(other.m_vtable),
			m_value(std::exchange(other.m_value, nullptr)),
			m_is_mutable(other.m_is_mutable),
			m_is_owning(other.m_is_owning)
		{
		}

		API operator bool() const { return m_value != nullptr; }

		API value& operator=(const value& other)
		{
			if (this == &other)
			{
				return *this;
			}

			clear();

			m_vtable = other.m_vtable;
			m_value = other.m_value;
			m_is_mutable = other.m_is_mutable;
			m_is_owning = other.m_is_owning;

			if (m_is_owning && m_value != nullptr)
			{
				assert(m_vtable->can_copy());
				m_value = std::malloc(m_vtable->get_size());
				m_vtable->copy_construct(m_value, other.m_value);
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

			m_vtable = other.m_vtable;
			m_value = std::exchange(other.m_value, nullptr);
			m_is_mutable = other.m_is_mutable;
			m_is_owning = other.m_is_owning;

			return *this;
		}

		API ~value()
		{
			clear();
		}

		API void clear()
		{
			if (m_is_owning && m_value != nullptr)
			{
				m_vtable->destruct(m_value);
				std::free(m_value);
			}
			m_vtable = {};
			m_value = nullptr;
			m_is_owning = false;
			m_is_mutable = false;
		}

		API const void* const_data() const { return m_value; }

		API void* mutable_data()
		{
			assert(m_is_mutable);
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

		API type_id get_type_id() const { return m_vtable->get_type_id(); }

		API bool is_mutable() const { return m_is_mutable; }
		API bool is_owning() const { return m_is_owning; }

	private:
		inplace_vtable<vtable> m_vtable{};
		void* m_value{};
		std::uint8_t m_is_mutable : 1{};
		std::uint8_t m_is_owning : 1{};
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
		
		std::string_view m_name{};

		std::span<const value> m_traits{};
		std::span<const data_data> m_data{};
		std::span<const func_data> m_funcs{};
		
		type_id m_id{};
	};

	struct module_data
	{
		std::reference_wrapper<const registry_data> m_reg;

		std::string_view m_name{};

		std::span<const type_data> m_types{};
		std::span<const func_data> m_funcs{};
	};

	template<typename T>
	struct is_supported_param_type : std::bool_constant<undecorated<T>>
	{
	};

	template<undecorated T>
	struct is_supported_param_type<T&> : std::bool_constant<true>
	{
	};

	template<undecorated T>
	struct is_supported_param_type<const T&> : std::bool_constant<true>
	{
	};

	export template<typename T>
	concept supported_param_type = is_supported_param_type<T>::value;

	struct func_data
	{
		struct vtable
		{
			virtual ~vtable() = default;
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
					auto forwardCast = [&]<typename ParamT, size_t Idx>() -> ParamT
					{
						if constexpr (std::is_const_v<ParamT> || undecorated<ParamT>)
						{
							return *static_cast<std::remove_reference_t<std::add_const_t<ParamT>>*>(args[Idx].const_data());
						}
						else
						{
							return *static_cast<std::remove_reference_t<ParamT>*>(args[Idx].mutable_data());
						}
					};

					if constexpr (std::is_same_v<Ret, void>)
					{
						FuncPtr(forwardCast.template operator() < Params, Indices > ()...);
						return value{};
					}
					else
					{
						// TODO support references
						return value::create_owning(FuncPtr(forwardCast.template operator() < Params, Indices > ()...));
					}
				}(std::make_index_sequence<sizeof...(Params)>());
			}
		};

		inplace_vtable<vtable> m_vtable{};
		
		std::string_view m_name{};
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

		API module_data& alloc_module()
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
		auto get_funcs() const { return std::span< const func_data>{ m_funcs, m_func_size }; }

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

	export template<typename>
	class func_strong_handle{};

	export class func_handle
	{
	public:
		API func_handle(const detail::func_data& data) : m_data(data) {}

		API std::string_view get_name() const { return m_data.get().m_name; }

		template<typename... Args>
		value invoke_unchecked(Args&&... args) const
		{
			std::array<value, sizeof...(Args)> packed{
				
				[]<typename Arg>(Arg&& arg) -> value
				{
					if constexpr (std::is_same_v<remove_decoration_t<Arg>, value>)
					{
						return arg.is_mutable() ? value::create_ref(arg) : value::create_view(arg);
					}
					else if constexpr (std::is_const_v<Arg>)
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

		std::reference_wrapper<const detail::func_data> m_data;
	};

	export class type_handle
	{
	public:
		API type_handle(const detail::type_data& data) : m_data(data) {}

		API type_id get_id() const { return m_data.get().m_id; }
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
		template<undecorated T, typename Prev>
		class type_builder;

		template<auto FuncPtr, typename Prev> requires is_func<FuncPtr>
		class func_builder;
		
		class builder_destination
		{
		public:
			virtual ~builder_destination() = default;

			// Allocates pointer-stable object in contiguous buffer
			virtual value& alloc_value() = 0;
			virtual detail::type_data& alloc_type() = 0;
			virtual detail::data_data& alloc_data() = 0;
			virtual detail::func_data& alloc_func() = 0;

			virtual const detail::registry_data& get_reg() = 0;
		};

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

		template<typename Derived>
		class func_part
		{
		public:
			template<auto FuncPtr> requires is_func<FuncPtr>
			func_builder<FuncPtr, Derived> begin_func(std::string_view name)
			{
				return func_builder<FuncPtr, Derived>(static_cast<Derived&>(*this), name);
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
			public type_part<module_builder>,
			public func_part<module_builder>
		{
		public:
			module_builder(registry_builder& prev, detail::registry_data& destination, std::string_view name) :
				builder_base(destination),
				m_prev(prev),
				m_target( destination.alloc_module() )
			{
				m_target.m_name = name;
				m_target.m_funcs = { destination.m_funcs + destination.m_func_size, 0ull};
				m_target.m_types = { destination.m_types + destination.m_types_size, 0ull };
			}

			API registry_builder& end_module()
			{
				// TODO: pretty ugly
				m_target.m_funcs = { m_target.m_funcs.data(), static_cast<size_t>(get_registry().m_funcs + get_registry().m_func_size - m_target.m_funcs.data())};
				m_target.m_types = { m_target.m_types.data(), static_cast<size_t>(get_registry().m_types + get_registry().m_types_size - m_target.m_types.data())};
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

		template<undecorated T, typename Prev>
		class type_builder :
			public builder_base,
			public type_part<type_builder<T, Prev>>,
			public func_part<type_builder<T, Prev>>
		{
		public:
			using prev = Prev;
			using type = T;

			type_builder(prev& prev, std::string_view name) :
				builder_base(builder_base::get_registry(prev)),
				m_prev(prev),
				m_target(get_registry().alloc_type())
			{
				m_target = detail::type_data{ .m_reg = get_registry(), .m_name = name,  .m_id = make_type_id<T>() };
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
				m_target = detail::data_data{ .m_reg = get_registry(), .m_name = name };
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

		template<auto FuncPtr, typename Prev> requires is_func<FuncPtr>
		class func_builder : 
			public builder_base
		{
		public:
			using prev = Prev;

			func_builder(prev& prev, std::string_view name) :
				builder_base(builder_base::get_registry(prev)),
				m_prev(prev),
				m_target(get_registry().alloc_func())
			{
				m_target = detail::func_data{
					.m_name = name,
					.m_reg = get_registry(),
				};

				m_target.m_vtable.set<detail::func_data::vtable_impl<FuncPtr, func_sig_t<decltype(FuncPtr)>>>();
			}

			prev& end_func()
			{
				return m_prev;
			}

		protected:
			prev& m_prev;
			detail::func_data& m_target;
		};
	}

	export API builder::registry_builder begin_registry() { return {}; }
}

