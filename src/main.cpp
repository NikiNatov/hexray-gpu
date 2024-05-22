#include "core/logger.h"

int main(int argc, char** argv)
{
    Logger::Initialize();

    HEXRAY_TRACE("This is trace message!");
    HEXRAY_INFO("This is info message!");
    HEXRAY_WARNING("This is warning message!");
    HEXRAY_ERROR("This is error message!");
    HEXRAY_ASSERT(false, "This is assert {}", "msg");
}