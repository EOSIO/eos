#pragma once

// These are handcrafted or otherwise tricky to generate with our tool chain

static const char entry_wast[] = R"=====(
(module
 (import "env" "assert" (func $assert (param i32 i32)))
 (import "env" "now" (func $now (result i32)))
 (table 0 anyfunc)
 (memory $0 1)
 (export "memory" (memory $0))
 (export "entry" (func $entry))
 (export "init" (func $init))
 (export "apply" (func $apply))
 (func $entry
  (i32.store offset=4
   (i32.const 0)
   (call $now)
  )
 )
 (func $init
 )
 (func $apply (param $0 i64) (param $1 i64)
  (call $assert
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