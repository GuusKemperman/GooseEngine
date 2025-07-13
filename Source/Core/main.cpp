
import std;

#include <windows.h>


std::vector<std::string> GetExportedFunctions(HMODULE hModule)
{
    // Get DOS header
    PIMAGE_DOS_HEADER dosHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(hModule);
    if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE) 
    {
        throw std::runtime_error{ "Invalid DOS signature" };
    }

    // Get NT headers
    PIMAGE_NT_HEADERS ntHeaders = reinterpret_cast<PIMAGE_NT_HEADERS>(
        reinterpret_cast<BYTE*>(hModule) + dosHeader->e_lfanew);
    if (ntHeaders->Signature != IMAGE_NT_SIGNATURE) 
    {
        throw std::runtime_error{ "Invalid NT signature" };
    }

    // Get export directory
    IMAGE_DATA_DIRECTORY exportDir = ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
    if (exportDir.VirtualAddress == 0) 
    {
        throw std::runtime_error{ "No export directory found" };
    }

    PIMAGE_EXPORT_DIRECTORY exportDirectory = reinterpret_cast<PIMAGE_EXPORT_DIRECTORY>(
        reinterpret_cast<BYTE*>(hModule) + exportDir.VirtualAddress);

    DWORD* names = reinterpret_cast<DWORD*>(
        reinterpret_cast<BYTE*>(hModule) + exportDirectory->AddressOfNames);
    DWORD count = exportDirectory->NumberOfNames;

    std::vector<std::string> ret{};

    for (DWORD i = 0; i < count; ++i)
    {
        const char* name = reinterpret_cast<const char*>(
        reinterpret_cast<BYTE*>(hModule) + names[i]);
        ret.emplace_back(name);
    }

    return ret;
}


int main()
{
	HMODULE module = LoadLibraryA(R"(ModuleDLLTest\Build\x64\Debug\ModuleDLLTest.dll)");

    if (!module) 
    {
        std::cerr << "Failed to load DLL. Error: " << GetLastError() << '\n';
        return 1;
    }

    auto names = GetExportedFunctions(module);

    for (const auto& name : names)
    {
        std::cout << name << std::endl;
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

