#include "Log.hpp"
#include <iostream>

Logger::Logger() {
    std::lock_guard lock{m_mutex};
    m_outfile.open("Log.txt");
}

void Logger::msg(std::string_view message) {
    std::lock_guard lock{m_mutex};
    m_outfile << message << std::endl;
    std::cout << "LOG: " << message << std::endl;
}

Logger::~Logger() {
    m_outfile.close();
}
Logger Logger::lg;