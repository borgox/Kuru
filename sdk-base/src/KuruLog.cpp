#include <Kuru/Log.hpp>

#include <iostream>
#include <string>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace Kuru {
namespace {
LogFunc gLogger = nullptr;
}

void SetLogger(LogFunc logger) noexcept {
    gLogger = logger;
}

void Log(LogLevel level, std::string_view message) {
    const std::string owned(message);
    if (gLogger) {
        gLogger(level, owned.c_str());
        return;
    }
    // Default: OutputDebugString
#if defined(_WIN32)
    OutputDebugStringA(("[Kuru] " + owned + "\n").c_str());
#else
    std::clog << owned << '\n';
#endif
}

void Trace(std::string_view message) { Log(LogLevel::Trace, message); }
void Info(std::string_view message) { Log(LogLevel::Info, message); }
void Warning(std::string_view message) { Log(LogLevel::Warning, message); }
void Error(std::string_view message) { Log(LogLevel::Error, message); }

} // namespace Kuru
