export module test_logger;

import std;
import logger;
import test_core;

using namespace ge::test_core;
using namespace ge::test_core::assert;

// Test basic logging functionality
UNIT_TEST(logger, basic_logging)
{
    ge::logger logger{};
    logger.clear();
    const size_t initial_size = std::ranges::size(logger.get_logged_messages());

    logger.log(ge::severity::message, "Test message"); const auto src = std::source_location::current();

    const auto& messages = logger.get_logged_messages();
    is_eq(messages.size(), initial_size + 1);

    const auto& entry = messages.back();
    is_eq(entry.m_severity, ge::severity::message);
    is_true(entry.m_logged_text.contains("Test message"));
    is_eq(std::string_view(entry.m_src.file_name()), std::string_view(src.file_name()));
    is_eq(entry.m_src.line(), src.line());
}

// Test formatted logging
UNIT_TEST(logger, formatted_logging)
{
    ge::logger logger{};
    logger.clear();
    const int value = 42;

    logger.log(ge::severity::verbose, "Value: {}", value);
    logger.log(ge::severity::verbose, "Value: {} {} {}", value, value, value);

    const auto& messages = logger.get_logged_messages();
    is_true(messages.front().m_logged_text.contains("Value: 42"));
    is_true(messages.back().m_logged_text.contains("Value: 42 42 42"));
}

// Test severity filtering
UNIT_TEST(logger, severity_filtering)
{
    std::stringstream output{};

    ge::logger logger{ output, output };
    logger.clear();
    logger.set_severity(ge::severity::warning);

    logger.log(ge::severity::message, "Should not appear");
    logger.log(ge::severity::warning, "Should appear");
    logger.log(ge::severity::error, "Should appear");

    const auto& messages = logger.get_logged_messages();
    is_eq(messages.size(), 3);
    is_eq(messages.front().m_severity, ge::severity::message);
    is_eq(std::next(messages.begin())->m_severity, ge::severity::warning);
    is_eq(messages.back().m_severity, ge::severity::error);

    std::string output_str = output.str();

    is_true(output_str.contains("Should appear"));
    is_true(!output_str.contains("Should not appear"));
}

// Test log clearing
UNIT_TEST(logger, clear_log)
{
    ge::logger logger{};
    logger.log(ge::severity::message, "Test");

    logger.clear();
    is_true(logger.get_logged_messages().empty());
    is_eq(logger.get_max_num_characters_stored(), 2097152);
}

// Test character limit enforcement
UNIT_TEST(logger, zero_character_limit)
{
    ge::logger logger{};
    logger.clear();
    logger.log(ge::severity::message, "Message1");

    // Lower to less than currently available
	logger.set_max_num_characters_stored(0);

    is_eq(logger.get_logged_messages().size(), 0);

    logger.log(ge::severity::message, "Msg2");
    logger.log(ge::severity::message, "LongMessage3");

    is_eq(logger.get_logged_messages().size(), 0);

    is_eq(logger.get_max_num_characters_stored(), 0);
}

// Test multi-threaded logging using test_threading API
UNIT_TEST(logger, multi_threaded_logging)
{
    ge::logger logger{};
    logger.clear();

    auto format_thread_id = [](size_t thread_idx) {
        return std::format("Thread {}", thread_idx);
        };

    auto result = test_threading(
        [&](const threading_test_arg& arg) {
            logger.log(ge::severity::verbose,
                format_thread_id(arg.m_thread_idx));
        },
        100,  // Iterations per thread
        10    // Number of threads
    );

    const auto& messages = logger.get_logged_messages();
    is_eq(messages.size(), result.m_num_threads * result.m_num_iterations_per_thread);

    // Verify each thread's logs were recorded correctly
    for (size_t thread_idx = 0; thread_idx < result.m_num_threads; thread_idx++) {
        size_t count = std::ranges::count_if(messages, [&](const auto& entry) {
            return entry.m_logged_text.contains(format_thread_id(thread_idx));
            });
        is_eq(count, result.m_num_iterations_per_thread);
    }
}

// Test fatal severity exception
UNIT_TEST(logger, fatal_exception)
{
    ge::logger logger{};
    const std::string_view msg = "Fatal error occurred";

    auto e = expect_exception<std::runtime_error>([&] {
        logger.log(ge::severity::fatal, msg);
        });

    is_true(std::string(e.what()).contains(msg));
}

// Test source location capture
UNIT_TEST(logger, source_location)
{
    ge::logger logger{};
    logger.clear();
    logger.log(ge::severity::warning, "Location test"); const auto test_location = std::source_location::current();

    const auto& entry = logger.get_logged_messages().back();
    is_eq(std::string_view{ entry.m_src.file_name() }, test_location.file_name());
    is_eq(std::string_view{ entry.m_src.function_name() }, test_location.function_name());
    is_eq(entry.m_src.line(), test_location.line());
}

// Test stream output redirection
UNIT_TEST(logger, stream_output)
{
    std::ostringstream cout_buffer;
    std::ostringstream cerr_buffer;

    {
        ge::logger logger{ cout_buffer, cerr_buffer };
        logger.clear();

        logger.log(ge::severity::verbose, "Verbose to cout");
        logger.log(ge::severity::message, "Message to cout");
        logger.log(ge::severity::warning, "Warning to cerr");
        logger.log(ge::severity::error, "Error to cerr");

        (void)expect_exception<std::runtime_error>(
            [&]
            {
                logger.log(ge::severity::fatal, "Fatal to cerr");
            });
    }

    const std::string cout_output = cout_buffer.str();
    const std::string cerr_output = cerr_buffer.str();

    is_true(cout_output.contains("Verbose to cout"));
    is_true(cout_output.contains("Message to cout"));
    is_false(cout_output.contains("Warning to cerr"));
    is_false(cout_output.contains("Error to cerr"));
    is_false(cout_output.contains("Fatal to cerr"));

    is_false(cerr_output.contains("Verbose to cout"));
    is_false(cerr_output.contains("Message to cout"));
    is_true(cerr_output.contains("Warning to cerr"));
    is_true(cerr_output.contains("Error to cerr"));
    is_true(cerr_output.contains("Fatal to cerr"));
}

// Test thread safety with mixed operations
UNIT_TEST(logger, thread_safety_mixed_operations)
{
    ge::logger logger{};
    logger.clear();
    logger.set_max_num_characters_stored(1000);

    auto mixed_operations = [&](const threading_test_arg& arg) {
        switch (arg.m_iteration % 4) {
        case 0:
            logger.log(ge::severity::verbose,
                "Log from thread {}", arg.m_thread_idx);
            break;
        case 1:
            logger.clear();
            break;
        case 2:
            logger.set_severity(static_cast<ge::severity>(arg.m_iteration % 5));
            break;
        case 3:
            logger.set_max_num_characters_stored(arg.m_iteration * 10);
            break;
        }
        };

    test_threading(mixed_operations, 100, 5);

    // Final sanity checks
    is_true(logger.get_logged_messages().size() <= 1000);
    const auto sev = logger.get_severity();
    is_true(sev >= ge::severity::verbose && sev <= ge::severity::fatal);
}