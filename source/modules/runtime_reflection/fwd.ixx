export module runtime_reflection:fwd;

// Forward declarations are placed here, so MSVC does not get confuse forward declarations in
// separate partitions as distinct types
namespace ge::refl
{
	export class type_handle;
	export class func_handle;
	export class data_handle;
	export class module_handle;
	export class registry;

	export struct data_data;
	export struct func_data;
	export struct type_data;
	export struct module_data;
	export struct registry_data;

	export struct type_trait {};
	export struct data_trait {};
	export struct func_trait {};
}
