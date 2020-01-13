#pragma once
#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/types.hpp>
#include <variant>
#include <chrono>
#include <vector>
#include <openssl/rand.h>

namespace eosio { namespace chain {
   template <typename TrxCtx>
   class subjective_data {
      public:
         subjective_data(const TrxCtx& ctx) : _ctx(ctx) {}

         const TrxCtx& get_trx_context()const { return _ctx; }

         uint32_t get_trx_cpu() {
            uint32_t trx_cpu = static_cast<uint32_t>(_ctx.billed_cpu_time_us);
            EOS_ASSERT(trx_cpu >= _trx_cpu, subjective_data_exception, "billed cpu time should be greater than last call");
            _trx_cpu = trx_cpu;
            size_t old_size = data.size();
            data.resize(old_size+sizeof(trx_cpu));
            memcpy(data.data() + old_size, &trx_cpu, sizeof(trx_cpu));
            return trx_cpu;
         }
         uint64_t get_time() {
            using namespace std::chrono;
            uint64_t time_us = duration_cast<std::chrono::microseconds>(high_resolution_clock::now().time_since_epoch()).count();
            EOS_ASSERT(time_us >= _time_us, subjective_data_exception, "wall clock time should be greater than last call");
            _time_us = time_us;
            size_t old_size = data.size();
            data.resize(old_size+sizeof(time_us));
            memcpy(data.data() + old_size, &time_us, sizeof(time_us));
            return time_us;
         }
         void get_random(char* bytes, uint32_t size) {
            RAND_bytes((uint8_t*)bytes, size);
            size_t old_size = data.size();
            data.resize(old_size+size);
            memcpy(data.data() + old_size, bytes, size);
         }
         inline bool validate_trx_cpu(uint32_t final_billed_cpu)const { return final_billed_cpu >= _trx_cpu; }
         inline bool validate_wall_time(const fc::microseconds& block_time)const {
            uint64_t lower_bound = fc::microseconds(block_time - fc::milliseconds(500)).count();
            uint64_t upper_bound = fc::microseconds(block_time + fc::milliseconds(500)).count();
            return (_time_us == 0) || (lower_bound <= _time_us && _time_us <= upper_bound);
         }

         std::vector<uint8_t> data;

      private:
         const TrxCtx& _ctx;
         uint32_t _trx_cpu = 0;
         uint64_t _time_us = 0;
   };
}} // namespace eosio::chain
