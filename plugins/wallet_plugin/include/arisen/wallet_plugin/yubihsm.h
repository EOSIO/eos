/*
 * Copyright (c) 2016 Yubico AB
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/**
 @mainpage

 @section Introduction

 Libyubihsm is a library for communicating with a YubiHSM device.

 @section Usage

 To use the library include <yubihsm.h> and pass the -lyubihsm flag to the
 linker.
 Debug output is controlled with the function yh_set_verbosity().

 First step of using a YubiHSM is to init the library with yh_init(), init a
 connector with yh_init_connector() and then connect it with yh_connect_best().
 After this a session must be established with yh_create_session_derived() and
 yh_authenticate_session().
 When a session is established commands can be exchanged over it, the functions
 in the namespace yh_util are high-level convenience functions that do a
 specific task with the device.

 @section api API Reference

 yubihsm.h
 All public functions and definitions

 @section example Code example
 Here is a small example of establishing a session with a YubiHSM and fetching
 some random before shutting down the session.
 \code{.c}
 int main(void) {
   yh_connector *connector = NULL;
   yh_session *session = NULL;
   uint8_t context[YH_CONTEXT_LEN] = {0};
   uint8_t data[128] = {0};
   size_t data_len = sizeof(data);

   assert(yh_init() == YHR_SUCCESS);
   assert(yh_init_connector("http://localhost:12345", &connector) ==
 YHR_SUCCESS);
   assert(yh_connect_best(&connector, 1, NULL) == YHR_SUCCESS);
   assert(yh_create_session_derived(connector, 1, YH_DEFAULT_PASSWORD,
 strlen(YH_DEFAULT_PASSWORD), false, context, sizeof(context), &session) ==
 YHR_SUCCESS);
   assert(yh_authenticate_session(session, context, sizeof(context)) ==
 YHR_SUCCESS);
   assert(yh_util_get_random(session, sizeof(data), data, &data_len) ==
 YHR_SUCCESS);
   assert(data_len == sizeof(data));
   assert(yh_util_close_session(session) == YHR_SUCCESS);
   assert(yh_destroy_session(&session) == YHR_SUCCESS);
   assert(yh_disconnect(connector) == YHR_SUCCESS);
 }
 \endcode

 */

/** @file yubihsm.h
 *
 * Everything you need for yubihsm.
 */

#ifndef YUBIHSM_H
#define YUBIHSM_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

/// Length of context array for authentication
#define YH_CONTEXT_LEN 16
/// Length of host challenge for authentication
#define YH_HOST_CHAL_LEN 8
/// Maximum length of message buffer
#define YH_MSG_BUF_SIZE 2048
/// Length of authentication keys
#define YH_KEY_LEN 16
/// Device vendor ID
#define YH_VID 0x1050
/// Device product ID
#define YH_PID 0x0030
/// Response flag for commands
#define YH_CMD_RESP_FLAG 0x80
/// Max items the device may hold
#define YH_MAX_ITEMS_COUNT                                                     \
  256 // TODO: should this really be defined in the API?
/// Max sessions the device may hold
#define YH_MAX_SESSIONS 16 // TODO: same here, really part of the API?
/// Default encryption key
#define YH_DEFAULT_ENC_KEY                                                     \
  "\x09\x0b\x47\xdb\xed\x59\x56\x54\x90\x1d\xee\x1c\xc6\x55\xe4\x20"
/// Default MAC key
#define YH_DEFAULT_MAC_KEY                                                     \
  "\x59\x2f\xd4\x83\xf7\x59\xe2\x99\x09\xa0\x4c\x45\x05\xd2\xce\x0a"
/// Default authentication key password
#define YH_DEFAULT_PASSWORD "password"
/// Salt to be used for PBKDF2 key derivation
#define YH_DEFAULT_SALT "Yubico"
/// Number of iterations for PBKDF2 key derivation
#define YH_DEFAULT_ITERS 10000
/// Length of capabilities array
#define YH_CAPABILITIES_LEN 8
/// Max log entries the device may hold
#define YH_MAX_LOG_ENTRIES 64 // TODO: really part of the API?
/// Length of object labels
#define YH_OBJ_LABEL_LEN 40
/// Max number of domains
#define YH_MAX_DOMAINS 16

// Debug levels
/// No messages
#define YH_VERB_QUIET 0x00
/// Intermediate results
#define YH_VERB_INTERMEDIATE 0x01
/// Crypto results
#define YH_VERB_CRYPTO 0x02
/// Raw messages
#define YH_VERB_RAW 0x04
/// General info
#define YH_VERB_INFO 0x08
/// Error messages
#define YH_VERB_ERR 0x10
/// All previous options enabled
#define YH_VERB_ALL 0xff

/// This is the overhead when doing aes-ccm wrapping, 1 byte identifier, 13
/// bytes nonce and 16 bytes mac
#define YH_CCM_WRAP_OVERHEAD (1 + 13 + 16)

#ifdef __cplusplus
extern "C" {
#endif

/// Reference to a connector
typedef struct yh_connector yh_connector;

/// Reference to a session
typedef struct yh_session yh_session;

/// Capabilitites representation
typedef struct {
  /// Capabilities is represented as an 8 byte uint8_t array
  uint8_t capabilities[YH_CAPABILITIES_LEN];
} yh_capabilities;

/**
 * Return codes.
 **/
typedef enum {
  /// Success
  YHR_SUCCESS = 0,
  /// Memory error
  YHR_MEMORY = -1,
  /// Init error
  YHR_INIT_ERROR = -2,
  /// Net error
  YHR_NET_ERROR = -3,
  /// Connector not found
  YHR_CONNECTOR_NOT_FOUND = -4,
  /// Invalid parameters
  YHR_INVALID_PARAMS = -5,
  /// Wrong length
  YHR_WRONG_LENGTH = -6,
  /// Buffer too small
  YHR_BUFFER_TOO_SMALL = -7,
  /// Cryptogram error
  YHR_CRYPTOGRAM_MISMATCH = -8,
  /// Authenticate session error
  YHR_AUTH_SESSION_ERROR = -9,
  /// MAC not matching
  YHR_MAC_MISMATCH = -10,
  /// Device success
  YHR_DEVICE_OK = -11,
  /// Invalid command
  YHR_DEVICE_INV_COMMAND = -12,
  /// Malformed command / invalid data
  YHR_DEVICE_INV_DATA = -13,
  /// Invalid session
  YHR_DEVICE_INV_SESSION = -14,
  /// Message encryption / verification failed
  YHR_DEVICE_AUTH_FAIL = -15,
  /// All sessions are allocated
  YHR_DEVICE_SESSIONS_FULL = -16,
  /// Session creation failed
  YHR_DEVICE_SESSION_FAILED = -17,
  /// Storage failure
  YHR_DEVICE_STORAGE_FAILED = -18,
  /// Wrong length
  YHR_DEVICE_WRONG_LENGTH = -19,
  /// Wrong permissions for operation
  YHR_DEVICE_INV_PERMISSION = -20,
  /// Log buffer is full and forced audit is set
  YHR_DEVICE_LOG_FULL = -21,
  /// Object not found
  YHR_DEVICE_OBJ_NOT_FOUND = -22,
  /// Id use is illegal
  YHR_DEVICE_ID_ILLEGAL = -23,
  /// OTP submitted is invalid
  YHR_DEVICE_INVALID_OTP = -24,
  /// Device is in demo mode and has to be power cycled
  YHR_DEVICE_DEMO_MODE = -25,
  /// The command execution has not terminated
  YHR_DEVICE_CMD_UNEXECUTED = -26,
  /// Unknown error
  YHR_GENERIC_ERROR = -27,
  /// Object with that ID already exists
  YHR_DEVICE_OBJECT_EXISTS = -28,
  /// Connector operation failed
  YHR_CONNECTOR_ERROR = -29
} yh_rc;

/// Macro to define command and response command
#define ADD_COMMAND(c, v) c = v, c##_R = v | YH_CMD_RESP_FLAG

/**
 * Command definitions
 */
typedef enum {
  /// Echo
  ADD_COMMAND(YHC_ECHO, 0x01),
  /// Create session
  ADD_COMMAND(YHC_CREATE_SES, 0x03),
  /// Authenticate session
  ADD_COMMAND(YHC_AUTH_SES, 0x04),
  /// Session message
  ADD_COMMAND(YHC_SES_MSG, 0x05),
  /// Get device info
  ADD_COMMAND(YHC_GET_DEVICE_INFO, 0x06),
  /// BSL
  ADD_COMMAND(YHC_BSL, 0x07),
  /// Reset
  ADD_COMMAND(YHC_RESET, 0x08),
  /// Close session
  ADD_COMMAND(YHC_CLOSE_SES, 0x40),
  /// Storage statistics
  ADD_COMMAND(YHC_STATS, 0x041),
  /// Put opaque
  ADD_COMMAND(YHC_PUT_OPAQUE, 0x42),
  /// Get opaque
  ADD_COMMAND(YHC_GET_OPAQUE, 0x43),
  /// Put authentication key
  ADD_COMMAND(YHC_PUT_AUTHKEY, 0x44),
  /// Put asymmetric key
  ADD_COMMAND(YHC_PUT_ASYMMETRIC_KEY, 0x45),
  /// Generate asymmetric key
  ADD_COMMAND(YHC_GEN_ASYMMETRIC_KEY, 0x46),
  /// Sign data with PKCS1
  ADD_COMMAND(YHC_SIGN_DATA_PKCS1, 0x47),
  /// List objects
  ADD_COMMAND(YHC_LIST, 0x48),
  /// Decrypt data with PKCS1
  ADD_COMMAND(YHC_DECRYPT_PKCS1, 0x49),
  /// Export an object wrapped
  ADD_COMMAND(YHC_EXPORT_WRAPPED, 0x4a),
  /// Import a wrapped object
  ADD_COMMAND(YHC_IMPORT_WRAPPED, 0x4b),
  /// Put wrap key
  ADD_COMMAND(YHC_PUT_WRAP_KEY, 0x4c),
  /// Get audit logs
  ADD_COMMAND(YHC_GET_LOGS, 0x4d),
  /// Get object information
  ADD_COMMAND(YHC_GET_OBJECT_INFO, 0x4e),
  /// Put a global option
  ADD_COMMAND(YHC_PUT_OPTION, 0x4f),
  /// Get a global option
  ADD_COMMAND(YHC_GET_OPTION, 0x50),
  /// Get pseudo random data
  ADD_COMMAND(YHC_GET_PSEUDO_RANDOM, 0x51),
  /// Put HMAC key
  ADD_COMMAND(YHC_PUT_HMAC_KEY, 0x52),
  /// HMAC data
  ADD_COMMAND(YHC_HMAC_DATA, 0x53),
  /// Get a public key
  ADD_COMMAND(YHC_GET_PUBKEY, 0x54),
  /// Sign data with PSS
  ADD_COMMAND(YHC_SIGN_DATA_PSS, 0x55),
  /// Sign data with ECDSA
  ADD_COMMAND(YHC_SIGN_DATA_ECDSA, 0x56),
  /// Perform a ECDH exchange
  ADD_COMMAND(YHC_DECRYPT_ECDH, 0x57),
  /// Delete an object
  ADD_COMMAND(YHC_DELETE_OBJECT, 0x58),
  /// Decrypt data with OAEP
  ADD_COMMAND(YHC_DECRYPT_OAEP, 0x59),
  /// Generate HMAC key
  ADD_COMMAND(YHC_GENERATE_HMAC_KEY, 0x5a),
  /// Generate wrap key
  ADD_COMMAND(YHC_GENERATE_WRAP_KEY, 0x5b),
  /// Verify HMAC data
  ADD_COMMAND(YHC_VERIFY_HMAC, 0x5c),
  /// SSH Certify
  ADD_COMMAND(YHC_SSH_CERTIFY, 0x5d),
  /// Put template
  ADD_COMMAND(YHC_PUT_TEMPLATE, 0x5e),
  /// Get template
  ADD_COMMAND(YHC_GET_TEMPLATE, 0x5f),
  /// Decrypt OTP
  ADD_COMMAND(YHC_OTP_DECRYPT, 0x60),
  /// Create OTP AEAD
  ADD_COMMAND(YHC_OTP_AEAD_CREATE, 0x61),
  /// Create OTP AEAD from random
  ADD_COMMAND(YHC_OTP_AEAD_RANDOM, 0x62),
  /// Rewrap OTP AEAD
  ADD_COMMAND(YHC_OTP_AEAD_REWRAP, 0x63),
  /// Attest an asymmetric key
  ADD_COMMAND(YHC_ATTEST_ASYMMETRIC, 0x64),
  /// Put OTP AEAD key
  ADD_COMMAND(YHC_PUT_OTP_AEAD_KEY, 0x65),
  /// Generate OTP AEAD key
  ADD_COMMAND(YHC_GENERATE_OTP_AEAD_KEY, 0x66),
  /// Set log index
  ADD_COMMAND(YHC_SET_LOG_INDEX, 0x67),
  /// Wrap data
  ADD_COMMAND(YHC_WRAP_DATA, 0x68),
  /// Unwrap data
  ADD_COMMAND(YHC_UNWRAP_DATA, 0x69),
  /// Sign data with EDDSA
  ADD_COMMAND(YHC_SIGN_DATA_EDDSA, 0x6a),
  /// Blink the device
  ADD_COMMAND(YHC_BLINK, 0x6b),
  /// Error
  YHC_ERROR = 0x7f,
} yh_cmd;

#undef ADD_COMMAND

/**
 * Object types
 */
typedef enum {
  /// Opaque object
  YH_OPAQUE = 0x01,
  /// Authentication key
  YH_AUTHKEY = 0x02,
  /// Asymmetric key
  YH_ASYMMETRIC = 0x03,
  /// Wrap key
  YH_WRAPKEY = 0x04,
  /// HMAC key
  YH_HMACKEY = 0x05,
  /// Template
  YH_TEMPLATE = 0x06,
  /// OTP AEAD key
  YH_OTP_AEAD_KEY = 0x07,
  /// Public key (virtual..)
  YH_PUBLIC = 0x83,
} yh_object_type;

/// Max number of algorithms defined here
#define YH_MAX_ALGORITHM_COUNT 0xff
/**
 * Algorithms
 */
typedef enum {
  YH_ALGO_RSA_PKCS1_SHA1 = 1,
  YH_ALGO_RSA_PKCS1_SHA256 = 2,
  YH_ALGO_RSA_PKCS1_SHA384 = 3,
  YH_ALGO_RSA_PKCS1_SHA512 = 4,
  YH_ALGO_RSA_PSS_SHA1 = 5,
  YH_ALGO_RSA_PSS_SHA256 = 6,
  YH_ALGO_RSA_PSS_SHA384 = 7,
  YH_ALGO_RSA_PSS_SHA512 = 8,
  YH_ALGO_RSA_2048 = 9,
  YH_ALGO_RSA_3072 = 10,
  YH_ALGO_RSA_4096 = 11,
  YH_ALGO_EC_P256 = 12,
  YH_ALGO_EC_P384 = 13,
  YH_ALGO_EC_P521 = 14,
  YH_ALGO_EC_K256 = 15,
  YH_ALGO_EC_BP256 = 16,
  YH_ALGO_EC_BP384 = 17,
  YH_ALGO_EC_BP512 = 18,
  YH_ALGO_HMAC_SHA1 = 19,
  YH_ALGO_HMAC_SHA256 = 20,
  YH_ALGO_HMAC_SHA384 = 21,
  YH_ALGO_HMAC_SHA512 = 22,
  YH_ALGO_EC_ECDSA_SHA1 = 23,
  YH_ALGO_EC_ECDH = 24,
  YH_ALGO_RSA_OAEP_SHA1 = 25,
  YH_ALGO_RSA_OAEP_SHA256 = 26,
  YH_ALGO_RSA_OAEP_SHA384 = 27,
  YH_ALGO_RSA_OAEP_SHA512 = 28,
  YH_ALGO_AES128_CCM_WRAP = 29,
  YH_ALGO_OPAQUE_DATA = 30,
  YH_ALGO_OPAQUE_X509_CERT = 31,
  YH_ALGO_MGF1_SHA1 = 32,
  YH_ALGO_MGF1_SHA256 = 33,
  YH_ALGO_MGF1_SHA384 = 34,
  YH_ALGO_MGF1_SHA512 = 35,
  YH_ALGO_TEMPL_SSH = 36,
  YH_ALGO_YUBICO_OTP_AES128 = 37,
  YH_ALGO_YUBICO_AES_AUTH = 38,
  YH_ALGO_YUBICO_OTP_AES192 = 39,
  YH_ALGO_YUBICO_OTP_AES256 = 40,
  YH_ALGO_AES192_CCM_WRAP = 41,
  YH_ALGO_AES256_CCM_WRAP = 42,
  YH_ALGO_EC_ECDSA_SHA256 = 43,
  YH_ALGO_EC_ECDSA_SHA384 = 44,
  YH_ALGO_EC_ECDSA_SHA512 = 45,
  YH_ALGO_EC_ED25519 = 46,
  YH_ALGO_EC_P224 = 47,
} yh_algorithm;

/**
 * Global options
 */
typedef enum {
  /// Forced audit mode
  YH_OPTION_FORCE_AUDIT = 1,
  /// Audit logging per command
  YH_OPTION_COMMAND_AUDIT = 3,
} yh_option;

/**
 * Options for the connector, set with yh_set_connector_option()
 */
typedef enum {
  /// File with CA certificate to validate the connector with (const char *) not
  /// implemented on windows
  YH_CONNECTOR_HTTPS_CA = 1,
  /// Proxy server to use for connecting to the connector (const char *) not
  /// implemented on windows
  YH_CONNECTOR_PROXY_SERVER = 2,
} yh_connector_option;

/// Size that the log digest is truncated to
#define YH_LOG_DIGEST_SIZE 16
#pragma pack(push, 1)
/**
 * Logging struct as returned by device
 */
typedef struct {
  /// Monotonically increasing index
  uint16_t number;
  /// What command was executed @see yh_cmd
  uint8_t command;
  /// Length of in-data
  uint16_t length;
  /// ID of authentication key used
  uint16_t session_key;
  /// ID of first object used
  uint16_t target_key;
  /// ID of second object used
  uint16_t second_key;
  /// Command result @see yh_cmd
  uint8_t result;
  /// Systick at time of execution
  uint32_t systick;
  /// Truncated sha256 digest of this last digest + this entry
  uint8_t digest[YH_LOG_DIGEST_SIZE];
} yh_log_entry;

/**
 * Object descriptor
 */
typedef struct {
  /// Object capabilities @see yh_capabilities
  yh_capabilities capabilities;
  /// Object ID
  uint16_t id;
  /// Object length
  uint16_t len;
  /// Object domains
  uint16_t domains;
  /// Object type
  yh_object_type type;
  /// Object algorithm
  yh_algorithm algorithm;
  /// Object sequence
  uint8_t sequence;
  /// Object origin
  uint8_t origin;
  /// Object label
  char label[YH_OBJ_LABEL_LEN + 1];
  /// Object delegated capabilities
  yh_capabilities delegated_capabilities;
} yh_object_descriptor;
#pragma pack(pop)

static const struct {
  const char *name;
  int bit;
} yh_capability[] = {
  {"asymmetric_decrypt_ecdh", 0x0b},
  {"asymmetric_decrypt_oaep", 0x0a},
  {"asymmetric_decrypt_pkcs", 0x09},
  {"asymmetric_gen", 0x04},
  {"asymmetric_sign_ecdsa", 0x07},
  {"asymmetric_sign_eddsa", 0x08},
  {"asymmetric_sign_pkcs", 0x05},
  {"asymmetric_sign_pss", 0x06},
  {"attest", 0x22},
  {"audit", 0x18},
  {"export_under_wrap", 0x10},
  {"export_wrapped", 0x0c},
  {"delete_asymmetric", 0x29},
  {"delete_authkey", 0x28},
  {"delete_hmackey", 0x2b},
  {"delete_opaque", 0x27},
  {"delete_otp_aead_key", 0x2d},
  {"delete_template", 0x2c},
  {"delete_wrapkey", 0x2a},
  {"generate_otp_aead_key", 0x24},
  {"generate_wrapkey", 0x0f},
  {"get_opaque", 0x00},
  {"get_option", 0x12},
  {"get_randomness", 0x13},
  {"get_template", 0x1a},
  {"hmackey_generate", 0x15},
  {"hmac_data", 0x16},
  {"hmac_verify", 0x17},
  {"import_wrapped", 0x0d},
  {"otp_aead_create", 0x1e},
  {"otp_aead_random", 0x1f},
  {"otp_aead_rewrap_from", 0x20},
  {"otp_aead_rewrap_to", 0x21},
  {"otp_decrypt", 0x1d},
  {"put_asymmetric", 0x03},
  {"put_authkey", 0x02},
  {"put_hmackey", 0x14},
  {"put_opaque", 0x01},
  {"put_option", 0x11},
  {"put_otp_aead_key", 0x23},
  {"put_template", 0x1b},
  {"put_wrapkey", 0x0e},
  {"reset", 0x1c},
  {"ssh_certify", 0x19},
  {"unwrap_data", 0x26},
  {"wrap_data", 0x25},
};

static const struct {
  const char *name;
  yh_algorithm algorithm;
} yh_algorithms[] = {
  {"aes128-ccm-wrap", YH_ALGO_AES128_CCM_WRAP},
  {"aes192-ccm-wrap", YH_ALGO_AES192_CCM_WRAP},
  {"aes256-ccm-wrap", YH_ALGO_AES256_CCM_WRAP},
  {"ecbp256", YH_ALGO_EC_BP256},
  {"ecbp384", YH_ALGO_EC_BP384},
  {"ecbp512", YH_ALGO_EC_BP512},
  {"ecdsa-sha1", YH_ALGO_EC_ECDSA_SHA1},
  {"ecdsa-sha256", YH_ALGO_EC_ECDSA_SHA256},
  {"ecdsa-sha384", YH_ALGO_EC_ECDSA_SHA384},
  {"ecdsa-sha512", YH_ALGO_EC_ECDSA_SHA512},
  {"ecdh", YH_ALGO_EC_ECDH},
  {"eck256", YH_ALGO_EC_K256},
  {"ecp224", YH_ALGO_EC_P224},
  {"ecp256", YH_ALGO_EC_P256},
  {"ecp384", YH_ALGO_EC_P384},
  {"ecp521", YH_ALGO_EC_P521},
  {"ed25519", YH_ALGO_EC_ED25519},
  {"hmac-sha1", YH_ALGO_HMAC_SHA1},
  {"hmac-sha256", YH_ALGO_HMAC_SHA256},
  {"hmac-sha384", YH_ALGO_HMAC_SHA384},
  {"hmac-sha512", YH_ALGO_HMAC_SHA512},
  {"mgf1-sha1", YH_ALGO_MGF1_SHA1},
  {"mgf1-sha256", YH_ALGO_MGF1_SHA256},
  {"mgf1-sha384", YH_ALGO_MGF1_SHA384},
  {"mgf1-sha512", YH_ALGO_MGF1_SHA512},
  {"opaque", YH_ALGO_OPAQUE_DATA},
  {"rsa2048", YH_ALGO_RSA_2048},
  {"rsa3072", YH_ALGO_RSA_3072},
  {"rsa4096", YH_ALGO_RSA_4096},
  {"rsa-pkcs1-sha1", YH_ALGO_RSA_PKCS1_SHA1},
  {"rsa-pkcs1-sha256", YH_ALGO_RSA_PKCS1_SHA256},
  {"rsa-pkcs1-sha384", YH_ALGO_RSA_PKCS1_SHA384},
  {"rsa-pkcs1-sha512", YH_ALGO_RSA_PKCS1_SHA512},
  {"rsa-pss-sha1", YH_ALGO_RSA_PSS_SHA1},
  {"rsa-pss-sha256", YH_ALGO_RSA_PSS_SHA256},
  {"rsa-pss-sha384", YH_ALGO_RSA_PSS_SHA384},
  {"rsa-pss-sha512", YH_ALGO_RSA_PSS_SHA512},
  {"rsa-oaep-sha1", YH_ALGO_RSA_OAEP_SHA1},
  {"rsa-oaep-sha256", YH_ALGO_RSA_OAEP_SHA256},
  {"rsa-oaep-sha384", YH_ALGO_RSA_OAEP_SHA384},
  {"rsa-oaep-sha512", YH_ALGO_RSA_OAEP_SHA512},
  {"template-ssh", YH_ALGO_TEMPL_SSH},
  {"x509-cert", YH_ALGO_OPAQUE_X509_CERT},
  {"yubico-aes-auth", YH_ALGO_YUBICO_AES_AUTH},
  {"yubico-otp-aes128", YH_ALGO_YUBICO_OTP_AES128},
  {"yubico-otp-aes192", YH_ALGO_YUBICO_OTP_AES192},
  {"yubico-otp-aes256", YH_ALGO_YUBICO_OTP_AES256},
};

static const struct {
  const char *name;
  yh_object_type type;
} yh_types[] = {
  {"authkey", YH_AUTHKEY},         {"asymmetric", YH_ASYMMETRIC},
  {"hmackey", YH_HMACKEY},         {"opaque", YH_OPAQUE},
  {"otpaeadkey", YH_OTP_AEAD_KEY}, {"template", YH_TEMPLATE},
  {"wrapkey", YH_WRAPKEY},
};

static const struct {
  const char *name;
  yh_option option;
} yh_options[] = {
  {"command_audit", YH_OPTION_COMMAND_AUDIT},
  {"force_audit", YH_OPTION_FORCE_AUDIT},
};

/// Origin is generated
#define YH_ORIGIN_GENERATED 0x01
/// Origin is imported
#define YH_ORIGIN_IMPORTED 0x02
/// Origin is wrapped (note: this is used in combination with objects original
/// origin)
#define YH_ORIGIN_IMPORTED_WRAPPED 0x10

/**
 * Return a string describing an error condition
 *
 * @param err yh_rc error code
 *
 * @return String with descriptive error
 **/
const char *yh_strerror(yh_rc err);

/**
 * Set verbosity
 * This function may be called prior to global library initialization.
 *
 * @param verbosity
 *
 * @return yh_rc error code
 **/
yh_rc yh_set_verbosity(uint8_t verbosity);

/**
 * Get verbosity
 *
 * @param verbosity
 *
 * @return yh_rc error code
 **/
yh_rc yh_get_verbosity(uint8_t *verbosity);

/**
 * Set file for debug output
 *
 * @param output
 *
 * @return void
 **/
void yh_set_debug_output(FILE *output);

/**
 * Global library initialization
 *
 * @return yh_rc error code
 **/
yh_rc yh_init(void);

/**
 * Global library cleanup
 *
 * @return yh_rc error code
 **/
yh_rc yh_exit(void);

/**
 * Instantiate a new connector
 *
 * @param url URL to associate with this connector
 * @param connector reference to connector
 *
 * @return yh_rc error code
 */
yh_rc yh_init_connector(const char *url, yh_connector **connector);

/**
 * Set connector options
 *
 * @param connector connector to set an option on
 * @param opt option to set @see yh_connector_option
 * @param val value to set, type is specific for the given option
 *
 * @return yh_rc error code
 **/
yh_rc yh_set_connector_option(yh_connector *connector, yh_connector_option opt,
                              const void *val);

/**
 * Connect to all specified connectors
 *
 * @param connectors pointer of connector array
 * @param n_connectors number of connectors in array (will be set to
 *successful connectors on return)
 * @param timeout timeout in seconds
 *
 * @return yh_rc error code
 **/
yh_rc yh_connect_all(yh_connector **connectors, size_t *n_connectors,
                     int timeout);

/**
 * Connect to one connector in array
 *
 * @param connectors pointer of connector array
 * @param n_connectors number of connectors in array
 * @param idx index of connected connector, may be NULL
 *
 * @return yh_rc error code
 **/
yh_rc yh_connect_best(yh_connector **connectors, size_t n_connectors, int *idx);

/**
 * Disconnect from connector
 *
 * @param connector connector to disconnect from
 *
 * @return yh_rc error code
 **/
yh_rc yh_disconnect(yh_connector *connector);

/**
 * Send a plain message to a connector
 *
 * @param connector connector to send to
 * @param cmd command to send
 * @param data data to send
 * @param data_len data length
 * @param response_cmd response command
 * @param response response data
 * @param response_len response length
 *
 * @return yh_rc error code
 **/
yh_rc yh_send_plain_msg(yh_connector *connector, yh_cmd cmd,
                        const uint8_t *data, size_t data_len,
                        yh_cmd *response_cmd, uint8_t *response,
                        size_t *response_len);

/**
 * Send an encrypted message over a session
 *
 * @param session session to send over
 * @param cmd command to send
 * @param data data to send
 * @param data_len data length
 * @param response_cmd response command
 * @param response response data
 * @param response_len response length
 *
 * @return yh_rc error code
 **/
yh_rc yh_send_secure_msg(yh_session *session, yh_cmd cmd, const uint8_t *data,
                         size_t data_len, yh_cmd *response_cmd,
                         uint8_t *response, size_t *response_len);

/**
 * Create a session with keys derived frm password
 *
 * @param connector connector to create the session with
 * @param auth_keyset_id ID of the authentication key to be used
 * @param password password to derive keys from
 * @param password_len length of the password in bytes
 * @param recreate_session session will be recreated if expired, this caches the
 *password in memory
 * @param context context data for the authentication
 * @param context_len context length
 * @param session created session
 *
 * @return yh_rc error code
 **/
yh_rc yh_create_session_derived(yh_connector *connector,
                                uint16_t auth_keyset_id,
                                const uint8_t *password, size_t password_len,
                                bool recreate_session, uint8_t *context,
                                size_t context_len, yh_session **session);

/**
 * Create a session
 *
 * @param connector connector to create the session with
 * @param auth_keyset_id ID of the authentication key
 * @param key_enc encryption key
 * @param key_enc_len length of encryption key
 * @param key_mac MAC key
 * @param key_mac_len length of MAC key
 * @param recreate_session session will be recreated if expired, this caches the
 *password in memory
 * @param context context data for the authentication
 * @param context_len context length
 * @param session created session
 *
 * @return yh_rc error code
 **/
yh_rc yh_create_session(yh_connector *connector, uint16_t auth_keyset_id,
                        const uint8_t *key_enc, size_t key_enc_len,
                        const uint8_t *key_mac, size_t key_mac_len,
                        bool recreate_session, uint8_t *context,
                        size_t context_len, yh_session **session);

/**
 * Begin create extenal session
 *
 * @param connector connector to create the session with
 * @param auth_keyset_id ID of the authentication key
 * @param context context data for the authentication
 * @param context_len length of context data
 * @param card_cryptogram card cryptogram
 * @param card_cryptogram_len catd cryptogram length
 * @param session created session
 *
 * @return yh_rc error code
 **/
yh_rc yh_begin_create_session_ext(yh_connector *connector,
                                  uint16_t auth_keyset_id, uint8_t *context,
                                  size_t context_len, uint8_t *card_cryptogram,
                                  size_t card_cryptogram_len,
                                  yh_session **session);

/**
 * Finish creating external session
 *
 * @param connector connector to create the session with
 * @param session session
 * @param key_senc session encryption key
 * @param key_senc_len session encrypt key length
 * @param key_smac session MAC key
 * @param key_smac_len session MAC key length
 * @param key_srmac session return MAC key
 * @param key_srmac_len session return MAC key length
 * @param context context data
 * @param context_len context length
 * @param card_cryptogram card cryptogram
 * @param card_cryptogram_len card cryptogram length
 *
 * @return yh_rc error code
 **/
yh_rc yh_finish_create_session_ext(yh_connector *connector, yh_session *session,
                                   const uint8_t *key_senc, size_t key_senc_len,
                                   const uint8_t *key_smac, size_t key_smac_len,
                                   const uint8_t *key_srmac,
                                   size_t key_srmac_len, uint8_t *context,
                                   size_t context_len, uint8_t *card_cryptogram,
                                   size_t card_cryptogram_len);

/**
 * Free data associated with session
 *
 * @param session session to destroy
 *
 * @return yh_rc error code
 **/
yh_rc yh_destroy_session(yh_session **session);

/**
 * Authenticate session
 *
 * @param session session to authenticate
 * @param context context data
 * @param context_len context length
 *
 * @return yh_rc error code
 **/
yh_rc yh_authenticate_session(yh_session *session, uint8_t *context,
                              size_t context_len);

// Utility and convenience functions below

/**
 * Get device info
 *
 * @param connector connector to send over
 * @param major version major
 * @param minor version minor
 * @param patch version path
 * @param serial serial number
 * @param log_total total number of log entries
 * @param log_used log entries used
 * @param algorithms algorithms array
 * @param n_algorithms number of algorithms
 *
 * @return yh_rc error code
 **/
yh_rc yh_util_get_device_info(yh_connector *connector, uint8_t *major,
                              uint8_t *minor, uint8_t *patch, uint32_t *serial,
                              uint8_t *log_total, uint8_t *log_used,
                              yh_algorithm *algorithms, size_t *n_algorithms);

/**
 * List objects
 *
 * @param session session to use
 * @param id ID to filter by (0 to not filter by ID)
 * @param type Type to filter by (0 to not filter by type) @see yh_object_type
 * @param domains Domains to filter by (0 to not filter by domain)
 * @param capabilities Capabilities to filter by (0 to not filter by
 *capabilities) @see yh_capabilities
 * @param algorithm Algorithm to filter by (0 to not filter by algorithm)
 * @param label Label to filter by
 * @param objects Array of objects returned
 * @param n_objects Max length of objects (will be set to number found on
 *return)
 *
 * @return yh_rc error code
 **/
yh_rc yh_util_list_objects(yh_session *session, uint16_t id,
                           yh_object_type type, uint16_t domains,
                           const yh_capabilities *capabilities,
                           yh_algorithm algorithm, const char *label,
                           yh_object_descriptor *objects, size_t *n_objects);

/**
 * Get object info
 *
 * @param session session to use
 * @param id Object ID
 * @param type Object type
 * @param object object information
 *
 * @return yh_rc error code
 **/
yh_rc yh_util_get_object_info(yh_session *session, uint16_t id,
                              yh_object_type type,
                              yh_object_descriptor *object);

/**
 * Get Public key
 *
 * @param session session to use
 * @param id Object ID
 * @param data Data out
 * @param datalen Data length
 * @param algorithm Algorithm of object
 *
 * @return yh_rc error code
 **/
yh_rc yh_util_get_pubkey(yh_session *session, uint16_t id, uint8_t *data,
                         size_t *datalen, yh_algorithm *algorithm);

/**
 * Close session
 *
 * @param session session to close
 *
 * @return yh_rc error code
 **/
yh_rc yh_util_close_session(yh_session *session);

/**
 * Sign data using PKCS1 v1.5
 *
 * <tt>in</tt> is either a raw hashed message (sha1, sha256, sha384 or sha512)
 *or that with correct digestinfo pre-pended.
 *
 * @param session session to use
 * @param key_id Object ID
 * @param hashed if data is only hashed
 * @param in in data to sign
 * @param in_len length of in
 * @param out signed data
 * @param out_len length of signed data
 *
 * @return yh_rc error code
 **/
yh_rc yh_util_sign_pkcs1v1_5(yh_session *session, uint16_t key_id, bool hashed,
                             const uint8_t *in, size_t in_len, uint8_t *out,
                             size_t *out_len);

/**
 * Sign data using PSS
 *
 * <tt>in</tt> is a raw hashed message (sha1, sha256, sha384 or sha512).
 *
 * @param session session to use
 * @param key_id Object ID
 * @param in data to sign
 * @param in_len length of in
 * @param out signed data
 * @param out_len length of signed data
 * @param salt_len length of salt
 * @param mgf1Algo algorithm for mgf1
 *
 * @return yh_rc error code
 **/
yh_rc yh_util_sign_pss(yh_session *session, uint16_t key_id, const uint8_t *in,
                       size_t in_len, uint8_t *out, size_t *out_len,
                       size_t salt_len, yh_algorithm mgf1Algo);

/**
 * Sign data using ECDSA
 *
 * <tt>in</tt> is a raw hashed message, a truncated hash to the curve length or
 *a padded hash to the curve length.
 *
 * @param session session to use
 * @param key_id Object ID
 * @param in data to sign
 * @param in_len length of in
 * @param out signed data
 * @param out_len length of signed data
 *
 * @return yh_rc error code
 **/
yh_rc yh_util_sign_ecdsa(yh_session *session, uint16_t key_id,
                         const uint8_t *in, size_t in_len, uint8_t *out,
                         size_t *out_len);

/**
 * Sign data using EDDSA
 *
 * @param session session to use
 * @param key_id Object ID
 * @param in data to sign
 * @param in_len length of in
 * @param out signed data
 * @param out_len length of signed data
 *
 * @return yh_rc error code
 **/
yh_rc yh_util_sign_eddsa(yh_session *session, uint16_t key_id,
                         const uint8_t *in, size_t in_len, uint8_t *out,
                         size_t *out_len);

/**
 * HMAC data
 *
 * @param session session to use
 * @param key_id Object ID
 * @param in data to hmac
 * @param in_len length of in
 * @param out HMAC
 * @param out_len length of HMAC
 *
 * @return yh_rc error code
 **/
yh_rc yh_util_hmac(yh_session *session, uint16_t key_id, const uint8_t *in,
                   size_t in_len, uint8_t *out, size_t *out_len);

/**
 * Get pseudo random data
 *
 * @param session session to use
 * @param len length of data to get
 * @param out random data out
 * @param out_len length of random data
 *
 * @return yh_rc error code
 **/
yh_rc yh_util_get_random(yh_session *session, size_t len, uint8_t *out,
                         size_t *out_len);

/**
 * Import RSA key
 *
 * @param session session to use
 * @param key_id Object ID
 * @param label Label
 * @param domains domains
 * @param capabilities capabilities
 * @param algorithm algorithm
 * @param p P
 * @param q Q
 *
 * @return yh_rc error code
 **/
yh_rc yh_util_import_key_rsa(yh_session *session, uint16_t *key_id,
                             const char *label, uint16_t domains,
                             const yh_capabilities *capabilities,
                             yh_algorithm algorithm, const uint8_t *p,
                             const uint8_t *q);

/**
 * Import EC key
 *
 * @param session session to use
 * @param key_id Object ID
 * @param label Label
 * @param domains domains
 * @param capabilities capabilities
 * @param algorithm algorithm
 * @param s S
 *
 * @return yh_rc error code
 **/
yh_rc yh_util_import_key_ec(yh_session *session, uint16_t *key_id,
                            const char *label, uint16_t domains,
                            const yh_capabilities *capabilities,
                            yh_algorithm algorithm, const uint8_t *s);

/**
 * Import ED key
 *
 * @param session session to use
 * @param key_id Object ID
 * @param label Label
 * @param domains domains
 * @param capabilities capabilities
 * @param algorithm algorithm
 * @param k k
 *
 * @return yh_rc error code
 **/
yh_rc yh_util_import_key_ed(yh_session *session, uint16_t *key_id,
                            const char *label, uint16_t domains,
                            const yh_capabilities *capabilities,
                            yh_algorithm algorithm, const uint8_t *k);

/**
 * Import HMAC key
 *
 * @param session session to use
 * @param key_id Object ID
 * @param label Label
 * @param domains domains
 * @param capabilities capabilities
 * @param algorithm algorithm
 * @param key key data
 * @param key_len length of key
 *
 * @return yh_rc error code
 **/
yh_rc yh_util_import_key_hmac(yh_session *session, uint16_t *key_id,
                              const char *label, uint16_t domains,
                              const yh_capabilities *capabilities,
                              yh_algorithm algorithm, const uint8_t *key,
                              size_t key_len);

/**
 * Generate RSA key
 *
 * @param session session to use
 * @param key_id Object ID
 * @param label Label
 * @param domains domains
 * @param capabilities capabilities
 * @param algorithm algorithm
 *
 * @return yh_rc error code
 **/
yh_rc yh_util_generate_key_rsa(yh_session *session, uint16_t *key_id,
                               const char *label, uint16_t domains,
                               const yh_capabilities *capabilities,
                               yh_algorithm algorithm);

/**
 * Generate EC key
 *
 * @param session session to use
 * @param key_id Object ID
 * @param label Label
 * @param domains domains
 * @param capabilities capabilities
 * @param algorithm algorithm
 *
 * @return yh_rc error code
 **/
yh_rc yh_util_generate_key_ec(yh_session *session, uint16_t *key_id,
                              const char *label, uint16_t domains,
                              const yh_capabilities *capabilities,
                              yh_algorithm algorithm);

/**
 * Generate ED key
 *
 * @param session session to use
 * @param key_id Object ID
 * @param label Label
 * @param domains domains
 * @param capabilities capabilities
 * @param algorithm algorithm
 *
 * @return yh_rc error code
 **/
yh_rc yh_util_generate_key_ed(yh_session *session, uint16_t *key_id,
                              const char *label, uint16_t domains,
                              const yh_capabilities *capabilities,
                              yh_algorithm algorithm);

/**
 * Verify HMAC data
 *
 * @param session session to use
 * @param key_id Object ID
 * @param signature HMAC
 * @param signature_len HMAC length
 * @param data data to verify
 * @param data_len data length
 * @param verified if verification succeeded
 *
 * @return yh_rc error code
 **/
yh_rc yh_util_hmac_verify(yh_session *session, uint16_t key_id,
                          const uint8_t *signature, size_t signature_len,
                          const uint8_t *data, size_t data_len, bool *verified);

/**
 * Generate HMAC key
 *
 * @param session session to use
 * @param key_id Object ID
 * @param label Label
 * @param domains domains
 * @param capabilities capabilities
 * @param algorithm algorithm
 *
 * @return yh_rc error code
 **/
yh_rc yh_util_generate_key_hmac(yh_session *session, uint16_t *key_id,
                                const char *label, uint16_t domains,
                                const yh_capabilities *capabilities,
                                yh_algorithm algorithm);

/**
 * Decrypt PKCS1 v1.5 data
 *
 * @param session session to use
 * @param key_id Object ID
 * @param in Encrypted data
 * @param in_len length of encrypted data
 * @param out Decrypted data
 * @param out_len length of decrypted data
 *
 * @return yh_rc error code
 **/
yh_rc yh_util_decrypt_pkcs1v1_5(yh_session *session, uint16_t key_id,
                                const uint8_t *in, size_t in_len, uint8_t *out,
                                size_t *out_len);

/**
 * Decrypt OAEP data
 *
 * @param session session to use
 * @param key_id Object ID
 * @param in Encrypted data
 * @param in_len length of encrypted data
 * @param out Decrypted data
 * @param out_len length of decrypted data
 * @param label OAEP label
 * @param label_len label length
 * @param mgf1Algo MGF1 algorithm
 *
 * @return yh_rc error code
 **/
yh_rc yh_util_decrypt_oaep(yh_session *session, uint16_t key_id,
                           const uint8_t *in, size_t in_len, uint8_t *out,
                           size_t *out_len, const uint8_t *label,
                           size_t label_len, yh_algorithm mgf1Algo);

/**
 * Perform ECDH key exchange
 *
 * @param session session to use
 * @param key_id Object ID
 * @param in public key
 * @param in_len length of public key
 * @param out Agreed key
 * @param out_len length of agreed key
 *
 * @return yh_rc error code
 **/
yh_rc yh_util_decrypt_ecdh(yh_session *session, uint16_t key_id,
                           const uint8_t *in, size_t in_len, uint8_t *out,
                           size_t *out_len);

/**
 * Delete an object
 *
 * @param session session to use
 * @param id Object ID
 * @param type Object type
 *
 * @return yh_rc error code
 **/
yh_rc yh_util_delete_object(yh_session *session, uint16_t id,
                            yh_object_type type);

/**
 * Export an object under wrap
 *
 * @param session session to use
 * @param wrapping_key_id ID of wrapping key
 * @param target_type Type of object
 * @param target_id ID of object
 * @param out wrapped data
 * @param out_len length of wrapped data
 *
 * @return yh_rc error code
 **/
yh_rc yh_util_export_wrapped(yh_session *session, uint16_t wrapping_key_id,
                             yh_object_type target_type, uint16_t target_id,
                             uint8_t *out, size_t *out_len);

/**
 * Import a wrapped object
 *
 * @param session session to use
 * @param wrapping_key_id ID of wrapping key
 * @param in wrapped data
 * @param in_len length of wrapped data
 * @param target_type what type the imported object has
 * @param target_id ID of imported object
 *
 * @return yh_rc error code
 **/
yh_rc yh_util_import_wrapped(yh_session *session, uint16_t wrapping_key_id,
                             const uint8_t *in, size_t in_len,
                             yh_object_type *target_type, uint16_t *target_id);

/**
 * Import a wrap key
 *
 * @param session session to use
 * @param key_id Object ID
 * @param label label
 * @param domains domains
 * @param capabilities capabilities
 * @param algorithm algorithm
 * @param delegated_capabilities delegated capabilities
 * @param in key
 * @param in_len key length
 *
 * @return yh_rc error code
 **/
yh_rc yh_util_import_key_wrap(yh_session *session, uint16_t *key_id,
                              const char *label, uint16_t domains,
                              const yh_capabilities *capabilities,
                              yh_algorithm algorithm,
                              const yh_capabilities *delegated_capabilities,
                              const uint8_t *in, size_t in_len);

/**
 * Generate a wrap key
 *
 * @param session session to use
 * @param key_id Object ID
 * @param label label
 * @param domains domains
 * @param capabilities capabilities
 * @param algorithm algorithm
 * @param delegated_capabilities delegated capabilitites
 *
 * @return yh_rc error code
 **/
yh_rc yh_util_generate_key_wrap(yh_session *session, uint16_t *key_id,
                                const char *label, uint16_t domains,
                                const yh_capabilities *capabilities,
                                yh_algorithm algorithm,
                                const yh_capabilities *delegated_capabilities);

/**
 * Get logs
 *
 * @param session session to use
 * @param unlogged_boot number of unlogged boots
 * @param unlogged_auth number of unlogged authentications
 * @param out array of log entries
 * @param n_items number of items in out
 *
 * @return yh_rc error code
 **/
yh_rc yh_util_get_logs(yh_session *session, uint16_t *unlogged_boot,
                       uint16_t *unlogged_auth, yh_log_entry *out,
                       size_t *n_items);

/**
 * Set the log index
 *
 * @param session session to use
 * @param index index to set
 *
 * @return yh_rc error code
 **/
yh_rc yh_util_set_log_index(yh_session *session, uint16_t index);

/**
 * Get opaque object
 *
 * @param session session to use
 * @param object_id Object ID
 * @param out data
 * @param out_len length of out
 *
 * @return yh_rc error code
 **/
yh_rc yh_util_get_opaque(yh_session *session, uint16_t object_id, uint8_t *out,
                         size_t *out_len);

/**
 * Import opaque object
 *
 * @param session session to use
 * @param object_id Object ID
 * @param label label
 * @param domains domains
 * @param capabilities
 * @param algorithm algorithm
 * @param in object data
 * @param in_len length of in
 *
 * @return
 **/
yh_rc yh_util_import_opaque(yh_session *session, uint16_t *object_id,
                            const char *label, uint16_t domains,
                            const yh_capabilities *capabilities,
                            yh_algorithm algorithm, const uint8_t *in,
                            size_t in_len);

/**
 * SSH certify
 *
 * @param session session to use
 * @param key_id Key ID
 * @param template_id Template ID
 * @param sig_algo signature algorithm
 * @param in Certificate request
 * @param in_len length of in
 * @param out Signature
 * @param out_len length of out
 *
 * @return yh_rc error code
 **/
yh_rc yh_util_ssh_certify(yh_session *session, uint16_t key_id,
                          uint16_t template_id, yh_algorithm sig_algo,
                          const uint8_t *in, size_t in_len, uint8_t *out,
                          size_t *out_len);

/**
 * Import authentication key
 *
 * @param session session to use
 * @param key_id Object ID
 * @param label label
 * @param domains domains
 * @param capabilities capabilities
 * @param delegated_capabilities delegated capabilities
 * @param password password to derive key from
 * @param password_len password length in bytes
 *
 * @return yh_rc error code
 **/
yh_rc yh_util_import_authkey(yh_session *session, uint16_t *key_id,
                             const char *label, uint16_t domains,
                             const yh_capabilities *capabilities,
                             const yh_capabilities *delegated_capabilities,
                             const uint8_t *password, size_t password_len);

/**
 * Get template
 *
 * @param session session to use
 * @param object_id Object ID
 * @param out data
 * @param out_len length of out
 *
 * @return yh_rc error code
 **/
yh_rc yh_util_get_template(yh_session *session, uint16_t object_id,
                           uint8_t *out, size_t *out_len);

/**
 * Import template
 *
 * @param session session to use
 * @param object_id Object ID
 * @param label label
 * @param domains domains
 * @param capabilities capabilities
 * @param algorithm algorithm
 * @param in data
 * @param in_len length of in
 *
 * @return yh_rc error code
 **/
yh_rc yh_util_import_template(yh_session *session, uint16_t *object_id,
                              const char *label, uint16_t domains,
                              const yh_capabilities *capabilities,
                              yh_algorithm algorithm, const uint8_t *in,
                              size_t in_len);

/**
 * Create OTP AEAD
 *
 * @param session session to use
 * @param key_id Object ID
 * @param key OTP key
 * @param private_id OTP private id
 * @param out AEAD
 * @param out_len length of out
 *
 * @return yh_rc error code
 **/
yh_rc yh_util_otp_aead_create(yh_session *session, uint16_t key_id,
                              const uint8_t *key, const uint8_t *private_id,
                              uint8_t *out, size_t *out_len);

/**
 * Create OTP AEAD from random
 *
 * @param session session to use
 * @param key_id Object ID
 * @param out AEAD
 * @param out_len length of out
 *
 * @return yh_rc error code
 **/
yh_rc yh_util_otp_aead_random(yh_session *session, uint16_t key_id,
                              uint8_t *out, size_t *out_len);

/**
 * Decrypt OTP
 *
 * @param session session to use
 * @param key_id Object ID
 * @param aead AEAD
 * @param aead_len length of AEAD
 * @param otp OTP
 * @param useCtr OTP use counter
 * @param sessionCtr OTP session counter
 * @param tstph OTP timestamp high
 * @param tstpl OTP timestamp low
 *
 * @return yh_rc error code
 **/
yh_rc yh_util_otp_decrypt(yh_session *session, uint16_t key_id,
                          const uint8_t *aead, size_t aead_len,
                          const uint8_t *otp, uint16_t *useCtr,
                          uint8_t *sessionCtr, uint8_t *tstph, uint16_t *tstpl);

/**
 * Import OTP AEAD Key
 *
 * @param session session to use
 * @param key_id Object ID
 * @param label label
 * @param domains domains
 * @param capabilities capabilities
 * @param nonce_id nonce ID
 * @param in key
 * @param in_len length of in
 *
 * @return
 **/
yh_rc yh_util_put_otp_aead_key(yh_session *session, uint16_t *key_id,
                               const char *label, uint16_t domains,
                               const yh_capabilities *capabilities,
                               uint32_t nonce_id, const uint8_t *in,
                               size_t in_len);

/**
 * Generate OTP AEAD Key
 *
 * @param session session to use
 * @param key_id Object ID
 * @param label label
 * @param domains domains
 * @param capabilities capabilities
 * @param algorithm algorithm
 * @param nonce_id nonce ID
 *
 * @return yh_rc error code
 **/
yh_rc yh_util_generate_otp_aead_key(yh_session *session, uint16_t *key_id,
                                    const char *label, uint16_t domains,
                                    const yh_capabilities *capabilities,
                                    yh_algorithm algorithm, uint32_t nonce_id);

/**
 * Attest asymmetric key
 *
 * @param session session to use
 * @param key_id Object ID
 * @param attest_id Attestation key ID
 * @param out Certificate
 * @param out_len length of out
 *
 * @return yh_rc error code
 **/
yh_rc yh_util_attest_asymmetric(yh_session *session, uint16_t key_id,
                                uint16_t attest_id, uint8_t *out,
                                size_t *out_len);

/**
 * Put global option
 *
 * @param session session to use
 * @param option option
 * @param len length of option data
 * @param val option data
 *
 * @return yh_rc error code
 **/
yh_rc yh_util_put_option(yh_session *session, yh_option option, size_t len,
                         uint8_t *val);

/**
 * Get global option
 *
 * @param session session to use
 * @param option option
 * @param out option data
 * @param out_len length of out
 *
 * @return yh_rc error code
 **/
yh_rc yh_util_get_option(yh_session *session, yh_option option, uint8_t *out,
                         size_t *out_len);

/**
 * Get storage statistics
 *
 * @param session session to use
 * @param total_records total records available
 * @param free_records number of free records
 * @param total_pages total pages available
 * @param free_pages number of free pages
 * @param page_size page size in bytes
 *
 * @return yh_rc error code
 **/
yh_rc yh_util_get_storage_stats(yh_session *session, uint16_t *total_records,
                                uint16_t *free_records, uint16_t *total_pages,
                                uint16_t *free_pages, uint16_t *page_size);

/**
 * Wrap data
 *
 * @param session session to use
 * @param key_id Object ID
 * @param in data to wrap
 * @param in_len length of in
 * @param out wrapped data
 * @param out_len length of out
 *
 * @return yh_rc error code
 **/
yh_rc yh_util_wrap_data(yh_session *session, uint16_t key_id, const uint8_t *in,
                        size_t in_len, uint8_t *out, size_t *out_len);

/**
 * Unwrap data
 *
 * @param session session to use
 * @param key_id Object ID
 * @param in wrapped data
 * @param in_len length of in
 * @param out unwrapped data
 * @param out_len length of out
 *
 * @return yh_rc error code
 **/
yh_rc yh_util_unwrap_data(yh_session *session, uint16_t key_id,
                          const uint8_t *in, size_t in_len, uint8_t *out,
                          size_t *out_len);

/**
 * Blink the device
 *
 * @param session session to use
 * @param seconds seconds to blink
 *
 * @return yh_rc error code
 **/
yh_rc yh_util_blink(yh_session *session, uint8_t seconds);

/**
 * Reset the device
 *
 * @param session session to use
 *
 * @return yh_rc error code. This function will normally return a network error
 **/
yh_rc yh_util_reset(yh_session *session);

/**
 * Get session ID
 *
 * @param session session to use
 * @param sid session ID
 *
 * @return yh_rc error code
 **/
yh_rc yh_get_session_id(yh_session *session, uint8_t *sid);

/**
 * Check if the connector has a device connected
 *
 * @param connector connector
 *
 * @return true or false
 **/
bool yh_connector_has_device(yh_connector *connector);

/**
 * Get the connector version
 *
 * @param connector connector
 * @param major major version
 * @param minor minor version
 * @param patch patch version
 *
 * @return yh_rc error code
 **/
yh_rc yh_get_connector_version(yh_connector *connector, uint8_t *major,
                               uint8_t *minor, uint8_t *patch);

/**
 * Get connector address
 *
 * @param connector connector
 * @param address pointer to string address
 *
 * @return yh_rc error code
 **/
yh_rc yh_get_connector_address(yh_connector *connector, char **const address);

/**
 * Convert capability string to byte array
 *
 * @param capability string of capabilities
 * @param result capabilities
 *
 * @return yh_rc error code
 **/
yh_rc yh_capabilities_to_num(const char *capability, yh_capabilities *result);

/**
 * Convert capability byte array to strings
 *
 * @param num capabilities
 * @param result array of string pointers
 * @param n_result number of elements of result
 *
 * @return yh_rc error code
 **/
yh_rc yh_num_to_capabilities(const yh_capabilities *num, const char *result[],
                             size_t *n_result);

/**
 * Check if capability is set
 *
 * @param capabilities capabilities
 * @param capability capability string
 *
 * @return true or false
 **/
bool yh_check_capability(const yh_capabilities *capabilities,
                         const char *capability);

/**
 * Merge two sets of capabilities
 *
 * @param a a set of capabilities
 * @param b a set of capabilities
 * @param result resulting set of capabilities
 *
 * @return yh_rc error code
 **/
yh_rc yh_merge_capabilities(const yh_capabilities *a, const yh_capabilities *b,
                            yh_capabilities *result);

/**
 * Filter one set of capabilities with another
 *
 * @param capabilities set of capabilities
 * @param filter capabilities to filter with
 * @param result resulting set of capabilities
 *
 * @return yh_rc error code
 **/
yh_rc yh_filter_capabilities(const yh_capabilities *capabilities,
                             const yh_capabilities *filter,
                             yh_capabilities *result);

/**
 * Check if algorithm is an RSA algorithm
 *
 * @param algorithm algorithm
 *
 * @return true or false
 **/
bool yh_is_rsa(yh_algorithm algorithm);

/**
 * Check if algorithm is an EC algorithm
 *
 * @param algorithm algorithm
 *
 * @return true or false
 **/
bool yh_is_ec(yh_algorithm algorithm);

/**
 * Check if algorithm is an ED algorithm
 *
 * @param algorithm algorithm
 *
 * @return true or false
 **/
bool yh_is_ed(yh_algorithm algorithm);

/**
 * Check if algorithm is a HMAC algorithm
 *
 * @param algorithm algorithm
 *
 * @return true or false
 **/
bool yh_is_hmac(yh_algorithm algorithm);

/**
 * Get algorithm bitlength
 *
 * @param algorithm algorithm
 * @param result bitlength
 *
 * @return yh_rc error code
 **/
yh_rc yh_get_key_bitlength(yh_algorithm algorithm, size_t *result);

/**
 * Convert algorithm to string
 *
 * @param algo algorithm
 * @param result string
 *
 * @return yh_rc error code
 **/
yh_rc yh_algo_to_string(yh_algorithm algo, char const **result);

/**
 * Convert string to algorithm
 *
 * @param string algorithm as string
 * @param algo algorithm
 *
 * @return yh_rc error code
 **/
yh_rc yh_string_to_algo(const char *string, yh_algorithm *algo);

/**
 * Convert type to string
 *
 * @param type type
 * @param result string
 *
 * @return yh_rc error code
 **/
yh_rc yh_type_to_string(yh_object_type type, char const **result);

/**
 * Convert string to type
 *
 * @param string type as string
 * @param type type
 *
 * @return yh_rc error code
 **/
yh_rc yh_string_to_type(const char *string, yh_object_type *type);

/**
 * Convert string to option
 *
 * @param string option as string
 * @param option option
 *
 * @return yh_rc error code
 **/
yh_rc yh_string_to_option(const char *string, yh_option *option);

/**
 * Verify an array of log entries
 *
 * @param logs pointer to an array of log entries
 * @param n_items number of items logs
 * @param last_previous_log optional pointer to the entry before the first entry
 *in logs
 *
 * @return true or false
 **/
bool yh_verify_logs(yh_log_entry *logs, size_t n_items,
                    yh_log_entry *last_previous_log);

/**
 * Parse a string to a domains parameter.
 *
 * @param domains string of the format 1,2,3
 * @param result resulting parsed domain parameter
 *
 * @return yh_rc error code
 **/
yh_rc yh_parse_domains(const char *domains, uint16_t *result);

/**
 * Write out domains to a string.
 *
 * @param domains encoded domains
 * @param string string to hold the result
 * @param max_len maximum length of string
 *
 * @return yh_rc error code
 **/
yh_rc yh_domains_to_string(uint16_t domains, char *string, size_t max_len);
#ifdef __cplusplus
}
#endif

#endif
