#pragma once

namespace eosio { namespace chain {

///
/// Provide an extra `version` context to a stream to unpack eosio::chain::block_header_state
///
/// eosio::chain::block_header_state was not designed to be extensible by itself. In order to
/// add new field to eosio::chain::block_header_state which provie backward compatiblity, we 
/// need to add version support for eosio::chain::block_header_state. The version is not embedded 
/// to eosio::chain::block_header_state, it is derived from the version of snapshot and fork 
/// database. This class provides the version information so that eosio::chain::block_header_state
/// can be correctly unpacked. 
/// 
/// For snapshot and fork database version 1, the `block_header_state.state_extension` field 
//  should be ignored during unpacking. 
/// 
template <typename Stream>
struct block_header_state_unpack_stream {

   block_header_state_unpack_stream(Stream& stream, bool has_state_extension)
       : strm(stream)
       , has_block_header_state_extension(has_state_extension) {}
   Stream&     strm;
   uint32_t    has_block_header_state_extension; 
   inline void read(char* data, std::size_t len) { strm.read(data, len); }
   inline auto get(char& c) ->decltype(strm.get(c)) { return strm.get(c); }
};
}}

