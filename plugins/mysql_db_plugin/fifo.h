/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <mutex>
#include <condition_variable>
#include <atomic>
#include <deque>
#include <vector>
#include <boost/noncopyable.hpp>

namespace eosio {

constexpr std::size_t FIFO_MAX_SIZE = 1000; // TODO: is cache useless?
constexpr std::size_t FIFO_POP_SIZE = 1000;

template <typename T>
class fifo : public boost::noncopyable {
public:
    enum class behavior {
        blocking, not_blocking
    };

    fifo(std::size_t max_size = FIFO_MAX_SIZE);
    void push(const T& element);
    std::vector<T> pop(std::size_t num = FIFO_POP_SIZE);
    void set_behavior(behavior value);
    void awaken();

private:
    std::mutex mux_;
    std::condition_variable not_empty_cv_;
    std::condition_variable not_full_cv_;
    std::atomic<behavior> behavior_{behavior::blocking};
    std::deque<T> deque_;
    std::size_t max_size_;
};

template <typename T>
fifo<T>::fifo(std::size_t max_size) {
    max_size_ = max_size;
}

template <typename T>
void fifo<T>::push(const T& element) {
    std::unique_lock<std::mutex> lock(mux_);

    if (deque_.size() >= max_size_) {
        not_full_cv_.wait(lock, [&] {
            return behavior_ == behavior::not_blocking || deque_.size() < max_size_;
        });
    }

    deque_.push_back(element);
    not_empty_cv_.notify_one();
}

template <typename T>
std::vector<T> fifo<T>::pop(std::size_t num) {
    std::unique_lock<std::mutex> lock(mux_);
    if (deque_.empty()) {
        not_empty_cv_.wait(lock, [&] {
            return behavior_ == behavior::not_blocking || !deque_.empty();
        });
    }

    std::vector<T> result;
    for (std::size_t i = 0; i < num && !deque_.empty(); ++i) {
        result.push_back(std::move(deque_.front()));
        deque_.pop_front();
    }
    not_full_cv_.notify_all();
    return result;
}

template <typename T>
void fifo<T>::set_behavior(behavior value) {
    behavior_ = value;
    not_empty_cv_.notify_all();
    not_full_cv_.notify_all();
}

template <typename T>
void fifo<T>::awaken() {
    set_behavior(behavior::not_blocking);
}

}
