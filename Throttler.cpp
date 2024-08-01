#include "Throttler.hpp"

std::unordered_map<std::size_t,AbstractThrottle*> Throttler::funcMap;
Throttler* Throttler::instance = nullptr;
