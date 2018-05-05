#ifndef CONSUMER_H
#define CONSUMER_H

#include <thread>
#include <future>

#include "database.h"

namespace eosio {

class consumer
{
public:
    consumer(std::shared_ptr<database> db);
    virtual ~consumer();

    virtual void consume() = 0;

    void start();
    virtual void stop();

private:
    void run(std::future<void> future_obj);

    std::shared_ptr<database> m_db;
    std::shared_ptr<std::thread> m_thread;
    std::promise<void> m_exit_signal;
};

} // namespace

#endif // CONSUMER_H
