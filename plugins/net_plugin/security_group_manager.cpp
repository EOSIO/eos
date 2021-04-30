#include <eosio/net_plugin/security_group_manager.hpp>

#include <eosio/chain/exceptions.hpp>

namespace eosio {
   bool security_group_manager::update_cache(const uint32_t version, const participant_list_t& participant_list) {
      if(version == version_)
         return false;
      EOS_ASSERT(version == version_ +1, eosio::chain::plugin_exception,
                 "The active security group version should only ever increase by one. Current version: "
                 "${current}, new version: ${new}", ("current", version_)("new", version));
      version_ = version;
      cache_ = participant_list;
      return true;
   }
}
