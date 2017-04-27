#pragma once
#include <eos/chain/types.hpp>


namespace eos { namespace chain {

   struct PermissionLevel {
      account_name      account;
      permission_name   level;
      uint16_t          weight;
   };

   struct KeyPermission {
      public_key_type key;
      uint16_t        weight;
   };

   struct Authority {
      uint32_t                threshold = 0;
      vector<PermissionLevel> accounts;
      vector<KeyPermission>   keys;

      bool validate() const {
         if (threshold == 0)
            return false;
         uint32_t score = 0;
         for (const auto& p : accounts)
            score += p.weight;
         for (const auto& p : keys)
            score += p.weight;
         return score >= threshold;
      }
      set<account_name> referenced_accounts() const {
         set<account_name> results;
         std::transform(accounts.begin(), accounts.end(), std::inserter(results, results.begin()),
                        [](const PermissionLevel& p) { return p.account; });
         return results;
      }
   };

} }  // eos::chain

FC_REFLECT(eos::chain::PermissionLevel, (account)(level)(weight))
FC_REFLECT(eos::chain::KeyPermission, (key)(weight))
FC_REFLECT(eos::chain::Authority, (threshold)(accounts)(keys))
