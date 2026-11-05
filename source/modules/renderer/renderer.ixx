export module renderer;

export import :utils;
export import :target;

export import core_traits;
import stl;
import math;

namespace ge
{
	// TODO these function is only here because we dont support having a sequence_range 'rendering'

	REFL_FUNC( ge::traits::system{} )
	export API void start_of_rendering( env::framebuffer& target )
	{
		draw_circle( target, { .pix = 0xff0ff0ff }, {}, 20 );
	}
	
	REFL_FUNC( ge::traits::system{}, ge::traits::order_after< ge::start_of_rendering >{} )
	export API void end_of_rendering( env::framebuffer& target )
	{
		draw_circle( target, { .pix = 0xffffffff }, {}, 10 );
	}
} // namespace ge
