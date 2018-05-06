#ifndef CONSUMER_H
#define CONSUMER_H

#include <thread>
#include <atomic>

namespace eosio {

class consumer
{
public:
    virtual ~consumer();

    virtual void consume() = 0;

    void start();
    virtual void stop();

private:
    void run();

    std::shared_ptr<std::thread> m_thread;
    std::atomic<bool> m_exit;
};

} // namespace

#endif // CONSUMER_H
