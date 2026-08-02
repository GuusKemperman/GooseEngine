#include <cassert>

import stl;
import modules;

import windows;

int main()
{
	// TODO Not really good to assume this
	assert( std::filesystem::current_path().string().ends_with("bin") );

	ge::windows::modules::loader windows_loader{};
	ge::modules::load_modules_in_folder( windows_loader, std::filesystem::current_path() );
}
