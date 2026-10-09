export module export_core:traits;

import runtime_reflection;

namespace ge::traits
{
	export struct exporter : refl::func_trait
	{
		template< auto Func >
		void on_apply( const refl::builders::func_builder< Func >& )
		{
		}
	};

} // namespace ge::traits
