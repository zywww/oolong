#include <stdlib.h>
#include <iostream>
#include <string>
#include <sstream>

namespace oolong
{
    // example: LogInfo << "hello world";
    // date pid level content sourceFile:line
    // 2018-10-10 07:13:04 1345 INFO - hello world - Logger.cpp:32
    class Logger
    {
    public:
        enum LogLevel
        {
            Debug,
            Info,
            Warn,
            Error,
            Fatal,
            EnumSize
        };

        Logger(LogLevel level, const std::string& srcFile, uint32_t line, const std::string& functionName) :
            level_(level),
            fileInfo_(srcFile + ":" + std::to_string(line)),
            functionName_(functionName)
        {
            // todo 是否自动log errno
            // todo 日志头差日期 pid 
            const char* levelName[LogLevel::EnumSize] = 
            {
                "Debug",
                "Info",
                "Warn",
                "Error",
                "Fatal"
            };
            ss_ << levelName[level];
        }

        ~Logger()
        {
            if (level_ >= logLevel()) 
            {
                ss_ << " - " << fileInfo_ << " " << functionName_ << "()\n";
                std::cout << ss_.str();
            }
            if (level_ > LogLevel::Fatal)
                abort();
        }

        static LogLevel logLevel();
        static void setLogLevel(LogLevel level);

        std::ostream& stream() { return ss_ << " - "; }

    private:
        LogLevel level_;
        std::string fileInfo_;
        std::string functionName_;
        std::stringstream ss_;
    };
    
    extern Logger::LogLevel g_logLevel;
    inline Logger::LogLevel Logger::logLevel() { return g_logLevel; }
    inline void Logger::setLogLevel(Logger::LogLevel level) { g_logLevel = level; }

} // namespace oolong

#define LogDebug oolong::Logger(oolong::Logger::Debug, __FILE__, __LINE__, __FUNCTION__).stream()
#define LogInfo oolong::Logger(oolong::Logger::Info, __FILE__, __LINE__, __FUNCTION__).stream()
#define LogWarn oolong::Logger(oolong::Logger::Warn, __FILE__, __LINE__, __FUNCTION__).stream()
#define LogError oolong::Logger(oolong::Logger::Error, __FILE__, __LINE__, __FUNCTION__).stream()
#define LogFatal oolong::Logger(oolong::Logger::Fatal, __FILE__, __LINE__, __FUNCTION__).stream()


// todo 是否提供条件log语句,如 LogInfoIf(1>3) << "1 > 3";