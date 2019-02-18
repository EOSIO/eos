#include <eosio/chain/apply_context.hpp>
#include <eosio/chain/controller.hpp>
#include <eosio/chain/transaction_context.hpp>

#include <chain_api.hpp>

using namespace fc;
using namespace eosio::chain;

static controller *s_ctrl = nullptr;
static controller::config *s_cfg = nullptr;
static chain_api_cpp s_api = {};

//vm_api.cpp
namespace eosio {
   namespace chain {
      apply_context *get_apply_context();
   }
}

static inline apply_context& ctx() {
   return *get_apply_context();
}

void n2str(uint64_t n, string& str_name) {
   str_name = name(n).to_string();
}

uint64_t str2n(string& str_name) {
   return name(str_name).value;
}

bool get_code(uint64_t contract, digest_type& code_id, const eosio::chain::shared_string** code) {
   try {
      const auto& a = ctx().control.get_account( contract );
      code_id = a.code_version;
      *code = &a.code;
      return true;
   } catch (...) {
   }
   return false;
}

const char* get_code_ex( uint64_t receiver, size_t* size ) {
   if (!ctx().is_account(receiver)) {
      *size = 0;
      return nullptr;
   }

   auto& a = ctx().control.get_account(receiver);
   *size = a.code.size();
   return a.code.data();
}

string get_state_dir() {
   return s_cfg->state_dir.string();
}

bool contracts_console() {
   return s_ctrl->contracts_console();
}

void resume_billing_timer() {
   ctx().trx_context.resume_billing_timer();
}

void pause_billing_timer() {
   ctx().trx_context.pause_billing_timer();
}

bool is_producing_block() {
   return s_ctrl->is_producing_block();
}

std::ostringstream& get_console_stream() {
   return ctx().get_console_stream();
}

void console_append( long double var ) {
   return ctx().console_append(var);
}

//vm_exceptions.cpp
void vm_throw_exception(int type, const char* fmt, ...);

void chain_api_init(controller *ctrl, controller::config *cfg) {
    static bool init = false;
    if (init) {
        return;
    }
    init = true;

    s_ctrl = ctrl;
    s_cfg = cfg;

    s_api.n2str = n2str;
    s_api.str2n = str2n;
    s_api.get_code = get_code;
    s_api.get_code_ex = get_code_ex;
    s_api.get_state_dir = get_state_dir;
    s_api.contracts_console = contracts_console;
    s_api.resume_billing_timer = resume_billing_timer;
    s_api.pause_billing_timer = pause_billing_timer;
    s_api.is_producing_block = is_producing_block;
    s_api.throw_exception = vm_throw_exception;
    s_api.get_console_stream = get_console_stream;
    s_api.console_append = console_append;

    register_chain_api_cpp(&s_api);
}
