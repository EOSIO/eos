#pragma once

// These are handcrafted or otherwise tricky to generate with our tool chain

static const char table_oob_wast[] = R"=====(
(module
 (import "env" "eosio_assert" (func $assert (param i32 i32)))
 (import "env" "printi" (func $printi (param i64)))
 (type $SIG$vj (func (param i64)))
 (table 128 anyfunc)
 (memory $0 1)
 (export "apply" (func $apply))
 (func $apply (param $0 i64) (param $1 i64)
   (call_indirect (type $SIG$vj)
     (i64.shr_u
       (get_local $1)
       (i64.const 32)
     )
     (i32.wrap/i64
       (get_local $1)
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