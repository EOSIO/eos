#pragma once

// These are handcrafted or otherwise tricky to generate with our tool chain

static const char entry_wast[] = R"=====(
(module
 (import "env" "eos_assert" (func $eos_assert (param i32 i32)))
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
  (call $eos_assert
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
 (import "env" "eos_assert" (func $eos_assert (param i32 i32)))
 (table 0 anyfunc)
 (memory $0 1)
 (export "memory" (memory $0))
 (export "apply" (func $apply))
 (func $apply (param $0 i64) (param $1 i64)
  (if (i64.eq (get_local $1) (i64.const 0))
    (set_global $g0 (i64.const 444))
  )
  (if (i64.eq (get_local $1) (i64.const 1))
    (call $eos_assert (i64.eq (get_global $g0) (i64.const 2)) (i32.const 0))
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

static const char biggest_memory_wast[] = R"=====(
(module
 (import "env" "sbrk" (func $sbrk (param i32) (result i32)))
 (table 0 anyfunc)
 (memory $0 16)
 (export "memory" (memory $0))
 (export "apply" (func $apply))
 
 (func $apply (param $0 i64) (param $1 i64)
   (drop
     (call $sbrk
       (i32.const 1)
     )
   )
 )
)
)=====";

static const char too_big_memory_wast[] = R"=====(
(module
 (table 0 anyfunc)
 (memory $0 17)
 (export "memory" (memory $0))
 (export "apply" (func $apply))
 (func $apply (param $0 i64) (param $1 i64))
)
)=====";

static const char valid_sparse_table[] = R"=====(
(module
 (table 1024 anyfunc)
 (func $apply (param $0 i64) (param $1 i64))
 (elem (i32.const 0) $apply)
 (elem (i32.const 1022) $apply $apply)
)
)=====";

static const char too_big_table[] = R"=====(
(module
 (table 1025 anyfunc)
 (func $apply (param $0 i64) (param $1 i64))
 (elem (i32.const 0) $apply)
 (elem (i32.const 1022) $apply $apply)
)
)=====";

static const char memory_init_borderline[] = R"=====(
(module
 (memory $0 16)
 (data (i32.const 65532) "sup!")
)
)=====";

static const char memory_init_toolong[] = R"=====(
(module
 (memory $0 16)
 (data (i32.const 65533) "sup!")
)
)=====";

static const char memory_init_negative[] = R"=====(
(module
 (memory $0 16)
 (data (i32.const -1) "sup!")
)
)=====";

static const char memory_table_import[] = R"=====(
(module
 (table  (import "foo" "table") 10 anyfunc)
 (memory (import "nom" "memory") 0)
)
)=====";
