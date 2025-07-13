
import std;
import module_loading;

int main()
{
    for (auto dir_entry : std::filesystem::directory_iterator{ "bin" })
    {
        if (!ge::module_loading::is_shared_lib(dir_entry.path()))
        {
            continue;
        }

        try
        {
            std::shared_ptr platform_module = ge::module_loading::load_platform_module(dir_entry.path());

            if (platform_module == nullptr)
            {
                continue;
            }

            std::vector<ge::module_loading::exported_func> funcs = get_exported_functions(*platform_module);

            std::println("From {}:", dir_entry.path().string());
            for (auto& func : funcs)
            {
                std::println("\t{}", func.m_name);
            }
        }
        catch (const std::exception& e)
        {
            std::println(std::cerr, "{}", e.what());
        }
    }
    return 0;
}

