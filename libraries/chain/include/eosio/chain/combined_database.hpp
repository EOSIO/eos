#pragma once

#include <chain_kv/chain_kv.hpp>
#include <chainbase/chainbase.hpp>
#include <eosio/chain/kv_chainbase_objects.hpp>

// It's a fatal condition if chainbase and chain_kv get out of sync with each
// other due to exceptions.
#define CATCH_AND_EXIT_DB_FAILURE()                                                                                    \
   catch (...) {                                                                                                       \
   throw;\
      elog("Error while using database");                                                                              \
      /* return -2 -- it's what programs/nodeos/main.cpp reports for std::exception */                                 \
      std::_Exit(-2);                                                                                                  \
   }

namespace eosio { namespace chain {

   struct combined_session {
      std::unique_ptr<chainbase::database::session> cb_session    = {};
      chain_kv::undo_stack*                         kv_undo_stack = {};
      chainbase::database*                          db;

      combined_session(chainbase::database& cb_database, chain_kv::undo_stack& kv_undo_stack)
         : kv_undo_stack{ &kv_undo_stack }, db{ &cb_database } {
         try {
            cb_session = std::make_unique<chainbase::database::session>(cb_database.start_undo_session(true));
            // If this fails chainbase will roll back safely.
            kv_undo_stack.push(false);
         } catch(chain_kv::exception& e) {
            EOS_THROW(chain_kv_exception, "chain-kv error: ${message}", ("message", e.what()));
         }
      }

      combined_session()                        = default;
      combined_session(const combined_session&) = delete;
      combined_session(combined_session&& src) { *this = std::move(src); }

      ~combined_session() {
         undo();
      }

      combined_session& operator=(combined_session&& src) {
         cb_session        = std::move(src.cb_session);
         kv_undo_stack     = src.kv_undo_stack;
         db                = src.db;
         src.kv_undo_stack = nullptr;
         return *this;
      }

      void push() noexcept {
         if (cb_session)
            cb_session->push();
         cb_session    = nullptr;
         kv_undo_stack = nullptr;
      }

      void squash() {
         try {
            if (kv_undo_stack)
               kv_undo_stack->squash(false);
            if (cb_session)
               cb_session->squash();
            cb_session    = nullptr;
            kv_undo_stack = nullptr;
         }
         FC_LOG_AND_RETHROW()
      }

      void undo() noexcept {
         if (cb_session)
            cb_session->undo();
         if (kv_undo_stack) {
            try {
               kv_undo_stack->undo(false);
            } catch(...) {
               // Revision is out-of-sync with chainbase
               db->modify(db->get<kv_db_config_object>(), [this](auto& cfg) {
                  cfg.rocksdb_revision = kv_undo_stack->revision();
               });
            }
         }
         cb_session    = nullptr;
         kv_undo_stack = nullptr;
      }
   };

}} // namespace eosio::chain
