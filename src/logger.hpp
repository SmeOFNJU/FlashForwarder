#ifndef __UMDGW_LOGGER_HPP_INCLUDED__
#define __UMDGW_LOGGER_HPP_INCLUDED__

#include"log4cplus/loggingmacros.h"
#include"log4cplus/configurator.h"
#include"log4cplus/logger.h"
#include<string>
#include<vector>
namespace umdgw {
  using namespace log4cplus;
  class logger_t {
  public:
    static void initialize(std::string config) {
      if (initialized_ == true)
        return;
      PropertyConfigurator configurator(config);
      configurator.configure();
      logger_.push_back(Logger::getRoot());
      initialized_ = true;
    }
    static std::vector<Logger> logger_;
  private:
    static bool initialized_;
    logger_t() {};
    logger_t(const logger_t&) = delete;
    logger_t operator=(const logger_t&) = delete;

  };

#define LOGINFO(message) LOG4CPLUS_INFO(logger_t::logger_[0], LOG4CPLUS_TEXT(message));
#define LOGDEBUG(message) LOG4CPLUS_DEBUG(logger_t::logger_[0], LOG4CPLUS_TEXT(message));
#define LOGERROR(message) LOG4CPLUS_ERROR(logger_t::logger_[0], LOG4CPLUS_TEXT(message));
#define LOGTRACE(message) LOG4CPLUS_TRACE(logger_t::logger_[0], LOG4CPLUS_TEXT(message));
#define LOGERROR(message) LOG4CPLUS_ERROR(logger_t::logger_[0], LOG4CPLUS_TEXT(message));
#define LOGFATAL(message) LOG4CPLUS_FATAL(logger_t::logger_[0], LOG4CPLUS_TEXT(message));
}




#endif // !__UMDGW_LOGGER_HPP_INCLUDED__

