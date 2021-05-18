#include <thread>
#include <mutex>
#include <string_view>
#include <fstream>

class Logger {
    Logger();
public:
    Logger(Logger const& other) = delete;
    Logger& operator=(Logger const& other) = delete;
    Logger(Logger&& other) = delete;
    Logger& operator=(Logger&& other) = delete;
    ~Logger();

    void msg(std::string_view message);
    static Logger lg;

private:
    std::mutex m_mutex;
    std::ofstream m_outfile;
};
