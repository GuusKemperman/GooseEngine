module;

#define WIN32_LEAN_AND_MEAN
#include <cassert>
#include <windows.h>

#undef min
#undef max

#include <gl/GL.h>

export module renderer;

export import core_traits;
import stl;

namespace ge
{
	export struct ivec2
	{
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

	export struct platform_window_base
	{
		virtual ~platform_window_base() = default;
	};

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
		export struct window
		{
			std::unique_ptr< platform_window_base > m_window{};
		};

		REFL_TYPE( ge::traits::environment{} )
		export struct framebuffer
		{
			std::vector< pixel > m_pixels{};
			ivec2 m_size{};
		};

		// TODO proper input

		//REFL_TYPE( ge::traits::environment{} )
		//export struct inputs
		//{
		//	float m_x_axis{};
		//	float m_y_axis{};
		//};
	} // namespace env

} // namespace ge

namespace
{
	// TODO proper input;
	int horizontal_axis{};

	struct windows_window : ge::platform_window_base
	{
		HWND hWnd{};
		HDC ghDC{};
		HGLRC ghRC{};

		ge::ivec2 size{};
	};

	windows_window& get_windows_window( const ge::env::window& window )
	{
		return *static_cast< windows_window* >( window.m_window.get() );
	}

	void ConfigurePixelFormat( HDC hdc )
	{
		PIXELFORMATDESCRIPTOR pfd;

		// TODO re-examine these values
		pfd.nSize = sizeof( PIXELFORMATDESCRIPTOR );
		pfd.nVersion = 1;
		pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER | PFD_DEPTH_DONTCARE;
		pfd.dwLayerMask = PFD_MAIN_PLANE;
		pfd.iPixelType = PFD_TYPE_COLORINDEX;
		pfd.cColorBits = 8;
		pfd.cDepthBits = 0;
		pfd.cAccumBits = 0;
		pfd.cStencilBits = 0;

		int pixelformat = ChoosePixelFormat( hdc, &pfd );
		assert( pixelformat != 0 );

		[[maybe_unused]] bool succesfullySetPixelFormat = SetPixelFormat( hdc, pixelformat, &pfd );
		assert( succesfullySetPixelFormat );
	}

	void resize( windows_window& window )
	{
		RECT rect;
		GetClientRect( window.hWnd, &rect );
		window.size.width = static_cast< int >( rect.right );
		window.size.height = static_cast< int >( rect.bottom );
	}

	windows_window& get_userdata( HWND hWnd )
	{
		return *reinterpret_cast< windows_window* >( GetWindowLongPtr( hWnd, GWLP_USERDATA ) );
	}

	/* main window procedure */
	LONG WINAPI MainWndProc( HWND hWnd, std::uint32_t uMsg, WPARAM wParam, LPARAM lParam )
	{
		switch( uMsg )
		{
		case WM_CREATE:
		{
			CREATESTRUCT* pCreate = reinterpret_cast< CREATESTRUCT* >( lParam );
			windows_window& window = *reinterpret_cast< windows_window* >( pCreate->lpCreateParams );
			SetWindowLongPtr( hWnd, GWLP_USERDATA, reinterpret_cast< LONG_PTR >( &window ) );

			window.hWnd = hWnd;
			window.ghDC = GetDC( hWnd );

			ConfigurePixelFormat( window.ghDC );

			window.ghRC = wglCreateContext( window.ghDC );
			wglMakeCurrent( window.ghDC, window.ghRC );

			resize( window );
			break;
		}
		case WM_PAINT:
		{
			PAINTSTRUCT ps;
			BeginPaint( hWnd, &ps );
			EndPaint( hWnd, &ps );
			break;
		}

		case WM_SIZE:
		{
			windows_window& window = get_userdata( hWnd );
			resize( window );
			break;
		}
		case WM_CLOSE:
		case WM_DESTROY:
		{
			windows_window& window = *reinterpret_cast< windows_window* >( GetWindowLongPtr( hWnd, GWLP_USERDATA ) );

			if( window.ghRC )
			{
				wglDeleteContext( window.ghRC );
				window.ghRC = nullptr;
			}
			if( window.ghDC )
			{
				ReleaseDC( hWnd, window.ghDC );
				window.ghDC = nullptr;
			}

			if( uMsg == WM_CLOSE )
			{
				DestroyWindow( hWnd );
			}
			else
			{
				PostQuitMessage( 0 );
			}

			break;
		}

		case WM_KEYDOWN:
		{
			switch( wParam )
			{
			case VK_LEFT:
				horizontal_axis = -1;
				break;
			case VK_RIGHT:
				horizontal_axis = 1;
				break;
			default:
				break;
			}
			break;
		}
		case WM_KEYUP:
		{
			switch( wParam )
			{
			case VK_LEFT:
			case VK_RIGHT:
				horizontal_axis = 0;
				break;
			default:
				break;
			}
			break;
		}

		default:
			return static_cast< LONG >( DefWindowProc( hWnd, uMsg, wParam, lParam ) );
		}

		return 1;
	}

	std::unique_ptr< windows_window > create_window( std::string_view app_name, int width, int height )
	{
		std::unique_ptr window = std::make_unique< windows_window >();

		HINSTANCE hInstance = GetModuleHandle( nullptr ); // what WinMain gives you

		WNDCLASS wndclass;

		/* Register the frame class */
		wndclass.style = 0;
		wndclass.lpfnWndProc = ( WNDPROC )MainWndProc;
		wndclass.cbClsExtra = 0;
		wndclass.cbWndExtra = 0;
		wndclass.hInstance = hInstance;
		wndclass.hIcon = LoadIcon( hInstance, app_name.data() );
		wndclass.hCursor = LoadCursor( NULL, IDC_ARROW );
		wndclass.hbrBackground = ( HBRUSH )( COLOR_WINDOW + 1 );
		wndclass.lpszMenuName = app_name.data();
		wndclass.lpszClassName = app_name.data();

		if( !RegisterClass( &wndclass ) )
			return FALSE;

		/* Create the frame */
		window->hWnd = CreateWindow(
			app_name.data(),
			app_name.data(),
			WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			width,
			height,
			NULL,
			NULL,
			hInstance,
			window.get() );

		assert( window->hWnd != nullptr );

		ShowWindow( window->hWnd, SW_SHOW );
		UpdateWindow( window->hWnd );

		return window;
	}

	constexpr std::uint32_t ScaleColor( std::uint32_t c, std::uint32_t scale )
	{
		const std::uint32_t rb = ( ( ( c & 0xff00ff ) * scale ) >> 8 ) & 0x00ff00ff;
		const std::uint32_t ag = ( ( ( c & 0xff00ff00 ) >> 8 ) * scale ) & 0xff00ff00;
		return rb + ag;
	}

	constexpr auto sqr( auto x )
	{
		return x * x;
	}

} // namespace

namespace ge
{
	namespace draw2d
	{
		export API void circle( env::framebuffer& target, pixel col, ivec2 pos, int radius )
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

		export API void circle( env::framebuffer& target, pixel col, ivec2 pos, int radius, int lineThickness )
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

		export API void rectangle( env::framebuffer& target, pixel col, ivec2 topLeft, ivec2 bottomRight )
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
	} // namespace draw2d

	REFL_FUNC( ge::traits::system{} )
	export API void initialize_window( env::window& window )
	{
		if( window.m_window != nullptr )
		{
			return;
		}

		window.m_window = create_window( "Goose Engine", 1024, 1024 );
	}

	REFL_FUNC( ge::traits::system{}, ge::traits::order_after< ge::initialize_window >{} )
	export API void process_messages()
	{
		MSG msg;

		while( PeekMessage( &msg, NULL, 0, 0, PM_NOREMOVE ) == TRUE )
		{
			if( GetMessage( &msg, NULL, 0, 0 ) )
			{
				TranslateMessage( &msg );
				DispatchMessage( &msg );
			}
			else
			{
				std::exit( 0 );
			}
		}
	}

	REFL_FUNC( ge::traits::system{}, ge::traits::order_after< ge::process_messages >{} )
	export API void swap_buffers( const env::window& window, env::framebuffer& framebuffer )
	{
		windows_window& windows_window = get_windows_window( window );

		glDrawPixels(
			framebuffer.m_size.width,
			framebuffer.m_size.height,
			GL_RGBA,
			GL_UNSIGNED_BYTE,
			framebuffer.m_pixels.data() );

		glFlush();
		SwapBuffers( windows_window.ghDC );

		// Clean up for next frame
		glClear( GL_COLOR_BUFFER_BIT /*| GL_DEPTH_BUFFER_BIT*/ );

		framebuffer.m_size = windows_window.size;
		framebuffer.m_pixels.clear();
		framebuffer.m_pixels.resize( static_cast< size_t >( framebuffer.m_size.width * framebuffer.m_size.height ), {} );

		glViewport( 0, 0, framebuffer.m_size.width, framebuffer.m_size.height );
	}

} // namespace ge
