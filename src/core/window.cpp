#include "window.h"

EXTERN_C IMAGE_DOS_HEADER __ImageBase;
static HINSTANCE s_hInstance;

// ------------------------------------------------------------------------------------------------------------------------------------
Window::Window(const WindowDescription& description)
    : m_Description(description), m_Minimized(false)
{
	s_hInstance = (HINSTANCE)&__ImageBase;

	// Initialize window
	m_WindowClass = {};
	m_WindowClass.cbSize = sizeof(m_WindowClass);
	m_WindowClass.style = CS_OWNDC;
	m_WindowClass.lpfnWndProc = WindowProcSetup;
	m_WindowClass.cbClsExtra = 0;
	m_WindowClass.cbWndExtra = 0;
	m_WindowClass.hInstance = s_hInstance;
	m_WindowClass.hCursor = nullptr;
	m_WindowClass.hbrBackground = nullptr;
	m_WindowClass.lpszMenuName = nullptr;
	m_WindowClass.lpszClassName = "Atom Engine Window";
	RegisterClassEx(&m_WindowClass);

	m_WindowRect.left = 100;
	m_WindowRect.right = m_Description.Width + m_WindowRect.left;
	m_WindowRect.top = 100;
	m_WindowRect.bottom = m_Description.Height + m_WindowRect.top;

	HEXRAY_ASSERT_MSG(AdjustWindowRect(&m_WindowRect, WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU | WS_SIZEBOX, FALSE), "Failed to adjust window rect!");

	m_WindowHandle = CreateWindowEx(0, m_WindowClass.lpszClassName, m_Description.Title.c_str(), WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU | WS_MAXIMIZEBOX | WS_SIZEBOX, CW_USEDEFAULT, CW_USEDEFAULT, m_WindowRect.right - m_WindowRect.left, m_WindowRect.bottom - m_WindowRect.top, nullptr, nullptr, s_hInstance, this);

	HEXRAY_ASSERT_MSG(m_WindowHandle, "Failed to create the window!");

	ShowWindow(m_WindowHandle, SW_NORMAL);
}

// ------------------------------------------------------------------------------------------------------------------------------------
Window::~Window()
{
	UnregisterClass(m_WindowClass.lpszClassName, s_hInstance);
	DestroyWindow(m_WindowHandle);
}

// ------------------------------------------------------------------------------------------------------------------------------------
void Window::ProcessEvents()
{
	MSG msg = {};
	while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
	{
		if (msg.message == WM_QUIT)
			return;

		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

// ------------------------------------------------------------------------------------------------------------------------------------
void Window::SwapBuffers()
{
	// TODO: Add functionality here when we have a D3D12 swapchain
}

// ------------------------------------------------------------------------------------------------------------------------------------
void Window::SetMinimized(bool state)
{
	m_Minimized = state;
}

// ------------------------------------------------------------------------------------------------------------------------------------
void Window::ToggleVSync()
{
	m_Description.VSync = !m_Description.VSync;
}

// ------------------------------------------------------------------------------------------------------------------------------------
void Window::SetTitle(const std::string& title)
{
	SetWindowText(m_WindowHandle, title.c_str());
}

// ------------------------------------------------------------------------------------------------------------------------------------
void Window::Resize(int width, int height)
{
	GetWindowRect(m_WindowHandle, &m_WindowRect);
	MoveWindow(m_WindowHandle, m_WindowRect.top, m_WindowRect.left, width, height, false);
	m_WindowRect.right = m_WindowRect.left + width;
	m_WindowRect.bottom = m_WindowRect.top + height;
}

// ------------------------------------------------------------------------------------------------------------------------------------
LRESULT WINAPI Window::WindowProcSetup(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	if (Msg == WM_CREATE)
	{
		const CREATESTRUCTW* const pCreate = reinterpret_cast<CREATESTRUCTW*>(lParam);
		Window* const window = static_cast<Window*>(pCreate->lpCreateParams);
		SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
		SetWindowLongPtr(hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&Window::WindowProc));
		return WindowProc(hWnd, Msg, wParam, lParam);
	}

	return DefWindowProc(hWnd, Msg, wParam, lParam);
}

// ------------------------------------------------------------------------------------------------------------------------------------
LRESULT WINAPI Window::WindowProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	Window* window = reinterpret_cast<Window*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

	switch (Msg)
	{
		case WM_DESTROY:
		case WM_CLOSE:
		{
			WindowClosedEvent e;
			window->m_Description.EventCallback(e);
			PostQuitMessage(0);
			return 0;
		}
		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
		{
			KeyPressedEvent e(static_cast<Key>(wParam));
			window->m_Description.EventCallback(e);
			break;
		}
		case WM_KEYUP:
		case WM_SYSKEYUP:
		{
			KeyReleasedEvent e(static_cast<Key>(wParam));
			window->m_Description.EventCallback(e);
			break;
		}
		case WM_MOUSEMOVE:
		{
			const POINTS pt = MAKEPOINTS(lParam);

			MouseMovedEvent e(pt.x, pt.y);
			window->m_Description.EventCallback(e);
			break;
		}
		case WM_LBUTTONDOWN:
		case WM_RBUTTONDOWN:
		{
			MouseButtonPressedEvent e(static_cast<MouseButton>(wParam));
			window->m_Description.EventCallback(e);
			break;
		}
		case WM_LBUTTONUP:
		case WM_RBUTTONUP:
		{
			MouseButtonReleasedEvent e(static_cast<MouseButton>(wParam));
			window->m_Description.EventCallback(e);
			break;
		}
		case WM_MOUSEWHEEL:
		{
			const POINTS pt = MAKEPOINTS(lParam);
			const int delta = GET_WHEEL_DELTA_WPARAM(wParam);

			MouseScrolledEvent e(0, delta);
			window->m_Description.EventCallback(e);
			break;
		}
		case WM_SIZE:
		{
			window->m_Description.Width = LOWORD(lParam);
			window->m_Description.Height = HIWORD(lParam);

			if (wParam == SIZE_MINIMIZED)
				window->SetMinimized(true);
			else if (wParam == SIZE_RESTORED)
				window->SetMinimized(false);

			WindowResizedEvent e(window->m_Description.Width, window->m_Description.Height);
			window->m_Description.EventCallback(e);
			break;
		}
	}

	return DefWindowProc(hWnd, Msg, wParam, lParam);
}
