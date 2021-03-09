
#include <eosio/chain/apply_context.hpp>
#include <eosio/chain/webassembly/interface.hpp>

namespace eosio {
namespace chain {
namespace webassembly {

int64_t interface::propose_security_group_participants_add(span<const char> packed_participants) {
   datastream<const char*> ds(packed_participants.data(), packed_participants.size());
   flat_set<account_name>  participants;
   fc::raw::unpack(ds, participants);
   return context.control.propose_security_group_participants_add(participants);
}

int64_t interface::propose_security_group_participants_remove(span<const char> packed_participants) {
   datastream<const char*> ds(packed_participants.data(), packed_participants.size());
   flat_set<account_name>  participants;
   fc::raw::unpack(ds, participants);
   return context.control.propose_security_group_participants_remove(participants);
}

bool interface::in_active_security_group(span<const char> packed_participants) const {
   datastream<const char*> ds(packed_participants.data(), packed_participants.size());
   flat_set<account_name>  participants;
   fc::raw::unpack(ds, participants);
   return context.control.in_active_security_group(participants);
}

uint32_t interface::get_active_security_group(span<char> packed_security_group) const {
   datastream<size_t> size_strm;
   const auto&        active_security_group = context.control.active_security_group();
   fc::raw::pack(size_strm, active_security_group);
   if (size_strm.tellp() <= packed_security_group.size()) {
      datastream<char*> ds(packed_security_group.data(), packed_security_group.size());
      fc::raw::pack(ds, active_security_group);
   }
   return size_strm.tellp();
}

} // namespace webassembly
} // namespace chain
} // namespace eosio
