#pragma once
#ifdef DOLPHIN
#include <common/log.h>
#include <fmt/format.h>
#else
#include <spdlog/spdlog.h>
#endif
#include <string>
#include <string_view>
#include <vector>

namespace decaf
{

void
initialiseLogging(std::string_view filename);

#ifdef DOLPHIN
std::shared_ptr<Logger>
makeLogger(std::string name);
#else
std::shared_ptr<spdlog::logger>
makeLogger(std::string name,
           std::vector<spdlog::sink_ptr> userSinks = {});
#endif

} // namespace decaf
