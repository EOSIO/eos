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
 (export "apply" (func $apply))
 (func $entry
  (i32.store offset=4
   (i32.const 0)
   (call $now)
  )
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

static const char simple_no_memory_wast[] = R"=====(
(module
 (import "env" "memcpy" (func $memcpy (param i32 i32 i32) (result i32)))
 (table 0 anyfunc)
 (export "apply" (func $apply))
 (func $apply (param $0 i64) (param $1 i64)
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
 (import "env" "assert" (func $assert (param i32 i32)))
 (table 0 anyfunc)
 (memory $0 1)
 (export "memory" (memory $0))
 (export "apply" (func $apply))
 (func $apply (param $0 i64) (param $1 i64)
  (if (i64.eq (get_local $1) (i64.const 0))
    (set_global $g0 (i64.const 444))
  )
  (if (i64.eq (get_local $1) (i64.const 1))
    (call $assert (i64.eq (get_global $g0) (i64.const 2)) (i32.const 0))
  )
 )
 (global $g0 (mut i64) (i64.const 2))
)
)=====";

static const char current_memory_wast[] = R"=====(
(module
 (table 0 anyfunc)
 (memory $0 1)
 (export "memory" (memory $0))
 (export "apply" (func $apply))
 (func $apply (param $0 i64) (param $1 i64)
   (drop
     (current_memory)
   )
 )
)
)=====";

static const char grow_memory_wast[] = R"=====(
(module
 (table 0 anyfunc)
 (memory $0 1)
 (export "memory" (memory $0))
 (export "apply" (func $apply))
 (func $apply (param $0 i64) (param $1 i64)
   (drop
     (grow_memory
       (i32.const 20)
     )
   )
 )
)
)=====";