#pragma once

#include <string_view>

namespace Kuru {

enum class LogLevel {
    Trace,
    Info,
    Warning,
    Error,
};

using LogFunc = void (*)(LogLevel level, const char* message);

void SetLogger(LogFunc logger) noexcept;
void Log(LogLevel level, std::string_view message);
void Trace(std::string_view message);
void Info(std::string_view message);
void Warning(std::string_view message);
void Error(std::string_view message);

} // namespace Kuru
