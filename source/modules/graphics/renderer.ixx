module;

// i hate windows.h
#define WIN32_LEAN_AND_MEAN
#define NOGDICAPMASKS
#define NOSYSMETRICS
#define NOMENUS
#define NOICONS
#define NOSYSCOMMANDS
#define NORASTEROPS
#define OEMRESOURCE
#define NOATOM
#define NOCLIPBOARD
#define NOCTLMGR
#define NODRAWTEXT
#define NOKERNEL
#define NONLS
#define NOMB
#define NOMEMMGR
#define NOMETAFILE
#define NOMINMAX
#define NOOPENFILE
#define NOSCROLL
#define NOSERVICE
#define NOSOUND
#define NOTEXTMETRIC
#define NOWH
#define NOCOMM
#define NOKANJI
#define NOHELP
#define NOPROFILER
#define NODEFERWINDOWPOS
#define NOMCX

#include <cassert>
#include <windows.h>

#define GET_X_LPARAM( lp ) ( ( int )( short )LOWORD( lp ) )
#define GET_Y_LPARAM( lp ) ( ( int )( short )HIWORD( lp ) )

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
			ivec2 m_size{ .width = 1024, .height = 1024 };
		};

		REFL_TYPE( ge::traits::environment{} )
		export struct framebuffer
		{
			std::vector< pixel > m_pixels{};
			ivec2 m_size{};
		};

		REFL_TYPE( ge::traits::environment{} )
		export struct pixel_size
		{
			float m_size{ 1.0f };
		};
	} // namespace env

} // namespace ge

namespace
{
	// TODO proper input;
	int horizontal_axis{};
	ge::ivec2 mouse{};

	struct windows_window;

	struct windows_userdata
	{
		std::reference_wrapper< ge::env::window > m_window_env;
		std::reference_wrapper< windows_window > m_windows_window;
	};

	struct windows_window : ge::platform_window_base
	{
		windows_window( ge::env::window& window_env )
			: m_userdata( window_env, *this )
		{
		}

		windows_userdata m_userdata;
		HWND hWnd{};
		HDC ghDC{};
		HGLRC ghRC{};
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

	void resize( ge::env::window& window )
	{
		RECT rect;
		GetClientRect( get_windows_window( window ).hWnd, &rect );
		window.m_size.width = static_cast< int >( rect.right );
		window.m_size.height = static_cast< int >( rect.bottom );
	}

	windows_userdata& get_userdata( HWND hWnd )
	{
		return *reinterpret_cast< windows_userdata* >( GetWindowLongPtr( hWnd, GWLP_USERDATA ) );
	}

	/* main window procedure */
	LONG WINAPI MainWndProc( HWND hWnd, std::uint32_t uMsg, WPARAM wParam, LPARAM lParam )
	{
		switch( uMsg )
		{
		case WM_CREATE:
		{
			CREATESTRUCT* pCreate = reinterpret_cast< CREATESTRUCT* >( lParam );
			windows_userdata& userdata = *reinterpret_cast< windows_userdata* >( pCreate->lpCreateParams );
			windows_window& windows_window = userdata.m_windows_window;

			SetWindowLongPtr( hWnd, GWLP_USERDATA, reinterpret_cast< LONG_PTR >( &userdata ) );

			windows_window.hWnd = hWnd;
			windows_window.ghDC = GetDC( hWnd );

			ConfigurePixelFormat( windows_window.ghDC );

			windows_window.ghRC = wglCreateContext( windows_window.ghDC );
			wglMakeCurrent( windows_window.ghDC, windows_window.ghRC );

			// Needed for flipping the screen vertically (together with glPixelZoom)
			glRasterPos2f( -1, 1 );

			resize( userdata.m_window_env );
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
			resize( get_userdata( hWnd ).m_window_env );
			break;
		}
		case WM_CLOSE:
		case WM_DESTROY:
		{
			windows_window& window = get_userdata( hWnd ).m_windows_window;

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
			//case WM_MOUSEMOVE:
			//{

			//	int x = GET_X_LPARAM( lParam );
			//	int y = GET_Y_LPARAM( lParam );

			//	mouse.x = static_cast< int >( static_cast< float >( x ) / ge::s_pixel_size );
			//	mouse.y = static_cast< int >( static_cast< float >( y ) / ge::s_pixel_size );

			//	std::cout << mouse.x << ", " << mouse.y << std::endl;
			//	break;
			//}

		default:
			return static_cast< LONG >( DefWindowProc( hWnd, uMsg, wParam, lParam ) );
		}

		return 1;
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
	export API void initialize_window( env::window& window_env )
	{
		if( window_env.m_window != nullptr )
		{
			return;
		}

		windows_window& window = [ &window_env ]() -> windows_window&
		{
			window_env.m_window = std::make_unique< windows_window >( window_env );
			return get_windows_window( window_env );
		}();

		HINSTANCE hInstance = GetModuleHandle( nullptr ); // what WinMain gives you

		static constexpr std::string_view app_name = "Goose Engine";

		{
			WNDCLASS wndclass;

			/* Register the frame class */
			wndclass.style = 0;
			wndclass.lpfnWndProc = ( WNDPROC )MainWndProc;
			wndclass.cbClsExtra = 0;
			wndclass.cbWndExtra = 0;
			wndclass.hInstance = hInstance;
			wndclass.hbrBackground = ( HBRUSH )( COLOR_WINDOW + 1 );
			wndclass.hIcon = LoadIcon( hInstance, app_name.data() );
			wndclass.hCursor = LoadCursor( NULL, IDC_ARROW );
			wndclass.lpszMenuName = app_name.data();
			wndclass.lpszClassName = app_name.data();

			[[maybe_unused]] ATOM registeredClass = RegisterClass( &wndclass );
			assert( registeredClass );
		}

		/* Create the frame */
		window.hWnd = CreateWindow(
			app_name.data(),
			app_name.data(),
			WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			window_env.m_size.x,
			window_env.m_size.y,
			NULL,
			NULL,
			hInstance,
			&window.m_userdata );

		assert( window.hWnd != nullptr );

		ShowWindow( window.hWnd, SW_SHOW );
		UpdateWindow( window.hWnd );
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
	export API void swap_buffers( const env::window& window, const env::pixel_size& pixel_size, env::framebuffer& framebuffer )
	{
		windows_window& windows_window = get_windows_window( window );

		glPixelZoom( pixel_size.m_size, -pixel_size.m_size );
		glDrawPixels(
			framebuffer.m_size.width,
			framebuffer.m_size.height,
			GL_RGBA,
			GL_UNSIGNED_BYTE,
			framebuffer.m_pixels.data() );

		glFlush();
		SwapBuffers( windows_window.ghDC );

		// Don't clear, we assume the game will populate the entire screen (or clear themselves by writing to the full framebuffer)

		framebuffer.m_size = window.m_size;
		framebuffer.m_pixels.resize( static_cast< size_t >( framebuffer.m_size.width * framebuffer.m_size.height ), {} );

		glViewport( 0, 0, framebuffer.m_size.width, framebuffer.m_size.height );
	}

	REFL_FUNC(
		ge::traits::system{},
		ge::traits::order_before< ge::swap_buffers >{},
		ge::traits::order_after< ge::process_messages >{} )
	export API void draw_something( env::framebuffer& target )
	{
		draw2d::circle( target, { .pix = 0xffffffff }, mouse, 10 );
	}
} // namespace ge
