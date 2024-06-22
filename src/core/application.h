#pragma once

#include "core/core.h"
#include "core/event.h"
#include "core/window.h"
#include "core/timer.h"
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
    ~Application();

    void Run();

    void OnEvent(Event& event);

    const ApplicationDescription& GetDescription() const { return m_Description; }
    Window* GetWindow() { return m_Window.get(); }
    const Window* GetWindow() const { return m_Window.get(); }
    inline GraphicsContext* GetGraphicsContext() { return m_GraphicsContext.get(); }

    FORCEINLINE void SetFixedDeltaTime(double deltaTime)
    {
        m_FixedDeltaTime = deltaTime;
    }

    FORCEINLINE void SetMaxDeltaTime(double deltaTime)
    {
        m_MaxDeltaTime = deltaTime;
    }

    FORCEINLINE double GetDeltaTime() const
    {
        return m_DeltaTime;
    }

    FORCEINLINE double GetGameTime() const
    {
        return m_CurrentTime;
    }

    FORCEINLINE double GetRealTime() const
    {
        return m_RealTime;
    }

    FORCEINLINE double GetAvgDeltaTimeMS() const
    {
        return m_AvgDeltaTimeMS;
    }

    FORCEINLINE double GetAvgFPS() const
    {
        return 1000. / m_AvgDeltaTimeMS;
    }

    inline void SetMaxFPS(uint16_t fps)
    {
        m_MaxFPS = fps;
    }

    const std::filesystem::path& GetExecutablePath() const { return m_ExecutablePath; }

    inline static Application* GetInstance() { return ms_Instance; }
private:
    bool OnWindowClosed(WindowClosedEvent& event);
    bool OnWindowResized(WindowResizedEvent& event);
    bool OnKeyPressed(KeyPressedEvent& event);
    bool OnMouseButtonPressed(MouseButtonPressedEvent& event);
    bool OnMouseScrolledPressed(MouseScrolledEvent& event);
private:
    void BeginFrame();
    void Update();
    void EndFrame();
private:
    void CompileShaders();
    void ParseCommandlineArgs();
    void OpenScene(const std::filesystem::path& filepath);
    void InitSceneRenderer();
private:
    ApplicationDescription m_Description;
    std::unique_ptr<Window> m_Window;
    std::unique_ptr<GraphicsContext> m_GraphicsContext;
    std::shared_ptr<Scene> m_Scene;
    std::shared_ptr<Renderer> m_SceneRenderer;

    Timer m_Timer;
    double m_DeltaTime;
    double m_CurrentTime;
    double m_RealTime;
    double m_LastTime;
    double m_MaxDeltaTime;
    double m_FixedDeltaTime;
    double m_AvgDeltaTimeMS;
    uint16_t m_MaxFPS;
    bool m_IsRunning = false;

    RendererDescription m_RendererDescription;
    std::filesystem::path m_ExecutablePath;

    static Application* ms_Instance;
};