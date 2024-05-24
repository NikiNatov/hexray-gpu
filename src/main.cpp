#include "core/application.h"

int main(int argc, char** argv)
{
#if defined(HEXRAY_DEBUG)
    // Memory leaks check
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    ApplicationDescription appDesc;
    appDesc.CommandLineArgs.Count = argc;
    appDesc.CommandLineArgs.Args = argv;

    Application* app = new Application(appDesc);
    app->Run();
    delete app;
}