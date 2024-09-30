#include "log.h"
#include <mutex>

const int LOGFILEMAXSIZE = (10 * 1024 * 1024); //10M
const string SUFFIX = ".txt";

static mutex gmutex;

#ifdef _WIN32
#pragma warning(disable:4996)
#endif

bool	LogCustom::m_LogToConsole = true;
FILE *	LogCustom::m_PLogFile = NULL;
string	LogCustom::m_LogDirectory = string();
string	LogCustom::m_LogFileName = string();
string	LogCustom::m_LogPreFix = string();


static LogCustom* g_LogCustom = NULL;

LogCustom::LogCustom(const string& logDir, const string& logFileName, const string& logPreFix, const bool isLogToConsole)
{
	m_LogToConsole = isLogToConsole;
	m_LogDirectory = logDir;
	m_LogFileName = logFileName;
	m_LogPreFix = logPreFix;
	Init();
}

LogCustom::~LogCustom()
{
	gmutex.lock();
	if (m_PLogFile)
	{
		fclose(m_PLogFile);
		m_PLogFile = NULL;
	}
	gmutex.unlock();
}

void LogCustom::Initance(const string& logDir, const string& logFileName, const string& logPreFix, const bool isLogToConsole)
{
	if (NULL == g_LogCustom)
	{
		g_LogCustom = new LogCustom(logDir, logFileName, logPreFix, isLogToConsole);
	}
}

void  LogCustom::Release()
{
	if (NULL != g_LogCustom)
	{
		delete g_LogCustom;
		g_LogCustom = NULL;
	}
}

void LogCustom::Init()
{
	string logPath = m_LogDirectory + "/" + m_LogFileName +SUFFIX;

	printf("logPath:%s\n", logPath.c_str());
	gmutex.lock();
	m_PLogFile = fopen(logPath.c_str(), "a");
	if (NULL == m_PLogFile)
	{
        Log("fopen_s %s failed errorcode:%d !", logPath.c_str(), errno);
	}
	gmutex.unlock();
	return;
}

const char* LogCustom::Printf(const char* format, ...)
{
	std::lock_guard<std::mutex> lock(gmutex);

	static char buf[1024] = {0};
	va_list arg_ptr;
	va_start(arg_ptr, format);

	char prefixBuf[30] = "";

    sprintf(prefixBuf, "%s %s ", m_LogPreFix.c_str(), GetTime().c_str());

	memset(buf, 0, sizeof(buf));
    vsnprintf(buf, sizeof(buf), format, arg_ptr);
	
	string logStr(prefixBuf);
    logStr += buf;
#ifdef _WIN32
    OutputDebugStringA(logStr.c_str());
#endif
    if (true == m_LogToConsole)
	{
		printf("%s", buf);
	}

	if (m_PLogFile != NULL)
	{
		long nFileLen;
		fseek(m_PLogFile, 0, SEEK_END);
		nFileLen = ftell(m_PLogFile);
		if (nFileLen >= LOGFILEMAXSIZE)
		{
			//delete old backup log file
			remove((m_LogDirectory + "/bak" + m_LogFileName + SUFFIX).c_str());

			//rename log file 
			fclose(m_PLogFile);
			rename((m_LogDirectory + "/" + m_LogFileName + SUFFIX).c_str(), (m_LogDirectory + "/bak" + m_LogFileName + SUFFIX).c_str());

			//create a new log file
			m_PLogFile = fopen((m_LogDirectory + "/" + m_LogFileName + SUFFIX).c_str(), "a");
		}

		static bool showDataTime = true;
		if (true == showDataTime)
		{
            fprintf(m_PLogFile, "%s %s\n", GetTime().c_str(), GetDateTime().c_str());
			showDataTime = false;
		}
		
        fprintf(m_PLogFile, "%s %s", GetTime().c_str(), buf);
		//fprintf(m_PLogFile, "%s", buf);

		fflush(m_PLogFile);
	}

	va_end(arg_ptr);

	return buf;
}

string GetTime(void)
{
	char buf[1024];

    //beijing time
    time_t t = time(0);
	struct  tm      ptm;
	struct  timeb   stTimeb;

	ftime(&stTimeb);
#ifdef _WIN32
	localtime_s(&ptm, &t);
#elif defined Linux
	localtime_r(&t, &ptm);
#endif

	memset(buf, 0, sizeof(buf));
    sprintf(buf, "%02d:%02d:%02d-%03d", ptm.tm_hour, ptm.tm_min, ptm.tm_sec, stTimeb.millitm);

	return string(buf);
}

string GetDateTime(void)
{
	char buf[1024];

	//beijing time
	time_t t = time(0);
	struct  tm      ptm;
	struct  timeb   stTimeb;

	ftime(&stTimeb);
#ifdef _WIN32
	localtime_s(&ptm, &t);
#elif defined Linux
	localtime_r(&t, &ptm);
#endif

	sprintf(buf, "%04d/%02d/%02d %02d:%02d:%02d-%03d", ptm.tm_year + 1900, ptm.tm_mon + 1, ptm.tm_mday, ptm.tm_hour, ptm.tm_min, ptm.tm_sec, stTimeb.millitm);

	return string(buf);
}

int64_t LogCustom::GetTimeStampMS()
{
    std::chrono::milliseconds us =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch());
    return us.count();
}

int64_t LogCustom::GetTimeStampS()
{
    std::chrono::seconds us =
        std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now().time_since_epoch());
    return us.count();
}