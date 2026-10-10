module;

#include <assert.h>

export module export_core:rel_ptr;
import stl;

namespace rel
{
	export template< typename T >
	class ptr
	{
	public:
		constexpr ptr()
			: m_offset( get_offset_to( nullptr ) )
		{
		}

		template< typename U >
		explicit constexpr ptr( U* obj )
			requires std::convertible_to< U*, T* >
			: m_offset( get_offset_to( obj ) )
		{
		}

		template< typename U >
		constexpr ptr( const ptr< U >& other )
			requires std::convertible_to< U*, T* >
			: m_offset( get_offset_to( other.get() ) )
		{
		}

		template< typename U >
		constexpr ptr( ptr< U >&& other ) noexcept
			requires std::convertible_to< U*, T* >
			: m_offset( get_offset_to( other.get() ) )
		{
			other.reset();
		}

		template< typename U >
		constexpr ptr& operator=( const ptr< U >& other )
			requires std::convertible_to< U*, T* >

		{
			if( this == &other )
			{
				return *this;
			}

			m_offset = get_offset_to( other.get() );
			return *this;
		}

		template< typename U >
		constexpr ptr& operator=( ptr< U >&& other ) noexcept
			requires std::convertible_to< U*, T* >

		{
			if( this == &other )
			{
				return *this;
			}

			m_offset = get_offset_to( other.get() );
			other.reset();
			return *this;
		}

		constexpr ~ptr() = default;

		template< typename U >
		constexpr auto operator<=>( const ptr< U >& rhs ) const
			requires std::convertible_to< U*, T* >

		{
			return get() <=> rhs.get();
		}

		constexpr bool operator==( std::nullptr_t ) const
		{
			return get() == nullptr;
		}

		constexpr ptr operator+( std::ptrdiff_t num ) const
			requires !std::is_void_v< T >
		{
			return ptr{ get() + ( num * sizeof( T ) ) };
		}

		constexpr ptr& operator+=( std::ptrdiff_t num )
			requires !std::is_void_v< T >
		{
			m_offset += sizeof( T ) * num;
			return *this;
		}

		constexpr ptr operator-( std::ptrdiff_t num ) const
			requires !std::is_void_v< T >
		{
			return ptr{ m_offset - ( num * sizeof( T ) ) };
		}

		constexpr ptr& operator-=( std::ptrdiff_t num )
			requires !std::is_void_v< T >
		{
			m_offset -= sizeof( T ) * num;
			return *this;
		}

		constexpr ptr& operator++()
			requires !std::is_void_v< T >
		{
			m_offset += sizeof( T );
			return *this;
		}

		constexpr ptr operator++( int )
			requires !std::is_void_v< T >
		{
			ptr tmp;
			tmp.m_offset = m_offset + sizeof( T );
			return tmp;
		}

		constexpr ptr& operator--()
			requires !std::is_void_v< T >
		{
			m_offset -= sizeof( T );
			return *this;
		}

		constexpr ptr operator--( int )
			requires !std::is_void_v< T >
		{
			ptr tmp;
			tmp.m_offset = m_offset - sizeof( T );
			return tmp;
		}

		constexpr explicit operator bool() const
		{
			return operator==( nullptr );
		}

		constexpr T* get() const
		{
			std::ptrdiff_t address = std::bit_cast< std::ptrdiff_t >( this );
			return std::bit_cast< T* >( address + m_offset );
		}

		constexpr auto& operator*() const
			requires !std::is_void_v< T >
		{
			return *get();
		}

		constexpr T* operator->() const
		{
			return get();
		}

		constexpr void reset()
		{
			m_offset = get_offset_to( nullptr );
		}

	private:
		constexpr std::ptrdiff_t get_offset_to( T* obj )
		{
			std::ptrdiff_t self_addr = std::bit_cast< std::ptrdiff_t >( this );
			std::ptrdiff_t obj_addr = std::bit_cast< std::ptrdiff_t >( obj );
			return obj_addr - self_addr;
		}

		std::ptrdiff_t m_offset{};
	};

	export template< typename T >
	class span
	{
	public:
		span() = default;

		span( T* data, size_t size )
			: m_data( data )
			, m_size( size )
		{
		}

		explicit span( std::span< T > span )
			: m_data( span.data() )
			, m_size( span.size() )
		{
		}

		constexpr ptr< T > begin() const
		{
			return m_data;
		}

		constexpr ptr< T > end() const
		{
			return m_data + m_size;
		}

		constexpr bool empty() const
		{
			return m_size == 0;
		}

		constexpr size_t size() const
		{
			return m_size;
		}

		constexpr std::ptrdiff_t ssize() const
		{
			return static_cast< std::ptrdiff_t >( m_size );
		}

		constexpr auto& operator[]( size_t idx ) const
			requires !std::is_void_v< T >
		{
			assert( idx < m_size );
			return *( m_data + idx );
		}

		constexpr span sub_span( size_t offset, size_t size ) const
		{
			assert( size == 0 || offset + size < m_size );
			return { .m_data = m_data + offset, .m_size = size - offset };
		}

		constexpr ptr< T > data() const
		{
			return m_data;
		}

	private:
		ptr< T > m_data{};
		size_t m_size{};
	};
} // namespace rel
