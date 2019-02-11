/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
extern "C" {

uint32_t read_action_data( void* msg, uint32_t buffer_size ) {
   auto s = ctx().act.data.size();
   if( buffer_size == 0 || msg == NULL) return s;

   auto copy_size = std::min( (size_t)buffer_size, s );
   memcpy( msg, ctx().act.data.data(), copy_size );

   return copy_size;

}

uint32_t action_data_size() {
   return ctx().act.data.size();
}

void get_action_info(uint64_t* account, uint64_t* name) {
   *account = ctx().act.account;
   *name = ctx().act.name;
}

uint64_t current_receiver() {
   return ctx().receiver;
}

void require_recipient( uint64_t name ) {
   ctx().require_recipient(name);
}

void require_auth( uint64_t name ) {
   ctx().require_authorization(name);
}

void require_auth2( uint64_t name, uint64_t permission ) {
   ctx().require_authorization(name, permission);
}

bool has_auth( uint64_t name ) {
   return ctx().has_authorization(name);
}

bool is_account( uint64_t name ) {
   return ctx().is_account(name);
}

void send_inline(const char *data, size_t data_len) {
   //TODO: Why is this limit even needed? And why is it not consistently checked on actions in input or deferred transactions
   EOS_ASSERT( data_len < ctx().control.get_global_properties().configuration.max_inline_action_size, inline_action_too_big,
              "inline action too big" );

   action act;
   fc::raw::unpack<action>(data, data_len, act);
   ctx().execute_inline(std::move(act));
}

void send_context_free_inline(const char *data, size_t data_len) {
   //TODO: Why is this limit even needed? And why is it not consistently checked on actions in input or deferred transactions
   EOS_ASSERT( data_len < ctx().control.get_global_properties().configuration.max_inline_action_size, inline_action_too_big,
             "inline action too big" );

   action act;
   fc::raw::unpack<action>(data, data_len, act);
   ctx().execute_context_free_inline(std::move(act));
}

uint64_t  publication_time() {
   return static_cast<uint64_t>( ctx().trx_context.published.time_since_epoch().count() );
}

}
