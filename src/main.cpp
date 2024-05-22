#include "core/application.h"

int main(int argc, char** argv)
{
    ApplicationDescription appDesc;
    appDesc.CommandLineArgs.Count = argc;
    appDesc.CommandLineArgs.Args = argv;

    Application* app = new Application(appDesc);
    app->Run();
    delete app;
}