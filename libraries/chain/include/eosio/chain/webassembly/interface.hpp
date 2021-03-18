#pragma once

#include <eosio/chain/types.hpp>
#include <eosio/chain/webassembly/common.hpp>
#include <fc/crypto/sha1.hpp>
#include <boost/hana/string.hpp>

namespace eosio { namespace chain {
class apply_context;
namespace webassembly {

   class interface {
      public:
         interface(apply_context& ctx) : context(ctx) {}

         inline apply_context& get_context() { return context; }
         inline const apply_context& get_context() const { return context; }

         /**
          * Retrieve the signed_transaction.context_free_data[index].
          *
          * @ingroup context-free
          * @param index - the index of the context_free_data entry to retrieve.
          * @param[out] buffer - output buffer of the context_free_data entry.
          *
          * @retval -1 if the index is not valid.
          * @retval size of the cfd if the buffer is empty, otherwise return the amount of data copied onto the buffer.
         */
         int32_t get_context_free_data(uint32_t index, legacy_span<char> buffer) const;

         /**
          * Check if a feature is found on the activation set.
          *
          * @ingroup privileged
          * @param feature_name - 256-bit digest representing the feature to query.
          *
          * @return false (deprecated)
          *
          * @deprecated
         */
         int32_t is_feature_active(int64_t feature_name) const;

         /**
          * Activate a a consensus protocol upgrade.
          *
          * @ingroup privileged
          * @param feature_name - 256-bit digest representing the feature to activate.
          *
          * @deprecated
         */
         void activate_feature(int64_t feature_name) const;

         /**
          * Allows a privileged smart contract, e.g. the system contract, to pre-activate a consensus protocol upgrade feature.
          *
          * @ingroup privileged
          * @param feature_digest - 256-bit digest representing the feature to pre-activate.
         */
         void preactivate_feature(legacy_ptr<const digest_type> feature_digest);

         /**
          * Set the resource limits of an account.
          *
          * @ingroup privileged
          *
          * @param account - name of the account whose resource limit to be set.
          * @param ram_bytes - ram limit in absolute bytes.
          * @param net_weight - fractionally proportionate net limit of available resources based on (weight / total_weight_of_all_accounts).
          * @param cpu_weight - fractionally proportionate cpu limit of available resources based on (weight / total_weight_of_all_accounts).
         */
         void set_resource_limits(account_name account, int64_t ram_bytes, int64_t net_weight, int64_t cpu_weight);

         /**
          * Get the resource limits of an account
          *
          * @ingroup privileged
          *
          * @param account - name of the account whose resource limit to get.
          * @param[out] ram_bytes - output to hold retrieved ram limit in absolute bytes.
          * @param[out] net_weight - output to hold net weight.
          * @param[out] cpu_weight - output to hold cpu weight.
         */
         void get_resource_limits(account_name account, legacy_ptr<int64_t, 8> ram_bytes, legacy_ptr<int64_t, 8> net_weight, legacy_ptr<int64_t, 8> cpu_weight) const;

          /**
           * Get the current wasm limits configuration.
           *
           * The structure of the parameters is as follows:
           *
           * - max_mutable_global_bytes
           * The maximum total size (in bytes) used for mutable globals.
           * i32 and f32 consume 4 bytes and i64 and f64 consume 8 bytes.
           * Const globals are not included in this count.
           *
           * - max_table_elements
           * The maximum number of elements of a table.
           *
           * - max_section_elements
           * The maximum number of elements in each section.
           *
           * - max_linear_memory_init
           * The size (in bytes) of the range of memory that may be initialized.
           * Data segments may use the range [0, max_linear_memory_init).
           *
           * - max_func_local_bytes
           * The maximum total size (in bytes) used by parameters and local variables in a function.
           *
           * - max_nested_structures
           * The maximum nesting depth of structured control instructions.
           * The function itself is included in this count.
           *
           * - max_symbol_bytes
           * The maximum size (in bytes) of names used for import and export.
           *
           * - max_module_bytes
           * The maximum total size (in bytes) of a wasm module.
           *
           * - max_code_bytes
           * The maximum size (in bytes) of each function body.
           *
           * - max_pages
           * The maximum number of 64 KiB pages of linear memory that a contract can use.
           * Enforced when an action is executed. The initial size of linear memory is also checked at setcode.
           *
           * - max_call_depth
           * The maximum number of functions that may be on the stack. Enforced when an action is executed.
           *
           * @ingroup privileged
           *
           * @param[out] packed_parameters the ouput for the parameters.
           * @param max_version has no effect, but should be 0.
           *
           * @return the size of the packed parameters if packed_parameters is empty, otherwise it returns the amount of data written in packed_parameters.
         */
         uint32_t get_wasm_parameters_packed( span<char> packed_parameters, uint32_t max_version ) const;

         /**
           * Set the configuration for wasm limits.
           *
           * See get_wasm_parameters_packed documentation for more details on the structure of the packed_parameters.
           *
           * @ingroup privileged
           *
           * @param packed_parameters - a span containing the packed configuration to set.
         */
         void set_wasm_parameters_packed( span<const char> packed_parameters );

         /**
          * Update a single resource limit associated with an account.
          *
          * @ingroup privileged
          *
          * @param account - the account whose limits are being modified.
          * @param resource - the resource to update, which should be either ram, cpu, or net.
          * @param limit - the new limit.  A value of -1 means unlimited.
          *
          * @pre limit >= -1
         */
         void set_resource_limit(account_name account, name resource, int64_t limit);

         /**
          * Get a single resource limit associated with an account.
          *
          * @ingroup privileged
          *
          * @param account - the account whose limits are being modified
          * @param resource - the name of the resource limit which should be either ram, cpu, or net.
          *
          * @return the limit on the resource requested.
         */
         int64_t get_resource_limit(account_name account, name resource) const;

         /**
          * Proposes a schedule change using the legacy producer key format.
          *
          * @ingroup privileged
          *
          * @param packed_producer_schedule - vector of producer keys
          *
          * @return -1 if proposing a new producer schedule was unsuccessful, otherwise returns the version of the new proposed schedule.
         */
         int64_t set_proposed_producers(legacy_span<const char> packed_producer_schedule);

         /**
          * Proposes a schedule change with extended features.
          *
          * Valid formats:
          * 0 : serialized array of producer_keys. Using this format is exactly equivalent to set_proposed_producers.
          * 1 : serialized array of producer_authority's.
          *
          * @ingroup privileged
          *
          * @param packed_producer_format - format of the producer data blob.
          * @param packed_producer_schedule - packed data of representing the producer schedule in the format indicated.
          *
          * @return -1 if proposing a new producer schedule was unsuccessful, otherwise returns the version of the new proposed schedule.
         */
         int64_t set_proposed_producers_ex(uint64_t packed_producer_format, legacy_span<const char> packed_producer_schedule);

         /**
          * Retrieve the blockchain config parameters.
          *
          * @ingroup privileged
          *
          * @param[out] packed_blockchain_parameters - output buffer of the blockchain parameters.
          *
          * return the number of bytes copied to the buffer, or number of bytes required if the buffer is empty.
         */
         uint32_t get_blockchain_parameters_packed(legacy_span<char> packed_blockchain_parameters) const;

         /**
          * Set the blockchain parameters.
          *
          * @ingroup privileged
          *
          * @param packed_blockchain_parameters - a span containing the packed blockchain config parameters.
         */
         void set_blockchain_parameters_packed(legacy_span<const char> packed_blockchain_parameters);

         /**
          * Retrieve the blockchain config parameters.
          * The input buffer is a packed data stream which represents an encoded sequence of parameter_id pairs with the following format:
          * |varuint32:sequence_length | varuint32:parameter_id | ...
          * The output buffer is a packed data stream which represents an encoded sequence of parameter_id:paramter_value pairs with the following format:
          * |varuint32:sequence_length | varuint32:parameter_id | <various>:parameter_value | ...
          * The encoding of parameter_values should be specific to the parameter being set
          * The output buffer format should be valid input for set_parameters_packed.
          * For each known parameter_id in the input sequence there should be an associated entry in the output sequence with the current encoded parameter_value.
          *
          * @brief Retrieve the blockchain config parameters.
          * @ingroup privileged
          *
          * @param packed_parameter_ids - the input buffer with the format as described above.
          * @param[out] packed_parameters - the output buffer with the format as described above.
         */
         uint32_t get_parameters_packed( span<const char> packed_parameter_ids, span<char> packed_parameters) const;

         /**
          * Set the blockchain parameters.
          * It allows a system contract the ability to set parameters in a flexible manner.
          * The input buffer is a packed data stream which represents an encoded sequence of parameter_id:paramter_value pairs with the following format:
          * |varuint32:sequence_length | varuint32:parameter_id | <various>:parameter_value | ...
          * The encoding of parameter_values should be specific to the parameter being set.
          * Having duplicate parameter_ids encoded in the sequence should result in aborting the transaction context.
          * The presence of a parameter_id which is unknown OR which is known but tied to an unactivated consensus protocol
          * should result in aborting the transaction context.
          * There are no requirement for the ordering of items in the sequence.
          *
          * @brief Set the blockchain parameters in a flexible manner.
          * @ingroup privileged
          *
          * @param packed_parameters - buffer to hold the packed data with the format described above.
         */
         void set_parameters_packed( span<const char> packed_parameters );

         /**
          * Gets the maximum key size, maximum value size, and maximum iterators of a kv database and returns the size of the data.
          * The kv parameters are encoded as 16 bytes, representing four 32-bit little-endian values.
          * ```
          * +-------+---------------+---------------+---------------+---------------+
          * | byte  | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 |10 |11 |12 |13 |14 |15 |
          * +-------+---------------+---------------+---------------+---------------+
          * | field |   version     |   key limit   |  value limit  | max iterators |
          * +-------+---------------+---------------+---------------+---------------+
          * | type  |      0        |   32-bits LE  |  32-bits LE   |  32-bits LE   |
          * +-------+---------------+---------------+---------------+---------------+
          * ```
          *
          * @ingroup privileged
          * @param[out] packed_kv_parameters - the buffer containing the packed kv parameters.
          * @param max_version - has no effect, but should be 0.
          *
          * @return Returns the size required in the buffer (if the buffer is too small, nothing is written).
          *
         */
         uint32_t get_kv_parameters_packed(span<char> packed_kv_parameters, uint32_t max_version) const;

         /**
          * Sets the maximum key size, and maximum value size, and maximum iterators of a kv database.
          * Each database has independent limits. The key and value limits only apply to new items.
          * They do not apply to items written before they were applied.
          * If the database is invalid, if version is non-zero, or if buffer_size is less than 16, aborts the transaction.
          *
          * @ingroup privileged
          * @param packed_kv_parameters - the buffer containing the packed kv parameters to be set.
         */
         void set_kv_parameters_packed(span<const char> packed_kv_parameters);

         /**
          * Check if an account is privileged.
          *
          * @ingroup privileged
          * @param account - name of the account to be checked.
          *
          * @retval true if the account is privileged
          * @retval false otherwise
         */
         bool is_privileged(account_name account) const;

         /**
          * Set the privileged status of an account.
          *
          * @ingroup privileged
          * @param account - name of the account that we want to give the privileged status.
          * @param is_priv - privileged status (true or false).
         */
         void set_privileged(account_name account, bool is_priv);

         // softfloat api
         float _eosio_f32_add(float, float) const;
         float _eosio_f32_sub(float, float) const;
         float _eosio_f32_div(float, float) const;
         float _eosio_f32_mul(float, float) const;
         float _eosio_f32_min(float, float) const;
         float _eosio_f32_max(float, float) const;
         float _eosio_f32_copysign(float, float) const;
         float _eosio_f32_abs(float) const;
         float _eosio_f32_neg(float) const;
         float _eosio_f32_sqrt(float) const;
         float _eosio_f32_ceil(float) const;
         float _eosio_f32_floor(float) const;
         float _eosio_f32_trunc(float) const;
         float _eosio_f32_nearest(float) const;
         bool _eosio_f32_eq(float, float) const;
         bool _eosio_f32_ne(float, float) const;
         bool _eosio_f32_lt(float, float) const;
         bool _eosio_f32_le(float, float) const;
         bool _eosio_f32_gt(float, float) const;
         bool _eosio_f32_ge(float, float) const;
         double _eosio_f64_add(double, double) const;
         double _eosio_f64_sub(double, double) const;
         double _eosio_f64_div(double, double) const;
         double _eosio_f64_mul(double, double) const;
         double _eosio_f64_min(double, double) const;
         double _eosio_f64_max(double, double) const;
         double _eosio_f64_copysign(double, double) const;
         double _eosio_f64_abs(double) const;
         double _eosio_f64_neg(double) const;
         double _eosio_f64_sqrt(double) const;
         double _eosio_f64_ceil(double) const;
         double _eosio_f64_floor(double) const;
         double _eosio_f64_trunc(double) const;
         double _eosio_f64_nearest(double) const;
         bool _eosio_f64_eq(double, double) const;
         bool _eosio_f64_ne(double, double) const;
         bool _eosio_f64_lt(double, double) const;
         bool _eosio_f64_le(double, double) const;
         bool _eosio_f64_gt(double, double) const;
         bool _eosio_f64_ge(double, double) const;
         double _eosio_f32_promote(float) const;
         float _eosio_f64_demote(double) const;
         int32_t _eosio_f32_trunc_i32s(float) const;
         int32_t _eosio_f64_trunc_i32s(double) const;
         uint32_t _eosio_f32_trunc_i32u(float) const;
         uint32_t _eosio_f64_trunc_i32u(double) const;
         int64_t _eosio_f32_trunc_i64s(float) const;
         int64_t _eosio_f64_trunc_i64s(double) const;
         uint64_t _eosio_f32_trunc_i64u(float) const;
         uint64_t _eosio_f64_trunc_i64u(double) const;
         float _eosio_i32_to_f32(int32_t) const;
         float _eosio_i64_to_f32(int64_t) const;
         float _eosio_ui32_to_f32(uint32_t) const;
         float _eosio_ui64_to_f32(uint64_t) const;
         double _eosio_i32_to_f64(int32_t) const;
         double _eosio_i64_to_f64(int64_t) const;
         double _eosio_ui32_to_f64(uint32_t) const;
         double _eosio_ui64_to_f64(uint64_t) const;

         /**
          * Get the list of active producer names.
          *
          * @ingroup producer
          * @param[out] producers - output buffer containing the names of the current active producer names.
          *
          * @return number of bytes required (if the buffer is empty), or the number of bytes written to the buffer.
         */
         int32_t get_active_producers(legacy_span<account_name> producers) const;

         /**
          * Tests a given public key with the recovered public key from digest and signature.
          *
          * @ingroup crypto
          * @param digest - digest of the message that was signed.
          * @param sig - signature.
          * @param pub - public key.
         */
         void assert_recover_key(legacy_ptr<const fc::sha256> digest, legacy_span<const char> sig, legacy_span<const char> pub) const;

         /**
          * Calculates the public key used for a given signature on a given digest.
          *
          * @ingroup crypto
          * @param digest - digest of the message that was signed.
          * @param sig - signature.
          * @param[out] pub - output buffer for the public key result.
          *
          * @return size of data written on the buffer.
         */
         int32_t recover_key(legacy_ptr<const fc::sha256> digest, legacy_span<const char> sig, legacy_span<char> pub) const;

         /**
          * Tests if the sha256 hash generated from data matches the provided digest.
          *
          * @ingroup crypto
          * @param data - a span containing the data you want to hash.
          * @param hash_val - digest to compare to.
         */
         void assert_sha256(legacy_span<const char> data, legacy_ptr<const fc::sha256> hash_val) const;

         /**
          * Tests if the sha1 hash generated from data matches the provided digest.
          *
          * @ingroup crypto
          * @param data - a span containing the data you want to hash.
          * @param hash_val - digest to compare to.
         */
         void assert_sha1(legacy_span<const char> data, legacy_ptr<const fc::sha1> hash_val) const;

         /**
          * Tests if the sha512 hash generated from data matches the provided digest.
          *
          * @ingroup crypto
          * @param data - a span containing the data you want to hash.
          * @param hash_val - digest to compare to.
         */
         void assert_sha512(legacy_span<const char> data, legacy_ptr<const fc::sha512> hash_val) const;

         /**
          * Tests if the ripemd160 hash generated from data matches the provided digest.
          *
          * @ingroup crypto
          * @param data - a span containing the data you want to hash.
          * @param hash_val - digest to compare to.
         */
         void assert_ripemd160(legacy_span<const char> data, legacy_ptr<const fc::ripemd160> hash_val) const;

         /**
          * Hashes data using SHA256.
          *
          * @ingroup crypto
          * @param data - a span containing the data.
          * @param[out] hash_val - the resulting digest.
         */
         void sha256(legacy_span<const char> data, legacy_ptr<fc::sha256> hash_val) const;

         /**
          * Hashes data using SHA1.
          *
          * @ingroup crypto
          * @param data - a span containing the data.
          * @param[out] hash_val - the resulting digest.
         */
         void sha1(legacy_span<const char> data, legacy_ptr<fc::sha1> hash_val) const;

         /**
          * Hashes data using SHA512.
          *
          * @ingroup crypto
          * @param data - a span containing the data.
          * @param[out] hash_val - the hash
         */
         void sha512(legacy_span<const char> data, legacy_ptr<fc::sha512> hash_val) const;

         /**
          * Hashes data using RIPEMD160.
          *
          * @ingroup crypto
          * @param data - a span containing the data.
          * @param[out] hash_val - computed digest.
         */
         void ripemd160(legacy_span<const char> data, legacy_ptr<fc::ripemd160> hash_val) const;

         /**
          * Checks if a transaction is authorized by a provided set of keys and permissions.
          *
          * @ingroup permission
          * @param trx_data - serialized transaction.
          * @param pubkeys_data - serialized vector of provided public keys.
          * @param perms_data - serialized vector of provided permissions (empty permission name acts as wildcard).
          *
          * @retval true if transaction is authorized.
          * @retval false otherwise.
         */
         bool check_transaction_authorization(legacy_span<const char> trx_data, legacy_span<const char> pubkeys_data, legacy_span<const char> perms_data) const;

         /**
          * Checks if a permission is authorized by a provided delay and a provided set of keys and permissions.
          *
          * @ingroup permission
          * @param account - the account owner of the permission.
          * @param permission - the name of the permission to check for authorization.
          * @param pubkeys_data - serialized vector of provided public keys.
          * @param perms_data - serialized vector of provided permissions (empty permission name acts as wildcard).
          * @param delay_us - the provided delay in microseconds (cannot exceed INT64_MAX)
          *
          * @retval true if permission is authorized.
          * @retval false otherwise.
         */
         bool check_permission_authorization(account_name account, permission_name permission, legacy_span<const char> pubkeys_data, legacy_span<const char> perms_data, uint64_t delay_us) const;

         /**
          * Returns the last used time of a permission.
          *
          * @ingroup permission
          * @param account - the account owner of the permission.
          * @param permission - the name of the permission.
          *
          * @return the last used time (in microseconds since Unix epoch) of the permission.
         */
         int64_t get_permission_last_used(account_name account, permission_name permission) const;

         /**
          * Returns the creation time of an account.
          *
          * @ingroup permission
          * @param account - the account name.
          *
          * @return the creation time (in microseconds since Unix epoch) of the account.
         */
         int64_t get_account_creation_time(account_name account) const;

         /**
          * Verifies that an account exists in the set of provided auths on an action. Fails if not found.
          *
          * @ingroup authorization
          * @param account - the name of the account to be verified.
         */
         void require_auth(account_name account) const;

         /**
          * Verifies that an account with a specific permission exists in the set of provided auths on an action,
          *
          * @ingroup authorization
          * @param account - the name of the account to be verified.
          * @param permission - the name of the permission to be verified.
         */
         void require_auth2(account_name account, permission_name permission) const;

         /**
          * Test whether an account exists in the set of provided auths on an action.
          *
          * @ingroup authorization
          * @param account - name of the account to be tested.
          *
          * @retval true if the action has an auth with the account name.
          * @retval false otherwise.
         */
         bool has_auth(account_name account) const;

         /**
          * Add the specified account to set of accounts to be notified.
          *
          * @ingroup authorization
          * @param recipient - account to be notified.
         */
         void require_recipient(account_name recipient);

         /**
          * Verifies that n is an existing account.
          *
          * @ingroup authorization
          * @param account - name of the account to check.
          *
          * @return true if the account exists.
          * @return false otherwise.
         */
         bool is_account(account_name account) const;

         /**
          * Returns the time in microseconds from 1970 of the current block.
          *
          * @ingroup system
          *
          * @return time in microseconds from 1970 of the current block.
         */
         uint64_t current_time() const;

         /**
          * Returns the transaction's publication time.
          *
          * @ingroup system
          *
          * @return time in microseconds from 1970 of the publication_time.
         */
         uint64_t publication_time() const;

         /**
          * Check if specified protocol feature has been activated.
          *
          * @ingroup system
          * @param feature_digest - digest of the protocol feature.
          *
          * @retval true if the specified protocol feature has been activated.
          * @retval false otherwise.
         */
         bool is_feature_activated(legacy_ptr<const digest_type> feature_digest) const;

         /**
          * Return the name of the account that sent the current inline action.
          *
          * @ingroup system
          * @return name of account that sent the current inline action (empty name if not called from inline action).
         */
         name get_sender() const;

         /**
          * Aborts processing of this action and unwinds all pending changes.
          *
          * @ingroup context-free
         */
         void abort() const;

         /**
          * Aborts processing of this action if the test condition is false.
          *
          * @ingroup context-free
          * @param condition - test condition.
          * @param msg - string explaining the reason for failure.
         */
         void eosio_assert(bool condition, null_terminated_ptr msg) const;

         /**
          * Aborts processing of this action if the test condition is false.
          *
          * @ingroup context-free
          * @param condition - test condition.
          * @param msg - string explaining the reason for failure.
          */
         void eosio_assert_message(bool condition, legacy_span<const char> msg) const;

         /**
          * Aborts processing of this action if the test condition is false.
          * It can be used to provide an error code rather than a message string on assertion checks.
          * If the assertion fails, the provided error code will be made available through the exception message.
          *
          * @ingroup context-free
          * @param condition - test condition.
          * @param error_code - the error code associated.
         */
         void eosio_assert_code(bool condition, uint64_t error_code) const;

         /**
          * This method will abort execution of wasm without failing the contract.
          *
          * @ingroup context-free
          * @param code - the exit code
         */
         void eosio_exit(int32_t code) const;

         /**
          * Copy up to length bytes of the current action data to the specified location.
          *
          * @ingroup action
          * @param memory - a pointer where up to length bytes of the current action data will be copied.
          *
          * @return the number of bytes copied to msg, or number of bytes that can be copied if an empty span is passed.
         */
         int32_t read_action_data(legacy_span<char> memory) const;

         /**
          * Get the length of the current action's data field. This method is useful for dynamically sized actions.
          *
          * @ingroup action
          * @return the length of the current action's data field
         */
         int32_t action_data_size() const;

         /**
          * Get the current receiver of the action.
          *
          * @ingroup action
          * @return the name of the receiver
         */
         name current_receiver() const;

         /**
          * Sets a value (packed blob char array) to be included in the action receipt.
          *
          * @ingroup action
          * @param packed_blob - the packed blob
         */
         void set_action_return_value(span<const char> packed_blob);

         /**
          * Print a string.
          *
          * @ingroup console
          * @param str - the string to print
         */
         void prints(null_terminated_ptr str);

         /**
          * Prints string up to given length.
          *
          * @ingroup console
          * @param str - the string to print.
          */
         void prints_l(legacy_span<const char> str);

         /**
          * Prints value as a 64 bit signed integer.
          *
          * @ingroup console
          * @param val - 64 bit signed integer to be printed.
          */
         void printi(int64_t val);

         /**
          * Prints value as a 64 bit unsigned integer.
          *
          * @ingroup console
          * @param val - 64 bit unsigned integer to be printed.
          */
         void printui(uint64_t val);

         /**
          * Prints value as a 128 bit signed integer.
          *
          * @ingroup console
          * @param val - 128 bit signed integer to be printed.
          */
         void printi128(legacy_ptr<const __int128> val);

         /**
          * Prints value as a 128 bit unsigned integer.
          *
          * @ingroup console
          * @param val - 128 bit unsigned integer to be printed.
          */
         void printui128(legacy_ptr<const unsigned __int128> val);

         /**
          * Prints value as single-precision floating point number.
          *
          * @ingroup console
          * @param val - single-precision floating point number to be printed.
          */
         void printsf(float32_t val);

         /**
          * Prints value as double-precision floating point number.
          *
          * @ingroup console
          * @param val - double-precision floating point number to be printed
          */
         void printdf(float64_t val);

         /**
          * Prints value as quadruple-precision floating point number.
          *
          * @ingroup console
          * @param val - a pointer to the quadruple-precision floating point number to be printed
          */
         void printqf(legacy_ptr<const float128_t> val);

         /**
          * Prints a 64 bit names as base32 encoded string.
          *
          * @ingroup console
          * @param value - 64 bit name to be printed
          */
         void printn(name value);

         /**
          * Prints a 64 bit names as base32 encoded string
          *
          * @ingroup console
          * @param data - Hex name to be printed.
          */
         void printhex(legacy_span<const char> data);

         /**
          * Store a record in a primary 64-bit integer index table.
          *
          * @ingroup database primary-index
          *
          * @param scope - the scope where the table resides (implied to be within the code of the current receiver).
          * @param table - the name of the table within the current scope context.
          * @param payer - the account that pays for the storage.
          * @param id - id of the entry.
          * @param buffer - record to store.
          *
          * @return iterator to the newly created table row.
          * @post a new entry is created in the table.
         */
         int32_t db_store_i64(uint64_t scope, uint64_t table, uint64_t payer, uint64_t id, legacy_span<const char> buffer);

         /**
          * Update a record in a primary 64-bit integer index table.
          *
          * @ingroup database primary-index
          * @param itr - iterator to the table row containing the record to update.
          * @param payer -  the account that pays for the storage costs.
          * @param buffer - new updated record.
          * 
          * @remark This function does not allow changing the primary key of a 
          * table row. The serialized data that is stored in the table row of a 
          * primary table may include a primary key and that primary key value 
          * could be changed by the contract calling the db_update_i64 intrinsic; 
          * but that does not change the actual primary key of the table row.
          * 
          * @pre `itr` points to an existing table row in the table.
          * @post the record contained in the table row pointed to by `itr` is replaced with the new updated record.
          */
         void db_update_i64(int32_t itr, uint64_t payer, legacy_span<const char> buffer);

         /**
          * Remove a record inside a primary 64-bit integer index table.
          *
          * @ingroup database primary-index
          * @param itr - the iterator to the table row to remove.
          *
          *  @pre `itr` points to an existing table row in the tab.
          */
         void db_remove_i64(int32_t itr);

         /**
          * Get a record in a primary 64-bit integer index table.
          *
          * @ingroup database primary-index
          * @param itr - the iterator to the table row containing the record to retrieve.
          * @param[out] buffer - the buffer which will be filled with the retrieved record.
          *
          * @return size of the data copied into the buffer if buffer is not empty, or size of the retrieved record if the buffer is empty.
          * @pre `itr` points to an existing table row in the table.
          * @post `buffer` will be filled with the retrieved record (truncated to the first `len` bytes if necessary).
          */
         int32_t db_get_i64(int32_t itr, legacy_span<char> buffer);

         /**
          * Find the table row following the referenced table row in a primary 64-bit integer index table.
          *
          * @ingroup database primary-index
          * @param itr - the iterator to the referenced table row.
          * @param[out] primary - pointer to a `uint64_t` variable which will have its value set to the primary key of the next table row.
          *
          * @return iterator to the table row following the referenced table row (or the end iterator of the table if the referenced table row is the last one in the table).
          *
          * @post '*primary' will be replaced with the primary key of the table row following the referenced table row if it exists, otherwise primary will be left untouched.
          */
         int32_t db_next_i64(int32_t itr, legacy_ptr<uint64_t> primary);

         /**
          * Find the table row preceding the referenced table row in a primary 64-bit integer index table.
          *
          * @ingroup database primary-index
          * @param itr - the iterator to the referenced table row.
          * @param[out] primary - pointer to a `uint64_t` variable which will have its value set to the primary key of the next table row.
          *
          * @return iterator to the table row preceding the referenced table row assuming one exists (it will return -1 if the referenced table row is the first one in the table).
          * @post '*primary' will be replaced with the primary key of the table row preceding the referenced table row if it exists, otherwise primary will be left untouched.
          */
         int32_t db_previous_i64(int32_t itr, legacy_ptr<uint64_t> primary);

         /**
          * Find a table row in a primary 64-bit integer index table by primary key.
          *
          * @ingroup database primary-index
          * @param code - the name of the owner of the table.
          * @param scope - the scope where the table resides.
          * @param table - the table name.
          * @param id - the primary key of the record to look up.
          *
          * @return iterator to the table row with a primary key equal to id or the end iterator of the table if the table row could not be found.
          */
         int32_t db_find_i64(uint64_t code, uint64_t scope, uint64_t table, uint64_t id);

         /**
          * Find the table row in a primary 64-bit integer index table that matches the lowerbound condition for a given primary key.
          * Lowerbound record is the first nearest record which primary key is <= the given key.
          *
          * @ingroup database primary-index
          * @param code - the name of the owner of the table.
          * @param scope - the scope where the table resides.
          * @param table - the table name.
          * @param id - the primary key used as a pivot to determine the lowerbound record.
          *
          * @return iterator to the lowerbound record or the end iterator of the table if the table row could not be found.
          */
         int32_t db_lowerbound_i64(uint64_t code, uint64_t scope, uint64_t table, uint64_t id);

         /**
          * Find the table row in a primary 64-bit integer index table that matches the upperbound condition for a given primary key.
          * The table row that matches the upperbound condition is the first table row in the table with the lowest primary key that is > the given key.
          *
          * @ingroup database primary-index
          * @param code - the name of the owner of the table.
          * @param scope - the scope where the table resides.
          * @param table - the table name.
          * @param id - the primary key used as a pivot to determine the upperbound record.
          *
          * @return iterator to the upperbound record or the end iterator of the table if the table row could not be found.
          */
         int32_t db_upperbound_i64(uint64_t code, uint64_t scope, uint64_t table, uint64_t id);

         /**
          * Get an iterator representing just-past-the-end of the last table row of a primary 64-bit integer index table.
          *
          * @ingroup database primary-index
          * @param code - the name of the owner of the table.
          * @param scope - the scope where the table resides.
          * @param table - the table name.
          *
          * @return end iterator of the table.
          */
         int32_t db_end_i64(uint64_t code, uint64_t scope, uint64_t table);

         /**
          * Store an association of a 64-bit integer secondary key to a primary key in a secondary 64-bit integer index table.
          *
          * @ingroup database uint64_t-secondary-index
          * @param scope - the scope where the table resides (implied to be within the code of the current receiver).
          * @param table - the table name.
          * @param payer - the account that is paying for this storage.
          * @param id - the primary key to which to associate the secondary key.
          * @param secondary - the pointer to the key of the secondary index to store.
          *
          * @return iterator to the newly created secondary index.
          * @post new secondary key association between primary key `id` and secondary key `*secondary` is created in the secondary 64-bit integer index table.
          */
         int32_t db_idx64_store(uint64_t scope, uint64_t table, uint64_t payer, uint64_t id, legacy_ptr<const uint64_t> secondary);

         /**
          * Update an association for a 64-bit integer secondary key to a primary key in a secondary 64-bit integer index table.
          *
          * @ingroup database uint64_t-secondary-index
          * @param iterator - the iterator to the table row containing the secondary key association to update.
          * @param payer - the account that pays for the storage costs.
          * @param secondary - pointer to the **new** secondary key that will replace the existing one of the association.
          *
          * @pre `iterator` points to an existing table row in the table.
          * @post the secondary key of the table row pointed to by `iterator` is replaced by `*secondary`.
          */
         void db_idx64_update(int32_t iterator, uint64_t payer, legacy_ptr<const uint64_t> secondary);

         /**
          * Remove a table row from a secondary 64-bit integer index table.
          *
          * @ingroup database uint64_t-secondary-index
          * @param iterator - iterator to the table row to remove.
          *
          * @pre `iterator` points to an existing table row in the table.
          * @post the table row pointed to by `iterator` is removed and the associated storage costs are refunded to the payer.
          */
         void db_idx64_remove(int32_t iterator);

         /**
          * Find a table row in a secondary 64-bit integer index table by secondary key.
          *
          * @ingroup database uint64_t-secondary-index
          *
          * @param code - the name of the owner of the table.
          * @param scope - the scope where the table resides.
          * @param table - the table name.
          * @param secondary - the pointer to the secondary index key.
          * @param[out] primary - pointer to a 'uint64_t' variable which will have its value set to the primary key of the found table row.
          *
          * @return iterator to the first table row with a secondary key equal to `*secondary` or the end iterator of the table if the table row could not be found.
          * @post If and only if the table row is found, `*primary` will be replaced with the primary key of the found table row.
          */
         int32_t db_idx64_find_secondary(uint64_t code, uint64_t scope, uint64_t table, legacy_ptr<const uint64_t> secondary, legacy_ptr<uint64_t> primary);

         /**
          * Find a table row in a secondary 64-bit integer index table by primary key.
          *
          * @ingroup database uint64_t-secondary-index
          * @param code - the name of the owner of the table.
          * @param scope - the scope where the table resides.
          * @param table - the table name.
          * @param[out] secondary - pointer to a 'uint64_t' variable which will have its value set to the secondary key of the found table row.
          * @param primary - the primary key of the table row to look up.
          *
          * @return iterator to the table row with a primary key equal to `primary` or the end iterator of the table if the table row could not be found.
          * @post If and only if the table row is found, `*secondary` will be replaced with the secondary key of the found table row.
          */
         int32_t db_idx64_find_primary(uint64_t code, uint64_t scope, uint64_t table, legacy_ptr<uint64_t> secondary, uint64_t primary);

         /**
          * Find the table row in a secondary 64-bit integer index table that matches the lowerbound condition for a given secondary key.
          * Lowerbound secondary index is the first secondary index which key is <= the given secondary index key.
          *
          * @ingroup database uint64_t-secondary-index
          * @param code - the name of the owner of the table.
          * @param scope - the scope where the table resides.
          * @param table - the table name.
          * @param[out] secondary - pointer to secondary key first used to determine the lowerbound and which is then replaced with the secondary key of the found table row.
          * @param[out] primary - pointer to a `uint64_t` variable which will have its value set to the primary key of the found table row.
          *
          * @return iterator to the found table row or the end iterator of the table if the table row could not be found.
          *
          *  @post If and only if the table row is found, `*secondary` will be replaced with the secondary key of the found table row.
          *  @post If and only if the table row is found, `*primary` will be replaced with the primary key of the found table row.
          */
         int32_t db_idx64_lowerbound(uint64_t code, uint64_t scope, uint64_t table, legacy_ptr<uint64_t, 8> secondary, legacy_ptr<uint64_t, 8> primary);

         /**
          * Find the table row in a secondary 64-bit integer index table that matches the upperbound condition for a given secondary key.
          * The table row that matches the upperbound condition is the first table row in the table with the lowest secondary key that is > the given key.
          *
          * @ingroup database uint64_t-secondary-index
          * @param code - the name of the owner of the table.
          * @param scope - the scope where the table resides.
          * @param table - the table name.
          * @param[out] secondary - pointer to secondary key first used to determine the upperbound and which is then replaced with the secondary key of the found table row.
          * @param[out] primary - pointer to a `uint64_t` variable which will have its value set to the primary key of the found table row.
          *
          * @return iterator to the found table row or the end iterator of the table if the table row could not be found.
          */
         int32_t db_idx64_upperbound(uint64_t code, uint64_t scope, uint64_t table, legacy_ptr<uint64_t, 8> secondary, legacy_ptr<uint64_t, 8> primary);

         /**
          * Get an end iterator representing just-past-the-end of the last table row of a secondary 64-bit integer index table.
          *
          * @ingroup database uint64_t-secondary-index
          * @param code - the name of the owner of the table.
          * @param scope - the scope where the table resides.
          * @param table - the table name.
          *
          * @return end iterator of the table.
          */
         int32_t db_idx64_end(uint64_t code, uint64_t scope, uint64_t table);

         /**
          * Find the table row following the referenced table row in a secondary 64-bit integer index table.
          *
          * @ingroup database uint64_t-secondary-index
          * @param iterator - the iterator to the referenced table row.
          * @param[out] primary - pointer to a `uint64_t` variable which will have its value set to the primary key of the next table row.
          *
          * @return iterator to the table row following the referenced table row (or the end iterator of the table if the referenced table row is the last one in the table).
          * @pre `iterator` points to an existing table row in the table.
          * @post `*primary` will be replaced with the primary key of the table row following the referenced table row if it exists, otherwise `*primary` will be left untouched.
          */
         int32_t db_idx64_next(int32_t iterator, legacy_ptr<uint64_t> primary);

         /**
          * Find the table row preceding the referenced table row in a secondary 64-bit integer index table.
          *
          * @ingroup database uint64_t-secondary-index
          * @param iterator - the iterator to the referenced table row.
          * @param[out] primary - pointer to a `uint64_t` variable which will have its value set to the primary key of the previous table row.
          *
          * @return iterator to the table row preceding the referenced table row assuming one exists (it will return -1 if the referenced table row is the first one in the table).
          * @pre `iterator` points to an existing table row in the table or it is the end iterator of the table.
          * @post `*primary` will be replaced with the primary key of the table row preceding the referenced table row if it exists, otherwise `*primary` will be left untouched.
          */
         int32_t db_idx64_previous(int32_t iterator, legacy_ptr<uint64_t> primary);

         /**
          * Store an association of a 128-bit integer secondary key to a primary key in a secondary 128-bit integer index table.
          *
          * @ingroup database uint128_t-secondary-index
          * @param scope - the scope where the table resides (implied to be within the code of the current receiver).
          * @param table - the table name.
          * @param payer - the account that is paying for this storage.
          * @param id - the primary key to which to associate the secondary key.
          * @param secondary - the pointer to the key of the secondary index to store.
          *
          * @return iterator to the newly created secondary index.
          * @post new secondary key association between primary key `id` and secondary key `*secondary` is created in the secondary 128-bit integer index table.
          */
         int32_t db_idx128_store(uint64_t scope, uint64_t table, uint64_t payer, uint64_t id, legacy_ptr<const uint128_t> secondary);

         /**
          * Update an association for a 128-bit integer secondary key to a primary key in a secondary 128-bit integer index table.
          *
          * @ingroup database uint128_t-secondary-index
          * @param iterator - the iterator to the table row containing the secondary key association to update.
          * @param payer - the account that pays for the storage costs.
          * @param secondary - pointer to the **new** secondary key that will replace the existing one of the association.
          *
          * @pre `iterator` points to an existing table row in the table.
          * @post the secondary key of the table row pointed to by `iterator` is replaced by `*secondary`.
          */
         void db_idx128_update(int32_t iterator, uint64_t payer, legacy_ptr<const uint128_t> secondary);

         /**
          * Remove a table row from a secondary 128-bit integer index table.
          *
          * @ingroup database uint128_t-secondary-index
          * @param iterator - iterator to the table row to remove.
          *
          * @pre `iterator` points to an existing table row in the table.
          * @post the table row pointed to by `iterator` is removed and the associated storage costs are refunded to the payer.
          */
         void db_idx128_remove(int32_t iterator);

         /**
          * Find a table row in a secondary 128-bit integer index table by secondary key.
          *
          * @ingroup database uint128_t-secondary-index
          *
          * @param code - the name of the owner of the table.
          * @param scope - the scope where the table resides.
          * @param table - the table name.
          * @param secondary - the pointer to the secondary index key.
          * @param[out] primary - pointer to a 'uint64_t' variable which will have its value set to the primary key of the found table row.
          *
          * @return iterator to the first table row with a secondary key equal to `*secondary` or the end iterator of the table if the table row could not be found.
          * @post If and only if the table row is found, `*primary` will be replaced with the primary key of the found table row.
          */
         int32_t db_idx128_find_secondary(uint64_t code, uint64_t scope, uint64_t table, legacy_ptr<const uint128_t> secondary, legacy_ptr<uint64_t> primary);

         /**
          * Find a table row in a secondary 128-bit integer index table by primary key.
          *
          * @ingroup database uint128_t-secondary-index
          * @param code - the name of the owner of the table.
          * @param scope - the scope where the table resides.
          * @param table - the table name.
          * @param[out] secondary - pointer to a 'uint128_t' variable which will have its value set to the secondary key of the found table row.
          * @param primary - the primary key of the table row to look up.
          *
          * @return iterator to the table row with a primary key equal to `primary` or the end iterator of the table if the table row could not be found.
          * @post If and only if the table row is found, `*secondary` will be replaced with the secondary key of the found table row.
          */
         int32_t db_idx128_find_primary(uint64_t code, uint64_t scope, uint64_t table, legacy_ptr<uint128_t> secondary, uint64_t primary);

         /**
          * Find the table row in a secondary 128-bit integer index table that matches the lowerbound condition for a given secondary key.
          * Lowerbound secondary index is the first secondary index which key is <= the given secondary index key.
          *
          * @ingroup database uint128_t-secondary-index
          * @param code - the name of the owner of the table.
          * @param scope - the scope where the table resides.
          * @param table - the table name.
          * @param[out] secondary - pointer to secondary key first used to determine the lowerbound and which is then replaced with the secondary key of the found table row.
          * @param[out] primary - pointer to a `uint64_t` variable which will have its value set to the primary key of the found table row.
          *
          * @return iterator to the found table row or the end iterator of the table if the table row could not be found.
          *
          *  @post If and only if the table row is found, `*secondary` will be replaced with the secondary key of the found table row.
          *  @post If and only if the table row is found, `*primary` will be replaced with the primary key of the found table row.
          */
         int32_t db_idx128_lowerbound(uint64_t code, uint64_t scope, uint64_t table, legacy_ptr<uint128_t, 16> secondary, legacy_ptr<uint64_t, 8> primary);

         /**
          * Find the table row in a secondary 128-bit integer index table that matches the upperbound condition for a given secondary key.
          * The table row that matches the upperbound condition is the first table row in the table with the lowest secondary key that is > the given key.
          *
          * @ingroup database uint128_t-secondary-index
          * @param code - the name of the owner of the table.
          * @param scope - the scope where the table resides.
          * @param table - the table name.
          * @param[out] secondary - pointer to secondary key first used to determine the upperbound and which is then replaced with the secondary key of the found table row.
          * @param[out] primary - pointer to a `uint64_t` variable which will have its value set to the primary key of the found table row.
          *
          * @return iterator to the found table row or the end iterator of the table if the table row could not be found.
          */
         int32_t db_idx128_upperbound(uint64_t code, uint64_t scope, uint64_t table, legacy_ptr<uint128_t, 16> secondary, legacy_ptr<uint64_t, 8> primary);

         /**
          * Get an end iterator representing just-past-the-end of the last table row of a secondary 128-bit integer index table.
          *
          * @ingroup database uint128_t-secondary-index
          * @param code - the name of the owner of the table.
          * @param scope - the scope where the table resides.
          * @param table - the table name.
          *
          * @return end iterator of the table.
          */
         int32_t db_idx128_end(uint64_t code, uint64_t scope, uint64_t table);

         /**
          * Find the table row following the referenced table row in a secondary 128-bit integer index table.
          *
          * @ingroup database uint128_t-secondary-index
          * @param iterator - the iterator to the referenced table row.
          * @param[out] primary - pointer to a `uint64_t` variable which will have its value set to the primary key of the next table row.
          *
          * @return iterator to the table row following the referenced table row (or the end iterator of the table if the referenced table row is the last one in the table).
          * @pre `iterator` points to an existing table row in the table.
          * @post `*primary` will be replaced with the primary key of the table row following the referenced table row if it exists, otherwise `*primary` will be left untouched.
          */
         int32_t db_idx128_next(int32_t iterator, legacy_ptr<uint64_t> primary);

         /**
          * Find the table row preceding the referenced table row in a secondary 128-bit integer index table.
          *
          * @ingroup database uint128_t-secondary-index
          * @param iterator - the iterator to the referenced table row.
          * @param[out] primary - pointer to a `uint64_t` variable which will have its value set to the primary key of the previous table row.
          *
          * @return iterator to the table row preceding the referenced table row assuming one exists (it will return -1 if the referenced table row is the first one in the table).
          * @pre `iterator` points to an existing table row in the table or it is the end iterator of the table.
          * @post `*primary` will be replaced with the primary key of the table row preceding the referenced table row if it exists, otherwise `*primary` will be left untouched.
          */
         int32_t db_idx128_previous(int32_t iterator, legacy_ptr<uint64_t> primary);

         /**
          * Store an association of a 256-bit integer secondary key to a primary key in a secondary 256-bit integer index table.
          *
          * @ingroup database 256-bit-secondary-index
          * @param scope - the scope where the table resides (implied to be within the code of the current receiver).
          * @param table - the table name.
          * @param payer - the account that is paying for this storage.
          * @param id - the primary key to which to associate the secondary key.
          * @param data - pointer to the secondary key data stored as an array of 2 `uint128_t` integers.
          *
          * @return iterator to the newly created secondary index.
          * @post new secondary key association between primary key `id` and secondary key `*data` is created in the secondary 256-bit integer index table.
          */
         int32_t db_idx256_store(uint64_t scope, uint64_t table, uint64_t payer, uint64_t id, legacy_span<const uint128_t> data);

         /**
          * Update an association for a 256-bit integer secondary key to a primary key in a secondary 256-bit integer index table.
          *
          * @ingroup database 256-bit-secondary-index
          * @param iterator - the iterator to the table row containing the secondary key association to update.
          * @param payer - the account that pays for the storage costs.
          * @param data - pointer to the **new** secondary key data (which is stored as an array of 2 `uint128_t` integers) that will replace the existing one of the association.
          *
          * @pre `iterator` points to an existing table row in the table.
          * @post the secondary key of the table row pointed to by `iterator` is replaced by the specified secondary key.
          */
         void db_idx256_update(int32_t iterator, uint64_t payer, legacy_span<const uint128_t> data);

         /**
          * Remove a table row from a secondary 256-bit integer index table.
          *
          * @ingroup database 256-bit-secondary-index
          * @param iterator - iterator to the table row to remove.
          *
          * @pre `iterator` points to an existing table row in the table.
          * @post the table row pointed to by `iterator` is removed and the associated storage costs are refunded to the payer.
          */
         void db_idx256_remove(int32_t iterator);

         /**
          * Find a table row in a secondary 256-bit integer index table by secondary key.
          *
          * @ingroup database 256-bit-secondary-index
          *
          * @param code - the name of the owner of the table.
          * @param scope - the scope where the table resides.
          * @param table - the table name.
          * @param data - pointer to the secondary key data (which is stored as an array of 2 `uint128_t` integers) used to lookup the table row.
          * @param[out] primary - pointer to a 'uint64_t' variable which will have its value set to the primary key of the found table row.
          *
          * @return iterator to the first table row with a secondary key equal to the specified secondary key or the end iterator of the table if the table row could not be found.
          * @post If and only if the table row is found, `*primary` will be replaced with the primary key of the found table row.
          */
         int32_t db_idx256_find_secondary(uint64_t code, uint64_t scope, uint64_t table, legacy_span<const uint128_t> data, legacy_ptr<uint64_t> primary);

         /**
          * Find a table row in a secondary 256-bit integer index table by primary key.
          *
          * @ingroup database 256-bit-secondary-index
          * @param code - the name of the owner of the table.
          * @param scope - the scope where the table resides.
          * @param table - the table name.
          * @param[out] data - pointer to the array of 2 `uint128_t` integers which will act as the buffer to hold the retrieved secondary key of the found table row.
          * @param primary - the primary key of the table row to look up.
          *
          * @return iterator to the table row with a primary key equal to `data` or the end iterator of the table if the table row could not be found.
          * @post If and only if the table row is found, `data` will be replaced with the secondary key of the found table row.
          */
         int32_t db_idx256_find_primary(uint64_t code, uint64_t scope, uint64_t table, legacy_span<uint128_t> data, uint64_t primary);

         /**
          * Find the table row in a secondary 256-bit integer index table that matches the lowerbound condition for a given secondary key.
          * Lowerbound secondary index is the first secondary index which key is <= the given secondary index key.
          *
          * @ingroup database 256-bit-secondary-index
          * @param code - the name of the owner of the table.
          * @param scope - the scope where the table resides.
          * @param table - the table name.
          * @param[out] data - pointer to the secondary key data (which is stored as an array of 2 `uint128_t` integers) first used to determine the lowerbound and which is then replaced with the secondary key of the found table row.
          * @param[out] primary - pointer to a `uint64_t` variable which will have its value set to the primary key of the found table row.
          *
          * @return iterator to the found table row or the end iterator of the table if the table row could not be found.
          *
          *  @post If and only if the table row is found, `data` will be replaced with the secondary key of the found table row.
          *  @post If and only if the table row is found, `*primary` will be replaced with the primary key of the found table row.
          */
         int32_t db_idx256_lowerbound(uint64_t code, uint64_t scope, uint64_t table, legacy_span<uint128_t, 16> data, legacy_ptr<uint64_t, 8> primary);

         /**
          * Find the table row in a secondary 256-bit integer index table that matches the upperbound condition for a given secondary key.
          * The table row that matches the upperbound condition is the first table row in the table with the lowest secondary key that is > the given key.
          *
          * @ingroup database 256-bit-secondary-index
          * @param code - the name of the owner of the table.
          * @param scope - the scope where the table resides.
          * @param table - the table name.
          * @param[out] data - pointer to the secondary key data (which is stored as an array of 2 `uint128_t` integers) first used to determine the upperbound and which is then replaced with the secondary key of the found table row.
          * @param[out] primary - pointer to a `uint64_t` variable which will have its value set to the primary key of the found table row.
          *
          * @return iterator to the found table row or the end iterator of the table if the table row could not be found.
          *
          * @post If and only if the table row is found, the buffer pointed to by `data` will be filled with the secondary key of the found table row.
          * @post If and only if the table row is found, `*primary` will be replaced with the primary key of the found table row.
          */
         int32_t db_idx256_upperbound(uint64_t code, uint64_t scope, uint64_t table, legacy_span<uint128_t, 16> data, legacy_ptr<uint64_t, 8> primary);

         /**
          * Get an end iterator representing just-past-the-end of the last table row of a secondary 256-bit integer index table.
          *
          * @ingroup database 256-bit-secondary-index
          * @param code - the name of the owner of the table.
          * @param scope - the scope where the table resides.
          * @param table - the table name.
          *
          * @return end iterator of the table.
          */
         int32_t db_idx256_end(uint64_t code, uint64_t scope, uint64_t table);

         /**
          * Find the table row following the referenced table row in a secondary 256-bit integer index table.
          *
          * @ingroup database 256-bit-secondary-index
          * @param iterator - the iterator to the referenced table row.
          * @param[out] primary - pointer to a `uint64_t` variable which will have its value set to the primary key of the next table row.
          *
          * @return iterator to the table row following the referenced table row (or the end iterator of the table if the referenced table row is the last one in the table).
          * @pre `iterator` points to an existing table row in the table.
          * @post `*primary` will be replaced with the primary key of the table row following the referenced table row if it exists, otherwise `*primary` will be left untouched.
          */
         int32_t db_idx256_next(int32_t iterator, legacy_ptr<uint64_t> primary);

         /**
          * Find the table row preceding the referenced table row in a secondary 256-bit integer index table.
          *
          * @ingroup database 256-bit-secondary-index
          * @param iterator - the iterator to the referenced table row.
          * @param[out] primary - pointer to a `uint64_t` variable which will have its value set to the primary key of the previous table row.
          *
          * @return iterator to the table row preceding the referenced table row assuming one exists (it will return -1 if the referenced table row is the first one in the table).
          * @pre `iterator` points to an existing table row in the table or it is the end iterator of the table.
          * @post `*primary` will be replaced with the primary key of the table row preceding the referenced table row if it exists, otherwise `*primary` will be left untouched.
          */
         int32_t db_idx256_previous(int32_t iterator, legacy_ptr<uint64_t> primary);

         /**
          * Store an association of a double-precision floating-point secondary key to a primary key in a secondary double-precision floating-point index table.
          *
          * @ingroup database double-secondary-index
          * @param scope - the scope where the table resides (implied to be within the code of the current receiver).
          * @param table - the table name.
          * @param payer - the account that is paying for this storage.
          * @param id - the primary key to which to associate the secondary key.
          * @param secondary - pointer to the secondary key.
          *
          * @return iterator to the newly created secondary index.
          * @post new secondary key association between primary key `id` and secondary key `*secondary` is created in the secondary double-precision floating-point index table.
          */
         int32_t db_idx_double_store(uint64_t scope, uint64_t table, uint64_t payer, uint64_t id, legacy_ptr<const float64_t> secondary);

         /**
          * Update an association for a double-precision floating-point secondary key to a primary key in a secondary  double-precision floating-point index table.
          *
          * @ingroup database double-secondary-index
          * @param iterator - the iterator to the table row containing the secondary key association to update.
          * @param payer - the account that pays for the storage costs.
          * @param secondary - pointer to the **new** secondary key that will replace the existing one of the association.
          *
          * @pre `iterator` points to an existing table row in the table.
          * @post the secondary key of the table row pointed to by `iterator` is replaced by the specified secondary key.
          */
         void db_idx_double_update(int32_t iterator, uint64_t payer, legacy_ptr<const float64_t> secondary);

         /**
          * Remove a table row from a secondary double-precision floating-point index table.
          *
          * @ingroup database double-secondary-index
          * @param iterator - iterator to the table row to remove.
          *
          * @pre `iterator` points to an existing table row in the table.
          * @post the table row pointed to by `iterator` is removed and the associated storage costs are refunded to the payer.
          */
         void db_idx_double_remove(int32_t iterator);

         /**
          * Find a table row in a secondary double-precision floating-point index table by secondary key.
          *
          * @ingroup database double-secondary-index
          *
          * @param code - the name of the owner of the table.
          * @param scope - the scope where the table resides.
          * @param table - the table name.
          * @param secondary - Pointer to secondary key used to lookup the table row.
          * @param[out] primary - pointer to a 'uint64_t' variable which will have its value set to the primary key of the found table row.
          *
          * @return iterator to the first table row with a secondary key equal to the specified secondary key or the end iterator of the table if the table row could not be found.
          * @post If and only if the table row is found, `*primary` will be replaced with the primary key of the found table row.
          */
         int32_t db_idx_double_find_secondary(uint64_t code, uint64_t scope, uint64_t table, legacy_ptr<const float64_t> secondary, legacy_ptr<uint64_t> primary);

         /**
          * Find a table row in a secondary double-precision floating-point index table by primary key.
          *
          * @ingroup database double-secondary-index
          * @param code - the name of the owner of the table.
          * @param scope - the scope where the table resides.
          * @param table - the table name.
          * @param[out] secondary - pointer to a `double` variable which will have its value set to the secondary key of the found table row.
          * @param primary - the primary key of the table row to look up.
          *
          * @return iterator to the table row with a primary key equal to `secondary` or the end iterator of the table if the table row could not be found.
          * @post If and only if the table row is found, `secondary` will be replaced with the secondary key of the found table row.
          */
         int32_t db_idx_double_find_primary(uint64_t code, uint64_t scope, uint64_t table, legacy_ptr<float64_t> secondary, uint64_t primary);

         /**
          * Find the table row in a secondary double-precision floating-point index table that matches the lowerbound condition for a given secondary key.
          * Lowerbound secondary index is the first secondary index which key is <= the given secondary index key.
          *
          * @ingroup database double-secondary-index
          * @param code - the name of the owner of the table.
          * @param scope - the scope where the table resides.
          * @param table - the table name.
          * @param[out] secondary - Pointer to secondary key first used to determine the lowerbound and which is then replaced with the secondary key of the found table row.
          * @param[out] primary - pointer to a `uint64_t` variable which will have its value set to the primary key of the found table row.
          *
          * @return iterator to the found table row or the end iterator of the table if the table row could not be found.
          *
          *  @post If and only if the table row is found, `*secondary` will be replaced with the secondary key of the found table row.
          *  @post If and only if the table row is found, `*primary` will be replaced with the primary key of the found table row.
          */
         int32_t db_idx_double_lowerbound(uint64_t code, uint64_t scope, uint64_t table, legacy_ptr<float64_t, 8> secondary, legacy_ptr<uint64_t, 8> primary);

         /**
          * Find the table row in a secondary double-precision floating-point index table that matches the upperbound condition for a given secondary key.
          * The table row that matches the upperbound condition is the first table row in the table with the lowest secondary key that is > the given key.
          *
          * @ingroup database double-secondary-index
          * @param code - the name of the owner of the table.
          * @param scope - the scope where the table resides.
          * @param table - the table name.
          * @param[out] secondary - pointer to secondary key first used to determine the upperbound and which is then replaced with the secondary key of the found table row.
          * @param[out] primary - pointer to a `uint64_t` variable which will have its value set to the primary key of the found table row.
          *
          * @return iterator to the found table row or the end iterator of the table if the table row could not be found.
          *
          * @post If and only if the table row is found, the buffer pointed to by `*secondary` will be filled with the secondary key of the found table row.
          * @post If and only if the table row is found, `*primary` will be replaced with the primary key of the found table row.
          */
         int32_t db_idx_double_upperbound(uint64_t code, uint64_t scope, uint64_t table, legacy_ptr<float64_t, 8> secondary, legacy_ptr<uint64_t, 8> primary);

         /**
          * Get an end iterator representing just-past-the-end of the last table row of a secondary double-precision floating-point index table.
          *
          * @ingroup database double-secondary-index
          * @param code - the name of the owner of the table.
          * @param scope - the scope where the table resides.
          * @param table - the table name.
          *
          * @return end iterator of the table.
          */
         int32_t db_idx_double_end(uint64_t code, uint64_t scope, uint64_t table);

         /**
          * Find the table row following the referenced table row in a secondary double-precision floating-point index table.
          *
          * @ingroup database double-secondary-index
          * @param iterator - the iterator to the referenced table row.
          * @param[out] primary - pointer to a `uint64_t` variable which will have its value set to the primary key of the next table row.
          *
          * @return iterator to the table row following the referenced table row (or the end iterator of the table if the referenced table row is the last one in the table).
          * @pre `iterator` points to an existing table row in the table.
          * @post `*primary` will be replaced with the primary key of the table row following the referenced table row if it exists, otherwise `*primary` will be left untouched.
          */
         int32_t db_idx_double_next(int32_t iterator, legacy_ptr<uint64_t> primary);

         /**
          * Find the table row preceding the referenced table row in a secondary double-precision floating-point index table.
          *
          * @ingroup database double-secondary-index
          * @param iterator - the iterator to the referenced table row.
          * @param[out] primary - pointer to a `uint64_t` variable which will have its value set to the primary key of the previous table row.
          *
          * @return iterator to the table row preceding the referenced table row assuming one exists (it will return -1 if the referenced table row is the first one in the table).
          * @pre `iterator` points to an existing table row in the table or it is the end iterator of the table.
          * @post `*primary` will be replaced with the primary key of the table row preceding the referenced table row if it exists, otherwise `*primary` will be left untouched.
          */
         int32_t db_idx_double_previous(int32_t iterator, legacy_ptr<uint64_t> primary);

         /**
          * Store an association of a quadruple-precision floating-point secondary key to a primary key in a secondary quadruple-precision floating-point index table.
          *
          * @ingroup database long-double-secondary-index
          * @param scope - the scope where the table resides (implied to be within the code of the current receiver).
          * @param table - the table name.
          * @param payer - the account that is paying for this storage.
          * @param id - the primary key to which to associate the secondary key.
          * @param secondary - pointer to the secondary key.
          *
          * @return iterator to the newly created secondary index.
          * @post new secondary key association between primary key `id` and secondary key `*secondary` is created in the quadruple-precision floating-point index table.
          */
         int32_t db_idx_long_double_store(uint64_t scope, uint64_t table, uint64_t payer, uint64_t id, legacy_ptr<const float128_t> secondary);

         /**
          * Update an association for a quadruple-precision floating-point secondary key to a primary key in a secondary quadruple-precision floating-point index table.
          *
          * @ingroup database long-double-secondary-index
          * @param iterator - the iterator to the table row containing the secondary key association to update.
          * @param payer - the account that pays for the storage costs.
          * @param secondary - pointer to the **new** secondary key that will replace the existing one of the association.
          *
          * @pre `iterator` points to an existing table row in the table.
          * @post the secondary key of the table row pointed to by `iterator` is replaced by the specified secondary key.
          */
         void db_idx_long_double_update(int32_t iterator, uint64_t payer, legacy_ptr<const float128_t> secondary);

         /**
          * Remove a table row from a secondary quadruple-precision floating-point index table.
          *
          * @ingroup database long-double-secondary-index
          * @param iterator - iterator to the table row to remove.
          *
          * @pre `iterator` points to an existing table row in the table.
          * @post the table row pointed to by `iterator` is removed and the associated storage costs are refunded to the payer.
          */
         void db_idx_long_double_remove(int32_t iterator);

         /**
          * Find a table row in a secondary quadruple-precision floating-point index table by secondary key.
          *
          * @ingroup database long-double-secondary-index
          *
          * @param code - the name of the owner of the table.
          * @param scope - the scope where the table resides.
          * @param table - the table name.
          * @param secondary - Pointer to secondary key used to lookup the table row.
          * @param[out] primary - pointer to a 'uint64_t' variable which will have its value set to the primary key of the found table row.
          *
          * @return iterator to the first table row with a secondary key equal to the specified secondary key or the end iterator of the table if the table row could not be found.
          * @post If and only if the table row is found, `*primary` will be replaced with the primary key of the found table row.
          */
         int32_t db_idx_long_double_find_secondary(uint64_t code, uint64_t scope, uint64_t table, legacy_ptr<const float128_t> secondary, legacy_ptr<uint64_t> primary);

         /**
          * Find a table row in a secondary double-precision floating-point index table by primary key.
          *
          * @ingroup database long-double-secondary-index
          * @param code - the name of the owner of the table.
          * @param scope - the scope where the table resides.
          * @param table - the table name.
          * @param[out] secondary - pointer to a `long double` variable which will have its value set to the secondary key of the found table row.
          * @param primary - the primary key of the table row to look up.
          *
          * @return iterator to the table row with a primary key equal to `secondary` or the end iterator of the table if the table row could not be found.
          * @post If and only if the table row is found, `secondary` will be replaced with the secondary key of the found table row.
          */
         int32_t db_idx_long_double_find_primary(uint64_t code, uint64_t scope, uint64_t table, legacy_ptr<float128_t> secondary, uint64_t primary);

         /**
          * Find the table row in a secondary quadruple-precision floating-point index table that matches the lowerbound condition for a given secondary key.
          * Lowerbound secondary index is the first secondary index which key is <= the given secondary index key.
          *
          * @ingroup database long-double-secondary-index
          * @param code - the name of the owner of the table.
          * @param scope - the scope where the table resides.
          * @param table - the table name.
          * @param[out] secondary - Pointer to secondary key first used to determine the lowerbound and which is then replaced with the secondary key of the found table row.
          * @param[out] primary - pointer to a `uint64_t` variable which will have its value set to the primary key of the found table row.
          *
          * @return iterator to the found table row or the end iterator of the table if the table row could not be found.
          *
          *  @post If and only if the table row is found, `*secondary` will be replaced with the secondary key of the found table row.
          *  @post If and only if the table row is found, `*primary` will be replaced with the primary key of the found table row.
          */
         int32_t db_idx_long_double_lowerbound(uint64_t code, uint64_t scope, uint64_t table, legacy_ptr<float128_t, 8> secondary, legacy_ptr<uint64_t, 8> primary);

         /**
          * Find the table row in a secondary quadruple-precision floating-point index table that matches the upperbound condition for a given secondary key.
          * The table row that matches the upperbound condition is the first table row in the table with the lowest secondary key that is > the given key.
          *
          * @ingroup database long-double-secondary-index
          * @param code - the name of the owner of the table.
          * @param scope - the scope where the table resides.
          * @param table - the table name.
          * @param[out] secondary - pointer to secondary key first used to determine the upperbound and which is then replaced with the secondary key of the found table row.
          * @param[out] primary - pointer to a `uint64_t` variable which will have its value set to the primary key of the found table row.
          *
          * @return iterator to the found table row or the end iterator of the table if the table row could not be found.
          *
          * @post If and only if the table row is found, the buffer pointed to by `*secondary` will be filled with the secondary key of the found table row.
          * @post If and only if the table row is found, `*primary` will be replaced with the primary key of the found table row.
          */
         int32_t db_idx_long_double_upperbound(uint64_t code, uint64_t scope, uint64_t table, legacy_ptr<float128_t, 8> secondary,  legacy_ptr<uint64_t, 8> primary);

         /**
          * Get an end iterator representing just-past-the-end of the last table row of a secondary quadruple-precision floating-point index table.
          *
          * @ingroup database long-double-secondary-index
          * @param code - the name of the owner of the table.
          * @param scope - the scope where the table resides.
          * @param table - the table name.
          *
          * @return end iterator of the table.
          */
         int32_t db_idx_long_double_end(uint64_t code, uint64_t scope, uint64_t table);

         /**
          * Find the table row following the referenced table row in a secondary quadruple-precision floating-point index table.
          *
          * @ingroup database long-double-secondary-index
          * @param iterator - the iterator to the referenced table row.
          * @param[out] primary - pointer to a `uint64_t` variable which will have its value set to the primary key of the next table row.
          *
          * @return iterator to the table row following the referenced table row (or the end iterator of the table if the referenced table row is the last one in the table).
          * @pre `iterator` points to an existing table row in the table.
          * @post `*primary` will be replaced with the primary key of the table row following the referenced table row if it exists, otherwise `*primary` will be left untouched.
          */
         int32_t db_idx_long_double_next(int32_t iterator, legacy_ptr<uint64_t> primary);

         /**
          * Find the table row preceding the referenced table row in a secondary quadruple-precision floating-point index table.
          *
          * @ingroup database long-double-secondary-index
          * @param iterator - the iterator to the referenced table row.
          * @param[out] primary - pointer to a `uint64_t` variable which will have its value set to the primary key of the previous table row.
          *
          * @return iterator to the table row preceding the referenced table row assuming one exists (it will return -1 if the referenced table row is the first one in the table).
          * @pre `iterator` points to an existing table row in the table or it is the end iterator of the table.
          * @post `*primary` will be replaced with the primary key of the table row preceding the referenced table row if it exists, otherwise `*primary` will be left untouched.
          */
         int32_t db_idx_long_double_previous(int32_t iterator, legacy_ptr<uint64_t> primary);

         /**
          * Erase a key-value pair.
          *
          * @ingroup kv-database
          * @param contract - name of the contract associated with the kv pair.
          * @param key - the key associated with the kv pair to be erased.
          *
          * @return change in resource usage.
          */
         int64_t  kv_erase(uint64_t contract, span<const char> key);

         /**
          * Set a key-value pair.
          * If the key doesn't exist, then a new kv pair will be created.
          *
          * @ingroup kv-database
          * @param contract - name of the contract associated with the kv pair.
          * @param key - the key in the kv pair.
          * @param value - the value in the kv pair.
          * @param payer - name of the account paying for the resource.
          *
          * @return change in resource usage.
          */
         int64_t  kv_set(uint64_t contract, span<const char> key, span<const char> value, account_name payer);

         /**
          * Check the existence of a key.
          * If the key doesn't exist, it returns false, clears the temporary data buffer and sets *value_size to 0.
          * If the key does exist, it returns true, stores the value into the temporary data buffer
          * and sets *value_size to the value size.
          * Use kv_get_data to retrieve the value.
          *
          * @ingroup kv-database
          * @param contract - name of the contract associated with the kv pair.
          * @param key - the key to query.
          * @param[out] value_size - resulting value size (0 if the key doesn't exist).
          *
          * @return false if the provided key doesn't exist, true otherwise.
          */
         bool     kv_get(uint64_t contract, span<const char> key, uint32_t* value_size);

         /**
          * Fetches data from temporary buffer starting at offset.
          * Copies up to the data's size span passed as parameter.
          *
          * @ingroup kv-database
          * @param offset - position from where to start reading the value from the temporary buffer.
          * @param[out] data - span where the result value will be stored.
          *
          * @return amount of data in the temporary buffer.
          */
         uint32_t kv_get_data(uint32_t offset, span<char> data);

         /**
          * Create a kv iterator.
          *  The returned handle:
          *  - Starts at 1.
          *  - Counts up by 1 at each call, assuming no destroyed iterators are available.
          *  - If destroyed iterators are available, the most-recently-destroyed one is reinitialized and returned.
          *  The prefix limits the range of keys that the iterator covers. If the prefix is empty,
          *  the iterator covers the entire range of keys belonging to a contract within the database ID.
          *
          * @ingroup kv-database
          * @param contract - the contract associated with the kv iterator.
          * @param prefix - prefix to build the iterator.
          *
          * @return handle of the created iterator
          */
         uint32_t kv_it_create(uint64_t contract, span<const char> prefix);

         /**
          * Destroy a kv iterator.
          *
          * @ingroup kv-database
          * @param itr - the kv iterator to destroy.
          */
         void     kv_it_destroy(uint32_t itr);

         /**
          * Get the status of a kv iterator.
          *
          * @ingroup kv-database
          * @param itr - the kv iterator we want to know the status.
          *
          * @retval iterator_ok iterator is positioned at a kv pair.
          * @retval iterator_erased the kv pair that the iterator used to be positioned at, was erased.
          * @retval iterator_end iterator is out-of-bounds.
          */
         int32_t  kv_it_status(uint32_t itr);

         /**
          * Compare the key of two kv iterators.
          *
          * @ingroup kv-database
          * @param itr_a - first iterator.
          * @param itr_b - second iterator.
          *
          * @retval -1 if itr_a's key is less than itr_b's key
          * @retval 0 if itr_a's key is the same as itr_b's key
          * @retval 1 if itr_a's key is greater than itr_b's key
         */
         int32_t  kv_it_compare(uint32_t itr_a, uint32_t itr_b);

         /**
          * Compare the key of an iterator to a provided key.
          *
          * @ingroup kv-database
          * @param itr - the iterator to compare.
          * @param key - the key to compare.
          *
          * @retval -1 if itr's key is less than key.
          * @retval 0 if itr's key is the same as key.
          * @retval 1 itr's key is greater than key.
         */
         int32_t  kv_it_key_compare(uint32_t itr, span<const char> key);

         /**
          * Move a kv iterator to an out-of-bounds position.
          *
          * @ingroup kv-database
          * @param itr - the kv iterator we want to move.
          *
          * @return the status of the iterator (iterator_end).
         */
         int32_t  kv_it_move_to_end(uint32_t itr);

         /**
          * Move a kv iterator to the next position.
          *
          * @ingroup kv-database
          * @param itr - the iterator we want to move.
          * @param[out] found_key_size - size of the result key in the new position.
          * @param[out] found_value_size - size of the result value in the new position.
          *
          * @return the status of the iterator.
         */
         int32_t  kv_it_next(uint32_t itr, uint32_t* found_key_size, uint32_t* found_value_size);

         /**
          * Move a kv iterator to the previous position.
          *
          * @ingroup kv-database
          * @param itr - the iterator we want to move
          * @param[out] found_key_size - size of the result key in the new position.
          * @param[out] found_value_size - size of the result value in the new position.
          *
          * @return the status of the iterator.
         */
         int32_t  kv_it_prev(uint32_t itr, uint32_t* found_key_size, uint32_t* found_value_size);

         /**
          * Find the least non-deleted key which is >= the provided key.
          *
          * @ingroup kv-database
          * @param itr - the kv iterator.
          * @param key - the key we want to query
          * @param[out] found_key_size - size of the result found key
          * @param[out] found_value_size - size of the result found value
          *
          * @return the status of the iterator.
          * @post if a key is found, the new status is iterator_ok. If not found, the new status is iterator_end. kv_it_lower_bound never returns iterator_erased.
          * @post the size of the key and the size of the value at the iterator's new position, or zero if the iterator is at end, will be written to found_key_size and found_value_size.
         */
         int32_t  kv_it_lower_bound(uint32_t itr, span<const char> key, uint32_t* found_key_size, uint32_t* found_value_size);

         /**
          * Fetch the key from a kv iterator.
          *
          * @ingroup kv-database
          * @param itr - the kv iterator.
          * @param offset - position from where to start reading.
          * @param[out] dest - where the data will reside.
          * @param[out] actual_size - size of the key
          *
          * @return the status of the iterator passed as a parameter.
         */
         int32_t  kv_it_key(uint32_t itr, uint32_t offset, span<char> dest, uint32_t* actual_size);

         /**
          * Fetch the value from a kv iterator.
          *
          * @ingroup kv-database
          * @param itr - the iterator of the object we want to get the value.
          * @param offset - position from where to start reading.
          * @param[out] dest - where the value will reside.
          * @param[out] actual_size - size of the value.
          *
          * @return the status of the iterator passed as a parameter.
         */
         int32_t  kv_it_value(uint32_t itr, uint32_t offset, span<char> dest, uint32_t* actual_size);

         // memory api
         void* memcpy(memcpy_params) const;
         void* memmove(memcpy_params) const;
         int32_t memcmp(memcmp_params) const;
         void* memset(memset_params) const;

         /**
          * Send an inline action in the context of the parent transaction of this operation.
          *
          * @ingroup transaction
          * @param data - the inline action to be sent.
         */
         void send_inline(legacy_span<const char> data);

         /**
          * Send a context free inline action in the context of the parent transaction of this operation.
          *
          * @ingroup transaction
          * @param data - the packed free inline action to be sent.
         */
         void send_context_free_inline(legacy_span<const char> data);

         /**
          * Send a deferred transaction.
          *
          * @ingroup transaction
          * @param sender_id - account name of the sender of this deferred transaction.
          * @param payer - account name responsible for paying the RAM for this deferred transaction.
          * @param data - the packed transaction to be deferred.
          * @param replace_existing - if true, it will replace an existing transaction.
         */
         void send_deferred(legacy_ptr<const uint128_t> sender_id, account_name payer, legacy_span<const char> data, uint32_t replace_existing);

         /**
          * Cancels a deferred transaction.
          *
          * @ingroup transaction
          * @param val - The id of the sender.
          *
          * @retval false if transaction was not found.
          * @retval true if transaction was canceled.
         */
         bool cancel_deferred(legacy_ptr<const uint128_t> val);

         /**
          * Access a copy of the currently executing transaction.
          *
          * @ingroup context-free-transaction
          * @param[out] data - the currently executing transaction (packed).
          *
          * @retval false if transaction was not found.
          * @retval true if transaction was canceled.
         */
         int32_t read_transaction(legacy_span<char> data) const;

         /**
          * Gets the size of the currently executing transaction.
          *
          * @ingroup context-free-transaction
          *
          * @return size of the currently executing transaction.
         */
         int32_t transaction_size() const;

         /**
          * Gets the expiration of the currently executing transaction.
          *
          * @ingroup context-free-transaction
          *
          * @return expiration of the currently executing transaction in seconds since Unix epoch.
         */
         int32_t expiration() const;

         /**
          * Gets the block number used for TAPOS on the currently executing transaction.
          *
          * @ingroup context-free-transaction
          *
          * @return block number used for TAPOS on the currently executing transaction.
         */
         int32_t tapos_block_num() const;

         /**
          * Gets the block prefix used for TAPOS on the currently executing transaction.
          *
          * @ingroup context-free-transaction
          *
          * @return block prefix used for TAPOS on the currently executing transaction.
         */
         int32_t tapos_block_prefix() const;

         /**
          * Retrieve the indicated action from the active transaction.
          *
          * @ingroup context-free-transaction
          *
          * @param type - 0 for context free action, 1 for action.
          * @param index - the index of the requested action.
          * @param[out] buffer - the action we want (packed).
          *
          * @return the number of bytes written on the buffer or -1 if there was an error.
         */
         int32_t get_action(uint32_t type, uint32_t index, legacy_span<char> buffer) const;

         // compiler builtins api
         void __ashlti3(legacy_ptr<int128_t>, uint64_t, uint64_t, uint32_t) const;
         void __ashrti3(legacy_ptr<int128_t>, uint64_t, uint64_t, uint32_t) const;
         void __lshlti3(legacy_ptr<int128_t>, uint64_t, uint64_t, uint32_t) const;
         void __lshrti3(legacy_ptr<int128_t>, uint64_t, uint64_t, uint32_t) const;
         void __divti3(legacy_ptr<int128_t>, uint64_t, uint64_t, uint64_t, uint64_t) const;
         void __udivti3(legacy_ptr<uint128_t>, uint64_t, uint64_t, uint64_t, uint64_t) const;
         void __multi3(legacy_ptr<int128_t>, uint64_t, uint64_t, uint64_t, uint64_t) const;
         void __modti3(legacy_ptr<int128_t>, uint64_t, uint64_t, uint64_t, uint64_t) const;
         void __umodti3(legacy_ptr<uint128_t>, uint64_t, uint64_t, uint64_t, uint64_t) const;
         void __addtf3(legacy_ptr<float128_t>, uint64_t, uint64_t, uint64_t, uint64_t) const;
         void __subtf3(legacy_ptr<float128_t>, uint64_t, uint64_t, uint64_t, uint64_t) const;
         void __multf3(legacy_ptr<float128_t>, uint64_t, uint64_t, uint64_t, uint64_t) const;
         void __divtf3(legacy_ptr<float128_t>, uint64_t, uint64_t, uint64_t, uint64_t) const;
         void __negtf2(legacy_ptr<float128_t>, uint64_t, uint64_t) const;
         void __extendsftf2(legacy_ptr<float128_t>, float) const;
         void __extenddftf2(legacy_ptr<float128_t>, double) const;
         double __trunctfdf2(uint64_t, uint64_t) const;
         float __trunctfsf2(uint64_t, uint64_t) const;
         int32_t __fixtfsi(uint64_t, uint64_t) const;
         int64_t __fixtfdi(uint64_t, uint64_t) const;
         void __fixtfti(legacy_ptr<int128_t>, uint64_t, uint64_t) const;
         uint32_t __fixunstfsi(uint64_t, uint64_t) const;
         uint64_t __fixunstfdi(uint64_t, uint64_t) const;
         void __fixunstfti(legacy_ptr<uint128_t>, uint64_t, uint64_t) const;
         void __fixsfti(legacy_ptr<int128_t>, float) const;
         void __fixdfti(legacy_ptr<int128_t>, double) const;
         void __fixunssfti(legacy_ptr<uint128_t>, float) const;
         void __fixunsdfti(legacy_ptr<uint128_t>, double) const;
         double __floatsidf(int32_t) const;
         void __floatsitf(legacy_ptr<float128_t>, int32_t) const;
         void __floatditf(legacy_ptr<float128_t>, uint64_t) const;
         void __floatunsitf(legacy_ptr<float128_t>, uint32_t) const;
         void __floatunditf(legacy_ptr<float128_t>, uint64_t) const;
         double __floattidf(uint64_t, uint64_t) const;
         double __floatuntidf(uint64_t, uint64_t) const;
         int32_t __cmptf2(uint64_t, uint64_t, uint64_t, uint64_t) const;
         int32_t __eqtf2(uint64_t, uint64_t, uint64_t, uint64_t) const;
         int32_t __netf2(uint64_t, uint64_t, uint64_t, uint64_t) const;
         int32_t __getf2(uint64_t, uint64_t, uint64_t, uint64_t) const;
         int32_t __gttf2(uint64_t, uint64_t, uint64_t, uint64_t) const;
         int32_t __letf2(uint64_t, uint64_t, uint64_t, uint64_t) const;
         int32_t __lttf2(uint64_t, uint64_t, uint64_t, uint64_t) const;
         int32_t __unordtf2(uint64_t, uint64_t, uint64_t, uint64_t) const;

         // security group api
         /**
          * Propose new participants to the security group.
          *
          * @ingroup security-group
          * @param packed_participants - the buffer containing the packed participants.
          * 
          * @return -1 if proposing a new security group was unsuccessful, otherwise returns 0.
         */
         int64_t add_security_group_participants(span<const char> packed_participants);

         /**
          * Propose to remove participants from the security group.
          *
          * @ingroup security-group
          * @param packed_participants - the buffer containing the packed participants.
          * 
          * @return -1 if proposing a new security group was unsuccessful, otherwise returns 0.
         */
         int64_t remove_security_group_participants(span<const char> packed_participants);

         /**
          * Check if the specified accounts are all in the active security group.
          *
          * @ingroup security-group
          * @param packed_participants - the buffer containing the packed participants.
          * 
          * @return Returns true if the specified accounts are all in the active security group. 
         */
         bool in_active_security_group(span<const char> packed_participants) const;

         /**
          * Gets the active security group
          *
          * @ingroup security-group
          * @param[out] packed_security_group - the buffer containing the packed security_group.
          *
          * @return Returns the size required in the buffer (if the buffer is too small, nothing is written).
          *
         */
         uint32_t get_active_security_group(span<char> packed_security_group) const;

      private:
         apply_context& context;
   };

}}} // ns eosio::chain::webassembly
