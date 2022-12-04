
#include <assert.h>
#include <string>
#include <map>
#include <iostream>
#include <atomic>
#include <random>

#include <Remotery.h>
#include <windows.h>

#include "Utils.h"
#include "WorkQueue.h"
#include "Semaphore.h"
#include "Strand.h"

long ranged_rand(long a, long b)
{
	return a + random_at_most(b - a);
}

struct ThreadInfo
{
	std::string name;
	std::thread th;
	double totalTime = 0;
	double totalWork = 0;
	double totalBlocked = 0;
};

Spinner gSpinner;

struct Foo {
	explicit Foo(int n, WorkQueue& wq) : strand(wq)
	{
		name = "Conn " + std::to_string(n);
	}
    
	void doWorkLocked(int durationMs) {
		using namespace std::chrono;

		auto workStart = nowMs();
		auto th = (*Callstack<ThreadInfo>::begin())->getKey();

		rmt_BeginCPUSampleDynamic(name.c_str(), 0);

		// Do the blocking, and time it
		auto blockingStart = nowMs();
		rmt_BeginCPUSample(Blocked, 0);
		mtx.lock();
		rmt_EndCPUSample();
		auto blockingEnd = nowMs();

		// Work
		rmt_BeginCPUSample(Working, 0);
		gSpinner.spinMs(durationMs);
		mtx.unlock();
		rmt_EndCPUSample();

		rmt_EndCPUSample();
		auto workEnd = nowMs();

		auto blocked = blockingEnd - blockingStart;
		auto work = (workEnd - workStart) - blocked;
		totalWork += work;
		totalBlocked += blocked;
		th->totalWork += work;
		th->totalBlocked += blocked;
	}

	void doWorkUnlocked(int durationMs)
	{
		using namespace std::chrono;
		auto workStart = nowMs();
		auto th = (*Callstack<ThreadInfo>::begin())->getKey();

		rmt_BeginCPUSampleDynamic(name.c_str(), 0);

		// Work
		rmt_BeginCPUSample(Working, 0);
		gSpinner.spinMs(durationMs);
		rmt_EndCPUSample();

		rmt_EndCPUSample();
		auto workEnd = nowMs();

		auto work = (workEnd - workStart);
		totalWork += work;
		th->totalWork += work;
	}

	std::mutex mtx;
	std::string name;
	double totalWork = 0;
	double totalBlocked = 0;

	Strand<WorkQueue> strand;
};


// What is available to run
#define WHAT_NOSTRANDS 1
#define WHAT_STRANDS 2
#define WHAT_SAMPLE 3

// Set this to one of the above, to specify what code to run 
#define WHAT WHAT_NOSTRANDS

#define NUM_THREADS 4
#define NUM_OBJECTS 8
#define WORKDURATION_MIN 5
#define WORKDURATION_MAX 15

void strandSample();

int main()
{
#if WHAT==WHAT_SAMPLE
	strandSample();
	return 0;
#endif

	time_t t;
	srand((unsigned)time(&t));
	timeBeginPeriod(1);

    Remotery* rmt;
    rmt_CreateGlobalInstance(&rmt);
	rmt_SetCurrentThreadName("MainThread");

	WorkQueue wq;

	std::vector<ThreadInfo> ths;
	ZeroSemaphore threadsReady;
	ZeroSemaphore threadsRunning;

	ths.resize(NUM_THREADS);
	for (int i = 0; i < NUM_THREADS; i++)
	{
		threadsReady.increment();
		threadsRunning.increment();
		ths[i].th = std::thread([&wq, &threadsReady, &threadsRunning, this_=&ths[i], i]
		{
			this_->name = formatStr("WorkerThread %d", i);
			rmt_SetCurrentThreadName(this_->name.c_str());

			threadsReady.decrement();
			auto start = nowMs();
			Callstack<ThreadInfo>::Context ctx(this_);
			wq.run();
			threadsRunning.decrement();
			this_->totalTime = nowMs() - start;
		});
	}

	std::vector<std::unique_ptr<Foo>> objs;
	for (int i = 0; i < NUM_OBJECTS; i++)
	{
		objs.push_back(std::make_unique<Foo>(i, wq));
	}

	int totalWorkMs = 0;

	std::atomic<int> itemsDone(0);
	int itemsTodo = 1000;

	// Prepare items beforehand, so it doesn't interfere with the actual work timing
	std::vector<std::function<void()>> items;
	for (int i = 0; i < itemsTodo; i++)
	{
		auto& obj = objs[ranged_rand(0, objs.size() - 1)];
		int ms = ranged_rand(WORKDURATION_MIN, WORKDURATION_MAX);
		totalWorkMs += ms;
		items.push_back([&obj, ms, &itemsDone]{
#if WHAT==WHAT_STRANDS
			obj->strand.dispatch([&obj, ms, &itemsDone] {
				itemsDone++;
				obj->doWorkUnlocked(ms);
			});
#else
			itemsDone++;
			obj->doWorkLocked(ms);
#endif
		});
	}

	threadsReady.wait();
	auto start = nowMs();
    // Let's go
	for (auto&& item : items)
		wq.push(std::move(item));
	wq.stop();
	threadsRunning.wait();
	auto end = nowMs();

	double mainThreadTotalTime = end - start;
	assert(itemsDone.load() == itemsTodo);

	for (auto&& t : ths)
		t.th.join();

    // Print statistics
	double totalWork = 0;
	double totalBlocked = 0;
	for(auto& obj : objs)
	{
		totalWork += obj->totalWork;
		totalBlocked += obj->totalBlocked;
		printf("%s: totalWork=%5.2f totalBlocked=%5.2f . BlockingOverhead=%3.2f\n",
			obj->name.c_str(), obj->totalWork, obj->totalBlocked, (obj->totalBlocked * 100) / (obj->totalWork + obj->totalBlocked));
	}

	{
		printf("THREADS\n");
		double totalWork = 0;
		double totalBlocked = 0;
		double totalTime = 0;
		for (auto& t : ths)
		{
			totalWork += t.totalWork;
			totalBlocked += t.totalBlocked;
			totalTime += t.totalTime;
			printf("\t%s: totalTime=%5.2f totalWork=%5.2f totalOverhead=%3.2f%%\n",
				t.name.c_str(), t.totalTime, t.totalWork, ((t.totalTime-t.totalWork) * 100) / (t.totalTime));
		}
		printf("\tTOTAL: totalTime=%5.3f totalWork=%5.3f totalOverhead=%3.3f%%\n",
			totalTime, totalWork, ((totalTime-totalWork) * 100) / (totalTime));
	}

	double totalOverhead = (1 - ((totalWork / NUM_THREADS) / mainThreadTotalTime)) * 100;
	printf("MAINTHREAD: totaTime=%5.2f totalWork=%5.2f totalOverhead=%3.2f%%\n",
		mainThreadTotalTime, totalWork, totalOverhead);

	Sleep(3000);
	printf("Done\n");
	rmt_DestroyGlobalInstance(rmt);
}