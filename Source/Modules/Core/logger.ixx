export module logger;

import std;

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
