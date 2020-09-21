#pragma once
#include <vector>

namespace eosio { namespace bv {
    enum class frame_type {
        UNKNOWN = 0,
        SNAPSHOT = 1,
        BLOCK_ENTRY_V1 = 2
    };

   class bv_sync_handler {
   public:
      virtual bool start_frame(frame_type ft)=0;
      virtual bool frame_data(std::vector<char>& data)=0; /* return true to continue sync */
      virtual bool end_frame()=0;
   };

   class bv_connection {
   public:
        virtual void sync_for_construction(bv_sync_handler& handler)=0;
   };

}}