#pragma once

#include <mutex>

template <class T>
class Monitor {
private:
    mutable T t_;
    mutable std::mutex mtx_;

public:
    using Type = T;
    Monitor() {}
    Monitor(T t_) : t_(std::move(t_)) {}
    template <typename F>
    auto operator()(F f) const -> decltype(f(t_)) {
        std::lock_guard<std::mutex> hold{mtx_};
        return f(t_);
    }
};

