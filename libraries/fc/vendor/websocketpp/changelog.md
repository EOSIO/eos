HEAD

0.7.0 - 2016-02-22
- MINOR BREAKING SOCKET POLICY CHANGE: Asio transport socket policy method 
  `cancel_socket` will now return `lib::asio::error_code` instead of `void`.
  Custom Asio transport socket policies will need to be updated accordingly.
  This does not affect anyone using the bundled socket policies.
- Feature: Basic support for the permessage-deflate extension. #344
- Feature: Allow accessing the local endpoint when using the Asio transport.
  This allows inspection of the address and port in cases where they are chosen
  by the operating system rather than the user. Thank you Andreas Weis and 
  Muzahid Hussain for reporting and related code. #458
- Feature: Add support for subprotocols in Hybi00. Thank you Lukas Obermann
  for reporting and a patch. #518
- Improvement: Better automatic std::chrono feature detection for Visual Studio
- Improvement: Major refactoring to bundled CMake build system. CMake can now be
  used to build all of the examples and the test suite. Thank you Thijs Wenker
  for a significant portion of this code. #378, #435, #449
- Improvement: In build environments where `lib::error_code` and 
  `lib::asio::error_code` match (such as using `boost::asio` with 
  `boost::system_error` or standalone asio with `std::system_error`, transport
  errors are passed through natively rather than being reported as a translated 
  `pass_through` error type.
- Improvement: Add a `get_transport_error` method to Asio transport connections
  to allow retrieving a machine readable native transport error.
- Improvement: Add `connection::get_response`, `connection::get_response_code`,
  and `connection::get_response_msg` methods to allow accessing additional
  information about the HTTP responses that WebSocket++ sends. #465 Thank you
  Flow86 for reporting.
- Improvement: Removes use of empty strings ("") in favor of `string::clear()`
  and `string::empty()`. This avoids generating unnecessary temporary objects.
  #468 Thank you Vladislav Yaroslavlev for reporting and a patch.
- Documentation: Adds an example demonstrating the use of external `io_service`
- Documentation: Adds a simple echo_client example.
- Documentation: Begins migration of the web based user manual into Doxygen.
- Bug: Fix memory leak when init_asio produces an error. #454 Thank you Mark 
  Grimes for reporting and fixing.
- Bug: Fix crash when processing a specially crafted HTTP header. Thank you Eli 
  Fidler for reporting, test cases, and a patch. #456
- Bug: Fix an issue where standalone Asio builds that use TLS would not compile
  due to lingering boost code. #448 Thank you mjsp for reporting
- Bug: Fix an issue where canceling a socket could throw an exception on some
  older Windows XP platforms. It now prints an appropriate set of log messages
  instead. Thank you Thijs Wenker for reporting and researching solutions. #460
- Bug: Fix an issue where deferred HTTP connections that start sending a very 
  long response before their HTTP handler ends would result in a second set of
  HTTP headers being injected into the output. Thank you Kevin Smith for
  reporting and providing test case details. #443
- Bug: Fix an issue where the wrong type of strand was being created. Thank you 
  Bastien Brunnenstein for reporting and a patch. #462
- Bug: Fix an issue where TLS includes were broken for Asio Standalone builds.
  Thank you giachi and Bastien Brunnenstein for reporting. #491
- Bug: Remove the use of cached read and write handlers in the Asio transport.
  This feature caused memory leaks when the io_service the connection was
  running on was abruptly stopped. There isn't a clean and safe way of using
  this optimization without global state and the associated locks. The locks
  perform worse. Thank you Xavier Gibert for reporting, test cases, and code.
  Fixes #490.
- Bug: Fix a heap buffer overflow when checking very short URIs. Thank you 
  Xavier Gibert for reporting and a patch #524
- Compatibility: Fixes a number of build & config issues on Visual Studio 2015
- Compatibility: Removes non-standards compliant masking behavior. #395, #469
- Compatibility: Replace deprecated use of auto_ptr on systems where unique_ptr
  is available.

0.6.0 - 2015-06-02
- MINOR BREAKING TRANSPORT POLICY CHANGE: Custom transport policies will now be
  required to include a new method `void set_uri(uri_ptr u)`. An implementation
  is not required. The stub transport policy includes an example stub method
  that can be added to any existing custom transport policy to fulfill this
  requirement. This does not affect anyone using the bundled transports or
  configs.
- MINOR BREAKING SOCKET POLICY CHANGE: Custom asio transport socket policies 
  will now be required to include a new method `void set_uri(uri_ptr u)`. Like
  with the transport layer, an implementation is not required. This does not 
  affect anyone using the bundled socket policies.
- MINOR BREAKING DEPENDENCY CHANGE: When using Boost versions greater than or 
  equal to 1.49 in C++03 mode, `libboost-chrono` is needed now instead of 
  `libboost-date_time`. Users with C++11 compilers or using Boost versions 1.48
  and earlier are not affected. Note: This change affects the bundled unit test
  suite.
- Feature: WebSocket++ Asio transport policy can now be used with the standalone
  version of Asio (1.8.0+) when a C++11 compiler and standard library are 
  present. This means that it is possible now to use WebSocket++'s Asio
  transport entirely without Boost. Thank you Robert Seiler for proof of concept
  code that was used as a guide for this implementation. Fixes #324 
- Feature: Adds a vectored/scatter-gather write handler to the iostream
  transport.
- Feature: Adds the ability to defer sending an HTTP response until sometime
  after the `http_handler` is run. This allows processing of long running http
  handlers to defer their response until it is ready without blocking the
  network thread. references #425
- Improvement: `echo_server_tls` has been update to demonstrate how to configure
  it for Mozilla's recommended intermediate and modern TLS security profiles.
- Improvement: `endpoint::set_timer` now uses a steady clock provided by 
  `boost::chrono` or `std::chrono` where available instead of the non-monotonic
  system clock. Thank you breyed for reporting. fixes #241
- Improvement: Outgoing TLS connections to servers using the SNI extension to
  choose a certificate will now work. Thank you moozzyk for reporting. 
  Fixes #400
- Improvement: Removes an unnecessary mutex lock in `get_con_from_hdl`.
- Cleanup: Asio transport policy has been refactored to remove many Boost
  dependencies. On C++03 compilers the `boost::noncopyable` dependency has been
  removed and the `boost::date_time` dependency has been replaced with the newer
  `boost::chrono` when possible. On C++11 compilers the `boost::aligned_storage`
  and `boost::date_time` dependencies are gone, replaced with equivalent C++11
  standard library features.
- Bug: Fixes a potential dangling pointer and inconsistent error message
  handling in `websocketpp::exception`. #432 Thank you Tom Swirly for the fix.

0.5.1 - 2015-02-27
- Bug: Fixes an issue where some frame data was counted against the max header
  size limit, resulting in connections that included a lot of frame data
  immediately after the opening handshake to fail.
- Bug: Fix a typo in the name of the set method for `max_http_body_size`. #406
  Thank you jplatte for reporting.

0.5.0 - 2015-01-22
- BREAKING UTILITY CHANGE: Deprecated methods `http::parser::parse_headers`,
  `http::response::parse_complete`, and `http::request::parse_complete` have
  been removed.
- Security: Disabled SSLv3 in example servers.
- Feature: Adds basic support for accessing HTTP request bodies in the http
  handler. #181
- Feature: Adds the ability to register a shutdown handler when using the
  iostream transport. This provides a clean interface for triggering the shut
  down of external sockets and other cleanup without hooking in to higher level
  WebSocket handlers.
- Feature: Adds the ability to register a write handler when using the iostream
  transport. This handler can be used to handle transport output in place of
  registering an ostream to write to.
- Feature: Adds a new logging policy that outputs to syslog. #386 Thank you Tom
  Hughes for submitting the initial version of this policy.
- Improvement: Message payload logging now prints text for text messages rather
  than binary.
- Improvement: Overhaul of handshake state machine. Should make it impossible
  for exceptions to bubble out of transport methods like `io_service::run`.
- Improvement: Overhaul of handshake error reporting. Fail handler error codes
  will be more detailed and precise. Adds new [fail] and [http] logging channels
  that log failed websocket connections and successful HTTP connections
  respectively. A new aggregate channel package, `alevel::access_core`, allows
  enabling connect, disconnect, fail, and http together. Successful HTTP
  connections will no longer trigger a fail handler.
- Improvement: Ability to terminate connection during an http handler to cleanly
  suppress the default outgoing HTTP response.
- Documentation: Add Sending & Receiving Messages step to chapter one of the
  `utility_client` tutorial. Update `utility_client` example to match.
- Cleanup: Removes unused files & STL includes. Adds required STL includes.
  Normalizes include order.
- Bug: Fixes a fatal state error when a handshake response is completed
  immediately after that handshake times out. #389
- Bug: MinGW fixes; C++11 feature detection, localtime use. #393 Thank you
  Schebb for reporting, code, and testing.
- Bug: Fixes an issue where `websocketpp::exception::what()` could return an out
  of scope pointer. #397 Thank you fabioang for reporting.
- Bug: Fixes an issue where endpoints were not reset properly after a call to
  `endpoint::listen` failed. #390 Thank you wyyqyl for reporting.

0.4.0 - 2014-11-04
- BREAKING API CHANGE: All WebSocket++ methods now throw an exception of type
  `websocketpp::exception` which derives from `std::exception`. This normalizes
  all exception types under the standard exception hierarchy and allows
  WebSocket++ exceptions to be caught in the same statement as others. The error
  code that was previously thrown is wrapped in the exception object and can be
  accessed via the `websocketpp::exception::code()` method.
- BREAKING API CHANGE: Custom logging policies have some new required
  constructors that take generic config settings rather than pointers to
  std::ostreams. This allows writing logging policies that do not involve the
  use of std::ostream. This does not affect anyone using the built in logging
  policies.
- BREAKING UTILITY CHANGE: `websocketpp::lib::net::htonll` and
  `websocketpp::lib::net::ntohll` have been prefixed with an underscore to avoid
  conflicts with similarly named macros in some operating systems. If you are
  using the WebSocket++ provided 64 bit host/network byte order functions you
  will need to switch to the prefixed versions.
- BREAKING UTILITY CHANGE: The signature of `base64_encode` has changed from
  `websocketpp::base64_encode(unsigned char const *, unsigned int)` to
  `websocketpp::base64_encode(unsigned char const *, size_t)`.
- BREAKING UTILITY CHANGE: The signature of `sha1::calc` has changed from
  `websocketpp::sha1::calc(void const *, int, unsigned char *)` to
  `websocketpp::sha1::calc(void const *, size_t, unsigned char *)`
- Feature: Adds incomplete `minimal_server` and `minimal_client` configs that
  can be used to build custom configs without pulling in the dependencies of
  `core` or `core_client`. These configs will offer a stable base config to
  future-proof custom configs.
- Improvement: Core library no longer has std::iostream as a dependency.
  std::iostream is still required for the optional iostream logging policy and
  iostream transport.
- Bug: C++11 Chrono support was being incorrectly detected by the `boost_config`
  header. Thank you Max Dmitrichenko for reporting and a patch.
- Bug: use of `std::put_time` is now guarded by a unique flag rather than a
  chrono library flag. Thank you Max Dmitrichenko for reporting.
- Bug: Fixes non-thread safe use of std::localtime. #347 #383
- Compatibility: Adjust usage of std::min to be more compatible with systems
  that define a min(...) macro.
- Compatibility: Removes unused parameters from all library, test, and example
  code. This assists with those developing with -Werror and -Wunused-parameter
  #376
- Compatibility: Renames ntohll and htonll methods to avoid conflicts with
  platform specific macros. #358 #381, #382 Thank you logotype, unphased,
  svendjo
- Cleanup: Removes unused functions, fixes variable shadow warnings, normalizes
  all whitespace in library, examples, and tests to 4 spaces. #376

0.3.0 - 2014-08-10
- Feature: Adds `start_perpetual` and `stop_perpetual` methods to asio transport
  These may be used to replace manually managed `asio::io_service::work` objects
- Feature: Allow setting pong and handshake timeouts at runtime.
- Feature: Allows changing the listen backlog queue length.
- Feature: Split tcp init into pre and post init.
- Feature: Adds URI method to extract query string from URI. Thank you Banaan
  for code. #298
- Feature: Adds a compile time switch to asio transport config to disable
  certain multithreading features (some locks, asio strands)
- Feature: Adds the ability to pause reading on a connection. Paused connections
  will not read more data from their socket, allowing TCP flow control to work
  without blocking the main thread.
- Feature: Adds the ability to specify whether or not to use the `SO_REUSEADDR`
  TCP socket option. The default for this value has been changed from `true` to
  `false`.
- Feature: Adds the ability to specify a maximum message size.
- Feature: Adds `close::status::get_string(...)` method to look up a human
  readable string given a close code value.
- Feature: Adds `connection::read_all(...)` method to iostream transport as a
  convenience method for reading all data into the connection buffer without the
  end user needing to manually loop on `read_some`.
- Improvement: Open, close, and pong timeouts can be disabled entirely by
  setting their duration to 0.
- Improvement: Numerous performance improvements. Including: tuned default
  buffer sizes based on profiling, caching of handler binding for async
  reads/writes, non-malloc allocators for read/write handlers, disabling of a
  number of questionably useful range sanity checks in tight inner loops.
- Improvement: Cleaned up the handling of TLS related errors. TLS errors will
  now be reported with more detail on the info channel rather than all being
  `tls_short_read` or `pass_through`. In addition, many cases where a TLS short
  read was in fact expected are no longer classified as errors. Expected TLS
  short reads and quasi-expected socket shutdown related errors will no longer
  be reported as unclean WebSocket shutdowns to the application. Information
  about them will remain in the info error channel for debugging purposes.
- Improvement: `start_accept` and `listen` errors are now reported to the caller
  either via an exception or an ec parameter.
- Improvement: Outgoing writes are now batched for improved message throughput
  and reduced system call and TCP frame overhead.
- Bug: Fix some cases of calls to empty lib::function objects.
- Bug: Fix memory leak of connection objects due to cached handlers holding on to
  reference counted pointers. #310 Thank you otaras for reporting.
- Bug: Fix issue with const endpoint accessors (such as `get_user_agent`) not
  compiling due to non-const mutex use. #292 Thank you logofive for reporting.
- Bug: Fix handler allocation crash with multithreaded `io_service`.
- Bug: Fixes incorrect whitespace handling in header parsing. #301 Thank you
  Wolfram Schroers for reporting
- Bug: Fix a crash when parsing empty HTTP headers. Thank you Thingol for
  reporting.
- Bug: Fix a crash following use of the `stop_listening` function. Thank you
  Thingol for reporting.
- Bug: Fix use of variable names that shadow function parameters. The library
  should compile cleanly with -Wshadow now. Thank you giszo for reporting. #318
- Bug: Fix an issue where `set_open_handshake_timeout` was ignored by server
  code. Thank you Robin Rowe for reporting.
- Bug: Fix an issue where custom timeout values weren't being propagated from
  endpoints to new connections.
- Bug: Fix a number of memory leaks related to server connection failures. #323
  #333 #334 #335 Thank you droppy and aydany for reporting and patches.
  reporting.
- Compatibility: Fix compile time conflict with Visual Studio's MIN/MAX macros.
  Thank you Robin Rowe for reporting.
- Documentation: Examples and test suite build system now defaults to clang on
  OS X

0.3.0-alpha4 - 2013-10-11
- HTTP requests ending normally are no longer logged as errors. Thank you Banaan
  for reporting. #294
- Eliminates spurious expired timers in certain error conditions. Thank you
  Banaan for reporting. #295
- Consolidates all bundled library licenses into the COPYING file. #294
- Updates bundled sha1 library to one with a cleaner interface and more
  straight-forward license. Thank you lotodore for reporting and Evgeni Golov
  for reviewing. #294
- Re-introduces strands to asio transport, allowing `io_service` thread pools to
  be used (with some limitations).
- Removes endpoint code that kept track of a connection list that was never used
  anywhere. Removes a lock and reduces connection creation/deletion complexity
  from O(log n) to O(1) in the number of connections.
- A number of internal changes to transport APIs
- Deprecates iostream transport `readsome` in favor of `read_some` which is more
  consistent with the naming of the rest of the library.
- Adds preliminary signaling to iostream transport of eof and fatal transport
  errors
- Updates transport code to use shared pointers rather than raw pointers to
  prevent asio from retaining pointers to connection methods after the
  connection goes out of scope. #293 Thank you otaras for reporting.
- Fixes an issue where custom headers couldn't be set for client connections
  Thank you Jerry Win and Wolfram Schroers for reporting.
- Fixes a compile error on visual studio when using interrupts. Thank you Javier
  Rey Neira for reporting this.
- Adds new 1012 and 1013 close codes per IANA registry
- Add `set_remote_endpoint` method to iostream transport.
- Add `set_secure` method to iostream transport.
- Fix typo in .gitattributes file. Thank you jstarasov for reporting this. #280
- Add missing locale include. Thank you Toninoso for reporting this. #281
- Refactors `asio_transport` endpoint and adds full documentation and exception
  free varients of all methods.
- Removes `asio_transport` endpoint method cancel(). Use `stop_listen()` instead
- Wrap internal `io_service` `run_one()` method
- Suppress error when trying to shut down a connection that was already closed

0.3.0-alpha3 - 2013-07-16
- Minor refactor to bundled sha1 library
- HTTP header comparisons are now case insensitive. #220, #275
- Refactors URI to be exception free and not use regular expressions. This
  eliminates the dependency on boost or C++11 regex libraries allowing native
  C++11 usage on GCC 4.4 and higher and significantly reduces staticly built
  binary sizes.
- Updates handling of Server and User-Agent headers to better handle custom
  settings and allow suppression of these headers for security purposes.
- Fix issue where pong timeout handler always fired. Thank you Steven Klassen
  for reporting this bug.
- Add ping and pong endpoint wrapper methods
- Add `get_request()` pass through method to connection to allow calling methods
  specific to the HTTP policy in use.
- Fix issue compile error with `WEBSOCKETPP_STRICT_MASKING` enabled and another
  issue where `WEBSOCKETPP_STRICT_MASKING` was not applied to incoming messages.
  Thank you Petter Norby for reporting and testing these bugs. #264
- Add additional macro guards for use with boost_config. Thank you breyed
  for testing and code. #261

0.3.0-alpha2 - 2013-06-09
- Fix a regression that caused servers being sent two close frames in a row
  to end a connection uncleanly. #259
- Fix a regression that caused spurious frames following a legitimate close
  frames to erroneously trigger handlers. #258
- Change default HTTP response error code when no http_handler is defined from
  500/Internal Server Error to 426/Upgrade Required
- Remove timezone from logger timestamp to work around issues with the Windows
  implementation of strftime. Thank you breyed for testing and code. #257
- Switch integer literals to char literals to improve VCPP compatibility.
  Thank you breyed for testing and code. #257
- Add MSVCPP warning suppression for the bundled SHA1 library. Thank you breyed
  for testing and code. #257

0.3.0-alpha1 - 2013-06-09
- Initial Release
