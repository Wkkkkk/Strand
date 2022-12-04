#pragma once

#include <assert.h>
#include <queue>
#include <functional>

#include "Callstack.h"
#include "Monitor.h"

//
// A strand serializes handler execution.
// It guarantees the following:
// - No handlers executes concurrently
// - Handlers are only executed from the specified Processor
// - Handler execution order is not guaranteed
//
// Specified Processor must implement the following interface:
//
//	template <typename F> void Processor::push(F w);
//		Add a new work item to the processor. F is a callable convertible
// to std::function<void()>
//
//	bool Processor::canDispatch();
//		Should return true if we are in the Processor's dispatching function in
// the current thread.
//
template <typename Processor>
class Strand {
public:
    Strand(Processor& proc) : m_proc(proc) {}

    Strand(const Strand&) = delete;
    Strand& operator=(const Strand&) = delete;

    // Executes the handler immediately if all the strand guarantees are met,
    // or posts the handler for later execution if the guarantees are not met
    // from inside this call
    template <typename F>
    void dispatch(F handler) {
        // If we are not currently in the processor dispatching function (in
        // this thread), then we cannot possibly execute the handler here, so
        // enqueue it and bail out
        if (!m_proc.canDispatch()) {
            post(std::move(handler));
            return;
        }

        // NOTE: At this point we know we are in a worker thread (because of the
        // check above)

        // If we are running the strand in this thread, then we can execute the
        // handler immediately without any other checks, since by design no
        // other threads can be running the strand
        if (runningInThisThread()) {
            handler();
            return;
        }

        // At this point we know we are in a worker thread, but not running the
        // strand in this thread.
        // The strand can still be running in another worker thread, so we need
        // to atomically enqueue the handler for the other thread to execute OR
        // mark the strand as running in this thread
        auto trigger = m_data([&](Data& data) {
            if (data.running) {
                data.q.push(std::move(handler));
                return false;
            } else {
                data.running = true;
                return true;
            }
        });

        if (trigger) {
            // Add a marker to the callstack, so the handler knows the strand is
            // running in the current thread
            Callstack<Strand>::Context ctx(this);
            handler();

            // Run any remaining handlers.
            // At this point we own the strand (It's marked as running in
            // this thread), and we don't release it until the queue is empty.
            // This means any other threads adding handlers to the strand will
            // enqueue them, and they will be executed here.
            run();
        }
    }

    // Post an handler for execution and returns immediately.
    // The handler is never executed as part of this call.
    template <typename F>
    void post(F handler) {
        // We atomically enqueue the handler AND check if we need to start the
        // running process.
        bool trigger = m_data([&](Data& data) {
            data.q.push(std::move(handler));
            if (data.running) {
                return false;
            } else {
                data.running = true;
                return true;
            }
        });

        // The strand was not running, so trigger a run
        if (trigger) {
            m_proc.push([this] { run(); });
        }
    }

    // Checks if we are currently running the strand in this thread
    bool runningInThisThread() {
        return Callstack<Strand>::contains(this) != nullptr;
    }

private:
    // Processes any enqueued handlers.
    // This assumes the strand is marked as running.
    // When there are no more handlers, it marks the strand as not running.
    void run() {
        Callstack<Strand>::Context ctx(this);
        while (true) {
            std::function<void()> handler;
            m_data([&](Data& data) {
                assert(data.running);
                if (data.q.size()) {
                    handler = std::move(data.q.front());
                    data.q.pop();
                } else {
                    data.running = false;
                }
            });

            if (handler)
                handler();
            else
                return;
        }
    }

    struct Data {
        bool running = false;
        std::queue<std::function<void()>> q;
    };
    Monitor<Data> m_data;
    Processor& m_proc;
};
