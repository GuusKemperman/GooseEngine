module;
#include <assert.h>

export module runtime_reflection:value;

import stl;
import :type_id;
import :details;

namespace ge::refl
{
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

		template<undecorated T, typename Arg>
		static value create_owning(Arg&& args)
		{
			void* buffer = std::malloc(sizeof(T));
			new (buffer)T(std::forward<Arg>(args));
			return value{ create_vtable<T>(), buffer, true, true };
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

		API void make_constant() { m_is_mutable = false; }

	private:
		inplace_vtable<vtable> m_vtable{};
		void* m_value{};
		std::uint8_t m_is_mutable : 1{};
		std::uint8_t m_is_owning : 1{};
	};
}
