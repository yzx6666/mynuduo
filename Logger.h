#pragma once

#include "noncopyable.h"

#include <string>

// LOG_INFO("%s %d", arg1, arg2)
#define LOG_INFO(logmsgFormat, ...) \
    do \
    {  \
        Logger &logger = Logger::instance(); \
        logger.setLogLevel(INFO); \
        char buf[1024] = {0}; \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf); \
    } while(0)

#define LOG_ERROR(logmsgFormat, ...) \
    do \
    {  \
        Logger &logger = Logger::instance(); \
        logger.setLogLevel(ERROR); \
        char buf[1024] = {0}; \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf); \
    } while(0)

#define LOG_FATAL(logmsgFormat, ...) \
    do \
    {  \
        Logger &logger = Logger::instance(); \
        logger.setLogLevel(FATAL); \
        char buf[1024] = {0}; \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf); \
        exit(-1); \
    } while(0)

#ifdef MUDBUG
#define LOG_DEBUG(logmsgFormat, ...) \
    do \
    {  \
        Logger &logger = Logger::instance(); \
        logger.setLogLevel(DEBUG); \
        char buf[1024] = {0}; \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf); \
    } while(0)
#else
    #define LOG_DEBUG(logmsgFormat, ...)
#endif

// ������־���� INFO ERROR FATAL DEBUG
enum LogLevel
{
    INFO, //��ͨ��Ϣ
    ERROR, //������Ϣ
    FATAL, //core��Ϣ
    DEBUG // ������Ϣ
};

// ���һ����־��
class Logger : noncopyable
{
public:
    // ��ȡ��־Ψһ��ʵ������
    static Logger& instance();
    // ������־����
    void setLogLevel(int level);
    // д��־
    void log(std::string msg);
private:
    int logLevel_;
    Logger(){ }
};