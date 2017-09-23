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

#include <eos/chain/config.hpp>
#include <eos/types/types.hpp>

#include <chainbase/chainbase.hpp>

#include <fc/container/flat_fwd.hpp>
#include <fc/io/varint.hpp>
#include <fc/io/enum_type.hpp>
#include <fc/crypto/sha224.hpp>
#include <fc/optional.hpp>
#include <fc/safe.hpp>
#include <fc/container/flat.hpp>
#include <fc/string.hpp>
#include <fc/io/raw.hpp>
#include <fc/uint128.hpp>
#include <fc/static_variant.hpp>
#include <fc/smart_ref_fwd.hpp>
#include <fc/crypto/ripemd160.hpp>
#include <fc/fixed_string.hpp>

#include <memory>
#include <vector>
#include <deque>
#include <cstdint>

#define OBJECT_CTOR1(NAME) \
    NAME() = delete; \
    public: \
    template<typename Constructor, typename Allocator> \
    NAME(Constructor&& c, chainbase::allocator<Allocator>) \
    { c(*this); }
#define OBJECT_CTOR2_MACRO(x, y, field) ,field(a)
#define OBJECT_CTOR2(NAME, FIELDS) \
    NAME() = delete; \
    public: \
    template<typename Constructor, typename Allocator> \
    NAME(Constructor&& c, chainbase::allocator<Allocator> a) \
    : id(0) BOOST_PP_SEQ_FOR_EACH(OBJECT_CTOR2_MACRO, _, FIELDS) \
    { c(*this); }
#define OBJECT_CTOR(...) BOOST_PP_OVERLOAD(OBJECT_CTOR, __VA_ARGS__)(__VA_ARGS__)

namespace eos { namespace chain {
   using                               std::map;
   using                               std::vector;
   using                               std::unordered_map;
   using                               std::string;
   using                               std::deque;
   using                               std::shared_ptr;
   using                               std::weak_ptr;
   using                               std::unique_ptr;
   using                               std::set;
   using                               std::pair;
   using                               std::enable_shared_from_this;
   using                               std::tie;
   using                               std::make_pair;

   using                               fc::smart_ref;
   using                               fc::variant_object;
   using                               fc::variant;
   using                               fc::enum_type;
   using                               fc::optional;
   using                               fc::unsigned_int;
   using                               fc::signed_int;
   using                               fc::time_point_sec;
   using                               fc::time_point;
   using                               fc::safe;
   using                               fc::flat_map;
   using                               fc::flat_set;
   using                               fc::static_variant;
   using                               fc::ecc::range_proof_type;
   using                               fc::ecc::range_proof_info;
   using                               fc::ecc::commitment_type;
   struct void_t{};

   using chainbase::allocator;
   using shared_string = boost::interprocess::basic_string<char, std::char_traits<char>, allocator<char>>;
   template<typename T>
   using shared_vector = boost::interprocess::vector<T, allocator<T>>;
   template<typename T>
   using shared_set = boost::interprocess::set<T, std::less<T>, allocator<T>>;

   using private_key_type = fc::ecc::private_key;
   using chain_id_type = fc::sha256;

   using eos::types::Name;
   using ActionName = Name;
   using eos::types::AccountName;
   using eos::types::PermissionName;
   using eos::types::Asset;
   using eos::types::ShareType;
   using eos::types::Authority;
   using eos::types::PublicKey;
   using eos::types::Transaction;
   using eos::types::PermissionName;
   using eos::types::TypeName;
   using eos::types::FuncName;
   using eos::types::Time;
   using eos::types::Field;
   using eos::types::String;
   using eos::types::UInt8;
   using eos::types::UInt16;
   using eos::types::UInt32;
   using eos::types::UInt64;
   using eos::types::UInt128;
   using eos::types::UInt256;
   using eos::types::Int8;
   using eos::types::Int16;
   using eos::types::Int32;
   using eos::types::Int64;
   using eos::types::Int128;
   using eos::types::Int256;
   using eos::types::uint128_t;

   using ProducerRound = std::array<AccountName, config::BlocksPerRound>;
   using RoundChanges = std::map<AccountName, AccountName>;

   /**
    * @brief Calculates the difference between two @ref ProducerRound objects
    * @param a The first round of producers
    * @param b The second round of producers
    * @return The properly sorted RoundChanges expressing the difference between a and b
    *
    * Calculates the difference between two rounds of producers, returning a @ref RoundChanges object. The returned
    * changes are sorted in the order defined by @ref block_header
    */
   RoundChanges operator-(ProducerRound a, ProducerRound b);

   /**
    * List all object types from all namespaces here so they can
    * be easily reflected and displayed in debug output.  If a 3rd party
    * wants to extend the core code then they will have to change the
    * packed_object::type field from enum_type to uint16 to avoid
    * warnings when converting packed_objects to/from json.
    */
   enum object_type
   {
      null_object_type,
      account_object_type,
      permission_object_type,
      permission_link_object_type,
      action_code_object_type,
      key_value_object_type,
      key128x128_value_object_type,
      action_permission_object_type,
      global_property_object_type,
      dynamic_global_property_object_type,
      block_summary_object_type,
      transaction_object_type,
      generated_transaction_object_type,
      producer_object_type,
      chain_property_object_type,
      account_control_history_object_type, ///< Defined by account_history_plugin
      account_transaction_history_object_type, ///< Defined by account_history_plugin
      transaction_history_object_type, ///< Defined by account_history_plugin
      public_key_history_object_type, ///< Defined by account_history_plugin
      balance_object_type, ///< Defined by native_contract library
      staked_balance_object_type, ///< Defined by native_contract library
      producer_votes_object_type, ///< Defined by native_contract library
      producer_schedule_object_type, ///< Defined by native_contract library
      proxy_vote_object_type, ///< Defined by native_contract library
      key64x64x64_value_object_type,
      keystr_value_object_type,
      OBJECT_TYPE_COUNT ///< Sentry value which contains the number of different object types
   };

   class account_object;
   class producer_object;

   using block_id_type = fc::sha256;
   using checksum_type = fc::sha256;
   using transaction_id_type = fc::sha256;
   using digest_type = fc::sha256;
   using generated_transaction_id_type = fc::sha256;
   using signature_type = fc::ecc::compact_signature;
   using weight_type = uint16_t;
   using Bytes = types::Bytes;

   using public_key_type = eos::types::PublicKey;
   
} }  // eos::chain

namespace fc {
  void to_variant(const eos::chain::shared_vector<eos::types::Field>& c, fc::variant& v);
  void from_variant(const fc::variant& v, eos::chain::shared_vector<eos::types::Field>& fields);
  void to_variant(const eos::chain::ProducerRound& r, fc::variant& v);
  void from_variant(const fc::variant& v, eos::chain::ProducerRound& r);
}

FC_REFLECT_ENUM(eos::chain::object_type,
                (null_object_type)
                (account_object_type)
                (permission_object_type)
                (permission_link_object_type)
                (action_code_object_type)
                (key_value_object_type)
                (key128x128_value_object_type)
                (action_permission_object_type)
                (global_property_object_type)
                (dynamic_global_property_object_type)
                (block_summary_object_type)
                (transaction_object_type)
                (generated_transaction_object_type)
                (producer_object_type)
                (chain_property_object_type)
                (account_control_history_object_type)
                (account_transaction_history_object_type)
                (transaction_history_object_type)
                (public_key_history_object_type)
                (balance_object_type)
                (staked_balance_object_type)
                (producer_votes_object_type)
                (producer_schedule_object_type)
                (proxy_vote_object_type)
                (key64x64x64_value_object_type)
                (keystr_value_object_type)
                (OBJECT_TYPE_COUNT)
               )
FC_REFLECT( eos::chain::void_t, )
