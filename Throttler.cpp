#include "Throttler.hpp"

std::unordered_map<std::size_t,AbstractThrottle*> Throttler::funcMap;
std::mutex Throttler::mapMutex;

ThrottleError::ThrottleError(const char *info):errorString(info){}

ThrottleError::ThrottleError(const std::string &info):errorString(info){}

ThrottleError::ThrottleError(std::string &&info):errorString(std::move(info)){}

const char *ThrottleError::what() const noexcept
{
    return errorString.data();
}
