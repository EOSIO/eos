extern "C" {

void _send_deferred(const uint128_t* sender_id, uint64_t payer, const char *data, size_t data_len, uint32_t replace_existing) {
   try {
      transaction trx;
      fc::raw::unpack<transaction>(data, data_len, trx);
      ctx().schedule_deferred_transaction(*sender_id, payer, std::move(trx), replace_existing);
   } FC_CAPTURE_AND_RETHROW((fc::to_hex(data, data_len)));
}

int _cancel_deferred(const uint128_t* val) {
   fc::uint128_t sender_id(*val>>64, uint64_t(*val) );
   return ctx().cancel_deferred_transaction( (unsigned __int128)sender_id );
}

size_t read_transaction(char *data, size_t buffer_size) {
   bytes trx = ctx().get_packed_transaction();

   auto s = trx.size();
   if( buffer_size == 0) return s;

   auto copy_size = std::min( buffer_size, s );
   memcpy( data, trx.data(), copy_size );

   return copy_size;
}

size_t transaction_size() {
   return ctx().get_packed_transaction().size();
}

int tapos_block_num() {
   return ctx().trx_context.trx.ref_block_num;
}

int tapos_block_prefix() {
   return ctx().trx_context.trx.ref_block_prefix;
}

uint32_t expiration() {
   return ctx().trx_context.trx.expiration.sec_since_epoch();
}

int get_action( uint32_t type, uint32_t index, char* buffer, size_t buffer_size ) {
   return ctx().get_action( type, index, buffer, buffer_size );
}

void assert_privileged() {
   EOS_ASSERT( ctx().privileged, unaccessible_api, "${code} does not have permission to call this API", ("code",ctx().receiver) );
}

void assert_context_free() {
   /* the context_free_data is not available during normal application because it is prunable */
   EOS_ASSERT( ctx().context_free, unaccessible_api, "this API may only be called from context_free apply" );
}

int get_context_free_data( uint32_t index, char* buffer, size_t buffer_size ) {
   return ctx().get_context_free_data( index, buffer, buffer_size );
}

}
