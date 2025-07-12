export module TestModule;

import std;

export __declspec(dllexport) int f() { return 1; }

extern "C"
{
	export __declspec(dllexport) void HelloWorld()
	{
		std::puts("Hello world!\n");
	}
}
