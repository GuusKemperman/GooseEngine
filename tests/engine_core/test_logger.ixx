export module test_logger;

import std;
import logger;
import test_core;

using namespace ge::test_core;
using namespace ge::test_core::assert;

UNIT_TEST(logger, hello_world)
{
	ge::core::logger& logger = a_context.m_module_manager.get().get_instance<ge::core::logger>();
	size_t initial_size = std::ranges::size(logger.get_logged_messages());

	logger.log(ge::core::message, "Hello world!");

	is_eq(std::ranges::size(logger.get_logged_messages()), initial_size + 1);

	const ge::core::log_entry& entry = logger.get_logged_messages()[initial_size];

	is_eq(entry.m_severity, ge::core::message);
	is_eq(std::string_view{ entry.m_src.file_name() }, std::source_location::current().file_name());
	is_eq(std::string_view{ entry.m_src.function_name() }, std::source_location::current().function_name());

	is_true(entry.m_logged_text.contains("Hello world!"));
}

UNIT_TEST(logger, multi_threading)
{
	ge::core::logger& logger = a_context.m_module_manager.get().get_instance<ge::core::logger>();

	size_t num_messages_per_thread = 10;

	auto work = 
		[&](std::string_view msg)
		{
			std::thread::id id = std::this_thread::get_id();
			std::hash<std::thread::id> hash{};
			size_t seed = hash(id);
			std::default_random_engine eng{ static_cast<std::uint32_t>(seed) };
			
			for (size_t i = 0; i < num_messages_per_thread; i++)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds{
					std::uniform_int_distribution{ 1, 10 }(eng) });

				logger.log(ge::core::message, msg);
			}
		};

	std::thread t1{ work, "Hello" };
	std::thread t2{ work, "world" };
	t1.join(); t2.join();

	auto count = [&](std::string_view msg) -> size_t
		{
			return std::ranges::count_if(logger.get_logged_messages(),
				[&](const ge::core::log_entry& entry)
				{
					return entry.m_logged_text.contains(msg);
				});
		};

	is_eq(num_messages_per_thread, count("Hello"));
	is_eq(num_messages_per_thread, count("world"));
}
