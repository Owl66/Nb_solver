#include "Log.hpp"
#include <iostream>


Logger& Logger::instance() {
    static Logger lg;
    return lg;
}

void Logger::msg(std::string_view message) {
    std::lock_guard lock{m_mutex};
    std::cout << "LOG: " << message << std::endl;
}