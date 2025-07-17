export module modules:modules_base;

namespace ge::modules
{
	export class module_base
	{
	public:
		virtual ~module_base() = default;

		int test{ };
	};
}
