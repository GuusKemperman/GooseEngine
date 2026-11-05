export module renderer;

export import :utils;
export import :target;

export import core_traits;
import stl;
import math;

namespace ge
{
	// Currently we don't do anything here, but this can be used to order other systems against
	REFL_FUNC( ge::traits::system{} )
	export API void end_of_rendering( env::framebuffer& )
	{
	}

	REFL_FUNC( ge::traits::system{}, ge::traits::order_before< ge::end_of_rendering >{} )
	export API void example_rendering_thing( env::framebuffer& target )
	{
		draw_circle( target, { .pix = 0xff0ff0ff }, { .x = 200, .y = 200 }, 20 );
	}
} // namespace ge
