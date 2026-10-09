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
		API pack_writer( size_t capacity )
			: m_data( std::make_unique< std::byte[] >( capacity ) )
			, m_capacity( capacity )
		{
		}

		API std::byte* reserve_bytes( size_t count, size_t min_alignment )
		{
			size_t size = m_size.load();

			while( true )
			{
				std::byte* address = &m_data[ size ];

				size_t remaining_bytes = size > m_capacity ? 0 : m_capacity - size;
				void* void_address = address;
				if( std::align( min_alignment, count, void_address, remaining_bytes ) != nullptr )
				{
					throw out_of_pack_capacity( "Unsufficient pack space allocated" );
				}

				std::byte* reserved_address = static_cast< std::byte* >( void_address );
				size_t next_size = reserved_address - m_data.get() + count;

				if( m_size.compare_exchange_weak( size, next_size, std::memory_order::release, std::memory_order::relaxed ) )
				{
					return reserved_address;
				}
			}
		}

		API size_t capacity() const
		{
			return m_capacity;
		}

	private:
		std::unique_ptr< std::byte[] > m_data{};
		size_t m_capacity{};
		std::atomic_size_t m_size{};
	};
} // namespace ge::exporter
