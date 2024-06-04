#pragma once

#include "core/core.h"
#include "core/event.h"
#include "core/window.h"
#include "rendering/graphicscontext.h"
#include "rendering/renderer.h"
#include "scene/scene.h"

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
    bool EnableAPIValidation = true;
};

class Application
{
public:
    Application(const ApplicationDescription& description);

    void Run();

    void OnEvent(Event& event);

    const ApplicationDescription& GetDescription() const { return m_Description; }
    const Window* GetWindow() const { return m_Window.get(); }
    inline GraphicsContext* GetGraphicsContext() { return m_GraphicsContext.get(); }

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
    std::unique_ptr<GraphicsContext> m_GraphicsContext;
    std::unique_ptr<Scene> m_Scene;
    std::shared_ptr<Renderer> m_SceneRenderer;



    bool m_IsRunning = false;

    static Application* ms_Instance;
};