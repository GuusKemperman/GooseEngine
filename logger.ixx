export module logger;

import std;

namespace ge
{
	export class logger
	{
	public:
		template <typename... T>
		void log(const std::format_string<T...> format, T&&... args);
	};
}

template<typename ...T>
void ge::logger::log(std::format_string<T...> format, T && ...args)
{
	std::println(format, std::forward<T>(args)...);
}