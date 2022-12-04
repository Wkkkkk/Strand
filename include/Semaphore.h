#pragma once

#include <mutex>
#include <condition_variable>

class Semaphore {
public:
    Semaphore(unsigned int count = 0) : count_(count) {}
    void notify();
    void wait();
    bool trywait();

private:
    std::mutex mtx_;
    std::condition_variable cv_;
    unsigned int count_;
};

// Blocks until the counter reaches zero
class ZeroSemaphore {
public:
    ZeroSemaphore() {}
    void increment();
    void decrement();
    void wait();
    bool trywait();

private:
    std::mutex mtx_;
    std::condition_variable cv_;
    int count_ = 0;
};