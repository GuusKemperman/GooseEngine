export module renderer:utils;

import :target;

import math;
import stl;

namespace ge
{
	export API void draw_circle( env::framebuffer& target, pixel col, ivec2 pos, int radius )
	{
		int endX = std::min( pos.x + radius, target.m_size.width ), endY = std::min( pos.y + radius, target.m_size.height );

		int outerRadiusSqrd = sqr( radius );

		for( int y = std::max( pos.y - radius, 0 ); y < endY; y++ )
		{
			for( int x = std::max( pos.x - radius, 0 ); x < endX; x++ )
			{
				int dx = pos.x - x, dy = pos.y - y;
				int distSqrd = sqr( dx ) + sqr( dy );
				if( distSqrd <= outerRadiusSqrd )
				{
					pixel& dst = target.m_pixels[ static_cast< size_t >( x + y * target.m_size.width ) ];
					dst = col;
				}
			}
		}
	}

	export API void draw_circle( env::framebuffer& target, pixel col, ivec2 pos, int radius, int lineThickness )
	{
		int endX = std::min( pos.x + radius, target.m_size.width ), endY = std::min( pos.y + radius, target.m_size.height );

		int outerRadiusSqrd = radius * radius, innerRadiusSqrd = sqr( radius - lineThickness );

		for( int y = std::max( pos.y - radius, 0 ); y < endY; y++ )
		{
			for( int x = std::max( pos.x - radius, 0 ); x < endX; x++ )
			{
				int dx = pos.x - x, dy = pos.y - y;
				int distSqrd = dx * dx + dy * dy;
				if( distSqrd <= outerRadiusSqrd && distSqrd >= innerRadiusSqrd )
				{
					pixel& dst = target.m_pixels[ static_cast< size_t >( x + y * target.m_size.width ) ];
					dst = col;
				}
			}
		}
	}

	export API void draw_rectangle( env::framebuffer& target, pixel col, ivec2 topLeft, ivec2 bottomRight )
	{
		int endX = std::min( bottomRight.x, target.m_size.width ), endY = std::min( bottomRight.y, target.m_size.height );

		for( int y = std::max( topLeft.y, 0 ); y < endY; y++ )
		{
			for( int x = std::max( topLeft.x, 0 ); x < endX; x++ )
			{
				pixel& dst = target.m_pixels[ static_cast< size_t >( x + y * target.m_size.width ) ];
				dst = col;
			}
		}
	}

} // namespace ge
