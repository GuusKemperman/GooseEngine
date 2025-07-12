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




// Method 2:

// # Compile everything
//
// 1. Pre-build-step: Generate dll-links, factory functions, reflect bindings, etc.. Parse C++, generate dependency tree
// 2. Compile everything
// 3. Every primary module interface can override module base class, and overrive functions for configuring runtime type, loaded by default, etc.
// 4. Said function is called once; if needed, module is kept loaded, otherwise unloaded.
//	  Can be loaded/unloaded depending on changes to settings. Cached config is generated again if cache is out date.
// 5. Success! 

// Intellisense/visual studio integration?
// Modules should compile independentely, e.g., the code generator/parser should be able to use logger, filio, and unit tests stuff.


class module
{
public:
	module();
};