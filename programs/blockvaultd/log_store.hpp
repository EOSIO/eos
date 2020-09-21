#pragma once

#include <libnuraft/log_store.hxx>
#include <filesystem>
#include <map>
#include <mutex>

namespace blockvault {

class log_store : public nuraft::log_store {
public:
    log_store(std::filesystem::path dir);

    ulong next_slot() const override;

    ulong start_index() const override;

    nuraft::ptr<nuraft::log_entry> last_entry() const override;

    ulong append(nuraft::ptr<nuraft::log_entry>& entry) override;

    void write_at(ulong index, nuraft::ptr<nuraft::log_entry>& entry) override;

    nuraft::ptr<std::vector<nuraft::ptr<nuraft::log_entry>>> log_entries(ulong start, ulong end) override;

    nuraft::ptr<std::vector<nuraft::ptr<nuraft::log_entry>>> log_entries_ext(
            ulong start, ulong end, int64 batch_size_hint_in_bytes = 0) override;

    nuraft::ptr<nuraft::log_entry> entry_at(ulong index) override;

    ulong term_at(ulong index) override;

    nuraft::ptr<nuraft::buffer> pack(ulong index, int32 cnt) override;

    void apply_pack(ulong index, nuraft::buffer& pack) override;

    bool compact(ulong last_log_index) override;

    bool flush() override { return true; }

private:

    std::filesystem::path log_dir_;
    ulong                 start_idx_, next_slot_;

    mutable std::mutex logs_lock_;
    
};

}
