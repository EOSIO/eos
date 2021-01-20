#pragma once
#include <eosio/chain/backing_store/db_key_value_format.hpp>
#include <eosio/chain/backing_store/chain_kv_payer.hpp>
#include <b1/session/rocks_session.hpp>
#include <b1/session/session.hpp>
#include <b1/session/undo_stack.hpp>
#include <eosio/chain/kv_chainbase_objects.hpp>

namespace eosio { namespace chain { namespace backing_store {
using rocks_db_type = eosio::session::session<eosio::session::rocksdb_t>;
using session_type = eosio::session::session<rocks_db_type>;
using kv_undo_stack_ptr = std::unique_ptr<eosio::session::undo_stack<rocks_db_type>>;
template<typename Object>
const char* contract_table_type();

// chainlib reserves prefixes 0x10 - 0x2F.
static constexpr char rocksdb_contract_kv_prefix = 0x11; // for KV API
static constexpr char rocksdb_contract_db_prefix = 0x12; // for DB API

namespace detail {
   class manage_stack {
   public:
      manage_stack(const kv_undo_stack_ptr& kv_undo_stack) : kv_undo_stack_(kv_undo_stack), undo_(kv_undo_stack->empty()) {
         if (undo_) {
            // Get a session to iterate over.
            kv_undo_stack->push();
         }
      }

      ~manage_stack(){
         if (undo_) {
            kv_undo_stack_->undo();
         }
      }
   private:
      const kv_undo_stack_ptr& kv_undo_stack_;
      const bool undo_;
   };
}

template <typename F>
bool read_rocksdb_entry(const eosio::session::shared_bytes& actual_db_kv_key,
                        const eosio::session::shared_bytes& value,
                        F& function) {
   uint64_t    contract;
   constexpr std::size_t type_size = 1;
   std::size_t key_prefix_size = type_size + sizeof(contract);
   EOS_ASSERT(actual_db_kv_key.size() >= key_prefix_size, database_exception, "Unexpected key in rocksdb");

   auto key_buffer = std::vector<char>{ actual_db_kv_key.data(), actual_db_kv_key.data() + actual_db_kv_key.size() };
   auto begin = std::begin(key_buffer) + type_size;
   auto end = std::begin(key_buffer) + key_prefix_size;
   b1::chain_kv::extract_key(begin, end, contract);

   const char* post_contract = actual_db_kv_key.data() + key_prefix_size;
   const std::size_t remaining = actual_db_kv_key.size() - key_prefix_size;
   return function(contract, post_contract, remaining, value.data(), value.size());
};

struct table_id_object_view {
   account_name code;  //< code should not be changed within a chainbase modifier lambda
   scope_name scope; //< scope should not be changed within a chainbase modifier lambda
   table_name table; //< table should not be changed within a chainbase modifier lambda
   account_name payer;
   uint32_t count = 0; /// the number of elements in the table
};

struct blob {
   std::string str_data;
   std::size_t size() const { return str_data.size(); }
   const char* data() const { return str_data.data(); }
};

template<typename DataStream>
inline DataStream &operator<<(DataStream &ds, const blob &b) {
   fc::raw::pack(ds, b.str_data);
   return ds;
}

template<typename DataStream>
inline DataStream &operator>>(DataStream &ds, blob &b) {
   fc::raw::unpack(ds, b.str_data);
   return ds;
}

struct primary_index_view {
   static primary_index_view create(uint64_t key, const char* value, std::size_t value_size) {
      backing_store::payer_payload pp(value, value_size);
      return primary_index_view{key, pp.payer, {std::string{pp.value, pp.value + pp.value_size}}};
   }
   uint64_t primary_key;
   account_name payer;
   blob value;
};

template<typename SecondaryKeyType>
struct secondary_index_view {
   uint64_t primary_key;
   account_name payer;
   SecondaryKeyType secondary_key;
};

template<typename Receiver>
class add_database_receiver {
public:
   add_database_receiver(Receiver& receiver, const chainbase::database& db) : receiver_(receiver), db_(db) {}
   template<typename Object>
   void add_row(const Object& row) {
      receiver_.add_row(row, db_);
   }

   void add_row(fc::unsigned_int x){
      receiver_.add_row(x, db_);
   }

private:
   Receiver& receiver_;
   const chainbase::database& db_;
};

// used to wrap a rocksdb_contract_db_table_writer to collect all primary and secondary keys and report them after the
// table_id_object_view is reported.  This also reports contract table related data in the format expected by snapshots
// (reporting the number of key rows (for primary and secondary key types) before reporting the rows. NOTE: using this
// collector really only makes sense with a key_context of complete, otherwise it is just extra processing with no
// benefit. It will produce invalid output if used with a key_context of complete_reverse
template<typename Receiver>
class rocksdb_whole_db_table_collector {
public:
   rocksdb_whole_db_table_collector(Receiver &r)
   : receiver_(r) {}

   void add_row(const chain::backing_store::primary_index_view& row){
      primary_indices_.emplace_back(row);
   }

   void add_row(const chain::backing_store::secondary_index_view<uint64_t>& row){
      secondary_indices<uint64_t>().emplace_back(row);
   }

   void add_row(const chain::backing_store::secondary_index_view<chain::uint128_t>& row){
      secondary_indices<chain::uint128_t>().emplace_back(row);
   }

   void add_row(const chain::backing_store::secondary_index_view<chain::key256_t>& row){
      secondary_indices<chain::key256_t>().emplace_back(row);
   }

   void add_row(const chain::backing_store::secondary_index_view<float64_t>& row){
      secondary_indices<float64_t>().emplace_back(row);
   }

   void add_row(const chain::backing_store::secondary_index_view<float128_t>& row){
      secondary_indices<float128_t>().emplace_back(row);
   }

   void add_row(const chain::backing_store::table_id_object_view& row){
      receiver_.add_row(row);

      auto add = [&rec=receiver_](const auto &row) { rec.add_row(row); };
      add(fc::unsigned_int(primary_indices_.size()));
      // now that the whole table has been found, now we can report the individual keys
      std::for_each(primary_indices_.begin(), primary_indices_.end(), add);
      primary_indices_.clear();
      std::tuple<uint64_t, uint128_t, key256_t, float64_t, float128_t> secondary_key_types;
      std::apply([this](auto... index) { (this->write_secondary_indices(index), ...); },
                 secondary_key_types);
   }
private:
   template<typename IndexType>
   auto& secondary_indices() {
      return std::get<std::vector<secondary_index_view<IndexType>>>(all_secondary_indices_);
   }

   template<typename IndexType>
   void write_secondary_indices(IndexType) {
      auto& indices = secondary_indices<IndexType>();
      auto add = [this](const auto& row) { receiver_.add_row(row); };
      add(fc::unsigned_int(indices.size()));
      std::for_each(indices.begin(), indices.end(), add);
      indices.clear();
   }

   Receiver& receiver_;
   std::vector<primary_index_view> primary_indices_;
   template<typename T>
   using secondary_index_views = std::vector<secondary_index_view<T>>;
   std::tuple<secondary_index_views<uint64_t>,
              secondary_index_views<uint128_t>,
              secondary_index_views<key256_t>,
              secondary_index_views<float64_t>,
              secondary_index_views<float128_t> >
         all_secondary_indices_;
};

constexpr auto process_all = []() { return true; };

// the context in which the table writer should be reporting to receiver_.add_row(...).  NOTE: for any context indicating
// that it reports complete/valid table_id_object_views, for it to be valid it must be passed all keys belonging to that
// table, if not, the count portion of the table_id_object_view will not be accurate
enum class key_context {
   complete,           // report keys (via receiver_.add_row) as they are seen, so table will be after its keys (use rocksdb_whole_db_table_collector to reverse this)
   complete_reverse,   // report keys (via receiver_.add_row) as they are seen, used when reverse iterating through a table space (do not use with rocksdb_whole_db_table_collector)
   standalone,         // report an incomplete table (only code/scope/table valid) prior to reporting its keys
   standalone_reverse, // report an incomplete table (only code/scope/table valid) prior to reporting its keys
   table_only,         // report only the table, primary and secondary keys are only processed enough to report a complete/valid table_id_object_view
   table_only_reverse  // report only the table, primary and secondary keys are only processed enough to report a complete/valid table_id_object_view
};

// processes any keys passed to it, reporting them to its receiver as it sees them.  Use a key_context to adjust this behavior,
// pass in a keep_processing lambda to indicate to one of the walk_*** methods that processing should stop (like for
// limiting time querying the database for an PRC call)
template<typename Receiver, typename Function = std::decay_t < decltype(process_all)>>
class rocksdb_contract_db_table_writer {
public:
   using key_type = db_key_value_format::key_type;

   rocksdb_contract_db_table_writer(Receiver &r, key_context context, Function keep_processing = process_all)
         : receiver_(r), context_(context), keep_processing_(keep_processing)  {}

   explicit rocksdb_contract_db_table_writer(Receiver &r)
         : receiver_(r), context_(key_context::complete), keep_processing_(process_all)  {}

   void extract_primary_index(b1::chain_kv::bytes::const_iterator remaining,
                              b1::chain_kv::bytes::const_iterator key_end, const char *value,
                              std::size_t value_size) {
      if (!is_table_only()) {
         uint64_t primary_key;
         EOS_ASSERT(b1::chain_kv::extract_key(remaining, key_end, primary_key), bad_composite_key_exception,
                    "DB intrinsic key-value store composite key is malformed");
         receiver_.add_row(primary_index_view::create(primary_key, value, value_size));
      }
      ++primary_count_;
   }

   template<typename IndexType>
   void extract_secondary_index(b1::chain_kv::bytes::const_iterator remaining,
                                b1::chain_kv::bytes::const_iterator key_end, const char *value,
                                std::size_t value_size) {
      if (!is_table_only()) {
         IndexType secondary_key;
         uint64_t primary_key;
         EOS_ASSERT(b1::chain_kv::extract_key(remaining, key_end, secondary_key), bad_composite_key_exception,
                    "DB intrinsic key-value store composite key is malformed");
         EOS_ASSERT(b1::chain_kv::extract_key(remaining, key_end, primary_key), bad_composite_key_exception,
                    "DB intrinsic key-value store composite key is malformed");
         backing_store::payer_payload pp(value, value_size);
         receiver_.add_row(secondary_index_view<IndexType>{primary_key, pp.payer, secondary_key});
      }
      ++secondary_count<IndexType>();
   }

   // DB Intrinsic Keys
   // PREFIX:
   // db intrinsic keys are all prefaced with the following information:
   // | contract | scope | table | type of key |
   // so when iterating through the database, all keys for a given contract/scope/table will be processed together
   //
   // TYPE OF KEY:
   // the "type of key" is an enum which ends up creating the following order for the type of keys:
   // primary key
   // primary to secondary key (not present in chainbase structure since it can have more than one index on the same structure -- these are ignored for this class's processing)
   // uint64_t secondary key
   // uint128_t secondary key
   // key256_t secondary key
   // float64_t secondary key
   // float128_t secondary key
   // table key (indicates end of table and payer)
   //
   // KEY PROCESSING (key_context):
   // keys may be processed as:
   // individual key (standalone)
   // -- this can be any key - primary, secondary, or table
   // table only (table_only, table_only_reversed)
   // -- all keys for a table are passed in and processed only to report a valid table_id_object_view, if the table is
   // passed in table_only, the table is determined to be complete when the table key is received, for
   // table_only_reversed all of the table_id_object_view data, other than the count, is determined when the
   // table key is received and the count is determined when the table is determined to be complete (either by a
   // new key being received that is for a different table, or the complete() method is called externally).
   // -- only the the table_id_object_view is reported, none of the other key's objects are reported
   // complete (complete, complete_reverse)
   // -- similar to table only, except all of the objects for the keys in the table are also reported
   bool operator()(uint64_t contract, const char *key, std::size_t key_size,
                   const char *value, std::size_t value_size) {
      b1::chain_kv::bytes composite_key(key, key + key_size);
      auto[scope, table, remaining, type] = backing_store::db_key_value_format::get_prefix_thru_key_type(
            composite_key);

      const name code = name{contract};
      if (!is_same_table(code, scope, table)) {
         // since the table is being traversed in reverse order and
         // we have identified a key for a new table, need to add
         // the key count for the completed table and report it
         complete();
      }

      if (!keep_processing_()) {
         // performing the check after retrieving the prefix to provide similar results for RPC calls
         // (indicating next scope)
         stopped_processing_ = true;
         table_context_ = table_id_object_view{code, scope, table, name{}, 0};
         return false;
      }

      if (type == key_type::table) {
         backing_store::payer_payload pp(value, value_size);


         if (is_reversed()) {
            // since table is reversed, we see the table key first,
            // so we have the whole table context, except for the
            // count of keys contained in the table, that will need
            // to be added later once the next table is identified
            // (so that we know that this table is 
            // complete - see complete())
            table_context_ = table_id_object_view{code, scope, table, pp.payer, 0};
         }
         else {
            table_context_ = table_id_object_view{code, scope, table, pp.payer, total_count()};
            receiver_.add_row(*table_context_);
         }

      } else if (type != key_type::primary_to_sec) {
         // for individual keys or reversed, need to report the table info for reference
         check_context(code, scope, table);
         std::invoke(extract_index_member_fun_[static_cast<int>(type)], this, remaining, composite_key.end(),
                     value, value_size);
      }
      return true;
   }

   std::optional<table_id_object_view> stopped_at() const {
      if (stopped_processing_) {
         return table_context_;
      }
      return std::optional<table_id_object_view>();
   }

   bool is_reversed() const {
      return context_ == key_context::complete_reverse || context_ == key_context::table_only_reverse || context_ == key_context::standalone_reverse;
   };

   bool is_standalone() const {
      return context_ == key_context::standalone || context_ == key_context::standalone_reverse;
   };

   bool is_table_only() const {
      return context_ == key_context::table_only || context_ == key_context::table_only_reverse;
   };

   // called to indicate processing is complete (to allow completion of reversed table processing)
   void complete() {
      if (is_reversed() && table_context_ && !stopped_processing_) {
         table_context_->count = total_count();
         receiver_.add_row(*table_context_);
      }
   }
private:
   template<typename IndexType>
   uint32_t& secondary_count() {
      constexpr auto index = static_cast<char>(db_key_value_format::derive_secondary_key_type<IndexType>()) -
                             static_cast<char>(db_key_value_format::key_type::sec_i64);
      return all_secondary_indices_count_[index];
   }

   void check_context(const name& code, const name& scope, const name& table) {
      // this method only has to do with standalone processing
      if (!is_standalone() || is_same_table(code, scope, table)) {
         return;
      }
      table_context_ = table_id_object_view{code, scope, table, name{}, 0};
      receiver_.add_row(*table_context_);
   }

   // calculates the total_count of keys, and zeros out the counts
   uint32_t total_count() {
      uint32_t total = primary_count_;
      for (auto& count : all_secondary_indices_count_) {
         total += count;
         count = 0;
      }
      primary_count_ = 0;
      return total;
   }

   bool is_same_table(const name& code, const name& scope, const name& table) const {
      return table_context_ &&
             table_context_->code == code &&
             table_context_->scope == scope &&
             table_context_->table == table;
   }

   Receiver &receiver_;
   const key_context context_;
   Function keep_processing_;
   std::optional<table_id_object_view> table_context_;

   using extract_index_member_fun_t = void (rocksdb_contract_db_table_writer::*)(
         b1::chain_kv::bytes::const_iterator, b1::chain_kv::bytes::const_iterator, const char *, std::size_t);
   constexpr static unsigned member_fun_count = static_cast<unsigned char>(key_type::sec_long_double) + 1;
   const static extract_index_member_fun_t extract_index_member_fun_[member_fun_count];
   uint32_t primary_count_ = 0;
   constexpr static unsigned secondary_indices = static_cast<char>(db_key_value_format::key_type::sec_long_double) -
                                                 static_cast<char>(db_key_value_format::key_type::sec_i64) + 1;
   uint32_t all_secondary_indices_count_[secondary_indices] = {};

   bool stopped_processing_ = false;
};

// array mapping db_key_value_format::key_type values (except table) to functions to break down to component parts
template<typename Receiver, typename Function>
const typename rocksdb_contract_db_table_writer<Receiver, Function>::extract_index_member_fun_t
      rocksdb_contract_db_table_writer<Receiver, Function>::extract_index_member_fun_[] = {
   &rocksdb_contract_db_table_writer<Receiver, Function>::extract_primary_index,
   nullptr,                                                                      // primary_to_sec type is covered by writing the secondary key type
   &rocksdb_contract_db_table_writer<Receiver, Function>::extract_secondary_index<uint64_t>,
   &rocksdb_contract_db_table_writer<Receiver, Function>::extract_secondary_index<uint128_t>,
   &rocksdb_contract_db_table_writer<Receiver, Function>::extract_secondary_index<key256_t>,
   &rocksdb_contract_db_table_writer<Receiver, Function>::extract_secondary_index<float64_t>,
   &rocksdb_contract_db_table_writer<Receiver, Function>::extract_secondary_index<float128_t>,
};

template<typename Receiver, typename Function = std::decay_t < decltype(process_all)>>
class rocksdb_contract_kv_table_writer {
public:
   rocksdb_contract_kv_table_writer(Receiver& r, const Function& keep_processing = process_all)
         : receiver_(r), keep_processing_(keep_processing) {}

   bool operator()(uint64_t contract, const char* key, std::size_t key_size,
                   const char* value, std::size_t value_size) {
      if (!keep_processing_()) {
         return false;
      }
      // In KV RocksDB, payer and actual data are packed together.
      // Extract them.
      backing_store::payer_payload pp(value, value_size);
      kv_object_view row{name(contract),
                         {{key, key + key_size}},
                         {{pp.value, pp.value + pp.value_size}},
                         pp.payer};
      receiver_.add_row(row);
      return true;
   }

private:
   Receiver& receiver_;
   const Function& keep_processing_;
};

namespace detail {
   // used to handle forward and reverse iteration and the limitations of no upper_bound
   class iterator_pair {
   public:
      iterator_pair(const eosio::session::shared_bytes& begin_key,
                    const eosio::session::shared_bytes& end_key,
                    bool is_reverse,
                    kv_undo_stack_ptr::element_type::variant_type& session) : is_reverse_(is_reverse) {
         EOS_ASSERT(begin_key < end_key, database_exception, "Invalid iterator_pair request: begin_key was greater than or equal to end_key.");
         if (is_reverse_) {
            current_ = session.lower_bound(end_key);
            end_ = session.lower_bound(begin_key);
            // since this is reverse iterating, then need to iterate backward if this is a greater-than iterator,
            // to get a greater-than-or-equal reverse iterator
            if (current_ == session.end() || (*current_).first > end_key) {
               --current_;
               EOS_ASSERT(current_ != session.end(), database_exception, "iterator_pair: failed to find lower bound of end_key");
            }
            // since this is reverse iterating, then need to iterate backward to get a less-than iterator
            --end_;
         }
         else {
            current_ = session.lower_bound(begin_key);
            end_ = session.lower_bound(end_key);
         }
      }

      bool valid() const {
         return current_ != end_;
      }

      void next() {
         if (is_reverse_)
            --current_;
         else
            ++current_;
      }

      session_type::iterator::value_type get() const {
         return *current_;
      }
   private:
      const bool is_reverse_;
      kv_undo_stack_ptr::element_type::variant_type::iterator current_;
      kv_undo_stack_ptr::element_type::variant_type::iterator end_;
   };

   template<typename Receiver, typename Function>
   bool is_reversed(const rocksdb_contract_db_table_writer<Receiver, Function>& writer) {
      return writer.is_reversed();
   }

   template<typename Function>
   bool is_reversed(const Function& ) {
      return false;
   }

   template<typename Receiver, typename Function>
   void complete(rocksdb_contract_db_table_writer<Receiver, Function>& writer) {
      writer.complete();
   }

   template<typename Function>
   void complete(Function& ) {
   }

}

template <typename F>
void walk_rocksdb_entries_with_prefix(const kv_undo_stack_ptr& kv_undo_stack,
                                      const eosio::session::shared_bytes& begin_key,
                                      const eosio::session::shared_bytes& end_key,
                                      F& function) {
   auto session = kv_undo_stack->top();

   detail::iterator_pair iter_pair(begin_key, end_key, detail::is_reversed(function), session);
   bool keep_processing = true;
   for (; keep_processing && iter_pair.valid(); iter_pair.next()) {
      const auto data = iter_pair.get();
      // iterating through the session will always return a valid value
      keep_processing = read_rocksdb_entry(data.first, *data.second, function);
   }
   // indicate processing is done
   detail::complete(function);
};

// will walk through all entries with the given prefix, so if passed an exact key, it will match that key
// and any keys with that key as a prefix
template <typename Receiver, typename Session, typename Function = std::decay_t < decltype(process_all)>>
bool process_rocksdb_entry(Session& session,
                           const eosio::session::shared_bytes& key,
                           Receiver& receiver) {
   if (!key) {
      return false;
   }
   const auto value = session.read(key);
   if (!value) {
      return false;
   }
   const char prefix = key[0];
   if (prefix == rocksdb_contract_kv_prefix) {
      rocksdb_contract_kv_table_writer kv_writer(receiver);
      read_rocksdb_entry(key, *value, kv_writer);
   }
   else {
      if (prefix != rocksdb_contract_db_prefix) {
         char buffer[10];
         const auto len = sprintf(buffer, "%02x", static_cast<uint8_t>(prefix));
         buffer[len] = '\0';
         FC_THROW_EXCEPTION(bad_composite_key_exception,
                            "Passed in key is prefixed with: ${prefix} which is neither the DB or KV prefix",
                            ("prefix", buffer));
      }

      rocksdb_contract_db_table_writer db_writer(receiver, key_context::standalone);
      read_rocksdb_entry(key, *value, db_writer);
   }
   return true;
}

template<typename Object>
const char* contract_table_type() {
   if constexpr (std::is_same_v<Object, kv_object_view>) {
      return "kv_object_view";
   }
   else if constexpr (std::is_same_v<Object, table_id_object_view>) {
      return "table_id_object_view";
   }
   else if constexpr (std::is_same_v<Object, primary_index_view>) {
      return "primary_index_view";
   }
   else if constexpr (std::is_same_v<Object, secondary_index_view<uint64_t>>) {
      return "secondary_index_view<uint64_t>";
   }
   else if constexpr (std::is_same_v<Object, secondary_index_view<eosio::chain::uint128_t>>) {
      return "secondary_index_view<uint128_t>";
   }
   else if constexpr (std::is_same_v<Object, secondary_index_view<eosio::chain::key256_t>>) {
      return "secondary_index_view<key256_t>";
   }
   else if constexpr (std::is_same_v<Object, secondary_index_view<float64_t>>) {
      return "secondary_index_view<float64_t>";
   }
   else {
      static_assert(std::is_same_v<Object, secondary_index_view<float128_t>>);
      return "secondary_index_view<float128_t>";
   }
}

template<typename SingleTypeReceiver, typename SingleObject, typename Exception>
struct single_type_error_receiver {
   template<typename Object>
   void add_row(const Object& row) {
      if constexpr (std::is_same_v<Object, table_id_object_view>) {
         static_cast<SingleTypeReceiver*>(this)->add_table_row(row);
      } else if constexpr (std::is_same_v<Object, SingleObject>) {
         static_cast<SingleTypeReceiver*>(this)->add_only_row(row);
      } else {
         FC_THROW_EXCEPTION(Exception, "Invariant failure, should not receive an add_row call of type: ${type}",
                            ("type", contract_table_type<std::decay_t < decltype(row)>>()));
      }
   }
};

template<typename TableReceiver, typename Exception>
struct table_only_error_receiver {
   template<typename Object>
   void add_row(const Object& row) {
      if constexpr (std::is_same_v<Object, table_id_object_view>) {
         static_cast<TableReceiver*>(this)->add_table_row(row);
      } else {
         FC_THROW_EXCEPTION(Exception, "Invariant failure, should not receive an add_row call of type: ${type}",
                            ("type", contract_table_type<std::decay_t < decltype(row)>>()));
      }
   }
};

}}}

FC_REFLECT(eosio::chain::backing_store::table_id_object_view, (code)(scope)(table)(payer)(count) )
FC_REFLECT(eosio::chain::backing_store::primary_index_view, (primary_key)(payer)(value) )
REFLECT_SECONDARY(eosio::chain::backing_store::secondary_index_view<uint64_t>)
REFLECT_SECONDARY(eosio::chain::backing_store::secondary_index_view<eosio::chain::uint128_t>)
REFLECT_SECONDARY(eosio::chain::backing_store::secondary_index_view<eosio::chain::key256_t>)
REFLECT_SECONDARY(eosio::chain::backing_store::secondary_index_view<float64_t>)
REFLECT_SECONDARY(eosio::chain::backing_store::secondary_index_view<float128_t>)

namespace fc {
   inline
   void to_variant( const eosio::chain::backing_store::blob& b, variant& v ) {
      v = variant(base64_encode(b.str_data.data(), b.str_data.size()));
   }

   inline
   void from_variant( const variant& v, eosio::chain::backing_store::blob& b ) {
      b.str_data = base64_decode(v.as_string());
   }
}
