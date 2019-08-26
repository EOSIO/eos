# EOSIO History Tools ![EOSIO Alpha](https://img.shields.io/badge/EOSIO-Alpha-blue.svg)

[![Software License](https://img.shields.io/badge/license-MIT-lightgrey.svg)](./LICENSE)

The history tools repo has these components:

* Database fillers connect to the nodeos state-history plugin and populate databases
* wasm-ql servers answer incoming queries by running server WASMs, which have read-only access to the databases
* The wasm-ql library, when combined with the CDT library, provides utilities that server WASMs and client WASMs need
* A set of example server WASMs and client WASMs

| App             | Fills LMDB | wasm-ql with LMDB          | Fills PostgreSQL | wasm-ql with PostgreSQL |
| --------------- | ---------- | -------------------------- | ---------------- | ---------------- |
| `fill-lmdb`     | Yes        |                            |                  |                  |        
| `wasm-ql-lmdb`  |            | Yes                        |                  |                  |            
| `combo-lmdb`    | Yes        | Yes                        |                  |                  |            
| `fill-pg`       |            |                            | Yes              |                  |        
| `wasm-ql-pg`    |            |                            |                  | Yes              |            
| `history-tools` | Yes*       | Yes*                       | Yes*             | Yes*             |            

Note: by default, `history-tools` does nothing; use the `--plugin` option to select plugins.

See the [documentation site](https://eosio.github.io/history-tools/)

# Alpha Release

This is the first alpha release of the EOSIO History Tools. It includes database fillers
(`fill-pg`, `fill-lmdb`) which pull data from nodeos's State History Plugin, and a new
query engine (`wasm-ql-pg`, `wasm-ql-lmdb`) which supports queries defined by wasm, along
with an emulation of the legacy `/v1/` RPC API.

This alpha release is designed to solicit community feedback. There are several potential
directions this toolset may take; we'd like feedback on which direction(s) may be most
useful. Please create issues about changes you'd like to see going forward.

Since this is an alpha release, it will likely have incompatible changes in the
future. Some of these may be driven by community feedback.

This release supports nodeos 1.8.x. It does not support 1.7.x or the 1.8 RC versions. This release
includes the following:

## fill-pg

`fill-pg` fills postgresql with data from nodeos's State History Plugin. It provides nearly all
data that applications which monitor the chain need. It provides the following:

* Header information from each block
* Transaction and action traces, including inline actions and deferred transactions
* Contract table history, at the block level
* Tables which track the history of chain state, including
  * Accounts, including permissions and linkauths
  * Account resource limits and usage
  * Contract code
  * Contract ABIs
  * Consensus parameters
  * Activated consensus upgrades

`fill-pg` keeps action data and contract table data in its original binary form. Future versions
may optionally support converting this to JSON.

To conserve space, `fill-pg` doesn't store blocks in postgresql. The majority of apps
don't need the blocks since:

* Blocks don't include inline actions and only include some deferred transactions. Most
  applications need to handle these, so should examine the traces instead. e.g. many transfers
  live in the inline actions and deferred transactions that blocks exclude.
* Most apps don't verify block signatures. If they do, then they should connect directly to
  nodeos's State History Plugin to get the necessary data. Note that contrary to
  popular belief, the data returned by the `/v1/get_block` RPC API is insufficient for
  signature verification since it uses a lossy JSON conversion.
* Most apps which currently use the `/v1/get_block` RPC API (e.g. `eosjs`) only need a tiny
  subset of the data within block; `fill-pg` stores this data. There are apps which use
  `/v1/get_block` incorrectly since their authors didn't realize the blocks miss
  critical data that their applications need.

`fill-pg` supports both full history and partial history (`trim` option). This allows users
to make their own tradeoffs. They can choose between supporting queries covering the entire
history of the chain, or save space by only covering recent history.

## wasm-ql-pg

EOSIO contracts store their data in a format which is convenient for them, but hard
on general-purpose query engines. e.g. the `/v1/get_table_rows` RPC API struggles to provide 
all the necessary query options that applications need. `wasm-ql-pg` allows contract authors
and application authors to design their own queries using the same 
[toolset](https://github.com/EOSIO/eosio.cdt) that they use to design contracts. This
gives them full access to current contract state, a history of contract state, and the
history of actions for that contract. `fill-pg` preserves this data in its original
format to support `wasm-ql-pg`.

wasm-ql supports two kinds of queries:
* Binary queries are the most flexible. A client-side wasm packs the query into a binary
  format, a server-side wasm running in wasm-ql executes the query then produces a result
  in binary format, then the client-side wasm converts the binary to JSON. The toolset
  helps authors create both wasms.
* An emulation of the `/v1/` RPC API.

We're considering dropping client-side wasms and switching the format of the first type
of query to JSON RPC, Graph QL, or another format. We're seeking feedback on this switch.

## fill-lmdb, wasm-ql-lmdb

This pair functions identically to `fill-pg` and `wasm-ql-pg`, but stores data using lmdb
instead of postgresql. Since lmdb is an embedded database instead of a database server,
this option may be simpler to administer.

## Contributing

[Contributing Guide](./CONTRIBUTING.md)

[Code of Conduct](./CONTRIBUTING.md#conduct)

## License

[MIT](./LICENSE)

## Important

See LICENSE for copyright and license terms.  Block.one makes its contribution on a voluntary basis as a member of the EOSIO community and is not responsible for ensuring the overall performance of the software or any related applications.  We make no representation, warranty, guarantee or undertaking in respect of the software or any related documentation, whether expressed or implied, including but not limited to the warranties or merchantability, fitness for a particular purpose and noninfringement. In no event shall we be liable for any claim, damages or other liability, whether in an action of contract, tort or otherwise, arising from, out of or in connection with the software or documentation or the use or other dealings in the software or documentation.  Any test results or performance figures are indicative and will not reflect performance under all conditions.  Any reference to any third party or third-party product, service or other resource is not an endorsement or recommendation by Block.one.  We are not responsible, and disclaim any and all responsibility and liability, for your use of or reliance on any of these resources. Third-party resources may be updated, changed or terminated at any time, so the information here may be out of date or inaccurate.