#pragma once
const char* currency_wast = R"=====(
(module
 (type $FUNCSIG$vj (func (param i64)))
 (type $FUNCSIG$vii (func (param i32 i32)))
 (type $FUNCSIG$iii (func (param i32 i32) (result i32)))
 (type $FUNCSIG$j (func (result i64)))
 (type $FUNCSIG$ijjjjii (func (param i64 i64 i64 i64 i32 i32) (result i32)))
 (type $FUNCSIG$ijjj (func (param i64 i64 i64) (result i32)))
 (type $FUNCSIG$ijjjii (func (param i64 i64 i64 i32 i32) (result i32)))
 (import "env" "assert" (func $assert (param i32 i32)))
 (import "env" "currentCode" (func $currentCode (result i64)))
 (import "env" "load_i64" (func $load_i64 (param i64 i64 i64 i64 i32 i32) (result i32)))
 (import "env" "readMessage" (func $readMessage (param i32 i32) (result i32)))
 (import "env" "remove_i64" (func $remove_i64 (param i64 i64 i64) (result i32)))
 (import "env" "requireAuth" (func $requireAuth (param i64)))
 (import "env" "requireNotice" (func $requireNotice (param i64)))
 (import "env" "store_i64" (func $store_i64 (param i64 i64 i64 i32 i32) (result i32)))
 (table 0 anyfunc)
 (memory $0 1)
 (data (i32.const 4) "0\04\00\00")
 (data (i32.const 16) "insufficient funds\00")
 (export "memory" (memory $0))
 (export "_Z23apply_currency_transferv" (func $_Z23apply_currency_transferv))
 (export "init" (func $init))
 (export "apply" (func $apply))
 (func $_Z23apply_currency_transferv
  (local $0 i64)
  (local $1 i64)
  (local $2 i32)
  (i32.store offset=4
   (i32.const 0)
   (tee_local $2
    (i32.sub
     (i32.load offset=4
      (i32.const 0)
     )
     (i32.const 48)
    )
   )
  )
  (i64.store offset=40
   (get_local $2)
   (i64.const 0)
  )
  (drop
   (call $readMessage
    (i32.add
     (get_local $2)
     (i32.const 24)
    )
    (i32.const 24)
   )
  )
  (set_local $0
   (i64.load offset=24
    (get_local $2)
   )
  )
  (call $requireNotice
   (i64.load offset=32
    (get_local $2)
   )
  )
  (call $requireNotice
   (get_local $0)
  )
  (call $requireAuth
   (i64.load offset=24
    (get_local $2)
   )
  )
  (i64.store offset=16
   (get_local $2)
   (i64.const 0)
  )
  (i64.store offset=8
   (get_local $2)
   (i64.const 0)
  )
  (drop
   (call $load_i64
    (i64.load offset=24
     (get_local $2)
    )
    (call $currentCode)
    (i64.const 497826380083)
    (i64.const 21967113313)
    (i32.add
     (get_local $2)
     (i32.const 16)
    )
    (i32.const 8)
   )
  )
  (drop
   (call $load_i64
    (i64.load offset=32
     (get_local $2)
    )
    (call $currentCode)
    (i64.const 497826380083)
    (i64.const 21967113313)
    (i32.add
     (get_local $2)
     (i32.const 8)
    )
    (i32.const 8)
   )
  )
  (call $assert
   (i64.ge_u
    (i64.load offset=16
     (get_local $2)
    )
    (i64.load offset=40
     (get_local $2)
    )
   )
   (i32.const 16)
  )
  (i64.store offset=16
   (get_local $2)
   (tee_local $1
    (i64.sub
     (i64.load offset=16
      (get_local $2)
     )
     (tee_local $0
      (i64.load offset=40
       (get_local $2)
      )
     )
    )
   )
  )
  (i64.store offset=8
   (get_local $2)
   (i64.add
    (get_local $0)
    (i64.load offset=8
     (get_local $2)
    )
   )
  )
  (set_local $0
   (i64.load offset=24
    (get_local $2)
   )
  )
  (block $label$0
   (block $label$1
    (br_if $label$1
     (i64.eq
      (get_local $1)
      (i64.const 0)
     )
    )
    (drop
     (call $store_i64
      (get_local $0)
      (i64.const 497826380083)
      (i64.const 21967113313)
      (i32.add
       (get_local $2)
       (i32.const 16)
      )
      (i32.const 8)
     )
    )
    (br $label$0)
   )
   (drop
    (call $remove_i64
     (get_local $0)
     (i64.const 497826380083)
     (i64.const 21967113313)
    )
   )
  )
  (drop
   (call $store_i64
    (i64.load
     (i32.add
      (i32.add
       (get_local $2)
       (i32.const 24)
      )
      (i32.const 8)
     )
    )
    (i64.const 497826380083)
    (i64.const 21967113313)
    (i32.add
     (get_local $2)
     (i32.const 8)
    )
    (i32.const 8)
   )
  )
  (i32.store offset=4
   (i32.const 0)
   (i32.add
    (get_local $2)
    (i32.const 48)
   )
  )
 )
 (func $init
  (local $0 i32)
  (i32.store offset=4
   (i32.const 0)
   (tee_local $0
    (i32.sub
     (i32.load offset=4
      (i32.const 0)
     )
     (i32.const 16)
    )
   )
  )
  (i64.store offset=8
   (get_local $0)
   (i64.const 1000000000)
  )
  (drop
   (call $store_i64
    (i64.const 862690298531)
    (i64.const 497826380083)
    (i64.const 21967113313)
    (i32.add
     (get_local $0)
     (i32.const 8)
    )
    (i32.const 8)
   )
  )
  (i32.store offset=4
   (i32.const 0)
   (i32.add
    (get_local $0)
    (i32.const 16)
   )
  )
 )
 (func $apply (param $0 i64) (param $1 i64)
  (block $label$0
   (br_if $label$0
    (i64.ne
     (get_local $0)
     (i64.const 862690298531)
    )
   )
   (br_if $label$0
    (i64.ne
     (get_local $1)
     (i64.const 624065709652)
    )
   )
   (call $_Z23apply_currency_transferv)
  )
 )
)
)=====";
