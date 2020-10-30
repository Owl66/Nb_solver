#include "Log.hpp"
#include <iostream>

Logger::Logger() {
    m_outfile.open("Log.txt");
}

Logger& Logger::instance() {
    static Logger lg;
    return lg;
}

void Logger::msg(std::string_view message) {
    std::lock_guard lock{m_mutex};
    m_outfile << message << std::endl;
    std::cout << "LOG: " << message << std::endl;
}

Logger::~Logger() {
    m_outfile.close();
}