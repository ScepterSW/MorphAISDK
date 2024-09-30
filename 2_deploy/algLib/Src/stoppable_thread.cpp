#include "stoppable_thread.h"
#include "log.h"

#define FN __FUNCTION__ //__func__
StoppableThread::StoppableThread(const char* threadName) :
	m_th(),
    m_thread_state(INIT),
	m_thread_name(threadName)
{}

StoppableThread::~StoppableThread()
{
	if (RUNNING == m_thread_state)
	{
		Stop();
        Join();
	}
}

int StoppableThread::Start()
{
    Log("%s", m_thread_name.c_str());
	if (INIT == m_thread_state
    || STOP == m_thread_state)
    {
        m_thread_state = RUNNING;
        m_th = thread(&StoppableThread::Process, this);
        pthread_setname_np(m_th.native_handle(), "alg");
	}
	else
	{
        Log("Do not repeat invoke Start:%s, if not Stop previous.", m_thread_name.c_str());
	}
	return 0;
}

//Checks if thread is requested to stop
bool StoppableThread::IsRunning()
{
	return (RUNNING == m_thread_state)?true:false;
}

void StoppableThread::Stop()
{
	Log("%s", m_thread_name.c_str());
	m_thread_state = STOP;
}

bool StoppableThread::IsStoped()
{
    return (STOP == m_thread_state)?true:false;
}

// Request the thread to stop by setting value in promise object
void StoppableThread::Join()
{
    Log("%s begin", m_thread_name.c_str());
    if (true == m_th.joinable())
    {
        m_th.join();
    }
    Log("%s end", m_thread_name.c_str());
}

void StoppableThread::Process()
{
    while (RUNNING == m_thread_state)
    {
        this->Run();
    }
}

short StoppableThread::CorePin(int coreID)
{
#ifdef RV1126
  short status=0;

  cpu_set_t set;

  CPU_ZERO(&set);

  if(coreID == -1)
  {
    status=-1;

    return status;
  }

  CPU_SET(coreID,&set);
  if(sched_setaffinity(0, sizeof(cpu_set_t), &set) < 0)
  {
    return -1;
  }
#endif
  return 1;
}
