#include <thread>
#include <mutex>
#include <string_view>
#include <fstream>

class Logger {
    public:
        Logger(Logger const& other) = delete;
        Logger& operator=(Logger const& other) = delete;
        Logger(Logger&& other) = delete;
        Logger& operator=(Logger&& other) = delete;

        Logger();
        ~Logger();
        

    static Logger& instance();
    void msg(std::string_view message);

    private:
        std::mutex m_mutex;
        std::ofstream m_outfile;

};

template <typename T>
void LOG(T&& t) {
    
    Logger::instance().msg(std::forward<T>(t));
}