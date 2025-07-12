import engine;
import file_io;
import logger;
import dependency_injection;


#include <windows.h>

int main()
{
	HMODULE module = LoadLibraryA(R"(ModuleDLLTest\Build\x64\Debug\ModuleDLLTest.dll)");

    if (!module) 
    {
        std::cerr << "Failed to load DLL. Error: " << GetLastError() << '\n';
        return 1;
    }

    // Get the address of the "HelloWorld" function
    auto helloWorldFunc = reinterpret_cast<void(*)()>(
        GetProcAddress(module, "HelloWorld")
        );

    if (!helloWorldFunc) 
    {
        std::cerr << "Failed to find function. Error: " << GetLastError() << '\n';
        FreeLibrary(module); // Cleanup
        return 1;
    }

    // Call the function
    helloWorldFunc();

    // Unload the DLL
    FreeLibrary(module);

    return 0;
	//HelloWorld();
	//return f();
	//ge::engine engine{ std::make_shared<ge::file_io>(), std::make_shared<test_logger>() };
	//engine.run();
}

