export module renderer:target;

import stl;
import math;

namespace ge
{
	export struct pixel
	{
		union
		{
			struct
			{
				std::uint8_t r;
				std::uint8_t g;
				std::uint8_t b;
				std::uint8_t a;
			} channels;
			std::uint32_t pix{};
		};
	};

	namespace env
	{
		REFL_TYPE( ge::traits::environment{} )
		export struct framebuffer
		{
			std::vector< pixel > m_pixels{};
			ivec2 m_size{};
		};
	} // namespace env
}