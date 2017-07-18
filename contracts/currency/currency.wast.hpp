const char* currency_wast = R"=====(
(module
 (type $FUNCSIG$ijjj (func (param i64 i64 i64) (result i32)))
 (type $FUNCSIG$ijjii (func (param i64 i64 i32 i32) (result i32)))
 (type $FUNCSIG$vj (func (param i64)))
 (type $FUNCSIG$ijjjii (func (param i64 i64 i64 i32 i32) (result i32)))
 (type $FUNCSIG$vii (func (param i32 i32)))
 (type $FUNCSIG$iii (func (param i32 i32) (result i32)))
 (import "env" "assert" (func $assert (param i32 i32)))
 (import "env" "load_i64" (func $load_i64 (param i64 i64 i64 i32 i32) (result i32)))
 (import "env" "readMessage" (func $readMessage (param i32 i32) (result i32)))
 (import "env" "remove_i64" (func $remove_i64 (param i64 i64 i64) (result i32)))
 (import "env" "requireAuth" (func $requireAuth (param i64)))
 (import "env" "requireNotice" (func $requireNotice (param i64)))
 (import "env" "store_i64" (func $store_i64 (param i64 i64 i32 i32) (result i32)))
 (table 0 anyfunc)
 (memory $0 1)
 (data (i32.const 4) "p\04\00\00")
 (data (i32.const 16) "integer underflow subtracting token balance\00")
 (data (i32.const 64) "integer overflow adding token balance\00")
 (export "memory" (memory $0))
 (export "_ZN8currency12storeAccountEyRKNS_7AccountE" (func $_ZN8currency12storeAccountEyRKNS_7AccountE))
 (export "_ZN8currency23apply_currency_transferERKNS_8TransferE" (func $_ZN8currency23apply_currency_transferERKNS_8TransferE))
 (export "init" (func $init))
 (export "apply" (func $apply))
 (func $_ZN8currency12storeAccountEyRKNS_7AccountE (param $0 i64) (param $1 i32)
  (block $label$0
   (br_if $label$0
    (i64.eq
     (i64.load offset=8
      (get_local $1)
     )
     (i64.const 0)
    )
   )
   (drop
    (call $store_i64
     (get_local $0)
     (i64.const 21967113313)
     (get_local $1)
     (i32.const 16)
    )
   )
   (return)
  )
  (drop
   (call $remove_i64
    (get_local $0)
    (i64.const 21967113313)
    (i64.load
     (get_local $1)
    )
   )
  )
 )
 (func $_ZN8currency23apply_currency_transferERKNS_8TransferE (param $0 i32)
  (local $1 i64)
  (local $2 i32)
  (i32.store offset=4
   (i32.const 0)
   (tee_local $2
    (i32.sub
     (i32.load offset=4
      (i32.const 0)
     )
     (i32.const 32)
    )
   )
  )
  (set_local $1
   (i64.load
    (get_local $0)
   )
  )
  (call $requireNotice
   (i64.load offset=8
    (get_local $0)
   )
  )
  (call $requireNotice
   (get_local $1)
  )
  (call $requireAuth
   (i64.load
    (get_local $0)
   )
  )
  (i64.store offset=16
   (get_local $2)
   (i64.const 21967113313)
  )
  (i64.store offset=24
   (get_local $2)
   (i64.const 0)
  )
  (drop
   (call $load_i64
    (i64.load
     (get_local $0)
    )
    (i64.const 862690298531)
    (i64.const 21967113313)
    (i32.add
     (get_local $2)
     (i32.const 16)
    )
    (i32.const 16)
   )
  )
  (i64.store
   (get_local $2)
   (i64.const 21967113313)
  )
  (i64.store offset=8
   (get_local $2)
   (i64.const 0)
  )
  (drop
   (call $load_i64
    (i64.load offset=8
     (get_local $0)
    )
    (i64.const 862690298531)
    (i64.const 21967113313)
    (get_local $2)
    (i32.const 16)
   )
  )
  (call $assert
   (i64.ge_u
    (i64.load offset=24
     (get_local $2)
    )
    (i64.load offset=16
     (get_local $0)
    )
   )
   (i32.const 16)
  )
  (i64.store offset=24
   (get_local $2)
   (i64.sub
    (i64.load offset=24
     (get_local $2)
    )
    (tee_local $1
     (i64.load offset=16
      (get_local $0)
     )
    )
   )
  )
  (call $assert
   (i64.ge_u
    (i64.add
     (get_local $1)
     (i64.load offset=8
      (get_local $2)
     )
    )
    (get_local $1)
   )
   (i32.const 64)
  )
  (i64.store offset=8
   (get_local $2)
   (i64.add
    (i64.load offset=8
     (get_local $2)
    )
    (i64.load offset=16
     (get_local $0)
    )
   )
  )
  (set_local $1
   (i64.load
    (get_local $0)
   )
  )
  (block $label$0
   (block $label$1
    (br_if $label$1
     (i64.eq
      (i64.load offset=24
       (get_local $2)
      )
      (i64.const 0)
     )
    )
    (drop
     (call $store_i64
      (get_local $1)
      (i64.const 21967113313)
      (i32.add
       (get_local $2)
       (i32.const 16)
      )
      (i32.const 16)
     )
    )
    (br $label$0)
   )
   (drop
    (call $remove_i64
     (get_local $1)
     (i64.const 21967113313)
     (i64.load offset=16
      (get_local $2)
     )
    )
   )
  )
  (set_local $1
   (i64.load
    (i32.add
     (get_local $0)
     (i32.const 8)
    )
   )
  )
  (block $label$2
   (block $label$3
    (br_if $label$3
     (i64.eq
      (i64.load
       (i32.add
        (get_local $2)
        (i32.const 8)
       )
      )
      (i64.const 0)
     )
    )
    (drop
     (call $store_i64
      (get_local $1)
      (i64.const 21967113313)
      (get_local $2)
      (i32.const 16)
     )
    )
    (br $label$2)
   )
   (drop
    (call $remove_i64
     (get_local $1)
     (i64.const 21967113313)
     (i64.load
      (get_local $2)
     )
    )
   )
  )
  (i32.store offset=4
   (i32.const 0)
   (i32.add
    (get_local $2)
    (i32.const 32)
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
  (i64.store
   (get_local $0)
   (i64.const 21967113313)
  )
  (drop
   (call $store_i64
    (i64.const 862690298531)
    (i64.const 21967113313)
    (get_local $0)
    (i32.const 16)
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
  (local $2 i32)
  (i32.store offset=4
   (i32.const 0)
   (tee_local $2
    (i32.sub
     (i32.load offset=4
      (i32.const 0)
     )
     (i32.const 32)
    )
   )
  )
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
   (i64.store offset=24
    (get_local $2)
    (i64.const 0)
   )
   (drop
    (call $readMessage
     (i32.add
      (get_local $2)
      (i32.const 8)
     )
     (i32.const 24)
    )
   )
   (call $_ZN8currency23apply_currency_transferERKNS_8TransferE
    (i32.add
     (get_local $2)
     (i32.const 8)
    )
   )
  )
  (i32.store offset=4
   (i32.const 0)
   (i32.add
    (get_local $2)
    (i32.const 32)
   )
  )
 )
)
)=====";
