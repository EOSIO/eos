#include <eosio/net_plugin/security_group_manager.hpp>

namespace eosio {
   bool security_group_manager::update_cache(const uint32_t version, const participant_list_t& participant_list) {
      if(version == version_)
         return false;
      version_ = version;
      cache_ = participant_list;
      return true;
   }
}
