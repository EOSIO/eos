#pragma once

#include <chain_kv/chain_kv.hpp>
#include <chainbase/chainbase.hpp>

// It's a fatal condition if chainbase and chain_kv get out of sync with each
// other due to exceptions.
#define CATCH_AND_EXIT_DB_FAILURE()                                                                                    \
   catch (...) {                                                                                                       \
      elog("Error while using database");                                                                              \
      /* return -2 -- it's what programs/nodeos/main.cpp reports for std::exception */                                 \
      std::_Exit(-2);                                                                                                  \
   }

namespace eosio { namespace chain {

   struct combined_session {
      std::unique_ptr<chainbase::database::session> cb_session    = {};
      chain_kv::undo_stack*                         kv_undo_stack = {};

      combined_session(chainbase::database& cb_database, chain_kv::undo_stack& kv_undo_stack)
          : kv_undo_stack{ &kv_undo_stack } {
         try {
            try {
               cb_session = std::make_unique<chainbase::database::session>(cb_database.start_undo_session(true));
               kv_undo_stack.push(false);
            }
            FC_LOG_AND_RETHROW()
         }
         CATCH_AND_EXIT_DB_FAILURE()
      }

      combined_session()                        = default;
      combined_session(const combined_session&) = delete;
      combined_session(combined_session&& src) { *this = std::move(src); }

      ~combined_session() { undo(); }

      combined_session& operator=(combined_session&& src) {
         cb_session        = std::move(src.cb_session);
         kv_undo_stack     = src.kv_undo_stack;
         src.kv_undo_stack = nullptr;
         return *this;
      }

      void push() {
         try {
            try {
               if (cb_session)
                  cb_session->push();
               cb_session    = nullptr;
               kv_undo_stack = nullptr;
            }
            FC_LOG_AND_RETHROW()
         }
         CATCH_AND_EXIT_DB_FAILURE()
      }

      void squash() {
         try {
            try {
               if (cb_session)
                  cb_session->squash();
               if (kv_undo_stack)
                  kv_undo_stack->squash(false);
               cb_session    = nullptr;
               kv_undo_stack = nullptr;
            }
            FC_LOG_AND_RETHROW()
         }
         CATCH_AND_EXIT_DB_FAILURE()
      }

      void undo() {
         try {
            try {
               if (cb_session)
                  cb_session->undo();
               if (kv_undo_stack)
                  kv_undo_stack->undo(false);
               cb_session    = nullptr;
               kv_undo_stack = nullptr;
            }
            FC_LOG_AND_RETHROW()
         }
         CATCH_AND_EXIT_DB_FAILURE()
      }
   };

}} // namespace eosio::chain
