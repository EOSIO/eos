test_ram_limit contract was compiled with apifiny.cdt v1.4.1

That contract was ported to compile with apifiny.cdt v1.5.0, but the test that uses it is very sensitive to stdlib/apifinylib changes, compilation flags and linker flags.

deferred_test, get_sender_test, proxy, reject_all, and test_api contracts were compiled with apifiny.cdt v1.6.1

The remaining contracts have been ported to compile with apifiny.cdt v1.6.x. They were compiled with a patched version of apifiny.cdt v1.6.0-rc1 (commit 1c9180ff5a1e431385180ce459e11e6a1255c1a4).
