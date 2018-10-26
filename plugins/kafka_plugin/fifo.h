#pragma once

#include <mutex>
#include <condition_variable>
#include <atomic>
#include <deque>
#include <vector>
#include <boost/noncopyable.hpp>

namespace kafka {

constexpr std::size_t FIFO_MAX_PUSH_SIZE = 1024;
constexpr std::size_t FIFO_MAX_POP_SIZE = 1024;

template <typename T>
class fifo : public boost::noncopyable {
public:
    fifo(std::size_t max_push_size = FIFO_MAX_PUSH_SIZE, std::size_t max_pop_size = FIFO_MAX_POP_SIZE);
    void push(const T& element);
    std::vector<T> pop();
    bool empty();
    void awaken();

private:
    std::mutex mux_;
    std::condition_variable not_empty_cv_;
    std::condition_variable not_full_cv_;
    bool non_blocking_{};
    std::deque<T> deque_;
    std::size_t max_push_size_;
    std::size_t max_pop_size_;
};

template <typename T>
fifo<T>::fifo(std::size_t max_push_size, std::size_t max_pop_size) {
    max_push_size_ = max_push_size;
    max_pop_size_ = max_pop_size;
}

template <typename T>
void fifo<T>::push(const T& element) {
    std::unique_lock<std::mutex> lock(mux_);

    if (deque_.size() >= max_push_size_) {
        not_full_cv_.wait(lock, [&] {
            return non_blocking_ || deque_.size() < max_push_size_;
        });
    }

    deque_.push_back(element);
    not_empty_cv_.notify_one();
}

template <typename T>
std::vector<T> fifo<T>::pop() {
    std::unique_lock<std::mutex> lock(mux_);
    if (deque_.empty()) {
        not_empty_cv_.wait(lock, [&] {
            return non_blocking_ || !deque_.empty();
        });
    }

    std::vector<T> result;
    for (std::size_t i = 0; i < max_pop_size_ && !deque_.empty(); ++i) {
        result.push_back(std::move(deque_.front()));
        deque_.pop_front();
    }
    not_full_cv_.notify_all();
    return result;
}

template <typename T>
bool fifo<T>::empty() {
    std::unique_lock<std::mutex> lock(mux_);
    return deque_.empty();
}

template <typename T>
void fifo<T>::awaken() {
    non_blocking_ = true;
    not_empty_cv_.notify_all();
    not_full_cv_.notify_all();
}

}
