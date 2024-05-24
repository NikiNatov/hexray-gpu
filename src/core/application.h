#pragma once

#include "core/core.h"
#include "core/event.h"
#include "core/window.h"

struct CommandLineArgs
{
    int Count = 0;
    char** Args = nullptr;

    const char* operator[](int index) const
    {
        HEXRAY_ASSERT(index < Count);
        return Args[index];
    }
};

struct ApplicationDescription
{
    std::string Name = "HexRay";
    CommandLineArgs CommandLineArgs;
    uint32_t WindowWidth = 1280;
    uint32_t WindowHeight = 720;
    bool VSync = true;
};

class Application
{
public:
    Application(const ApplicationDescription& description);

    void Run();

    void OnEvent(Event& event);

    const ApplicationDescription& GetDescription() const { return m_Description; }
    const Window* GetWindow() const { return m_Window.get(); }

    inline static Application* GetInstance() { return ms_Instance; }
private:
    bool OnWindowClosed(WindowClosedEvent& event);
    bool OnWindowResized(WindowResizedEvent& event);
    bool OnKeyPressed(KeyPressedEvent& event);
    bool OnMouseButtonPressed(MouseButtonPressedEvent& event);
    bool OnMouseScrolledPressed(MouseScrolledEvent& event);
private:
    ApplicationDescription m_Description;
    std::unique_ptr<Window> m_Window;

    bool m_IsRunning = false;

    static Application* ms_Instance;
};