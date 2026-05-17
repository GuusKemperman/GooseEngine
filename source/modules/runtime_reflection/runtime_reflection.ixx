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
	export class type_handle;
	export class func_handle;
	export class data_handle;

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
		return { detail::hash(__FUNCSIG__) };
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
			void copy_construct(void* dst, const void* src) const override
			{
				if constexpr (std::is_copy_constructible_v<T>)
				{
					new (dst)T(*static_cast<const T*>(src));
				}
				else
				{
					assert(false && "Cannot copy construct");
				}
			};
			bool can_move() const override { return std::is_move_constructible_v<T>; };
			void move_construct(void* dst, void* src) const override
			{
				if constexpr (std::is_move_constructible_v<T>)
				{
					new (dst)T(std::move(*static_cast<T*>(src)));
				}
				else
				{
					assert(false && "Cannot move construct");
				}
			};
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
		std::span<const data_data> m_datas{};
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
			auto* result = try_get_type_data(self, name);
			assert(result != nullptr);
			return *result;
		}


		API type_data* try_get_type_data(this auto&& self, type_id id)
		{
			auto end = self.m_types + self.m_types_size;
			auto it = std::find_if(self.m_types, end,
				[&](const type_data& type)
				{
					return type.m_id == id;
				});

			if (it != end)
			{
				return it;
			}
			return nullptr;
		}

		API auto& get_type_data(this auto&& self, type_id id)
		{
			auto* result = self.try_get_type_data(id);
			assert(result != nullptr);
			return *result;
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

		API type_data& alloc_type()
		{
			return alloc<sMaxNumTypes>(m_types, m_types_size);
		}
		API data_data& alloc_data()
		{
			return alloc<sMaxNumData>(m_datas, m_data_size);
		}
		API func_data& alloc_func()
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

		API std::string_view get_name() const { return m_data.get().m_name; }

		auto get_type(this const auto& self) { return type_handle{ self.m_data.get().m_type.type_data }; }

		// The type this member is located in
		auto get_outer_type(this const auto& self) { return type_handle{ self.m_data.get().m_outer_type }; }

		using setter_t = detail::data_data::setter_t;
		using getter_t = detail::data_data::getter_t;

		API setter_t get_setter() const { return m_data.get().m_set; }
		API getter_t get_getter() const { return m_data.get().m_get; }

		API auto traits() const { return m_data.get().m_traits; }

	private:
		std::reference_wrapper<const detail::data_data> m_data;
	};

	export class func_handle
	{
	public:
		API func_handle(const detail::func_data& data) : m_data(data) {}

		API std::string_view get_name() const { return m_data.get().m_name; }

		// The type this member is located in
		// API type_handle get_outer_type() const;

		API auto traits() const { return m_data.get().m_traits; }

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

		API auto datas() const { return detail::view_as_public_handles<data_handle>(m_data.get().m_data); }
		API auto funcs() const { return detail::view_as_public_handles<func_handle>(m_data.get().m_funcs); }
		API auto traits() const { return m_data.get().m_traits; }
		
	private:
		std::reference_wrapper<const detail::type_data> m_data;
	};

	export class module_handle
	{
	public:
		API module_handle(const detail::module_data& data) : m_data(data) {}

		API std::string_view get_name() const { return m_data.get().m_name; }
		
		API auto datas() const { return detail::view_as_public_handles<data_handle>(m_data.get().m_datas); }
		API auto types() const { return detail::view_as_public_handles<type_handle>(m_data.get().m_types); }
		API auto funcs() const { return detail::view_as_public_handles<func_handle>(m_data.get().m_funcs); }

	private:
		std::reference_wrapper<const detail::module_data> m_data;
	};

	export struct type_trait
	{
	};

	export struct data_trait
	{
	};

	export struct func_trait
	{
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
		class builder_base;

		export template<undecorated T>
		class type_builder;

		export template<undecorated T, std::derived_from<builder_base> Prev>
		class endable_type_builder;

		export template<auto FuncPtr> requires is_func<FuncPtr>
		class func_builder;
		
		export template<auto FuncPtr, std::derived_from<builder_base> Prev> requires is_func<FuncPtr>
		class endable_func_builder;

		export template<auto DataPtr> requires is_data<DataPtr>
		class data_builder;

		export template<auto DataPtr, std::derived_from<builder_base> Prev> requires is_data<DataPtr>
		class endable_data_builder;

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

		class type_part
		{
		public:
			template<undecorated T>
			auto begin_type(this auto&& self, std::string_view name)
			{
				return endable_type_builder<T, remove_decoration_t<decltype(self)>>{ self, name };
			}
		};

		class func_part
		{
		public:
			template<auto FuncPtr> requires is_func<FuncPtr>
			auto begin_func(this auto&& self, std::string_view name)
			{
				return endable_func_builder<FuncPtr, remove_decoration_t<decltype(self)>>{ self, name };
			}
		};

		class data_part
		{
		public:
			template<auto DataPtr> requires is_data<DataPtr>
			auto begin_data(this auto&& self, std::string_view name)
			{
				return endable_data_builder<DataPtr, remove_decoration_t<decltype(self)>>{ self, name };
			}
		};

		template<typename TraitBase>
		class trait_part
		{
		public:
			template<std::derived_from<TraitBase> TraitT>
			decltype(auto) trait(this auto&& self, TraitT&& trait = {})
			{
				detail::registry_data& reg = self.get_registry(self);
				value& data = reg.alloc_value();
				std::span<const value>& traits = self.m_target.m_traits;

				if (traits.empty())
				{
					traits = { &data, 1 };
				}
				else
				{
					assert(&data != traits.data() + traits.size());
					traits = { traits.data(), traits.size() + 1 };
				}

				// TODO most of our traits will be constexpr, wasteful to make a copy.
				// TODO ensure traits are not accessible as mutable
				//if constexpr (requires
				//{
				//	
				//})
				//{
				//	
				//}


				data = value::create_owning(std::forward<TraitT>(trait));
				return self;
			}
		};

		export class registry_builder;
	
		class module_builder :
			public builder_base,
			public type_part,
			public func_part
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
				m_target.m_datas = { destination.m_datas + destination.m_data_size, 0ull };
			}

			API registry_builder& end_module()
			{
				// TODO: pretty ugly
				m_target.m_funcs = { m_target.m_funcs.data(), static_cast<size_t>(get_registry().m_funcs + get_registry().m_func_size - m_target.m_funcs.data())};
				m_target.m_types = { m_target.m_types.data(), static_cast<size_t>(get_registry().m_types + get_registry().m_types_size - m_target.m_types.data())};
				m_target.m_datas = { m_target.m_datas.data(), static_cast<size_t>(get_registry().m_datas + get_registry().m_data_size - m_target.m_datas.data())};
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
				for (detail::data_data& data : std::span{ m_reg->m_datas, m_reg->m_data_size })
				{
					data.m_type.type_data = m_reg->get_type_data(data.m_type.type_id);
				}

				return registry{ std::move(m_reg) };
			}

		protected:
			std::unique_ptr<detail::registry_data> m_reg = std::make_unique<detail::registry_data>();
		};

		template<undecorated T>
		class type_builder :
			public builder_base,
			public type_part,
			public func_part,
			public data_part,
			public trait_part<type_trait>
		{
		public:
			using type = T;

			type_builder(detail::registry_data& reg, std::string_view name) :
				builder_base(reg),
				m_target(reg.alloc_type())
			{
				m_target.m_funcs = { reg.m_funcs + reg.m_func_size, 0ull };
				m_target = detail::type_data{ .m_reg = get_registry(), .m_name = name,  .m_id = make_type_id<T>() };
			}

		protected:
			friend trait_part;
			detail::type_data& m_target;
		};

		template<undecorated T, std::derived_from<builder_base> Prev>
		class endable_type_builder : 
			public type_builder<T>
		{
		public:
			endable_type_builder(Prev& prev, std::string_view name) :
				type_builder<T>(builder_base::get_registry(prev), name),
				m_prev(prev)
			{
			}
				

			Prev& end_type()
			{
				// TODO pretty ugly
				detail::registry_data& reg = builder_base::get_registry(m_prev);
				detail::type_data& target = type_builder<T>::m_target;
				target.m_funcs = { target.m_funcs.data(), static_cast<size_t>(reg.m_funcs + reg.m_func_size - target.m_funcs.data()) };
				return m_prev;
			}

		private:
			Prev& m_prev;
		};

		template<auto PtrToMember> requires is_data<PtrToMember>
		class data_builder :
			public builder_base,
			public trait_part<data_trait>
		{
		public:
			using data_ptr_t = data_ptr<decltype(PtrToMember)>;

			using outer_t = data_ptr_t::outer_type_t;
			using data_t = data_ptr_t::data_t;
			
			data_builder(detail::registry_data& reg, std::string_view name) :
				builder_base(reg),
				m_target(reg.alloc_data())
			{
				m_target = detail::data_data{
					.m_reg = reg,
					.m_type = detail::cached_type_data_ref{ { data_type_id } },
					.m_outer_type = reg.get_type_data(outer_type_id),
					.m_set = +[](value target_object, const value& new_value)
					{
						auto [outer, data] = get_setter_args(target_object, new_value);
						outer.*PtrToMember = data;
					},
					.m_get = +[](const value& target_object) -> value
					{
						const outer_t& outer = get_getter_args(target_object);
						return value::create_view(outer.*PtrToMember);
					},
					.m_name = name };
			}


			template<auto Setter> requires std::is_same_v<decltype(Setter), std::nullptr_t>
			decltype(auto) setter(this auto&& self)
			{
				self.m_target.m_set = nullptr;
				return self;
			}

			template<auto Setter> requires std::is_invocable_v<decltype(Setter), outer_t&, data_t>
			decltype(auto) setter(this auto&& self)
			{
				self.m_target.m_set = +[](value target_object, const value& new_value)
					{
						auto [outer, data] = get_setter_args(target_object, new_value);
						std::invoke(Setter, outer, data);
					};
				return self;
			}

			template<auto Getter> requires std::is_same_v<decltype(Getter), std::nullptr_t>
			decltype(auto) getter(this auto&& self)
			{
				self.m_target.m_get = nullptr;
				return self;
			}

			template<auto Getter> requires std::is_invocable_r_v<const data_t&, decltype(Getter), const outer_t&>
			decltype(auto) getter(this auto&& self)
			{
				self.m_target.m_get = +[](const value& target_object)
					{
						const outer_t& outer = get_getter_args(target_object);
						decltype(auto) result = std::invoke(Getter, outer);
						
						if constexpr (std::is_reference_v<decltype(result)>)
						{
							return value::create_view(result);
						}
						else
						{
							return value::create_owning(result);
						}
					};
				return self;
			}

		protected:
			friend trait_part;
			
			static std::pair<outer_t&, const data_t&> get_setter_args(value target_object, const value& new_value)
			{
				assert(target_object && new_value);
				assert(target_object.is_mutable());
				assert(target_object.get_type_id() == outer_type_id);
				assert(new_value.get_type_id() == data_type_id);

				outer_t* outer = target_object.as_mutable<outer_t>();
				const data_t* data = new_value.as_constant<data_t>();

				return { *outer, *data };
			}

			static const outer_t& get_getter_args(const value& target_object)
			{
				assert(target_object);
				assert(target_object.get_type_id() == outer_type_id);

				const outer_t* outer = target_object.as_constant<outer_t>();

				return { *outer };
			}

			static constexpr type_id data_type_id = make_type_id<data_t>();
			static constexpr type_id outer_type_id = make_type_id<outer_t>();

			detail::data_data& m_target;
		};

		export template<auto DataPtr, std::derived_from<builder_base> Prev> requires is_data<DataPtr>
		class endable_data_builder final : 
			public data_builder<DataPtr>
		{
		public:
			static_assert(std::is_base_of_v<type_builder<typename data_builder<DataPtr>::outer_t>, Prev>,
				"This data member does not belong to the type it's being attached to");

			endable_data_builder(Prev& prev, std::string_view name) :
				data_builder<DataPtr>(builder_base::get_registry(prev), name),
				m_prev(prev)
			{
			}

			Prev& end_data()
			{
				return m_prev;
			}

		private:
			Prev& m_prev;
		};

		template<auto FuncPtr> requires is_func<FuncPtr>
		class func_builder : 
			public builder_base,
			public trait_part<func_trait>
		{
		public:
			func_builder(detail::registry_data& reg, std::string_view name) :
				builder_base(reg),
				m_target(reg.alloc_func())
			{
				m_target = detail::func_data{
					.m_name = name,
					.m_reg = reg,
				};

				m_target.m_vtable.set<detail::func_data::vtable_impl<FuncPtr, func_sig_t<decltype(FuncPtr)>>>();
			}

		protected:
			friend trait_part;
			detail::func_data& m_target;
		};

		template<auto FuncPtr, std::derived_from<builder_base> Prev> requires is_func<FuncPtr>
		class endable_func_builder : 
			public func_builder<FuncPtr>
		{
		public:
			endable_func_builder(Prev& prev, std::string_view name) :
				func_builder<FuncPtr>(builder_base::get_registry(prev), name),
				m_prev(prev)
			{
				
			}

			Prev& end_func()
			{
				return m_prev;
			}

		private:
			Prev& m_prev;
		};
		
		export struct example_trait : func_trait, type_trait, data_trait
		{
			// on_apply is called immediately, while post_build is guaranteed to be called only after 
			// the registry is completely build

			template<auto FuncPtr>
			void on_apply([[maybe_unused]] builder::func_builder<FuncPtr>& builder)
			{

			}

			template<auto DataPtr>
			void on_apply([[maybe_unused]] builder::data_builder<DataPtr>& builder)
			{

			}

			template<typename T>
			void on_apply([[maybe_unused]] builder::type_builder<T>& builder)
			{

			}

			template<auto FuncPtr> // template is optional
			void post_build([[maybe_unused]] func_handle handle)
			{

			}

			template<auto DataPtr> // template is optional
			void post_build([[maybe_unused]] data_handle handle)
			{

			}

			template<typename T> // template is optional
			void post_build([[maybe_unused]] type_handle handle)
			{

			}

			void post_build([[maybe_unused]] func_handle handle)
			{

			}

			void post_build([[maybe_unused]] data_handle handle)
			{

			}

			void post_build([[maybe_unused]] type_handle handle)
			{

			}
		};

	}

	export API builder::registry_builder begin_registry() { return {}; }
}

