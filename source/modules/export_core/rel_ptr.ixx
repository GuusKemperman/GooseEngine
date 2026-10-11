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

		constexpr ptr( std::nullptr_t )
			: ptr()
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
					 && !std::is_same_v< U, T >
			: ptr( other.get() )
		{
		}

		constexpr ptr( const ptr& other )
			: ptr( other.get() )
		{
		}

		template< typename U >
		constexpr ptr( ptr< U >&& other ) noexcept
			requires std::convertible_to< U*, T* >
					 && !std::is_same_v< U, T >
			: ptr( other.get() )
		{
			other.reset();
		}

		constexpr ptr( ptr&& other ) noexcept
			: ptr( other.get() )
		{
			other.reset();
		}

		template< typename U >
		constexpr ptr& operator=( U* obj )
			requires std::convertible_to< U*, T* >
		{
			assign( obj );
			return *this;
		}

		template< typename U >
		constexpr ptr& operator=( const ptr< U >& other )
			requires std::convertible_to< U*, T* > && !std::is_same_v< U, T >
		{
			assign( other.get() );
			return *this;
		}

		constexpr ptr& operator=( const ptr& other )
		{
			if( this == &other )
			{
				return *this;
			}

			assign( other.get() );
			return *this;
		}

		template< typename U >
		constexpr ptr& operator=( ptr< U >&& other ) noexcept
			requires std::convertible_to< U*, T* > && !std::is_same_v< U, T >
		{
			assign( other.get() );
			other.reset();
			return *this;
		}

		constexpr ptr& operator=( ptr&& other ) noexcept
		{
			if( this == &other )
			{
				return *this;
			}

			assign( other.get() );
			other.reset();
			return *this;
		}

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
			return ptr{ get() + num };
		}

		constexpr ptr& operator+=( std::ptrdiff_t num )
			requires !std::is_void_v< T >
		{
			assign( get() + num );
			return *this;
		}

		constexpr ptr operator-( std::ptrdiff_t num ) const
			requires !std::is_void_v< T >
		{
			return ptr{ get() - num };
		}

		constexpr ptr& operator-=( std::ptrdiff_t num )
			requires !std::is_void_v< T >
		{
			assign( get() - num );
			return *this;
		}

		constexpr ptr& operator++()
			requires !std::is_void_v< T >
		{
			assign( get() + 1 );
			return *this;
		}

		constexpr ptr operator++( int )
			requires !std::is_void_v< T >
		{
			ptr tmp{ *this };
			++*this;
			return tmp;
		}

		constexpr ptr& operator--()
			requires !std::is_void_v< T >
		{
			assign( get() - 1 );
			return *this;
		}

		constexpr ptr operator--( int )
			requires !std::is_void_v< T >
		{
			ptr tmp{ *this };
			--*this;
			return tmp;
		}

		constexpr explicit operator bool() const
		{
			return *this != nullptr;
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

		void assign( T* obj )
		{
			m_offset = get_offset_to( obj );
		}

		std::ptrdiff_t m_offset{};
	};

	export template< typename T >
	class span
	{
	public:
		constexpr span() = default;

		constexpr span( T* data, size_t size )
			: m_data( data )
			, m_size( size )
		{
		}

		constexpr explicit span( std::span< T > span )
			: m_data( span.data() )
			, m_size( span.size() )
		{
		}

		constexpr span( const span& other )
			: m_data( other.m_data )
			, m_size( other.m_size )
		{
		}

		constexpr span( span&& other ) noexcept
			: m_data( std::exchange( other.m_data, nullptr ) )
			, m_size( std::exchange( other.m_size, 0ull ) )
		{
		}

		constexpr span& operator=( const span& other )
		{
			if( this == &other )
			{
				return *this;
			}

			m_data = other.m_data;
			m_size = other.m_size;
			return *this;
		}

		constexpr span& operator=( span&& other ) noexcept
		{
			if( this == &other )
			{
				return *this;
			}

			m_data = std::exchange( other.m_data, nullptr );
			m_size = std::exchange( other.m_size, 0ull );
			return *this;
		}

		constexpr T* begin() const
		{
			return m_data.get();
		}

		constexpr T* end() const
		{
			return m_data.get() + m_size;
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

		constexpr T& operator[]( size_t idx ) const
			requires !std::is_void_v< T >
		{
			assert( idx < m_size );
			return *( m_data.get() + idx );
		}

		constexpr T* data() const
		{
			return m_data.get();
		}

	private:
		ptr< T > m_data{};
		size_t m_size{};
	};
} // namespace rel
