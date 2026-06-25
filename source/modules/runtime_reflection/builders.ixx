module;
#include <assert.h>

export module runtime_reflection:builders;

import stl;
import :details;
import :data;

// TODO enforce no mixing attributes, e.g., one attribute, add data add attri to data, then another attribute to original type. This breaks contiguous span thing
// TODO enums
// TODO access (private/public)


namespace ge::refl::builders
{
	class builder_base;

	export class registry_builder;

	export class endable_registry_builder;

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

	template<typename TraitBase>
	class trait_part;

	class builder_destination
	{
	public:
		virtual ~builder_destination() = default;

		// Allocates pointer-stable object in contiguous buffer
		virtual value& alloc_value() = 0;
		virtual type_data& alloc_type() = 0;
		virtual data_data& alloc_data() = 0;
		virtual func_data& alloc_func() = 0;

		virtual const registry_data& get_reg() = 0;
	};

	class builder_base
	{
	protected:
		API builder_base(registry_builder& reg_builder) :
			m_registry_builder(reg_builder)
		{
		}

		API registry_data& get_registry() const;

		API registry_builder& get_registry_builder() const { return m_registry_builder; }

	private:
		friend trait_part;
		registry_builder& m_registry_builder;
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
		template<std::derived_from<TraitBase>... TraitsT>
		decltype(auto) add_traits(this auto&& self, TraitsT&&... traits)
		{
			registry_data& reg = self.get_registry();

			std::span<const value>& storedTraits = self.m_target.m_traits;
			value* begin = reg.m_values.end() - storedTraits.size();

			assert(storedTraits.empty() || storedTraits.data() == begin && "Traits were added non-contiguously");

			([&]<typename TraitT>(TraitT && trait)
			{
				// TODO most of our traits will be constexpr, wasteful to make a copy.
				value& data = reg.m_values.push_back(value::create_owning(std::forward<TraitT>(trait)));

				storedTraits = { begin, storedTraits.size() + 1 };

				if constexpr (requires (TraitT & mutTrait)
				{
					mutTrait.on_apply(self);
				})
				{
					data.as_mutable<TraitT>()->on_apply(self);
				}

				using data_t = std::remove_reference_t<decltype(self.m_target)>;
				using handle_t = data_t::handle_t;

				if constexpr (requires (TraitT & mutTrait, handle_t handle)
				{
					mutTrait.post_build(handle);
				})
				{
					registry_builder& reg_builder = self.get_registry_builder();
					reg_builder.post_build_events.push_back(
						{
							.m_invoke = +[](void* data, value* trait)
							{
								handle_t handle{ *static_cast<data_t*>(data) };
								trait->as_mutable<TraitT>()->post_build(handle);
							},
							.m_trait = &data,
							.m_data = &self.m_target
						}
					);
				}
			}.template operator() < TraitsT > (std::forward<TraitsT>(traits)), ...);

			return std::forward<decltype(self)>(self);
		}
	};

	class module_builder :
		public builder_base,
		public type_part,
		public func_part
	{
	public:
		API module_builder(const builder_base& prev, std::string_view name) :
			builder_base(prev),
			m_target(get_registry().m_modules.push_back(module_data{ .m_name = name }))
		{
			registry_data& reg = get_registry();
			m_target.m_name = name;
			m_target.m_funcs = { reg.m_funcs.end(), 0ull };
			m_target.m_types = { reg.m_types.end(), 0ull };
			m_target.m_datas = { reg.m_datas.end(), 0ull };
		}

	protected:
		module_data& m_target;
	};

	export template<std::derived_from<builder_base> Prev>
		class endable_module_builder :
		public module_builder
	{
	public:
		endable_module_builder(Prev& prev, std::string_view name) :
			module_builder(prev, name),
			m_prev(prev)
		{
		}

		API Prev& end_module()
		{
			registry_data& reg = builder_base::get_registry();
			module_data& target = module_builder::m_target;
			// TODO: pretty ugly
			target.m_funcs = { target.m_funcs.data(), static_cast<size_t>(reg.m_funcs.end() - target.m_funcs.data()) };
			target.m_types = { target.m_types.data(), static_cast<size_t>(reg.m_types.end() - target.m_types.data()) };
			target.m_datas = { target.m_datas.data(), static_cast<size_t>(reg.m_datas.end() - target.m_datas.data()) };
			return m_prev;
		}

	protected:
		Prev& m_prev;
	};

	export class registry_builder
		: public builder_base
	{
		auto& self() { return *this; }
	public:
		registry_builder() : builder_base(self()) {}

		virtual ~registry_builder() = default;

		API auto begin_module(this auto&& self, std::string_view name)
		{
			return endable_module_builder< remove_decoration_t<decltype(self)> >{ self, name };
		}

	protected:
		friend builder_base;
		friend trait_part;
		std::unique_ptr<registry_data> m_reg = std::make_unique<registry_data>();

		struct post_build_event
		{
			void(*m_invoke)(void*, value*);
			value* m_trait{};
			void* m_data{};
		};
		std::vector<post_build_event> post_build_events{};
	};

	export class endable_registry_builder : public registry_builder
	{
	public:
		API std::unique_ptr<registry_data> build() &&
		{
			for (data_data& data : m_reg->m_datas)
			{
				data.m_type.type_data = *std::ranges::find_if(m_reg->m_types,
					[&data](const type_data& type_data)
					{
						return type_data.m_id == data.m_type.type_id;
					});
			}

			for (post_build_event& post_build : post_build_events)
			{
				post_build.m_invoke(post_build.m_data, post_build.m_trait);
			}
			post_build_events.clear();

			for (value& trait : m_reg->m_values)
			{
				trait.make_constant();
			}

			return std::move(m_reg);
		}
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

		type_builder(const builder_base& prev, std::string_view name) :
			builder_base(prev),
			m_target(
				[&]() -> decltype(auto)
				{
					registry_data& reg = get_registry();
					assert(!std::ranges::any_of(reg.m_types, [](const type_data& existing)
						{
							return existing.m_id == make_type_id<T>();
						}));
					return reg.m_types.push_back(type_data{ .m_name = name,  .m_id = make_type_id<T>() });
				}()
					)
		{
		}

	protected:
		friend trait_part;
		friend data_builder;
		type_data& m_target;
	};

	template<undecorated T, std::derived_from<builder_base> Prev>
	class endable_type_builder :
		public type_builder<T>
	{
	public:
		endable_type_builder(Prev& prev, std::string_view name) :
			type_builder<T>(prev, name),
			m_prev(prev)
		{
			registry_data& reg = builder_base::get_registry();
			type_data& target = type_builder<T>::m_target;

			target.m_funcs = { reg.m_funcs.end(), 0ull };
			target.m_data = { reg.m_datas.end(), 0ull };
		}

		Prev& end_type()
		{
			// TODO pretty ugly
			registry_data& reg = builder_base::get_registry();
			type_data& target = type_builder<T>::m_target;
			target.m_funcs = { target.m_funcs.data(), static_cast<size_t>(reg.m_funcs.end() - target.m_funcs.data()) };
			target.m_data = { target.m_data.data(), static_cast<size_t>(reg.m_datas.end() - target.m_data.data()) };
			return m_prev;
		}

	protected:
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

		data_builder(const type_builder<outer_t>& prev, std::string_view name) :
			builder_base(prev),
			m_target(get_registry().m_datas.push_back(data_data{
				.m_type = cached_type_data_ref{ { data_type_id } },
				.m_outer_type = prev.m_target,
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
				.m_name = name }))
		{
		}

		template<auto Setter> requires std::is_same_v<decltype(Setter), std::nullptr_t>
		decltype(auto) setter(this auto&& self)
		{
			self.m_target.m_set = nullptr;
			return std::forward<decltype(self)>(self);
		}

		template<auto Setter> requires std::is_invocable_v<decltype(Setter), outer_t&, data_t>
		decltype(auto) setter(this auto&& self)
		{
			self.m_target.m_set = +[](value target_object, const value& new_value)
				{
					auto [outer, data] = get_setter_args(target_object, new_value);
					std::invoke(Setter, outer, data);
				};
			return std::forward<decltype(self)>(self);
		}

		template<auto Getter> requires std::is_same_v<decltype(Getter), std::nullptr_t>
		decltype(auto) getter(this auto&& self)
		{
			self.m_target.m_get = nullptr;
			return std::forward<decltype(self)>(self);
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
			return std::forward<decltype(self)>(self);
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

		data_data& m_target;
	};

	export template<auto DataPtr, std::derived_from<builder_base> Prev> requires is_data<DataPtr>
		class endable_data_builder final :
		public data_builder<DataPtr>
	{
	public:
		static_assert(std::is_base_of_v<type_builder<typename data_builder<DataPtr>::outer_t>, Prev>,
			"This data member does not belong to the type it's being attached to");

		endable_data_builder(Prev& prev, std::string_view name) :
			data_builder<DataPtr>(prev, name),
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
		func_builder(const builder_base& prev, std::string_view name) :
			builder_base(prev),
			m_target(get_registry().m_funcs.push_back(func_data{
				.m_name = name
				}
			))
		{
			m_target.m_vtable.set<func_data::vtable_impl<FuncPtr, func_sig_t<decltype(FuncPtr)>>>();
		}

	protected:
		friend trait_part;
		func_data& m_target;
	};

	template<auto FuncPtr, std::derived_from<builder_base> Prev> requires is_func<FuncPtr>
	class endable_func_builder :
		public func_builder<FuncPtr>
	{
	public:
		endable_func_builder(Prev& prev, std::string_view name) :
			func_builder<FuncPtr>(prev, name),
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

	registry_data& builder_base::get_registry() const
	{
		return *m_registry_builder.m_reg;
	}

	export API endable_registry_builder begin_registry() { return {}; }
}
