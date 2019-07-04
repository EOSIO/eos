#pragma once

#include <map>
#include <vector>

#include <cyberway/chaindb/controller.hpp>
#include <cyberway/chaindb/cache_item.hpp>

namespace cyberway { namespace chaindb {

    struct chaindb_guard final {
        chaindb_guard() = delete;
        chaindb_guard(const chaindb_guard&) = delete;
        chaindb_guard(chaindb_guard&& other) = delete;
        chaindb_guard& operator = (const chaindb_guard& other) = delete;
        chaindb_guard& operator = (chaindb_guard&& other) = delete;

        chaindb_guard(apply_context& context) :
           context_(context) {
        }

        ~chaindb_guard() {
            context_.cursors_guard = nullptr;
        }

        cursor_t add(account_name_t code, find_info&& info) {
            external_code_cursors_[code].emplace_back(std::forward<find_info>(info));
            return external_code_cursors_[code].back().cursor;
        }

        const find_info& get(account_name_t code, cursor_t id) const {
            const auto& code_cursors = external_code_cursors_.at(code);

            const auto& cursor_it = std::find(code_cursors.begin(), code_cursors.end(), id);

            EOS_ASSERT(cursor_it != code_cursors.end(),driver_invalid_cursor_exception,
                       "The cursor ${code}.${id} doesn't exist", ("code", code)("id", id));

            return *cursor_it;
        }

        void remove(account_name_t code, cursor_t id) {
            const auto code_cursors_it = external_code_cursors_.find(code);

            if (code_cursors_it != external_code_cursors_.end()) {
                auto& cursors = code_cursors_it->second;
                const auto cursor_pos = std::find(cursors.begin(), cursors.end(), id);
                cursors.erase(cursor_pos);
            }
        }

    private:
        std::map<account_name_t, std::vector<find_info>> external_code_cursors_;
        apply_context& context_;
    }; // class chaindb_guard

    struct chaindb_cursor_cache final {
        account_name_t   cache_code   = 0;
        cursor_t         cache_cursor = invalid_cursor;
        cache_object_ptr cache;

        void reset() {
            cache_code   = 0;
            cache_cursor = invalid_cursor;
            cache.reset();
        }

    }; // struct chaindb_cursor_cache

} } //namespace cyberway::chaindb
