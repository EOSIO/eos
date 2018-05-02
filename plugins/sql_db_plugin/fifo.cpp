#include "fifo.h"

namespace eosio {

void fifo::push(const data& d)
{
    std::lock_guard<std::mutex> lock(m_mux);
    m_deque.push_back(d);
}

fifo::data fifo::pop()
{
    std::lock_guard<std::mutex> lock(m_mux);
    auto e = m_deque.front();
    m_deque.pop_front();
    return e;
}

} // namespace
