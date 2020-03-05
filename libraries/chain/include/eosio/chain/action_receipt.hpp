#pragma once

#include <eosio/chain/types.hpp>
#include <eosio/chain/action.hpp>

namespace eosio { namespace chain {

   /**
    *  For each action dispatched this receipt is generated
    */
   struct action_receipt {
      account_name                    receiver;
      digest_type                     act_digest;
      uint64_t                        global_sequence = 0; ///< total number of actions dispatched since genesis
      uint64_t                        recv_sequence   = 0; ///< total number of actions with this receiver since genesis
      flat_map<account_name,uint64_t> auth_sequence;
      fc::unsigned_int                code_sequence = 0; ///< total number of setcodes
      fc::unsigned_int                abi_sequence  = 0; ///< total number of setabis

      static digest_type encode(char* bytes, uint32_t len) {
         digest_type::encoder enc;
         enc.write(bytes, len);
         return enc.result();
      }

      template <typename Encode, typename Extra>
      void generate_action_digest(Encode&& enc, const action& act, const Extra& extra, bool legacy=true) {
         if (legacy) {
            act_digest = digest_type::hash(act);
         } else {
            const action_base* base = &act;
            const auto action_size = fc::raw::pack_size(act);
            const auto extra_size  = fc::raw::pack_size(extra);
            std::vector<char> buff(action_size + extra_size);
            fc::datastream<char*> ds(buff.data(), action_size + extra_size);
            fc::raw::pack(ds, act);
            fc::raw::pack(ds, extra);
            digest_type hashes[2];
            const size_t lhs_size = fc::raw::pack_size(*base);
            hashes[0]  = enc(buff.data(), lhs_size);
            hashes[1]  = enc(buff.data() + lhs_size, fc::raw::pack_size(act.data) + extra_size);
            act_digest = enc((char*)&hashes[0], sizeof(hashes));
         }
      }

      template <typename Extra>
      void generate_action_digest(const action& act, const Extra& extra, bool legacy=true) {
         generate_action_digest(encode, act, extra, legacy);
      }

      digest_type digest()const {
         digest_type::encoder e;
         fc::raw::pack(e, receiver);
         fc::raw::pack(e, act_digest);
         fc::raw::pack(e, global_sequence);
         fc::raw::pack(e, recv_sequence);
         fc::raw::pack(e, auth_sequence);
         fc::raw::pack(e, code_sequence);
         fc::raw::pack(e, abi_sequence);
         return e.result();
      }
   };

} }  /// namespace eosio::chain

FC_REFLECT( eosio::chain::action_receipt,
            (receiver)(act_digest)(global_sequence)(recv_sequence)(auth_sequence)(code_sequence)(abi_sequence) )
