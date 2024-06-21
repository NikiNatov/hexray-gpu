#pragma once

#include "core/core.h"
#include "core/event.h"

using EventCallbackFn = std::function<void(Event&)>;

struct WindowDescription
{
    std::string Title = "Unnamed";
    uint32_t Width = 1280;
    uint32_t Height = 720;
    bool VSync = true;
    EventCallbackFn EventCallback;
};

class Window
{
public:
    Window(const WindowDescription& description);
    ~Window();

    void ProcessEvents();
    void SwapBuffers();
    void SetMinimized(bool state);
    void ToggleVSync();
    void SetTitle(const std::string& title);
    void Resize(int width, int height);

    inline const std::string& GetTitle() const { return m_Description.Title; }
    inline uint32_t GetWidth() const { return m_Description.Width; }
    inline uint32_t GetHeight() const { return m_Description.Height; }
    inline bool IsVSyncOn() const { return m_Description.VSync; }
    inline bool IsMinimized() const { return m_Minimized; }
    inline HWND GetWindowHandle() const { return m_WindowHandle; }
private:
    static LRESULT WINAPI WindowProcSetup(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
    static LRESULT WINAPI WindowProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
private:
    WindowDescription m_Description;
    bool m_Minimized;
    HWND m_WindowHandle;
    WNDCLASSEX m_WindowClass;
    RECT m_WindowRect;
};