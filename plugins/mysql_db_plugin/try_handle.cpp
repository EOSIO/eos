#include "try_handle.hpp"

namespace eosio {

void handle(std::function<void()> handler, const std::string& desc) {
    try {
        handler();
    } catch (fc::exception& e) {
        elog("FC Exception while ${desc}: ${e}", ("e", e.to_string())("desc", desc));
    } catch (std::exception& e) {
        elog("STD Exception while ${desc}: ${e}", ("e", e.what())("desc", desc));
    } catch (...) {
        elog("Unknown exception while ${desc}", ("desc", desc));
    }
}

void loop_handle(const std::atomic<bool>& done, const std::string& desc, std::function<void()> handler) {
    while (!done) {
        handle(handler, desc);
    }
    ilog("mysql_db_plugin ${desc} thread shutdown gracefully", ("desc", desc));
}

}
