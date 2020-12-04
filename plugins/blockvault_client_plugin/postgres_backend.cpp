#include "postgres_backend.hpp"
#include <boost/filesystem.hpp>
#include <fstream>
#include <iostream>
#include <limits.h>
#include <unistd.h>

namespace eosio {
namespace blockvault {

postgres_backend::postgres_backend(const std::string& options)
    : conn(options) {
   pqxx::work w(conn);
   w.exec("CREATE TABLE IF NOT EXISTS BlockData (watermark_bn bigint, watermark_ts bigint, lib bigint, "
          "block_id bytea, previous_block_id bytea, block oid, block_size bigint);"
          "CREATE TABLE IF NOT EXISTS SnapshotData (watermark_bn bigint, watermark_ts bigint, snapshot oid);");
   w.commit();

   conn.prepare(
       "insert_constructed_block",
       "INSERT INTO BlockData (watermark_bn, watermark_ts, lib, block_id, previous_block_id, block, block_size) "
       "SELECT $1, $2, $3, $4, $5, $6, $7  WHERE NOT "
       "EXISTS (SELECT * FROM BlockData WHERE (watermark_bn >= $1) OR (watermark_ts >= $2) OR (lib > $3) OR (block_id "
       "= $4))");

   conn.prepare(
       "insert_external_block",
       "INSERT INTO BlockData (watermark_bn, watermark_ts, lib, block_id, previous_block_id, block, block_size) SELECT "
       "COALESCE((SELECT MAX(watermark_bn) FROM BlockData),0), COALESCE((SELECT MAX(watermark_ts) FROM "
       "BlockData),0), $2, $3, $4, $5, $6 WHERE NOT "
       "EXISTS (SELECT * FROM BlockData WHERE lib >= $1 OR block_id = $3)");

   conn.prepare("get_block_insertion_result", "SELECT block from BlockData WHERE block=$1");

   conn.prepare("insert_snapshot",
                "INSERT INTO SnapshotData (watermark_bn, watermark_ts, snapshot) SELECT $1, $2, $3 WHERE NOT EXISTS "
                "(SELECT * FROM SnapshotData WHERE watermark_bn >= $1 OR watermark_ts >= $2)");

   conn.prepare("get_snapshot_insertion_result", "SELECT snapshot from SnapshotData WHERE snapshot=$1");

   conn.prepare("get_sync_watermark", "SELECT watermark_bn, watermark_ts FROM BlockData WHERE "
                                      "previous_block_id = $1 ORDER BY watermark_bn, watermark_ts LIMIT 1");

   conn.prepare("get_latest_snapshot", "SELECT snapshot FROM SnapshotData "
                                       "ORDER BY watermark_bn DESC, watermark_ts DESC LIMIT 1");

   conn.prepare("get_blocks_since_watermark", "SELECT block, block_size FROM BlockData WHERE "
                                              "watermark_bn >= $1 AND watermark_ts >= $2"
                                              "ORDER BY watermark_ts, watermark_bn");

   conn.prepare("get_all_blocks", "SELECT block, block_size FROM BlockData");

   conn.prepare("delete_outdated_block_lo",
                "SELECT lo_unlink(r.block) FROM BlockData r WHERE watermark_bn <= $1 OR watermark_ts <= $2;");
   conn.prepare("delete_outdated_block_data", "DELETE FROM BlockData WHERE watermark_bn <= $1 OR watermark_ts <= $2;");
   conn.prepare("delete_outdated_snapshot_lo",
                "SELECT lo_unlink(r.snapshot) FROM SnapshotData r WHERE watermark_bn < $1 OR watermark_ts < $2;");
   conn.prepare("delete_outdated_snapshot_data",
                "DELETE FROM SnapshotData WHERE watermark_bn < $1 OR watermark_ts < $2;");

   conn.prepare("has_block", "SELECT COUNT(*) FROM BlockData WHERE block_id = $1");
} // namespace blockvault

bool postgres_backend::propose_constructed_block(std::pair<uint32_t, uint32_t> watermark, uint32_t lib,
                                                 const std::vector<char>& block_content, std::string_view block_id,
                                                 std::string_view previous_block_id) {
   try {
      pqxx::work w(conn);

      pqxx::largeobjectaccess obj(w);
      obj.write(nullptr, 0);
      pqxx::binarystring      block_id_blob(block_id.data(), block_id.size());
      pqxx::binarystring      previous_block_id_blob(previous_block_id.data(), previous_block_id.size());
      w.exec_prepared0("insert_constructed_block", watermark.first, watermark.second, lib, block_id_blob,
                       previous_block_id_blob, obj.id(), block_content.size());
      auto r = w.exec_prepared1("get_block_insertion_result", obj.id());
      obj.write(block_content.data(), block_content.size());
      w.commit();
      return true;
   } catch (pqxx::unexpected_rows&) {
      return false;
   }
}

bool postgres_backend::append_external_block(uint32_t block_num, uint32_t lib, const std::vector<char>& block_content,
                                             std::string_view block_id, std::string_view previous_block_id) {
   try {
      pqxx::work w(conn);

      pqxx::largeobjectaccess obj(w);
      obj.write(nullptr, 0);
      pqxx::binarystring      block_id_blob(block_id.data(), block_id.size());
      pqxx::binarystring      previous_block_id_blob(previous_block_id.data(), previous_block_id.size());
      w.exec_prepared0("insert_external_block", block_num, lib, block_id_blob, previous_block_id_blob, obj.id(),
                       block_content.size());
      auto r = w.exec_prepared1("get_block_insertion_result", obj.id());
      obj.write(block_content.data(), block_content.size());
      w.commit();
      return true;

   } catch (pqxx::unexpected_rows& ex) {
      return false;
   }
}

bool postgres_backend::propose_snapshot(std::pair<uint32_t, uint32_t> watermark, const char* snapshot_filename) {

   try {
      std::filebuf infile;

      infile.open(snapshot_filename, std::ios::in);

      pqxx::work              w(conn);
      pqxx::largeobjectaccess obj(w);
      obj.write(nullptr, 0);

      w.exec_prepared0("insert_snapshot", watermark.first, watermark.second, obj.id());
      auto r = w.exec_prepared("get_snapshot_insertion_result", obj.id());

      if (!r.empty()) {

         const int chunk_size = 4096;
         char      chunk[chunk_size];

         auto sz = chunk_size;
         while (sz == chunk_size) {
            sz = infile.sgetn(chunk, chunk_size);
            obj.write(chunk, sz);
         };

         w.exec_prepared("delete_outdated_block_lo", watermark.first, watermark.second);
         w.exec_prepared("delete_outdated_block_data", watermark.first, watermark.second);
         w.exec_prepared("delete_outdated_snapshot_lo", watermark.first, watermark.second);
         w.exec_prepared("delete_outdated_snapshot_data", watermark.first, watermark.second);
      }

      w.commit();
      return !r.empty();

   } catch (pqxx::unexpected_rows&) {
      return false;
   }
}

void retrieve_blocks(backend::sync_callback& callback, pqxx::work& trx, pqxx::result r) {
   std::vector<char> block_data;

   for (const auto& x : r) {
      pqxx::oid               block_oid  = x[0].as<pqxx::oid>();
      uint64_t                block_size = x[1].as<uint64_t>();
      pqxx::largeobjectaccess obj(trx, block_oid, std::ios::in | std::ios::binary);
      block_data.resize(block_size);
      obj.read(block_data.data(), block_size);
      callback.on_block(std::string_view{block_data.data(), block_data.size()});
   }

   trx.commit();
}

void postgres_backend::sync(std::string_view previous_block_id, backend::sync_callback& callback) {
   pqxx::work trx(conn);

   if (previous_block_id.size()) {
      pqxx::binarystring blob(previous_block_id.data(), previous_block_id.size());
      auto               r = trx.exec_prepared("get_sync_watermark", blob);

      if (!r.empty()) {
         retrieve_blocks(
             callback, trx,
             trx.exec_prepared("get_blocks_since_watermark", r[0][0].as<uint32_t>(), r[0][1].as<uint32_t>()));
         return;
      }

      auto row = trx.exec_prepared1("has_block", blob);
      if (row[0].as<int>() != 0)
         // in this case, the client is up-to-date, nothing to sync.  
         return;
   }

   auto r = trx.exec_prepared("get_latest_snapshot");

   if (!r.empty()) {
      pqxx::largeobject snapshot_obj(r[0][0].as<pqxx::oid>());

      fc::temp_file temp_file;
      std::string   fname = temp_file.path().string();

      snapshot_obj.to_file(trx, fname.c_str());
      callback.on_snapshot(fname.c_str());
   }

   retrieve_blocks(callback, trx, trx.exec_prepared("get_all_blocks"));
}

} // namespace blockvault
} // namespace eosio