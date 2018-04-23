#pragma once
#include <eosio/chain/webassembly/common.hpp>

// These are handcrafted or otherwise tricky to generate with our tool chain

static const char call_depth_limit_wast[] = R"=====(
(module
 (table 0 anyfunc)
 (memory $0 1)
 (export "memory" (memory $0))
 (export "_foo" (func $_foo))
 (export "apply" (func $apply))
 (func $_foo (param $0 i32)
  (block $label$0
   (br_if $label$0
    (i32.eq
     (get_local $0)
     (i32.const 248)
    )
   )
   (call $_foo
    (i32.add
      (get_local $0)
     (i32.const 1)
    )
   )
  )
 )
 (func $apply (param $a i64) (param $b i64) (param $c i64)
   (call $_foo
     (i32.const 0)
   )
 )
)
)=====";

static const char entry_wast[] = R"=====(
(module
 (import "env" "require_auth" (func $require_auth (param i64)))
 (import "env" "eosio_assert" (func $eosio_assert (param i32 i32)))
 (import "env" "now" (func $now (result i32)))
 (table 0 anyfunc)
 (memory $0 1)
 (export "memory" (memory $0))
 (export "entry" (func $entry))
 (export "apply" (func $apply))
 (func $entry
  (i32.store offset=4
   (i32.const 0)
   (call $now)
  )
 )
 (func $apply (param $0 i64) (param $1 i64) (param $2 i64)
  (call $require_auth (i64.const 6121376101093867520))
  (call $eosio_assert
   (i32.eq
    (i32.load offset=4
     (i32.const 0)
    )
    (call $now)
   )
   (i32.const 0)
  )
 )
 (start $entry)
)
)=====";

static const char simple_no_memory_wast[] = R"=====(
(module
 (import "env" "require_auth" (func $require_auth (param i64)))
 (import "env" "memcpy" (func $memcpy (param i32 i32 i32) (result i32)))
 (table 0 anyfunc)
 (export "apply" (func $apply))
 (func $apply (param $0 i64) (param $1 i64) (param $2 i64)
    (call $require_auth (i64.const 11323361180581363712))
    (drop
       (call $memcpy
          (i32.const 0)
          (i32.const 1024)
          (i32.const 1024)
       )
    )
 )
)
)=====";

static const char mutable_global_wast[] = R"=====(
(module
 (import "env" "require_auth" (func $require_auth (param i64)))
 (import "env" "eosio_assert" (func $eosio_assert (param i32 i32)))
 (table 0 anyfunc)
 (memory $0 1)
 (export "memory" (memory $0))
 (export "apply" (func $apply))
 (func $apply (param $0 i64) (param $1 i64) (param $2 i64)
  (call $require_auth (i64.const 7235159549794234880))
  (if (i64.eq (get_local $2) (i64.const 0)) (then
    (set_global $g0 (i64.const 444))
    (return)
  ))
  (if (i64.eq (get_local $2) (i64.const 1)) (then
    (call $eosio_assert (i64.eq (get_global $g0) (i64.const 2)) (i32.const 0))
    (return)
  ))
  (call $eosio_assert (i32.const 0) (i32.const 0))
 )
 (global $g0 (mut i64) (i64.const 2))
)
)=====";

static const char biggest_memory_wast[] = R"=====(
(module
 (import "env" "eosio_assert" (func $$eosio_assert (param i32 i32)))
 (import "env" "require_auth" (func $$require_auth (param i64)))
 (table 0 anyfunc)
 (memory $$0 ${MAX_WASM_PAGES})
 (export "memory" (memory $$0))
 (export "apply" (func $$apply))
 (func $$apply (param $$0 i64) (param $$1 i64) (param $$2 i64)
  (call $$require_auth (i64.const 4294504710842351616))
  (call $$eosio_assert
   (i32.eq
     (grow_memory (i32.const 1))
     (i32.const -1)
   )
   (i32.const 0)
  )
 )
)
)=====";

static const char too_big_memory_wast[] = R"=====(
(module
 (table 0 anyfunc)
 (memory $$0 ${MAX_WASM_PAGES_PLUS_ONE})
 (export "memory" (memory $$0))
 (export "apply" (func $$apply))
 (func $$apply (param $$0 i64) (param $$1 i64) (param $$2 i64))
)
)=====";

static const char valid_sparse_table[] = R"=====(
(module
 (table 1024 anyfunc)
 (export "apply" (func $apply))
 (func $apply (param $0 i64) (param $1 i64) (param $2 i64))
 (elem (i32.const 0) $apply)
 (elem (i32.const 1022) $apply $apply)
)
)=====";

static const char too_big_table[] = R"=====(
(module
 (table 1025 anyfunc)
 (export "apply" (func $apply))
 (func $apply (param $0 i64) (param $1 i64) (param $2 i64))
 (elem (i32.const 0) $apply)
 (elem (i32.const 1022) $apply $apply)
)
)=====";

static const char memory_init_borderline[] = R"=====(
(module
 (memory $0 16)
 (export "apply" (func $apply))
 (func $apply (param $0 i64) (param $1 i64) (param $2 i64))
 (data (i32.const 65532) "sup!")
)
)=====";

static const char memory_init_toolong[] = R"=====(
(module
 (memory $0 16)
 (export "apply" (func $apply))
 (func $apply (param $0 i64) (param $1 i64) (param $2 i64))
 (data (i32.const 65533) "sup!")
)
)=====";

static const char memory_init_negative[] = R"=====(
(module
 (memory $0 16)
 (export "apply" (func $apply))
 (func $apply (param $0 i64) (param $1 i64) (param $2 i64))
 (data (i32.const -1) "sup!")
)
)=====";

static const char memory_table_import[] = R"=====(
(module
 (table  (import "foo" "table") 10 anyfunc)
 (memory (import "nom" "memory") 0)
 (export "apply" (func $apply))
 (func $apply (param $0 i64) (param $1 i64) (param $2 i64))
)
)=====";

static const char table_checker_wast[] = R"=====(
(module
 (import "env" "require_auth" (func $require_auth (param i64)))
 (import "env" "eosio_assert" (func $assert (param i32 i32)))
 (type $SIG$vj (func (param i64)))
 (table 1024 anyfunc)
 (memory $0 1)
 (export "apply" (func $apply))
 (func $apply (param $0 i64) (param $1 i64) (param $2 i64)
   (call $require_auth (i64.const 14547189746360123392))
   (call_indirect $SIG$vj
     (i64.shr_u
       (get_local $2)
       (i64.const 32)
     )
     (i32.wrap/i64
       (get_local $2)
     )
   )
 )
 (func $apple (type $SIG$vj) (param $0 i64)
   (call $assert
     (i64.eq
       (get_local $0)
       (i64.const 555)
     )
     (i32.const 0)
   )
 )
 (func $bannna (type $SIG$vj) (param $0 i64)
   (call $assert
     (i64.eq
       (get_local $0)
       (i64.const 7777)
     )
     (i32.const 0)
   )
 )
 (elem (i32.const 0) $apple)
 (elem (i32.const 1022) $apple $bannna)
)
)=====";

static const char table_checker_proper_syntax_wast[] = R"=====(
(module
 (import "env" "require_auth" (func $require_auth (param i64)))
 (import "env" "eosio_assert" (func $assert (param i32 i32)))
 (import "env" "printi" (func $printi (param i64)))
 (type $SIG$vj (func (param i64)))
 (table 1024 anyfunc)
 (memory $0 1)
 (export "apply" (func $apply))
 (func $apply (param $0 i64) (param $1 i64) (param $2 i64)
   (call $require_auth (i64.const 14547189746360123392))
   (call_indirect (type $SIG$vj)
     (i64.shr_u
       (get_local $2)
       (i64.const 32)
     )
     (i32.wrap/i64
       (get_local $2)
     )
   )
 )
 (func $apple (type $SIG$vj) (param $0 i64)
   (call $assert
     (i64.eq
       (get_local $0)
       (i64.const 555)
     )
     (i32.const 0)
   )
 )
 (func $bannna (type $SIG$vj) (param $0 i64)
   (call $assert
     (i64.eq
       (get_local $0)
       (i64.const 7777)
     )
     (i32.const 0)
   )
 )
 (elem (i32.const 0) $apple)
 (elem (i32.const 1022) $apple $bannna)
)
)=====";

static const char table_checker_small_wast[] = R"=====(
(module
 (import "env" "require_auth" (func $require_auth (param i64)))
 (import "env" "eosio_assert" (func $assert (param i32 i32)))
 (import "env" "printi" (func $printi (param i64)))
 (type $SIG$vj (func (param i64)))
 (table 128 anyfunc)
 (memory $0 1)
 (export "apply" (func $apply))
 (func $apply (param $0 i64) (param $1 i64) (param $2 i64)
   (call $require_auth (i64.const 14547189746360123392))
   (call_indirect (type $SIG$vj)
     (i64.shr_u
       (get_local $2)
       (i64.const 32)
     )
     (i32.wrap/i64
       (get_local $2)
     )
   )
 )
 (func $apple (type $SIG$vj) (param $0 i64)
   (call $assert
     (i64.eq
       (get_local $0)
       (i64.const 555)
     )
     (i32.const 0)
   )
 )
 (elem (i32.const 0) $apple)
)
)=====";

static const char global_protection_none_get_wast[] = R"=====(
(module
 (export "apply" (func $apply))
 (func $apply (param $0 i64) (param $1 i64) (param $2 i64)
   (drop (get_global 0))
 )
)
)=====";

static const char global_protection_some_get_wast[] = R"=====(
(module
 (global i32 (i32.const -11))
 (global i32 (i32.const 56))
 (export "apply" (func $apply))
 (func $apply (param $0 i64) (param $1 i64) (param $2 i64)
   (drop (get_global 1))
   (drop (get_global 2))
 )
)
)=====";

static const char global_protection_none_set_wast[] = R"=====(
(module
 (export "apply" (func $apply))
 (func $apply (param $0 i64) (param $1 i64) (param $2 i64)
   (set_global 0 (get_local 1))
 )
)
)=====";

static const char global_protection_some_set_wast[] = R"=====(
(module
 (global i64 (i64.const -11))
 (global i64 (i64.const 56))
 (export "apply" (func $apply))
 (func $apply (param $0 i64) (param $1 i64) (param $2 i64)
   (set_global 2 (get_local 1))
 )
)
)=====";

static const std::vector<uint8_t> global_protection_okay_get_wasm{
   0x00, 'a', 's', 'm', 0x01, 0x00, 0x00, 0x00,
   0x01, 0x07, 0x01, 0x60, 0x03, 0x7e, 0x7e, 0x7e, 0x00,            //type section containing a function as void(i64,i64,i64)
   0x03, 0x02, 0x01, 0x00,                                          //a function

   0x06, 0x06, 0x01, 0x7f, 0x00, 0x41, 0x75, 0x0b,                  //global

   0x07, 0x09, 0x01, 0x05, 'a', 'p', 'p', 'l', 'y', 0x00, 0x00,     //export function 0 as "apply"
   0x0a, 0x07, 0x01,                                                //code section
   0x05, 0x00,                                                      //function body start with length 5; no locals
   0x23, 0x00,                                                      //get global 0
   0x1a,                                                            //drop
   0x0b                                                             //end
};

static const std::vector<uint8_t> global_protection_none_get_wasm{
   0x00, 'a', 's', 'm', 0x01, 0x00, 0x00, 0x00,
   0x01, 0x07, 0x01, 0x60, 0x03, 0x7e, 0x7e, 0x7e, 0x00,            //type section containing a function as void(i64,i64,i64)
   0x03, 0x02, 0x01, 0x00,                                          //a function

   0x07, 0x09, 0x01, 0x05, 'a', 'p', 'p', 'l', 'y', 0x00, 0x00,     //export function 0 as "apply"
   0x0a, 0x07, 0x01,                                                //code section
   0x05, 0x00,                                                      //function body start with length 5; no locals
   0x23, 0x00,                                                      //get global 0
   0x1a,                                                            //drop
   0x0b                                                             //end
};

static const std::vector<uint8_t> global_protection_some_get_wasm{
   0x00, 'a', 's', 'm', 0x01, 0x00, 0x00, 0x00,
   0x01, 0x07, 0x01, 0x60, 0x03, 0x7e, 0x7e, 0x7e, 0x00,            //type section containing a function as void(i64,i64,i64)
   0x03, 0x02, 0x01, 0x00,                                          //a function

   0x06, 0x06, 0x01, 0x7f, 0x00, 0x41, 0x75, 0x0b,                  //global

   0x07, 0x09, 0x01, 0x05, 'a', 'p', 'p', 'l', 'y', 0x00, 0x00,     //export function 0 as "apply"
   0x0a, 0x07, 0x01,                                                //code section
   0x05, 0x00,                                                      //function body start with length 5; no locals
   0x23, 0x01,                                                      //get global 1
   0x1a,                                                            //drop
   0x0b                                                             //end
};

static const std::vector<uint8_t> global_protection_okay_set_wasm{
   0x00, 'a', 's', 'm', 0x01, 0x00, 0x00, 0x00,
   0x01, 0x07, 0x01, 0x60, 0x03, 0x7e, 0x7e, 0x7e, 0x00,            //type section containing a function as void(i64,i64,i64)
   0x03, 0x02, 0x01, 0x00,                                          //a function

   0x06, 0x06, 0x01, 0x7e, 0x01, 0x42, 0x75, 0x0b,                  //global.. this time with i64 & global mutablity

   0x07, 0x09, 0x01, 0x05, 'a', 'p', 'p', 'l', 'y', 0x00, 0x00,     //export function 0 as "apply"
   0x0a, 0x08, 0x01,                                                //code section
   0x06, 0x00,                                                      //function body start with length 6; no locals
   0x20, 0x00,                                                      //get local 0
   0x24, 0x00,                                                      //set global 0
   0x0b                                                             //end
};

static const std::vector<uint8_t> global_protection_some_set_wasm{
   0x00, 'a', 's', 'm', 0x01, 0x00, 0x00, 0x00,
   0x01, 0x07, 0x01, 0x60, 0x03, 0x7e, 0x7e, 0x7e, 0x00,            //type section containing a function as void(i64,i64,i64)
   0x03, 0x02, 0x01, 0x00,                                          //a function

   0x06, 0x06, 0x01, 0x7e, 0x01, 0x42, 0x75, 0x0b,                  //global.. this time with i64 & global mutablity

   0x07, 0x09, 0x01, 0x05, 'a', 'p', 'p', 'l', 'y', 0x00, 0x00,     //export function 0 as "apply"
   0x0a, 0x08, 0x01,                                                //code section
   0x06, 0x00,                                                      //function body start with length 6; no locals
   0x20, 0x00,                                                      //get local 0
   0x24, 0x01,                                                      //set global 1
   0x0b                                                             //end
};

static const char no_apply_wast[] = R"=====(
(module
 (func $apply (param $0 i64) (param $1 i64) (param $2 i64))
)
)=====";

static const char apply_wrong_signature_wast[] = R"=====(
(module
 (export "apply" (func $apply))
 (func $apply (param $0 i64) (param $1 f64))
)
)=====";

static const char import_injected_wast[] =                                            \
"(module"                                                                             \
" (export \"apply\" (func $apply))"                                                   \
" (import \"" EOSIO_INJECTED_MODULE_NAME "\" \"checktime\" (func $inj (param i32)))"  \
" (func $apply (param $0 i64) (param $1 i64) (param $2 i64))"                         \
")";
