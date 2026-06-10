export module runtime_reflection:handle_fwd;

// Single, shared forward-declaration point for the public handle types.
//
// The handles are defined in :handles / :module_handle, but :raw_data needs to
// name them (data_data::handle_t etc.) without depending on those partitions.
// Declaring them here - and importing this partition from every unit that
// either names or defines a handle - guarantees every translation unit refers
// to the exact same entity. (Forward-declaring them locally in :raw_data while
// defining them as exported classes elsewhere makes MSVC treat the declaration
// and the definition as distinct types, which breaks std::is_same_v on them.)
namespace ge::refl
{
	export class type_handle;
	export class func_handle;
	export class data_handle;
	export class module_handle;
}
