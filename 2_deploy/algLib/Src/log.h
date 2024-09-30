#ifndef _LOG_H_
#define _LOG_H_

#include <stdio.h>
#include <cstdarg>
#include <string>
#include <memory.h>
#include <time.h>
#include <sys/timeb.h>
using namespace std;

inline const char* basename(const char* fileName)
{
	std::string fileNameStr = fileName;
#ifdef _WIN32
	size_t index = fileNameStr.find_last_of('\\');
#elif defined Linux
    std::string::size_type index = fileNameStr.find_last_of('/');
#endif // _WIN32

    return (std::string::npos != index) ? (fileName + index + 1) : fileName;
}

#define	Log(FMT, ...)		LogCustom::Printf("[%s:%d:%s]:" FMT "\n",	\
                            basename(__FILE__), __LINE__, __FUNCTION__, ## __VA_ARGS__)

string GetTime(void);
string GetDateTime(void);

class LogCustom
{
	void Init();
	LogCustom(const string& logDir, const string& logFileName, const string& logPreFix, const bool isLogToConsole);
public:
	static void Initance(const string& logDir, const string& logFileName, const string& logPreFix, const bool isLogToConsole = false);
	static void Release();

	~LogCustom();
	static const char* Printf(const char* format, ...);
    static int64_t GetTimeStampMS();
    static int64_t GetTimeStampS();

private:
	static bool m_LogToConsole;
	static FILE * m_PLogFile;
	static string m_LogDirectory;
	static string m_LogFileName;
	static string m_LogPreFix;
};
#endif

