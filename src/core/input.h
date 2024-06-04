#pragma once

#include "core/core.h"
#include "core/keycodes.h"

#include <glm.hpp>

class Input
{
public:
	static void Initialize(HWND windowHandle);
	static bool IsKeyPressed(Key keycode);
	static bool IsMouseButtonPressed(MouseButton button);

	static glm::vec2 GetMousePosition();
	static void SetMouseCursor(bool enabled);
	static void SetMousePosition(const glm::vec2& position);
	static bool IsCursorEnabled();
private:
	static HWND ms_WindowHandle;
	static bool ms_CursorEnabled;
};