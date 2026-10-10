export module test_export_core:test_pack_writer;

import stl;
import export_core;
export import test_core;

using namespace ge::test_core;

namespace
{
	struct with_default
	{
		std::uint32_t m_value = 0xC0FFEE;
	};

	struct two_values
	{
		two_values( int a, float b )
			: m_a( a )
			, m_b( b )
		{
		}

		int m_a{};
		float m_b{};
	};

	struct alignas( 64 ) test_buffer
	{
		test_buffer()
		{
			// Non-zero, so default construction is visible
			m_bytes.fill( std::byte{ 0xAB } );
		}

		std::byte* at( size_t offset )
		{
			return m_bytes.data() + offset;
		}

		std::array< std::byte, 64 > m_bytes;
	};
} // namespace

namespace pack_writer_tests
{
	REFL_FUNC( ge::test_core::unit_test_trait{} )
	export API void new_writer_has_size_zero()
	{
		test_buffer buffer{};
		ge::exporter::pack_writer writer{ buffer.m_bytes };

		expect::is_eq( writer.size(), 0ull );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )
	export API void reserve_bytes_returns_start_of_buffer_first()
	{
		test_buffer buffer{};
		ge::exporter::pack_writer writer{ buffer.m_bytes };

		std::byte* address = writer.reserve_bytes( 16, 16 );

		expect::is_eq( address, buffer.at( 0 ) );
		expect::is_eq( writer.size(), 16ull );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )
	export API void reserve_bytes_consecutive_reservations_are_adjacent()
	{
		test_buffer buffer{};
		ge::exporter::pack_writer writer{ buffer.m_bytes };

		for( size_t i = 0; i < 5; i++ )
		{
			std::byte* address = writer.reserve_bytes( 7, 1 );
			expect::is_eq( address, buffer.at( i * 7 ) );
			expect::is_eq( writer.size(), ( i + 1 ) * 7 );
		}
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )
	export API void reserve_bytes_pads_to_requested_alignment()
	{
		test_buffer buffer{};
		ge::exporter::pack_writer writer{ buffer.m_bytes };

		std::byte* first = writer.reserve_bytes( 3, 1 );
		std::byte* second = writer.reserve_bytes( 8, 8 );
		std::byte* third = writer.reserve_bytes( 1, 32 );

		expect::is_eq( first, buffer.at( 0 ) );
		expect::is_eq( second, buffer.at( 8 ) );
		expect::is_eq( third, buffer.at( 32 ) );
		expect::is_eq( writer.size(), 33ull );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )
	export API void emplace_constructs_with_arguments()
	{
		test_buffer buffer{};
		ge::exporter::pack_writer writer{ buffer.m_bytes };

		two_values& value = writer.emplace< two_values >( 5, 2.5f );

		expect::is_eq( static_cast< void* >( &value ), buffer.at( 0 ) );
		expect::is_eq( value.m_a, 5 );
		expect::is_eq( value.m_b, 2.5f );
		expect::is_eq( writer.size(), sizeof( two_values ) );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )
	export API void emplace_array_default_constructs_every_element()
	{
		test_buffer buffer{};
		ge::exporter::pack_writer writer{ buffer.m_bytes };

		std::span< with_default > values = writer.emplace_array< with_default >( 4 );

		expect::is_eq( values.size(), 4ull );
		expect::is_eq( static_cast< void* >( values.data() ), buffer.at( 0 ) );
		expect::is_eq( writer.size(), 4 * sizeof( with_default ) );

		for( const with_default& value : values )
		{
			expect::is_eq( value.m_value, 0xC0FFEEu );
		}
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )
	export API void emplace_array_respects_alignment_of_element()
	{
		test_buffer buffer{};
		ge::exporter::pack_writer writer{ buffer.m_bytes };

		writer.reserve_bytes( 1, 1 );
		std::span< std::uint64_t > values = writer.emplace_array< std::uint64_t >( 2 );

		expect::is_eq( static_cast< void* >( values.data() ), buffer.at( 8 ) );
		expect::is_eq( writer.size(), 24ull );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )
	export API void reserve_bytes_exact_fit_succeeds()
	{
		test_buffer buffer{};
		ge::exporter::pack_writer writer{ buffer.m_bytes };

		std::byte* address = writer.reserve_bytes( 64, 1 );

		expect::is_eq( address, buffer.at( 0 ) );
		expect::is_eq( writer.size(), 64ull );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )
	export API void reserve_bytes_over_capacity_throws()
	{
		test_buffer buffer{};
		ge::exporter::pack_writer writer{ buffer.m_bytes };

		assert::expect_exception< ge::exporter::out_of_pack_capacity >( [ &writer ] { writer.reserve_bytes( 65, 1 ); } );
		expect::is_eq( writer.size(), 0ull );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )
	export API void reserve_bytes_when_full_throws()
	{
		test_buffer buffer{};
		ge::exporter::pack_writer writer{ buffer.m_bytes };

		writer.reserve_bytes( 64, 1 );

		assert::expect_exception< ge::exporter::out_of_pack_capacity >( [ &writer ] { writer.reserve_bytes( 1, 1 ); } );
		expect::is_eq( writer.size(), 64ull );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )
	export API void reserve_bytes_padding_past_capacity_throws()
	{
		test_buffer buffer{};
		ge::exporter::pack_writer writer{ std::span{ buffer.m_bytes }.first( 16 ) };

		writer.reserve_bytes( 1, 1 );

		// Fits without padding (1 + 15 == 16), but not once aligned to 8 (8 + 15 > 16)
		assert::expect_exception< ge::exporter::out_of_pack_capacity >( [ &writer ] { writer.reserve_bytes( 15, 8 ); } );
		expect::is_eq( writer.size(), 1ull );
	}

	REFL_FUNC( ge::test_core::unit_test_trait{} )
	export API void reserve_bytes_zero_size_buffer_throws()
	{
		ge::exporter::pack_writer writer{ std::span< std::byte >{} };

		assert::expect_exception< ge::exporter::out_of_pack_capacity >( [ &writer ] { writer.reserve_bytes( 1, 1 ); } );
		expect::is_eq( writer.size(), 0ull );
	}
} // namespace pack_writer_tests
