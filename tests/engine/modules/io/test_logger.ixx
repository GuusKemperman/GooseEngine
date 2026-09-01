export module test_io;

import stl;
import io;
export import test_core;

using namespace ge::test_core;
using namespace ge::test_core::assert;

namespace logger
{
	// Test basic logging functionality
	REFL_FUNC( ge::test_core::unit_test_trait{} )

	export API void basic_logging()
	{
		ge::logger logger{};
		logger.clear();
		const size_t initial_size = std::ranges::size( logger.get_logged_messages() );

		logger.log( ge::severity::message, "Test message" );
		const auto src = std::source_location::current();

		const auto& messages = logger.get_logged_messages();
		is_eq( messages.size(), initial_size + 1 );

		const auto& entry = messages.back();
		is_eq( entry.m_severity, ge::severity::message );
		is_true( entry.m_logged_text.contains( "Test message" ) );
		is_eq( std::string_view( entry.m_src.file_name() ), std::string_view( src.file_name() ) );
		is_eq( entry.m_src.line(), src.line() );
	}

	// Test formatted logging
	REFL_FUNC( ge::test_core::unit_test_trait{} )

	export API void formatted_logging()
	{
		ge::logger logger{};
		logger.clear();
		const int value = 42;

		logger.log( ge::severity::verbose, "Value: {}", value );
		logger.log( ge::severity::verbose, "Value: {} {} {}", value + 1, value + 2, value + 3 );

		const auto& messages = logger.get_logged_messages();
		is_true( messages.front().m_logged_text.contains( "Value: 42" ) );
		is_true( messages.back().m_logged_text.contains( "Value: 43 44 45" ) );
	}

	// Test severity filtering
	REFL_FUNC( ge::test_core::unit_test_trait{} )

	export API void severity_filtering()
	{
		std::stringstream output{};

		ge::logger logger{ output, output };
		logger.clear();
		logger.set_severity( ge::severity::warning );

		logger.log( ge::severity::message, "Should not appear" );
		logger.log( ge::severity::warning, "Should appear" );
		logger.log( ge::severity::error, "Should appear" );

		const auto& messages = logger.get_logged_messages();
		is_eq( messages.size(), 3ull );
		is_eq( messages.front().m_severity, ge::severity::message );
		is_eq( std::next( messages.begin() )->m_severity, ge::severity::warning );
		is_eq( messages.back().m_severity, ge::severity::error );

		std::string output_str = output.str();

		is_true( output_str.contains( "Should appear" ) );
		is_true( !output_str.contains( "Should not appear" ) );
	}

	// Test log clearing
	REFL_FUNC( ge::test_core::unit_test_trait{} )

	export API void clear_log()
	{
		ge::logger logger{};
		logger.log( ge::severity::message, "Test" );

		logger.clear();
		is_true( logger.get_logged_messages().empty() );
		is_eq( logger.get_max_num_characters_stored(), 2097152ull );
	}

	// Test character limit enforcement
	REFL_FUNC( ge::test_core::unit_test_trait{} )

	export API void zero_character_limit()
	{
		ge::logger logger{};
		logger.clear();
		logger.log( ge::severity::message, "Message1" );

		// Lower to less than currently available
		logger.set_max_num_characters_stored( 0 );

		is_eq( logger.get_logged_messages().size(), 0ull );

		logger.log( ge::severity::message, "Msg2" );
		logger.log( ge::severity::message, "LongMessage3" );

		is_eq( logger.get_logged_messages().size(), 0ull );

		is_eq( logger.get_max_num_characters_stored(), 0ull );
	}

	// Test source location capture
	REFL_FUNC( ge::test_core::unit_test_trait{} )

	export API void source_location()
	{
		ge::logger logger{};
		logger.clear();
		logger.log( ge::severity::warning, "Location test" );
		const auto test_location = std::source_location::current();

		const auto& entry = logger.get_logged_messages().back();
		is_eq( std::string_view{ entry.m_src.file_name() }, test_location.file_name() );
		is_eq( std::string_view{ entry.m_src.function_name() }, test_location.function_name() );
		is_eq( entry.m_src.line(), test_location.line() );
	}

	// Test stream output redirection
	REFL_FUNC( ge::test_core::unit_test_trait{} )

	export API void stream_output()
	{
		std::ostringstream cout_buffer;
		std::ostringstream cerr_buffer;

		{
			ge::logger logger{ cout_buffer, cerr_buffer };
			logger.clear();

			logger.log( ge::severity::verbose, "Verbose to cout" );
			logger.log( ge::severity::message, "Message to cout" );
			logger.log( ge::severity::warning, "Warning to cerr" );
			logger.log( ge::severity::error, "Error to cerr" );
		}

		const std::string cout_output = cout_buffer.str();
		const std::string cerr_output = cerr_buffer.str();

		is_true( cout_output.contains( "Verbose to cout" ) );
		is_true( cout_output.contains( "Message to cout" ) );
		is_false( cout_output.contains( "Warning to cerr" ) );
		is_false( cout_output.contains( "Error to cerr" ) );

		is_false( cerr_output.contains( "Verbose to cout" ) );
		is_false( cerr_output.contains( "Message to cout" ) );
		is_true( cerr_output.contains( "Warning to cerr" ) );
		is_true( cerr_output.contains( "Error to cerr" ) );
	}
}
