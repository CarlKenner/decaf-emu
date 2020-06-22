#pragma once
#ifdef DOLPHIN
#include <common/log.h>
#include <fmt/format.h>
extern std::shared_ptr<Logger>
   gCliLog;
#else
#include <spdlog/spdlog.h>
extern std::shared_ptr<spdlog::logger>
   gCliLog;
#endif

