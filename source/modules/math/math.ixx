export module math;

namespace ge
{
	export struct ivec2
	{

		// TODO more complete implementation, this is a bit sad
		API ivec2& operator+=(ivec2 v)
		{
			x += v.x;
			y += v.y;
			return *this;
		}

		union
		{
			int x{};
			int width;
		};

		union
		{
			int y{};
			int height;
		};
	};

	export constexpr auto sqr( auto x )
	{
		return x * x;
	}
}