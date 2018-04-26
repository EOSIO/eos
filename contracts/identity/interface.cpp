#include "interface.hpp"

namespace identity {

identity_name interface::get_claimed_identity( account_name acnt ) {
   return accounts_table( _self, acnt ).get_or_default(0);
}

account_name interface::get_owner_for_identity( uint64_t receiver, identity_name ident ) {
   // for each trusted owner certification
   //   check to see if the certification is still trusted
   //   check to see if the account has claimed it
   certs_table certs( _self, ident );
   auto idx = certs.template get_index<N(bytuple)>();
   auto itr = idx.lower_bound(certrow::key(N(owner), 1, 0));
   account_name owner = 0;
   while (itr != idx.end() && itr->property == N(owner) && itr->trusted) {
      if (sizeof(account_name) == itr->data.size()) {
         account_name account = *reinterpret_cast<const account_name*>(itr->data.data());
         if (ident == get_claimed_identity(account)) {
            if (is_trusted(itr->certifier) ) {
               // the certifier is still trusted
               if (!owner || owner == account) {
                  owner = account;
               } else {
                  //contradiction found: different owners certified for the same identity
                  return 0;
               }
            } else if (_self == receiver){
               //the certifier is no longer trusted, need to unset the flag
               idx.modify(itr, 0, [&](certrow& r) {
                     r.trusted = 0;
                  });
            } else {
               // the certifier is no longer trusted, but the code runs in read-only mode
            }
         }
      } else {
         // bad row - skip it
      }
      ++itr;
   }
   if (owner) {
      //owner found, no contradictions among certifications flaged as trusted
      return owner;
   }
   // trusted certification not found
   // let's see if any untrusted certifications became trusted
   itr = idx.lower_bound(certrow::key(N(owner), 0, 0));
   while (itr != idx.end() && itr->property == N(owner) && !itr->trusted) {
      if (sizeof(account_name) == itr->data.size()) {
         account_name account = *reinterpret_cast<const account_name*>(itr->data.data());
         if (ident == get_claimed_identity(account) && is_trusted(itr->certifier)) {
            if (_self == receiver) {
               // the certifier became trusted and we have permissions to update the flag
               idx.modify(itr, 0, [&](certrow& r) {
                     r.trusted = 1;
                  });
            }
            if (!owner || owner == account) {
               owner = account;
            } else {
               //contradiction found: different owners certified for the same identity
               return 0;
            }
         }
      } else {
         // bad row - skip it
      }
      ++itr;
   }
   return owner;
}

identity_name interface::get_identity_for_account( uint64_t receiver, account_name acnt ) {
   //  check what identity the account has self certified owner
   //  verify that a trusted certifier has confirmed owner
   auto identity = get_claimed_identity(acnt);
   return (identity != 0 && acnt == get_owner_for_identity(receiver, identity)) ? identity : 0;
}

} // namespace identity
