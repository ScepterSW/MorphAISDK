#ifndef _STOPPABLETHREAD_H_
#define _STOPPABLETHREAD_H_
#include <string>
#include <thread>
#include <atomic>

using std::string;
using std::thread;
using std::atomic;

enum ThreadState {
	INIT = -1,
	STOP = 0,
	RUNNING = 1,
};
/*
* Class that encapsulates promise and future object and
* provides API to set exit signal for the thread
*/
class StoppableThread
{
private:
	StoppableThread(StoppableThread && obj) {}
	StoppableThread & operator=(StoppableThread && obj) = delete;
protected:
    virtual void Process();
	short CorePin(int coreID);
public:
	StoppableThread(const char* threadName);
	virtual ~StoppableThread();

	// Task need to provide defination  for this function
	// It will be called by thread function
	virtual void Run() = 0;

	int Start();
	//Checks if thread is running
	bool IsRunning();
	// Request the thread to stop by setting value in promise object
	void Stop();
	bool IsStoped();
	void Join();

private:
	thread                  m_th;
    atomic<ThreadState>     m_thread_state;
	string                  m_thread_name;
};

#endif

///*
//* A Task class that extends the StoppableThread Task
//*/
//class MyTask : public StoppableThread
//{
//public:
//	// Function to be executed by thread function
//	void run()
//	{
//		std::cout << "Task Start" << std::endl;
//
//		// Check if thread is requested to stop ?
//		while (stopRequested() == false)
//		{
//			std::cout << "Doing Some Work" << std::endl;
//			std::this_thread::sleep_for(std::chrono::milliseconds(1000));
//
//		}
//		std::cout << "Task End" << std::endl;
//	}
//};
