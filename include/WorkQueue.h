#pragma once
#include <mutex>
#include <condition_variable>
#include <queue>
#include <functional>

#include "Callstack.h"

// Really simple Multiple producer / Multiple consumer work queue
class WorkQueue {
public:
    // Add a new work item
    template <typename F>
    void push(F w) {
        std::lock_guard<std::mutex> lock(mtx_);
        q_.push(std::move(w));
        cond_.notify_all();
    }

    // Continuously waits for and executes any work items, until "stop" is
    // called
    void run() {
        Callstack<WorkQueue>::Context ctx(this);
        while (true) {
            std::function<void()> w;
            {
                std::unique_lock<std::mutex> lock(mtx_);
                cond_.wait(lock, [this] { return !q_.empty(); });
                w = std::move(q_.front());
                q_.pop();
            }

            if (w) {
                w();
            } else {
                // An empty work item means we are shutting down, so enqueue
                // another empty work item. This will in turn shut down another
                // thread that is executing "run"
                push(nullptr);
                return;
            }
        };
    }

    // Causes any calls to "run" to exit.
    void stop() {
        push(nullptr);
    }

    // Tells if "run" is executing in the current thread
    bool canDispatch() {
        return Callstack<WorkQueue>::contains(this) != nullptr;
    }

private:
    std::condition_variable cond_;
    std::mutex mtx_;
    std::queue<std::function<void()>> q_;
};