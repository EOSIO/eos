/*
 * Copyright (c) 2017, Respective Authors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#pragma once
#include <eos/chain/protocol/types.hpp>

namespace eos { namespace chain {

   /**
    * @brief Contains per-node database configuration.
    *
    *  Transactions are evaluated differently based on per-node state.
    *  Settings here may change based on whether the node is syncing or up-to-date.
    *  Or whether the node is a producer node. Or if we're processing a
    *  transaction in a producer-signed block vs. a fresh transaction
    *  from the p2p network.  Or configuration-specified tradeoffs of
    *  performance/hardfork resilience vs. paranoia.
    */
   class node_property_object
   {
      public:
         node_property_object(){}
         ~node_property_object(){}

         uint32_t skip_flags = 0;
         std::map< block_id_type, std::vector< fc::variant_object > > debug_updates;
   };
} } // eos::chain
