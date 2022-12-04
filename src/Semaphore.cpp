#include "Semaphore.h"

void Semaphore::notify()
{
	std::unique_lock<std::mutex> lock(mtx_);
	count_++;
	cv_.notify_one();
}

void Semaphore::wait()
{
	std::unique_lock<std::mutex> lock(mtx_);
	cv_.wait(lock, [this]() {return count_ > 0; });
	while (count_ == 0) {
		cv_.wait(lock);
	}
	count_--;
}

bool Semaphore::trywait()
{
	std::unique_lock<std::mutex> lock(mtx_);
	if (count_)
	{
		count_--;
		return true;
	}
	else
	{
		return false;
	}
}

void ZeroSemaphore::increment()
{
	std::unique_lock<std::mutex> lock(mtx_);
	count_++;
}

void ZeroSemaphore::decrement()
{
	std::unique_lock<std::mutex> lock(mtx_);
	count_--;
	cv_.notify_all();
}

void ZeroSemaphore::wait()
{
	std::unique_lock<std::mutex> lock(mtx_);
	cv_.wait(lock, [this]()
	{
		return count_ == 0;
	});
}

bool ZeroSemaphore::trywait()
{
	std::unique_lock<std::mutex> lock(mtx_);
	if (count_ == 0)
		return true;
	else
		return false;
}