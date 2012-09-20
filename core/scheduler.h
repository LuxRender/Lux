#pragma once

#include <vector>

#include <boost/thread.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/bind.hpp>
#include <boost/version.hpp>
#include <boost/function.hpp>

#include <boost/interprocess/detail/atomic.hpp>

#if (BOOST_VERSION < 104800)
using boost::interprocess::detail::atomic_inc32;
#else
using boost::interprocess::ipcdetail::atomic_inc32;
#endif

/*
 * TODO:
 *
 * - Better documentation of API
 * - deleting of ended thread:local memory
 *   - by mean of Done function
 *   - by DelThread
 * - Pause/Resume function
 *   - use a barrier instead of a crappy sleep
 *   - should this code move at the end of each blocks ?
*/

namespace scheduling
{

class Scheduler;
class Thread;
class Range;

class Thread
{
public:
	virtual void Init() {}
	virtual void End() {}
	virtual ~Thread() {};

friend class Scheduler;
friend class Range;

private:
	static void Body(Thread* thread, Scheduler *scheduler);

	boost::thread thread;
	bool active;
};

typedef boost::function<void(Range *range)> TaskType;

class Scheduler
{
public:
	Scheduler(unsigned step);
	~Scheduler();

	void Launch(TaskType task, unsigned b_min, unsigned b_max, unsigned force_step=0);

	void Pause();
	void Resume();
	void Stop();
	void Done();

	void AddThread(Thread *thread);
	void DelThread();
	unsigned ThreadCount() const
	{
		return threads.size();
	}

	void FreeThreadLocalStorage();

friend class Thread;
friend class Range;

private:
	enum {PAUSED, RUNNING} state;

	TaskType GetTask();

	bool EndTask(Thread* thread);

	std::vector<Thread*> threads;
	std::vector<Thread*> threads_finished;

	TaskType current_task;

	boost::mutex mutex;
	boost::condition_variable condition;
	unsigned counter;

	unsigned start;
	unsigned end;
	unsigned current;
	unsigned step;
	unsigned default_step;
};

class Range
{
public:
	unsigned begin()
	{
		return atomic_init();
	}

	unsigned end()
	{
		return ~0u;
	}

	unsigned next()
	{
		if(++current < max)
			return current;
		
		// handle pause
		while (scheduler->state == Scheduler::PAUSED)
		{
			boost::this_thread::sleep(boost::posix_time::seconds(1));
		}

		return atomic_init();
	}

	// public for thread local data access
	Thread *thread;

friend class Thread;

private:
	unsigned atomic_init()
	{
		if(!thread->active)
			return end();

		unsigned new_value = scheduler->step * atomic_inc32(&scheduler->current);

		if(new_value < scheduler->end)
		{
			max = std::min(scheduler->end, new_value + scheduler->step);
			current = new_value;
			return new_value;
		}
		return end();
	}

	Range(Scheduler *sched, Thread *thread_data);

	unsigned current;
	unsigned max;

	Scheduler *scheduler;
};

}
