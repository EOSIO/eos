#include <boost/test/unit_test.hpp>
#include <chain_kv/chain_kv.hpp>

namespace chain_kv = eosio::chain_kv;

struct kv_values {
   std::vector<std::pair<chain_kv::bytes, chain_kv::bytes>> values;

   friend bool operator==(const kv_values& a, const kv_values& b) { return a.values == b.values; }
};

template <typename S>
S& operator<<(S& s, const kv_values& v) {
   s << "{\n";
   for (auto& kv : v.values) {
      s << "    {{";
      bool first = true;
      for (auto b : kv.first) {
         if (!first)
            s << ", ";
         first = false;
         char x[10];
         sprintf(x, "0x%02x", (uint8_t)b);
         s << x;
      }
      s << "}, {";
      first = true;
      for (auto b : kv.second) {
         if (!first)
            s << ", ";
         first = false;
         char x[10];
         sprintf(x, "0x%02x", (uint8_t)b);
         s << x;
      }
      s << "}},\n";
   }
   s << "}";
   return s;
}

#define KV_REQUIRE_EXCEPTION(expr, msg)                                                                                \
   BOOST_REQUIRE_EXCEPTION(expr, chain_kv::exception, [](auto& e) {                                                    \
      if (!strcmp(e.what(), msg))                                                                                      \
         return true;                                                                                                  \
      std::cerr << "expected: '"                                                                                       \
                << "\033[33m" << msg << "\033[0m'\ngot:      '\033[33m" << e.what() << "'\033[0m\n";                   \
      return false;                                                                                                    \
   });

inline kv_values get_all(chain_kv::database& db, const chain_kv::bytes& prefix) {
   kv_values                          result;
   std::unique_ptr<rocksdb::Iterator> rocks_it{ db.rdb->NewIterator(rocksdb::ReadOptions()) };
   rocks_it->Seek(chain_kv::to_slice(prefix));
   while (rocks_it->Valid()) {
      auto k = rocks_it->key();
      if (k.size() < prefix.size() || memcmp(k.data(), prefix.data(), prefix.size()))
         break;
      auto v = rocks_it->value();
      result.values.push_back({ chain_kv::to_bytes(k), chain_kv::to_bytes(v) });
      rocks_it->Next();
   }
   if (!rocks_it->status().IsNotFound())
      chain_kv::check(rocks_it->status(), "iterate: ");
   return result;
}

inline kv_values get_values(chain_kv::write_session& session, const std::vector<chain_kv::bytes>& keys) {
   kv_values result;
   for (auto& key : keys) {
      chain_kv::bytes value;
      if (auto value = session.get(chain_kv::bytes{ key }))
         result.values.push_back({ key, *value });
   }
   return result;
}

inline kv_values get_matching(chain_kv::view& view, uint64_t contract, const chain_kv::bytes& prefix = {}) {
   kv_values                result;
   chain_kv::view::iterator it{ view, contract, chain_kv::to_slice(prefix) };
   ++it;
   while (!it.is_end()) {
      auto kv = it.get_kv();
      if (!kv)
         throw chain_kv::exception("iterator read failure");
      result.values.push_back({ chain_kv::to_bytes(kv->key), chain_kv::to_bytes(kv->value) });
      ++it;
   }
   return result;
}

inline kv_values get_matching2(chain_kv::view& view, uint64_t contract, const chain_kv::bytes& prefix = {}) {
   kv_values                result;
   chain_kv::view::iterator it{ view, contract, chain_kv::to_slice(prefix) };
   --it;
   while (!it.is_end()) {
      auto kv = it.get_kv();
      if (!kv)
         throw chain_kv::exception("iterator read failure");
      result.values.push_back({ chain_kv::to_bytes(kv->key), chain_kv::to_bytes(kv->value) });
      --it;
   }
   std::reverse(result.values.begin(), result.values.end());
   return result;
}

inline kv_values get_it(const chain_kv::view::iterator& it) {
   kv_values result;
   auto      kv = it.get_kv();
   if (!kv)
      throw chain_kv::exception("iterator read failure");
   result.values.push_back({ chain_kv::to_bytes(kv->key), chain_kv::to_bytes(kv->value) });
   return result;
}
