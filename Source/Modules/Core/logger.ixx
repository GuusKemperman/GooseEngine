export module logger;

import std;
import modules;

namespace ge
{
	export class ilogger
	{
	public:
		template <typename... T>
		void log(const std::format_string<T...> format, T&&... args);

		virtual void println(std::string_view str) = 0;
	};

	export class logger : public ilogger
	{
	public:
		void println(std::string_view str) override;
	};
}

namespace logger
{
	export class module : public ge::modules::module_base
	{
	public:
		module(ge::modules::module_manager&)
		{
			//test_engine();
			//test_utils();
			//std::println(" helooo {}", /*test_utils(),*/ test_engine());
		}
	};
}

template<typename ...T>
void ge::ilogger::log(std::format_string<T...> format, T && ...args)
{
	println(std::format(format, std::forward<T>(args)...));
}

void ge::logger::println(std::string_view str)
{
	std::puts(str.data());
	std::putchar('\n');
}
