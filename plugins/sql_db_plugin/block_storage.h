/**
 *  @file
 *  @copyright defined in enumivo/LICENSE.txt
 */

#pragma once

#include "consumer_core.h"

#include <enumivo/chain/block_state.hpp>

namespace enumivo {

class block_storage : public consumer_core<chain::block_state_ptr>
{
public:
    void consume(const std::vector<chain::block_state_ptr>& blocks) override;
};

} // namespace
