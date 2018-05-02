#ifndef FIFO_H
#define FIFO_H

#include <mutex>
#include <deque>
#include <eosio/chain/block.hpp>
#include <eosio/chain/block_trace.hpp>

namespace eosio {

class fifo
{
public:
    using data = std::pair<chain::block_trace, chain::signed_block>;

    void push(const data&);
    data pop();

private:
    std::mutex m_mux;

    std::deque<data> m_deque;
};

} // namespace

#endif // FIFO_H
