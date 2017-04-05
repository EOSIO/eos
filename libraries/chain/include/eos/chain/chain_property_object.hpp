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

#include <eos/chain/immutable_chain_parameters.hpp>
#include <eos/chain/protocol/types.hpp>

#include "multi_index_includes.hpp"

namespace eos { namespace chain {

/**
 * Contains invariants which are set at genesis and never changed.
 */
class chain_property_object : public chainbase::object<chain_property_object_type, chain_property_object>
{
      OBJECT_CTOR(chain_property_object)

      id_type id;
      chain_id_type chain_id;
      immutable_chain_parameters immutable_parameters;
};

using chain_property_multi_index = chainbase::shared_multi_index_container<
   chain_property_object,
   indexed_by<
      ordered_unique<tag<by_id>, BOOST_MULTI_INDEX_MEMBER(chain_property_object, chain_property_object::id_type, id)>
   >
>;

} }

CHAINBASE_SET_INDEX_TYPE(eos::chain::chain_property_object, eos::chain::chain_property_multi_index)

FC_REFLECT(eos::chain::chain_property_object, (chain_id)(immutable_parameters))
