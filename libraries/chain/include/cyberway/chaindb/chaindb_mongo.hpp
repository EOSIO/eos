#pragma once

#include <string>
#include <memory>

#include <eosio/chain/chaindb/chaindb_common.hpp>

namespace cyberway { namespace chaindb {

    class mongodb: public chaindb_interface {
    public:
        mongodb(std::string);
        ~mongodb();

//        bool close_code(account_name_t code) override;
//
//        cursor_t lower_bound(const index_request&, void* key, size_t) override;
//        cursor_t upper_bound(const index_request&, void* key, size_t) override;
//        cursor_t find(const index_request&, primary_key_t, void* key, size_t) override;
//        cursor_t end(const index_request&) override;
//        cursor_t clone(cursor_t) override;
//        void     close(cursor_t) override;
//
//        primary_key_t current(cursor_t) override;
//        primary_key_t next(cursor_t) override;
//        primary_key_t prev(cursor_t) override;
//
//        int32_t       datasize(cursor_t) override;
//        primary_key_t data(cursor_t, void* data, const size_t size) override;
//
//        primary_key_t available_primary_key(const table_request&) override;
//
//        cursor_t      insert(const table_request&, primary_key_t, void* data, size_t) override;
//        primary_key_t update(const table_request&, primary_key_t, void* data, size_t) override;
//        primary_key_t remove(const table_request&, primary_key_t) override;

    private:
        struct mongodb_impl_;
        std::unique_ptr<mongodb_impl_> impl_;
    }; // class mongodb

} } // namespace cyberway::chaindb