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
 (data (i32.const 4) "P\04\00\00")
 (data (i32.const 48) "insufficient funds\00")
 (export "memory" (memory $0))
 (export "_Z23apply_currency_transferv" (func $_Z23apply_currency_transferv))
 (export "init" (func $init))
 (export "apply" (func $apply))
 (func $_Z23apply_currency_transferv
  (local $0 i32)
  (local $1 i32)
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
     (i32.const 112)
    )
   )
  )
  (call $_Z14currentMessageI8TransferET_v
   (i32.add
    (get_local $4)
    (i32.const 80)
   )
  )
  (i32.store offset=108
   (get_local $4)
   (i32.add
    (get_local $4)
    (i32.const 80)
   )
  )
  (call $_Z13requireNoticeIJyEEvyDpT_
   (i64.load offset=88
    (get_local $4)
   )
   (i64.load offset=80
    (get_local $4)
   )
  )
  (call $requireAuth
   (i64.load
    (i32.load offset=108
     (get_local $4)
    )
   )
  )
  (block $label$0
   (br_if $label$0
    (i32.load8_u offset=24
     (i32.const 0)
    )
   )
   (drop
    (call $_ZN15CurrencyAccountC2Ey
     (i32.const 16)
     (i64.const 0)
    )
   )
   (i32.store8 offset=24
    (i32.const 0)
    (i32.const 1)
   )
  )
  (block $label$1
   (br_if $label$1
    (i32.load8_u offset=40
     (i32.const 0)
    )
   )
   (drop
    (call $_ZN15CurrencyAccountC2Ey
     (i32.const 32)
     (i64.const 0)
    )
   )
   (i32.store8 offset=40
    (i32.const 0)
    (i32.const 1)
   )
  )
  (set_local $0
   (call $_ZN4NameC2Ey
    (i32.add
     (get_local $4)
     (i32.const 72)
    )
    (i64.load
     (i32.load offset=108
      (get_local $4)
     )
    )
   )
  )
  (set_local $1
   (call $_ZN4NameC2Ey
    (i32.add
     (get_local $4)
     (i32.const 64)
    )
    (call $_ZN9ConstNameILy21967113313EE5valueEv)
   )
  )
  (drop
   (call $_ZN2Db3getI15CurrencyAccountEEb4NameS2_RT_
    (i64.load
     (get_local $0)
    )
    (i64.load
     (get_local $1)
    )
    (i32.const 16)
   )
  )
  (set_local $0
   (call $_ZN4NameC2Ey
    (i32.add
     (get_local $4)
     (i32.const 56)
    )
    (i64.load offset=8
     (i32.load offset=108
      (get_local $4)
     )
    )
   )
  )
  (set_local $1
   (call $_ZN4NameC2Ey
    (i32.add
     (get_local $4)
     (i32.const 48)
    )
    (call $_ZN9ConstNameILy21967113313EE5valueEv)
   )
  )
  (drop
   (call $_ZN2Db3getI15CurrencyAccountEEb4NameS2_RT_
    (i64.load
     (get_local $0)
    )
    (i64.load
     (get_local $1)
    )
    (i32.const 32)
   )
  )
  (call $assert
   (i64.ge_u
    (i64.load offset=16
     (i32.const 0)
    )
    (i64.load offset=16
     (i32.load offset=108
      (get_local $4)
     )
    )
   )
   (i32.const 48)
  )
  (i64.store offset=16
   (i32.const 0)
   (tee_local $3
    (i64.sub
     (i64.load offset=16
      (i32.const 0)
     )
     (tee_local $2
      (i64.load offset=16
       (i32.load offset=108
        (get_local $4)
       )
      )
     )
    )
   )
  )
  (i64.store offset=32
   (i32.const 0)
   (i64.add
    (get_local $2)
    (i64.load offset=32
     (i32.const 0)
    )
   )
  )
  (block $label$2
   (block $label$3
    (br_if $label$3
     (i64.eq
      (get_local $3)
      (i64.const 0)
     )
    )
    (set_local $0
     (call $_ZN4NameC2Ey
      (i32.add
       (get_local $4)
       (i32.const 24)
      )
      (i64.load
       (i32.load offset=108
        (get_local $4)
       )
      )
     )
    )
    (set_local $1
     (call $_ZN4NameC2Ey
      (i32.add
       (get_local $4)
       (i32.const 16)
      )
      (call $_ZN9ConstNameILy21967113313EE5valueEv)
     )
    )
    (drop
     (call $_ZN2Db5storeI15CurrencyAccountEEl4NameS2_RKT_
      (i64.load
       (get_local $0)
      )
      (i64.load
       (get_local $1)
      )
      (i32.const 16)
     )
    )
    (br $label$2)
   )
   (set_local $0
    (call $_ZN4NameC2Ey
     (i32.add
      (get_local $4)
      (i32.const 40)
     )
     (i64.load
      (i32.load offset=108
       (get_local $4)
      )
     )
    )
   )
   (set_local $1
    (call $_ZN4NameC2Ey
     (i32.add
      (get_local $4)
      (i32.const 32)
     )
     (call $_ZN9ConstNameILy21967113313EE5valueEv)
    )
   )
   (drop
    (call $_ZN2Db6removeI15CurrencyAccountEEb4NameS2_
     (i64.load
      (get_local $0)
     )
     (i64.load
      (get_local $1)
     )
    )
   )
  )
  (set_local $0
   (call $_ZN4NameC2Ey
    (i32.add
     (get_local $4)
     (i32.const 8)
    )
    (i64.load offset=8
     (i32.load offset=108
      (get_local $4)
     )
    )
   )
  )
  (set_local $1
   (call $_ZN4NameC2Ey
    (get_local $4)
    (call $_ZN9ConstNameILy21967113313EE5valueEv)
   )
  )
  (drop
   (call $_ZN2Db5storeI15CurrencyAccountEEl4NameS2_RKT_
    (i64.load
     (get_local $0)
    )
    (i64.load
     (get_local $1)
    )
    (i32.const 32)
   )
  )
  (i32.store offset=4
   (i32.const 0)
   (i32.add
    (get_local $4)
    (i32.const 112)
   )
  )
 )
 (func $_Z14currentMessageI8TransferET_v (param $0 i32)
  (drop
   (call $readMessage
    (call $_ZN8TransferC2Ev
     (get_local $0)
    )
    (i32.const 24)
   )
  )
 )
 (func $_Z13requireNoticeIJyEEvyDpT_ (param $0 i64) (param $1 i64)
  (local $2 i32)
  (i32.store offset=4
   (i32.const 0)
   (tee_local $2
    (i32.sub
     (i32.load offset=4
      (i32.const 0)
     )
     (i32.const 16)
    )
   )
  )
  (i64.store offset=8
   (get_local $2)
   (get_local $0)
  )
  (i64.store
   (get_local $2)
   (get_local $1)
  )
  (call $requireNotice
   (i64.load offset=8
    (get_local $2)
   )
  )
  (call $requireNotice
   (i64.load
    (get_local $2)
   )
  )
  (i32.store offset=4
   (i32.const 0)
   (i32.add
    (get_local $2)
    (i32.const 16)
   )
  )
 )
 (func $_ZN15CurrencyAccountC2Ey (param $0 i32) (param $1 i64) (result i32)
  (local $2 i32)
  (i32.store offset=12
   (tee_local $2
    (i32.sub
     (i32.load offset=4
      (i32.const 0)
     )
     (i32.const 16)
    )
   )
   (get_local $0)
  )
  (i64.store
   (get_local $2)
   (get_local $1)
  )
  (i64.store
   (tee_local $2
    (i32.load offset=12
     (get_local $2)
    )
   )
   (get_local $1)
  )
  (get_local $2)
 )
 (func $_ZN2Db3getI15CurrencyAccountEEb4NameS2_RT_ (param $0 i64) (param $1 i64) (param $2 i32) (result i32)
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
  (i64.store offset=40
   (get_local $3)
   (get_local $0)
  )
  (i64.store offset=32
   (get_local $3)
   (get_local $1)
  )
  (i32.store offset=28
   (get_local $3)
   (get_local $2)
  )
  (i32.store offset=20
   (get_local $3)
   (i32.load offset=44
    (get_local $3)
   )
  )
  (i32.store offset=16
   (get_local $3)
   (i32.load offset=40
    (get_local $3)
   )
  )
  (set_local $2
   (call $_ZN4NameC2Ey
    (i32.add
     (get_local $3)
     (i32.const 8)
    )
    (call $currentCode)
   )
  )
  (i64.store
   (get_local $3)
   (tee_local $1
    (i64.load offset=32
     (get_local $3)
    )
   )
  )
  (set_local $2
   (call $_ZN2Db3getI15CurrencyAccountEEb4NameS2_S2_RT_
    (i64.load offset=16
     (get_local $3)
    )
    (i64.load
     (get_local $2)
    )
    (get_local $1)
    (i32.load offset=28
     (get_local $3)
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
  (get_local $2)
 )
 (func $_ZN4NameC2Ey (param $0 i32) (param $1 i64) (result i32)
  (local $2 i32)
  (i32.store offset=12
   (tee_local $2
    (i32.sub
     (i32.load offset=4
      (i32.const 0)
     )
     (i32.const 16)
    )
   )
   (get_local $0)
  )
  (i64.store
   (get_local $2)
   (get_local $1)
  )
  (i64.store
   (tee_local $2
    (i32.load offset=12
     (get_local $2)
    )
   )
   (get_local $1)
  )
  (get_local $2)
 )
 (func $_ZN9ConstNameILy21967113313EE5valueEv (result i64)
  (i64.const 21967113313)
 )
 (func $_ZN2Db6removeI15CurrencyAccountEEb4NameS2_ (param $0 i64) (param $1 i64) (result i32)
  (local $2 i32)
  (local $3 i32)
  (i32.store offset=4
   (i32.const 0)
   (tee_local $3
    (i32.sub
     (i32.load offset=4
      (i32.const 0)
     )
     (i32.const 32)
    )
   )
  )
  (i64.store offset=24
   (get_local $3)
   (get_local $0)
  )
  (i64.store offset=16
   (get_local $3)
   (get_local $1)
  )
  (set_local $1
   (call $_ZNK4NamecvyEv
    (i32.add
     (get_local $3)
     (i32.const 24)
    )
   )
  )
  (i64.store offset=8
   (get_local $3)
   (call $_ZN15CurrencyAccount7tableIdEv)
  )
  (set_local $2
   (call $remove_i64
    (get_local $1)
    (call $_ZNK4NamecvyEv
     (i32.add
      (get_local $3)
      (i32.const 8)
     )
    )
    (call $_ZNK4NamecvyEv
     (i32.add
      (get_local $3)
      (i32.const 16)
     )
    )
   )
  )
  (i32.store offset=4
   (i32.const 0)
   (i32.add
    (get_local $3)
    (i32.const 32)
   )
  )
  (i32.ne
   (get_local $2)
   (i32.const 0)
  )
 )
 (func $_ZN2Db5storeI15CurrencyAccountEEl4NameS2_RKT_ (param $0 i64) (param $1 i64) (param $2 i32) (result i32)
  (local $3 i32)
  (i32.store offset=4
   (i32.const 0)
   (tee_local $3
    (i32.sub
     (i32.load offset=4
      (i32.const 0)
     )
     (i32.const 32)
    )
   )
  )
  (i64.store offset=24
   (get_local $3)
   (get_local $0)
  )
  (i64.store offset=16
   (get_local $3)
   (get_local $1)
  )
  (i32.store offset=12
   (get_local $3)
   (get_local $2)
  )
  (set_local $1
   (call $_ZNK4NamecvyEv
    (i32.add
     (get_local $3)
     (i32.const 24)
    )
   )
  )
  (i64.store
   (get_local $3)
   (call $_ZN15CurrencyAccount7tableIdEv)
  )
  (set_local $2
   (call $store_i64
    (get_local $1)
    (call $_ZNK4NamecvyEv
     (get_local $3)
    )
    (call $_ZNK4NamecvyEv
     (i32.add
      (get_local $3)
      (i32.const 16)
     )
    )
    (i32.load offset=12
     (get_local $3)
    )
    (i32.const 8)
   )
  )
  (i32.store offset=4
   (i32.const 0)
   (i32.add
    (get_local $3)
    (i32.const 32)
   )
  )
  (get_local $2)
 )
 (func $init
  (local $0 i32)
  (local $1 i32)
  (local $2 i32)
  (local $3 i32)
  (i32.store offset=4
   (i32.const 0)
   (tee_local $3
    (i32.sub
     (i32.load offset=4
      (i32.const 0)
     )
     (i32.const 32)
    )
   )
  )
  (set_local $0
   (call $_ZN4NameC2Ey
    (i32.add
     (get_local $3)
     (i32.const 24)
    )
    (call $_ZN9ConstNameILy862690298531EE5valueEv)
   )
  )
  (set_local $1
   (call $_ZN4NameC2Ey
    (i32.add
     (get_local $3)
     (i32.const 16)
    )
    (call $_ZN9ConstNameILy21967113313EE5valueEv)
   )
  )
  (set_local $2
   (call $_ZN15CurrencyAccountC2Ey
    (i32.add
     (get_local $3)
     (i32.const 8)
    )
    (i64.const 1000000000)
   )
  )
  (drop
   (call $_ZN2Db5storeI15CurrencyAccountEEl4NameS2_RKT_
    (i64.load
     (get_local $0)
    )
    (i64.load
     (get_local $1)
    )
    (get_local $2)
   )
  )
  (i32.store offset=4
   (i32.const 0)
   (i32.add
    (get_local $3)
    (i32.const 32)
   )
  )
 )
 (func $_ZN9ConstNameILy862690298531EE5valueEv (result i64)
  (i64.const 862690298531)
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
     (i32.const 16)
    )
   )
  )
  (i64.store offset=8
   (get_local $2)
   (get_local $0)
  )
  (i64.store
   (get_local $2)
   (get_local $1)
  )
  (block $label$0
   (br_if $label$0
    (i64.ne
     (i64.load offset=8
      (get_local $2)
     )
     (call $_ZN9ConstNameILy862690298531EE5valueEv)
    )
   )
   (br_if $label$0
    (i64.ne
     (i64.load
      (get_local $2)
     )
     (call $_ZN9ConstNameILy624065709652EE5valueEv)
    )
   )
   (call $_Z23apply_currency_transferv)
  )
  (i32.store offset=4
   (i32.const 0)
   (i32.add
    (get_local $2)
    (i32.const 16)
   )
  )
 )
 (func $_ZN9ConstNameILy624065709652EE5valueEv (result i64)
  (i64.const 624065709652)
 )
 (func $_ZN8TransferC2Ev (param $0 i32) (result i32)
  (local $1 i32)
  (set_local $1
   (i32.load offset=4
    (i32.const 0)
   )
  )
  (i64.store offset=16
   (get_local $0)
   (i64.const 0)
  )
  (i32.store offset=12
   (i32.sub
    (get_local $1)
    (i32.const 16)
   )
   (get_local $0)
  )
  (get_local $0)
 )
 (func $_ZN2Db3getI15CurrencyAccountEEb4NameS2_S2_RT_ (param $0 i64) (param $1 i64) (param $2 i64) (param $3 i32) (result i32)
  (local $4 i32)
  (i32.store offset=4
   (i32.const 0)
   (tee_local $4
    (i32.sub
     (i32.load offset=4
      (i32.const 0)
     )
     (i32.const 48)
    )
   )
  )
  (i64.store offset=40
   (get_local $4)
   (get_local $0)
  )
  (i64.store offset=32
   (get_local $4)
   (get_local $1)
  )
  (i64.store offset=24
   (get_local $4)
   (get_local $2)
  )
  (i32.store offset=20
   (get_local $4)
   (get_local $3)
  )
  (set_local $2
   (i64.load offset=32
    (get_local $4)
   )
  )
  (set_local $1
   (i64.load offset=40
    (get_local $4)
   )
  )
  (i64.store offset=8
   (get_local $4)
   (tee_local $0
    (call $_ZN15CurrencyAccount7tableIdEv)
   )
  )
  (i32.store offset=16
   (get_local $4)
   (tee_local $3
    (call $load_i64
     (get_local $1)
     (get_local $2)
     (get_local $0)
     (i64.load offset=24
      (get_local $4)
     )
     (i32.load offset=20
      (get_local $4)
     )
     (i32.const 8)
    )
   )
  )
  (i32.store offset=4
   (i32.const 0)
   (i32.add
    (get_local $4)
    (i32.const 48)
   )
  )
  (i32.gt_s
   (get_local $3)
   (i32.const 0)
  )
 )
 (func $_ZN15CurrencyAccount7tableIdEv (result i64)
  (local $0 i64)
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
  (set_local $0
   (i64.load
    (call $_ZN4NameC2Ey
     (i32.add
      (get_local $1)
      (i32.const 8)
     )
     (call $_ZN9ConstNameILy497826380083EE5valueEv)
    )
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
 (func $_ZN9ConstNameILy497826380083EE5valueEv (result i64)
  (i64.const 497826380083)
 )
 (func $_ZNK4NamecvyEv (param $0 i32) (result i64)
  (i32.store offset=12
   (i32.sub
    (i32.load offset=4
     (i32.const 0)
    )
    (i32.const 16)
   )
   (get_local $0)
  )
  (i64.load
   (get_local $0)
  )
 )
)
)=====";
