#pragma once

#include "core/core.h"

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
};

class Application
{
public:
    Application(const ApplicationDescription& description);

    void Run();

    const ApplicationDescription& GetDescription() const { return m_Description; }

    inline static Application* GetInstance() { return ms_Instance; }
private:
    ApplicationDescription m_Description;
    bool m_IsRunning = false;

    static Application* ms_Instance;
};