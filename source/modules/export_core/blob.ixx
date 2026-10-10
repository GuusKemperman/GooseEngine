export module export_core:blob;

import stl;

namespace ge::exporter
{
	export struct out_of_pack_capacity : std::exception
	{
		using exception::exception;
	};

	export class pack_writer
	{
	public:
		API pack_writer( std::span< std::byte > buffer )
			: m_buffer( buffer )
		{
		}

		API std::byte* reserve_bytes( size_t count, size_t min_alignment )
		{
			size_t size = m_size.load();

			while( true )
			{
				std::byte* address = &m_buffer[ size ];

				size_t remaining_bytes = size > m_buffer.size() ? 0 : m_buffer.size() - size;
				void* void_address = address;
				if( std::align( min_alignment, count, void_address, remaining_bytes ) != nullptr )
				{
					throw out_of_pack_capacity( "Unsufficient pack space allocated" );
				}

				std::byte* reserved_address = static_cast< std::byte* >( void_address );
				size_t next_size = reserved_address - m_buffer.data() + count;

				if( m_size.compare_exchange_weak( size, next_size, std::memory_order::release, std::memory_order::relaxed ) )
				{
					return reserved_address;
				}
			}
		}

		template< typename T >
		std::span< T > emplace_array( size_t count )
		{
			std::byte* address = reserve_bytes( sizeof( T ) * count, alignof( T ) );
			std::span span{ std::bit_cast< T* >( address ), count };
			std::uninitialized_default_construct_n( span.data(), count );
			return span;
		}

		template< typename T, typename... Args >
		T& emplace( Args&&... args )
			requires std::is_constructible_v< T, Args... >
		{
			std::byte* address = reserve_bytes( sizeof( T ), alignof( T ) );
			return *new( address ) T( std::forward< Args >( args )... );
		}

		API size_t size() const
		{
			return m_size.load();
		}

	private:
		std::span< std::byte > m_buffer{};
		std::atomic_size_t m_size{};
	};
} // namespace ge::exporter
