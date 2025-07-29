export module modules:modules_base;

import std;

namespace ge::modules
{
	export class module_base
	{
	public:
		API virtual ~module_base() = default;
	};

	export template<typename derived_t>
	class module : public module_base
	{
	public:
		API static const type_info& get_derived_type_info();
	};

}

template <typename derived_t>
const type_info& ge::modules::module<derived_t>::get_derived_type_info()
{
	return typeid(derived_t);
}
