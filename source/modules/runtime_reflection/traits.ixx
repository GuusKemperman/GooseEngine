export module runtime_reflection:traits;

import :value;
import :type_id;
import :builders;
import stl;

namespace ge::refl
{
	export template<typename FuncSigT>
	struct invocable_trait;

	template<typename Ret, typename... Params>
	struct invocable_trait< Ret( Params... ) > : func_trait
	{
		Ret ( *m_invoke )( Params... );

		template<auto Func>
		void on_apply( const builders::func_builder< Func >& )
		{
			m_invoke = Func;
		}
	};
}
