module;

#include <cassert>

export module runtime_reflection:data;

import :value;
import :type_id;
import stl;

namespace ge::refl
{
	export struct type_data;
	export struct func_data;
	export struct data_data;
	export struct module_data;
	export struct registry_data;

	export struct data_trait
	{
	};

	export struct type_trait
	{
	};

	export struct func_trait
	{
	};

	struct cached_type_data_ref
	{
		// Will hold a type_id before the registry has completed building.
		// Will hold a type_data_type after the registry has completed building
		union
		{
			type_id type_id{};
			std::reference_wrapper< const type_data > type_data;
		};
	};

	struct data_data
	{
		using trait_base_t = data_trait;

		using setter_t = void ( * )( value target_object, const value& new_value );
		using getter_t = value ( * )( const value& target_object );

		cached_type_data_ref m_type;
		std::reference_wrapper< const type_data > m_outer_type;
		setter_t m_set{};
		getter_t m_get{};

		std::string_view m_name{};
		std::span< const value > m_traits{};
	};

	struct type_data
	{
		using trait_base_t = type_trait;

		std::string_view m_name{};

		std::span< const value > m_traits{};
		std::span< const data_data > m_data{};
		std::span< const func_data > m_funcs{};

		type_id m_id{};
	};

	struct module_data
	{
		std::string_view m_name{};

		std::span< const type_data > m_types{};
		std::span< const func_data > m_funcs{};
		std::span< const data_data > m_datas{};
	};

	struct func_data
	{
		using trait_base_t = func_trait;

		std::span< const value > m_traits{};

		std::string_view m_name{};
	};

	export template< typename T, size_t Capacity >
	struct buffer
	{
		T& push_back( T&& item )
		{
			assert( m_size < Capacity );
			T* dst = end();
			new( dst ) T( std::move( item ) );
			m_size++;
			return *dst;
		}

		[[maybe_unused]] buffer() = default;

		buffer( const buffer& ) = delete;
		buffer( buffer&& ) = delete;

		buffer& operator=( const buffer& ) = delete;
		buffer& operator=( buffer&& ) = delete;

		~buffer()
		{
			std::span< T > self = *this;

			for( T& item : self )
			{
				item.~T();
			}
		}

		T* data()
		{
			return reinterpret_cast< T* >( m_data.data() );
		}

		const T* data() const
		{
			return reinterpret_cast< const T* >( m_data.data() );
		}

		T* begin()
		{
			return data();
		}

		const T* begin() const
		{
			return data();
		}

		T* end()
		{
			return begin() + m_size;
		}

		const T* end() const
		{
			return begin() + m_size;
		}

		std::array< std::byte, sizeof( T ) * Capacity > m_data;
		size_t m_size{};
	};

	struct registry_data
	{
		buffer< module_data, 64 > m_modules{};
		buffer< type_data, 1024 > m_types{};
		buffer< func_data, 1024 > m_funcs{};
		buffer< data_data, 1024 > m_datas{};
		buffer< value, 1024 > m_values{};
	};
} // namespace ge::refl
