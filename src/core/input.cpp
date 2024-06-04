#include "input.h"

HWND Input::ms_WindowHandle = 0;
bool Input::ms_CursorEnabled = true;

// ------------------------------------------------------------------------------------------------------------------------------------
void Input::Initialize(HWND windowHandle)
{
	ms_WindowHandle = windowHandle;
}

// ------------------------------------------------------------------------------------------------------------------------------------
bool Input::IsKeyPressed(Key key)
{
	if (GetAsyncKeyState((int)key) & (1 << 15))
		return true;

	return false;
}

// ------------------------------------------------------------------------------------------------------------------------------------
bool Input::IsMouseButtonPressed(MouseButton button)
{
	if (GetAsyncKeyState((int)button) & (1 << 15))
		return true;

	return false;
}

// ------------------------------------------------------------------------------------------------------------------------------------
glm::vec2 Input::GetMousePosition()
{
	POINT point;

	bool result = GetCursorPos(&point);
	HEXRAY_ASSERT_MSG(result, "Failed to get the cursor position!");

	result = ScreenToClient(ms_WindowHandle, &point);
	HEXRAY_ASSERT_MSG(result, "Could not convert from screen coordinates to client coordinates!");

	return { point.x, point.y };
}

// ------------------------------------------------------------------------------------------------------------------------------------
void Input::SetMouseCursor(bool enabled)
{
	ms_CursorEnabled = enabled;

	if (enabled)
	{
		while (ShowCursor(true) < 0);
	}
	else
	{
		while (ShowCursor(false) >= 0);
	}
}

// ------------------------------------------------------------------------------------------------------------------------------------
void Input::SetMousePosition(const glm::vec2& position)
{
	POINT point = { (LONG)position.x, (LONG)position.y };
	ClientToScreen(ms_WindowHandle, &point);
	SetCursorPos(point.x, point.y);
}

// ------------------------------------------------------------------------------------------------------------------------------------
bool Input::IsCursorEnabled()
{
	return ms_CursorEnabled;
}
