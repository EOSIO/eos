#pragma once

namespace eosio { namespace chain {

///
/// Provide an extra `version` context to a stream to unpack eosio::chain::block_header_state and
/// eosio::chain::global_property_object
///
/// eosio::chain::block_header_state was not designed to be extensible by itself. In order to
/// add new field to eosio::chain::block_header_state which provie backward compatiblity, we 
/// need to add version support for eosio::chain::block_header_state. The version is not embedded 
/// to eosio::chain::block_header_state, it is derived from the version of snapshot and fork 
/// database. This class provides the version information so that eosio::chain::block_header_state
/// can be correctly unpacked. 
///
/// 
template <typename Stream>
struct versioned_unpack_stream {

   versioned_unpack_stream(Stream& stream, uint32_t ver)
       : strm(stream)
       , version(ver) {}
   Stream&     strm;
   uint32_t    version; 
   inline void read(char* data, std::size_t len) { strm.read(data, len); }
   inline auto get(char& c) ->decltype(strm.get(c)) { return strm.get(c); }
};
}}

