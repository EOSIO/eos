const char* currency_wast = R"=====(
(module
 (type $FUNCSIG$vj (func (param i64)))
 (type $FUNCSIG$ijjj (func (param i64 i64 i64) (result i32)))
 (type $FUNCSIG$ijjjii (func (param i64 i64 i64 i32 i32) (result i32)))
 (type $FUNCSIG$vii (func (param i32 i32)))
 (type $FUNCSIG$ijjjjii (func (param i64 i64 i64 i64 i32 i32) (result i32)))
 (type $FUNCSIG$iii (func (param i32 i32) (result i32)))
 (import "env" "assert" (func $assert (param i32 i32)))
 (import "env" "load_i64" (func $load_i64 (param i64 i64 i64 i64 i32 i32) (result i32)))
 (import "env" "printi" (func $printi (param i64)))
 (import "env" "readMessage" (func $readMessage (param i32 i32) (result i32)))
 (import "env" "remove_i64" (func $remove_i64 (param i64 i64 i64) (result i32)))
 (import "env" "requireAuth" (func $requireAuth (param i64)))
 (import "env" "requireNotice" (func $requireNotice (param i64)))
 (import "env" "store_i64" (func $store_i64 (param i64 i64 i64 i32 i32) (result i32)))
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
    (i64.eqz
     (i64.load
      (get_local $1)
     )
    )
   )
   (drop
    (call $store_i64
     (get_local $0)
     (i64.const 21967113313)
     (i64.const 21967113313)
     (get_local $1)
     (i32.const 8)
    )
   )
   (return)
  )
  (call $printi
   (get_local $0)
  )
  (drop
   (call $remove_i64
    (get_local $0)
    (i64.const 21967113313)
    (i64.const 21967113313)
   )
  )
 )
 (func $_ZN8currency23apply_currency_transferERKNS_8TransferE (param $0 i32)
  (local $1 i64)
  (local $2 i64)
  (local $3 i64)
  (local $4 i32)
  (i32.store offset=4
   (i32.const 0)
   (tee_local $4
    (i32.sub
     (i32.load offset=4
      (i32.const 0)
     )
     (i32.const 16)
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
  (set_local $1
   (call $_ZN8currency10getAccountEy
    (i64.load
     (get_local $0)
    )
   )
  )
  (set_local $2
   (call $_ZN8currency10getAccountEy
    (i64.load offset=8
     (get_local $0)
    )
   )
  )
  (call $assert
   (i64.ge_u
    (get_local $1)
    (i64.load offset=16
     (get_local $0)
    )
   )
   (i32.const 16)
  )
  (i64.store offset=8
   (get_local $4)
   (i64.sub
    (get_local $1)
    (tee_local $3
     (i64.load offset=16
      (get_local $0)
     )
    )
   )
  )
  (call $assert
   (i64.ge_u
    (i64.add
     (get_local $2)
     (get_local $3)
    )
    (get_local $3)
   )
   (i32.const 64)
  )
  (i64.store
   (get_local $4)
   (i64.add
    (get_local $2)
    (i64.load offset=16
     (get_local $0)
    )
   )
  )
  (call $_ZN8currency12storeAccountEyRKNS_7AccountE
   (i64.load
    (get_local $0)
   )
   (i32.add
    (get_local $4)
    (i32.const 8)
   )
  )
  (call $_ZN8currency12storeAccountEyRKNS_7AccountE
   (i64.load offset=8
    (get_local $0)
   )
   (get_local $4)
  )
  (i32.store offset=4
   (i32.const 0)
   (i32.add
    (get_local $4)
    (i32.const 16)
   )
  )
 )
 (func $_ZN8currency10getAccountEy (param $0 i64) (result i64)
  (local $1 i32)
  (i32.store offset=4
   (i32.const 0)
   (tee_local $1
    (i32.sub
     (i32.load offset=4
      (i32.const 0)
     )
     (i32.const 16)
    )
   )
  )
  (i64.store offset=8
   (get_local $1)
   (i64.const 0)
  )
  (drop
   (call $load_i64
    (get_local $0)
    (i64.const 862690298531)
    (i64.const 21967113313)
    (i64.const 21967113313)
    (i32.add
     (get_local $1)
     (i32.const 8)
    )
    (i32.const 8)
   )
  )
  (set_local $0
   (i64.load offset=8
    (get_local $1)
   )
  )
  (i32.store offset=4
   (i32.const 0)
   (i32.add
    (get_local $1)
    (i32.const 16)
   )
  )
  (get_local $0)
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
  (call $_ZN8currency12storeAccountEyRKNS_7AccountE
   (i64.const 862690298531)
   (i32.add
    (get_local $0)
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
  (local $2 i64)
  (local $3 i32)
  (i32.store offset=4
   (i32.const 0)
   (tee_local $3
    (i32.sub
     (i32.load offset=4
      (i32.const 0)
     )
     (i32.const 48)
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
    (get_local $3)
    (i64.const 0)
   )
   (drop
    (call $readMessage
     (i32.add
      (get_local $3)
      (i32.const 8)
     )
     (i32.const 24)
    )
   )
   (set_local $0
    (i64.load offset=8
     (get_local $3)
    )
   )
   (call $requireNotice
    (i64.load offset=16
     (get_local $3)
    )
   )
   (call $requireNotice
    (get_local $0)
   )
   (call $requireAuth
    (i64.load offset=8
     (get_local $3)
    )
   )
   (set_local $0
    (call $_ZN8currency10getAccountEy
     (i64.load offset=8
      (get_local $3)
     )
    )
   )
   (set_local $1
    (call $_ZN8currency10getAccountEy
     (i64.load offset=16
      (get_local $3)
     )
    )
   )
   (call $assert
    (i64.ge_u
     (get_local $0)
     (i64.load offset=24
      (get_local $3)
     )
    )
    (i32.const 16)
   )
   (i64.store offset=40
    (get_local $3)
    (i64.sub
     (get_local $0)
     (tee_local $2
      (i64.load offset=24
       (get_local $3)
      )
     )
    )
   )
   (call $assert
    (i64.ge_u
     (i64.add
      (get_local $2)
      (get_local $1)
     )
     (get_local $2)
    )
    (i32.const 64)
   )
   (i64.store offset=32
    (get_local $3)
    (i64.add
     (get_local $1)
     (i64.load offset=24
      (get_local $3)
     )
    )
   )
   (call $_ZN8currency12storeAccountEyRKNS_7AccountE
    (i64.load offset=8
     (get_local $3)
    )
    (i32.add
     (get_local $3)
     (i32.const 40)
    )
   )
   (call $_ZN8currency12storeAccountEyRKNS_7AccountE
    (i64.load offset=16
     (get_local $3)
    )
    (i32.add
     (get_local $3)
     (i32.const 32)
    )
   )
  )
  (i32.store offset=4
   (i32.const 0)
   (i32.add
    (get_local $3)
    (i32.const 48)
   )
  )
 )
)
)=====";
