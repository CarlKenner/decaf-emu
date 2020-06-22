#include "log.h"

#ifdef DOLPHIN
#include <Common/Logging/Log.h>
#else
#include <spdlog/spdlog.h>
#endif

Logger gLog;

void
Logger::log(Level lvl, std::string_view msg)
{
#ifdef DOLPHIN
   switch (lvl) {
   case Level::trace:
   case Level::debug:
      DEBUG_LOG(WIIU, "%s", msg);
      break;
   case Level::info:
      INFO_LOG(WIIU, "%s", msg);
      break;
   case Level::warn:
      WARN_LOG(WIIU, "%s", msg);
      break;
   case Level::err:
      ERROR_LOG(WIIU, "%s", msg);
      break;
   case Level::critical:
      NOTICE_LOG(WIIU, "%s", msg);
      break;
   }
#else
   if (mLogger) {
      auto logger = reinterpret_cast<spdlog::logger *>(mLogger.get());
      logger->log(static_cast<spdlog::level::level_enum>(lvl), msg);
   }
#endif
}

bool
Logger::should_log(Level level)
{
   if (!mLogger) {
      return false;
   }

#ifdef DOLPHIN
   return true;
#else
   auto logger = reinterpret_cast<spdlog::logger *>(mLogger.get());
   return logger->should_log(static_cast<spdlog::level::level_enum>(level));
#endif
}
