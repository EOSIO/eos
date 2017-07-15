const char* exchange_wast = R"=====(
(module
 (type $FUNCSIG$vj (func (param i64)))
 (type $FUNCSIG$vii (func (param i32 i32)))
 (type $FUNCSIG$i (func (result i32)))
 (type $FUNCSIG$ijjjjii (func (param i64 i64 i64 i64 i32 i32) (result i32)))
 (type $FUNCSIG$ijjj (func (param i64 i64 i64) (result i32)))
 (type $FUNCSIG$ijjjii (func (param i64 i64 i64 i32 i32) (result i32)))
 (type $FUNCSIG$vi (func (param i32)))
 (type $FUNCSIG$ijjii (func (param i64 i64 i32 i32) (result i32)))
 (type $FUNCSIG$ijji (func (param i64 i64 i32) (result i32)))
 (type $FUNCSIG$iii (func (param i32 i32) (result i32)))
 (type $FUNCSIG$iiii (func (param i32 i32 i32) (result i32)))
 (import "env" "assert" (func $assert (param i32 i32)))
 (import "env" "back_secondary_i128i128" (func $back_secondary_i128i128 (param i64 i64 i64 i32 i32) (result i32)))
 (import "env" "diveq_i128" (func $diveq_i128 (param i32 i32)))
 (import "env" "front_secondary_i128i128" (func $front_secondary_i128i128 (param i64 i64 i64 i32 i32) (result i32)))
 (import "env" "load_i64" (func $load_i64 (param i64 i64 i64 i64 i32 i32) (result i32)))
 (import "env" "load_primary_i128i128" (func $load_primary_i128i128 (param i64 i64 i64 i32 i32) (result i32)))
 (import "env" "memcpy" (func $memcpy (param i32 i32 i32) (result i32)))
 (import "env" "multeq_i128" (func $multeq_i128 (param i32 i32)))
 (import "env" "now" (func $now (result i32)))
 (import "env" "printi" (func $printi (param i64)))
 (import "env" "printi128" (func $printi128 (param i32)))
 (import "env" "printn" (func $printn (param i64)))
 (import "env" "prints" (func $prints (param i32)))
 (import "env" "readMessage" (func $readMessage (param i32 i32) (result i32)))
 (import "env" "remove_i128i128" (func $remove_i128i128 (param i64 i64 i32) (result i32)))
 (import "env" "remove_i64" (func $remove_i64 (param i64 i64 i64) (result i32)))
 (import "env" "requireAuth" (func $requireAuth (param i64)))
 (import "env" "store_i128i128" (func $store_i128i128 (param i64 i64 i32 i32) (result i32)))
 (import "env" "store_i64" (func $store_i64 (param i64 i64 i64 i32 i32) (result i32)))
 (table 0 anyfunc)
 (memory $0 1)
 (data (i32.const 4) "0\08\00\00")
 (data (i32.const 16) "notified on transfer that is not relevant to this exchange\00")
 (data (i32.const 80) "\n\nmatch bid: \00")
 (data (i32.const 96) ":\00")
 (data (i32.const 112) "match ask: \00")
 (data (i32.const 128) "\n\n\00")
 (data (i32.const 144) "invalid quantity\00")
 (data (i32.const 176) "order expired\00")
 (data (i32.const 192) " created bid for \00")
 (data (i32.const 224) " currency at price: \00")
 (data (i32.const 256) "\n\00")
 (data (i32.const 272) "order with this id already exists\00")
 (data (i32.const 320) "exchange.cpp\00")
 (data (i32.const 336) "\n No asks found, saving buyer account and storing bid\n\00")
 (data (i32.const 400) "order not completely filled\00")
 (data (i32.const 432) "asks found, lets see what matches\n\00")
 (data (i32.const 480) "lowest ask <= bid.price\n\00")
 (data (i32.const 512) "lowest_ask >= bid.price or buyer\'s bid has been filled\n\00")
 (data (i32.const 576) "saving buyer\'s account\n\00")
 (data (i32.const 608) " eos left over\00")
 (data (i32.const 624) "bid filled\n\00")
 (data (i32.const 640) " created sell for \00")
 (data (i32.const 672) "\n No bids found, saving seller account and storing ask\n\00")
 (data (i32.const 736) "\n bids found, lets see what matches\n\00")
 (data (i32.const 784) "saving ask\n\00")
 (data (i32.const 800) "unknown action\00")
 (data (i32.const 816) "remove\00")
 (data (i32.const 832) "store\00")
 (data (i32.const 848) "integer overflow adding token balance\00")
 (data (i32.const 896) "integer underflow subtracting token balance\00")
 (data (i32.const 944) "cast to 64 bit loss of precision\00")
 (data (i32.const 992) " \00")
 (data (i32.const 1008) ".\00")
 (data (i32.const 1024) "/\00")
 (data (i32.const 1040) "error storing record\00")
 (export "memory" (memory $0))
 (export "_ZN8exchange23apply_currency_transferERKN8currency8TransferE" (func $_ZN8exchange23apply_currency_transferERKN8currency8TransferE))
 (export "_ZN8exchange18apply_eos_transferERKN3eos8TransferE" (func $_ZN8exchange18apply_eos_transferERKN3eos8TransferE))
 (export "_ZN8exchange5matchERNS_3BidERNS_7AccountERNS_3AskES3_" (func $_ZN8exchange5matchERNS_3BidERNS_7AccountERNS_3AskES3_))
 (export "_ZN8exchange18apply_exchange_buyENS_8BuyOrderE" (func $_ZN8exchange18apply_exchange_buyENS_8BuyOrderE))
 (export "_ZN8exchange19apply_exchange_sellENS_9SellOrderE" (func $_ZN8exchange19apply_exchange_sellENS_9SellOrderE))
 (export "init" (func $init))
 (export "apply" (func $apply))
 (func $_ZN8exchange23apply_currency_transferERKN8currency8TransferE (param $0 i32)
  (local $1 i64)
  (local $2 i64)
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
  (set_local $2
   (i64.load
    (get_local $0)
   )
  )
  (block $label$0
   (block $label$1
    (br_if $label$1
     (i64.ne
      (tee_local $1
       (i64.load offset=8
        (get_local $0)
       )
      )
      (i64.const 179785961221)
     )
    )
    (call $_ZN8exchange10getAccountEy
     (get_local $3)
     (get_local $2)
    )
    (call $assert
     (i64.ge_u
      (i64.add
       (tee_local $2
        (i64.load offset=16
         (get_local $0)
        )
       )
       (i64.load offset=16
        (get_local $3)
       )
      )
      (get_local $2)
     )
     (i32.const 848)
    )
    (i64.store offset=16
     (get_local $3)
     (i64.add
      (i64.load offset=16
       (get_local $3)
      )
      (i64.load offset=16
       (get_local $0)
      )
     )
    )
    (call $_ZN8exchange4saveERKNS_7AccountE
     (get_local $3)
    )
    (br $label$0)
   )
   (block $label$2
    (br_if $label$2
     (i64.ne
      (get_local $2)
      (i64.const 179785961221)
     )
    )
    (call $requireAuth
     (get_local $1)
    )
    (call $_ZN8exchange10getAccountEy
     (get_local $3)
     (i64.load
      (i32.add
       (get_local $0)
       (i32.const 8)
      )
     )
    )
    (call $assert
     (i64.ge_u
      (i64.load offset=16
       (get_local $3)
      )
      (i64.load offset=16
       (get_local $0)
      )
     )
     (i32.const 896)
    )
    (i64.store offset=16
     (get_local $3)
     (i64.sub
      (i64.load offset=16
       (get_local $3)
      )
      (i64.load offset=16
       (get_local $0)
      )
     )
    )
    (call $_ZN8exchange4saveERKNS_7AccountE
     (get_local $3)
    )
    (br $label$0)
   )
   (call $assert
    (i32.const 0)
    (i32.const 16)
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
 (func $_ZN8exchange18apply_eos_transferERKN3eos8TransferE (param $0 i32)
  (local $1 i64)
  (local $2 i64)
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
  (set_local $2
   (i64.load
    (get_local $0)
   )
  )
  (block $label$0
   (block $label$1
    (br_if $label$1
     (i64.ne
      (tee_local $1
       (i64.load offset=8
        (get_local $0)
       )
      )
      (i64.const 179785961221)
     )
    )
    (call $_ZN8exchange10getAccountEy
     (get_local $3)
     (get_local $2)
    )
    (call $assert
     (i64.ge_u
      (i64.add
       (tee_local $2
        (i64.load offset=16
         (get_local $0)
        )
       )
       (i64.load offset=8
        (get_local $3)
       )
      )
      (get_local $2)
     )
     (i32.const 848)
    )
    (i64.store offset=8
     (get_local $3)
     (i64.add
      (i64.load offset=8
       (get_local $3)
      )
      (i64.load offset=16
       (get_local $0)
      )
     )
    )
    (call $_ZN8exchange4saveERKNS_7AccountE
     (get_local $3)
    )
    (br $label$0)
   )
   (block $label$2
    (br_if $label$2
     (i64.ne
      (get_local $2)
      (i64.const 179785961221)
     )
    )
    (call $requireAuth
     (get_local $1)
    )
    (call $_ZN8exchange10getAccountEy
     (get_local $3)
     (i64.load
      (i32.add
       (get_local $0)
       (i32.const 8)
      )
     )
    )
    (call $assert
     (i64.ge_u
      (i64.load offset=8
       (get_local $3)
      )
      (i64.load offset=16
       (get_local $0)
      )
     )
     (i32.const 896)
    )
    (i64.store offset=8
     (get_local $3)
     (i64.sub
      (i64.load offset=8
       (get_local $3)
      )
      (i64.load offset=16
       (get_local $0)
      )
     )
    )
    (call $_ZN8exchange4saveERKNS_7AccountE
     (get_local $3)
    )
    (br $label$0)
   )
   (call $assert
    (i32.const 0)
    (i32.const 16)
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
 (func $_ZN8exchange5matchERNS_3BidERNS_7AccountERNS_3AskES3_ (param $0 i32) (param $1 i32) (param $2 i32) (param $3 i32)
  (local $4 i32)
  (local $5 i64)
  (local $6 i64)
  (local $7 i64)
  (local $8 i64)
  (local $9 i64)
  (local $10 i64)
  (local $11 i32)
  (i32.store offset=4
   (i32.const 0)
   (tee_local $11
    (i32.sub
     (i32.load offset=4
      (i32.const 0)
     )
     (i32.const 48)
    )
   )
  )
  (set_local $5
   (i64.load offset=32
    (get_local $2)
   )
  )
  (i64.store offset=40
   (get_local $11)
   (i64.const 0)
  )
  (i64.store offset=32
   (get_local $11)
   (get_local $5)
  )
  (call $multeq_i128
   (i32.add
    (get_local $11)
    (i32.const 32)
   )
   (tee_local $4
    (i32.add
     (get_local $2)
     (i32.const 16)
    )
   )
  )
  (set_local $5
   (i64.load offset=32
    (get_local $11)
   )
  )
  (set_local $10
   (i64.load offset=40
    (get_local $11)
   )
  )
  (i64.store offset=24
   (get_local $11)
   (i64.const 0)
  )
  (i64.store offset=16
   (get_local $11)
   (i64.const 1000000000000000)
  )
  (i64.store offset=40
   (get_local $11)
   (get_local $10)
  )
  (i64.store offset=32
   (get_local $11)
   (get_local $5)
  )
  (call $diveq_i128
   (i32.add
    (get_local $11)
    (i32.const 32)
   )
   (i32.add
    (get_local $11)
    (i32.const 16)
   )
  )
  (set_local $10
   (i64.load offset=32
    (get_local $11)
   )
  )
  (call $assert
   (i64.eqz
    (i64.load offset=40
     (get_local $11)
    )
   )
   (i32.const 944)
  )
  (i64.store offset=8
   (get_local $11)
   (get_local $10)
  )
  (block $label$0
   (block $label$1
    (br_if $label$1
     (i64.ne
      (get_local $10)
      (tee_local $5
       (i64.load
        (select
         (i32.add
          (get_local $11)
          (i32.const 8)
         )
         (i32.add
          (get_local $0)
          (i32.const 32)
         )
         (i64.lt_u
          (get_local $10)
          (i64.load offset=32
           (get_local $0)
          )
         )
        )
       )
      )
     )
    )
    (set_local $10
     (i64.load
      (i32.add
       (get_local $2)
       (i32.const 32)
      )
     )
    )
    (br $label$0)
   )
   (i64.store offset=24
    (get_local $11)
    (i64.const 0)
   )
   (i64.store offset=16
    (get_local $11)
    (i64.const 1000000000000000)
   )
   (i64.store offset=40
    (get_local $11)
    (i64.const 0)
   )
   (i64.store offset=32
    (get_local $11)
    (get_local $5)
   )
   (call $multeq_i128
    (i32.add
     (get_local $11)
     (i32.const 32)
    )
    (i32.add
     (get_local $11)
     (i32.const 16)
    )
   )
   (set_local $10
    (i64.load offset=32
     (get_local $11)
    )
   )
   (i64.store offset=40
    (get_local $11)
    (i64.load offset=40
     (get_local $11)
    )
   )
   (i64.store offset=32
    (get_local $11)
    (get_local $10)
   )
   (call $diveq_i128
    (i32.add
     (get_local $11)
     (i32.const 32)
    )
    (get_local $4)
   )
   (set_local $10
    (i64.load offset=32
     (get_local $11)
    )
   )
   (call $assert
    (i64.eqz
     (i64.load offset=40
      (get_local $11)
     )
    )
    (i32.const 944)
   )
  )
  (set_local $6
   (i64.load offset=8
    (get_local $2)
   )
  )
  (set_local $7
   (i64.load
    (get_local $2)
   )
  )
  (set_local $8
   (i64.load offset=8
    (get_local $0)
   )
  )
  (set_local $9
   (i64.load
    (get_local $0)
   )
  )
  (call $prints
   (i32.const 80)
  )
  (call $printn
   (get_local $9)
  )
  (call $prints
   (i32.const 96)
  )
  (call $printi
   (get_local $8)
  )
  (call $prints
   (i32.const 112)
  )
  (call $printn
   (get_local $7)
  )
  (call $prints
   (i32.const 96)
  )
  (call $printi
   (get_local $6)
  )
  (call $prints
   (i32.const 128)
  )
  (call $assert
   (i64.ge_u
    (i64.load
     (tee_local $0
      (i32.add
       (get_local $0)
       (i32.const 32)
      )
     )
    )
    (get_local $5)
   )
   (i32.const 896)
  )
  (i64.store
   (get_local $0)
   (i64.sub
    (i64.load
     (get_local $0)
    )
    (get_local $5)
   )
  )
  (call $assert
   (i64.ge_u
    (i64.add
     (i64.load offset=8
      (get_local $3)
     )
     (get_local $5)
    )
    (get_local $5)
   )
   (i32.const 848)
  )
  (i64.store offset=8
   (get_local $3)
   (i64.add
    (i64.load offset=8
     (get_local $3)
    )
    (get_local $5)
   )
  )
  (call $assert
   (i64.ge_u
    (i64.load
     (tee_local $2
      (i32.add
       (get_local $2)
       (i32.const 32)
      )
     )
    )
    (get_local $10)
   )
   (i32.const 896)
  )
  (i64.store
   (get_local $2)
   (i64.sub
    (i64.load
     (get_local $2)
    )
    (get_local $10)
   )
  )
  (call $assert
   (i64.ge_u
    (i64.add
     (i64.load offset=16
      (get_local $1)
     )
     (get_local $10)
    )
    (get_local $10)
   )
   (i32.const 848)
  )
  (i64.store offset=16
   (get_local $1)
   (i64.add
    (i64.load offset=16
     (get_local $1)
    )
    (get_local $10)
   )
  )
  (i32.store offset=4
   (i32.const 0)
   (i32.add
    (get_local $11)
    (i32.const 48)
   )
  )
 )
 (func $_ZN8exchange18apply_exchange_buyENS_8BuyOrderE (param $0 i32)
  (local $1 i64)
  (local $2 i64)
  (local $3 i32)
  (local $4 i32)
  (local $5 i32)
  (local $6 i32)
  (local $7 i64)
  (local $8 i64)
  (local $9 i32)
  (i32.store offset=4
   (i32.const 0)
   (tee_local $9
    (i32.sub
     (i32.load offset=4
      (i32.const 0)
     )
     (i32.const 192)
    )
   )
  )
  (call $requireAuth
   (tee_local $7
    (i64.load
     (get_local $0)
    )
   )
  )
  (call $assert
   (i64.ne
    (tee_local $8
     (i64.load offset=32
      (get_local $0)
     )
    )
    (i64.const 0)
   )
   (i32.const 144)
  )
  (call $assert
   (i32.gt_u
    (i32.load offset=40
     (get_local $0)
    )
    (call $now)
   )
   (i32.const 176)
  )
  (set_local $1
   (i64.load
    (tee_local $6
     (i32.add
      (get_local $0)
      (i32.const 24)
     )
    )
   )
  )
  (set_local $2
   (i64.load offset=16
    (get_local $0)
   )
  )
  (call $printn
   (get_local $7)
  )
  (call $_ZN3eos5printIPKcJNS_5tokenIyLy19941EEES2_NS_5priceIS4_NS3_IyLy862690298531EEEEES2_EEEvT_DpT0_
   (i32.const 192)
   (get_local $8)
   (i32.const 224)
   (get_local $2)
   (get_local $1)
   (i32.const 256)
  )
  (i64.store
   (i32.add
    (i32.add
     (get_local $9)
     (i32.const 144)
    )
    (i32.const 24)
   )
   (i64.const 0)
  )
  (i64.store offset=160
   (get_local $9)
   (i64.const 1)
  )
  (i64.store offset=176
   (get_local $9)
   (i64.const 0)
  )
  (i32.store offset=156
   (get_local $9)
   (i32.load
    (i32.add
     (get_local $0)
     (i32.const 12)
    )
   )
  )
  (i32.store offset=152
   (get_local $9)
   (i32.load
    (i32.add
     (get_local $0)
     (i32.const 8)
    )
   )
  )
  (i32.store offset=148
   (get_local $9)
   (i32.load
    (i32.add
     (get_local $0)
     (i32.const 4)
    )
   )
  )
  (i32.store offset=144
   (get_local $9)
   (i32.load
    (get_local $0)
   )
  )
  (call $assert
   (i32.ne
    (call $load_primary_i128i128
     (i64.const 179785961221)
     (i64.const 179785961221)
     (i64.const 626978)
     (i32.add
      (get_local $9)
      (i32.const 144)
     )
     (i32.const 48)
    )
    (i32.const 48)
   )
   (i32.const 272)
  )
  (call $prints
   (i32.const 320)
  )
  (call $printi
   (i64.const 139)
  )
  (call $prints
   (i32.const 256)
  )
  (call $_ZN8exchange10getAccountEy
   (i32.add
    (get_local $9)
    (i32.const 112)
   )
   (get_local $7)
  )
  (call $assert
   (i64.ge_u
    (i64.load offset=120
     (get_local $9)
    )
    (get_local $8)
   )
   (i32.const 896)
  )
  (i64.store offset=120
   (get_local $9)
   (i64.sub
    (i64.load offset=120
     (get_local $9)
    )
    (get_local $8)
   )
  )
  (i64.store
   (tee_local $3
    (i32.add
     (i32.add
      (get_local $9)
      (i32.const 64)
     )
     (i32.const 24)
    )
   )
   (i64.const 0)
  )
  (i64.store offset=80
   (get_local $9)
   (i64.const 1)
  )
  (i64.store offset=72
   (get_local $9)
   (i64.const 0)
  )
  (i64.store offset=64
   (get_local $9)
   (i64.const 0)
  )
  (i64.store offset=96
   (get_local $9)
   (i64.const 0)
  )
  (block $label$0
   (block $label$1
    (block $label$2
     (br_if $label$2
      (i32.ne
       (call $front_secondary_i128i128
        (i64.const 179785961221)
        (i64.const 179785961221)
        (i64.const 626978)
        (i32.add
         (get_local $9)
         (i32.const 64)
        )
        (i32.const 48)
       )
       (i32.const 48)
      )
     )
     (call $prints
      (i32.const 432)
     )
     (call $_ZN8exchange10getAccountEy
      (i32.add
       (get_local $9)
       (i32.const 32)
      )
      (i64.load offset=64
       (get_local $9)
      )
     )
     (block $label$3
      (br_if $label$3
       (select
        (i64.gt_u
         (i64.load
          (tee_local $5
           (i32.add
            (i32.add
             (get_local $9)
             (i32.const 64)
            )
            (i32.const 16)
           )
          )
         )
         (i64.load
          (tee_local $4
           (i32.add
            (get_local $0)
            (i32.const 16)
           )
          )
         )
        )
        (i64.gt_u
         (tee_local $8
          (i64.load
           (get_local $3)
          )
         )
         (tee_local $7
          (i64.load
           (get_local $6)
          )
         )
        )
        (i64.eq
         (get_local $8)
         (get_local $7)
        )
       )
      )
      (set_local $6
       (i32.add
        (get_local $9)
        (i32.const 96)
       )
      )
      (loop $label$4
       (call $prints
        (i32.const 480)
       )
       (call $_ZN8exchange5matchERNS_3BidERNS_7AccountERNS_3AskES3_
        (get_local $0)
        (i32.add
         (get_local $9)
         (i32.const 112)
        )
        (i32.add
         (get_local $9)
         (i32.const 64)
        )
        (i32.add
         (get_local $9)
         (i32.const 32)
        )
       )
       (br_if $label$3
        (i64.ne
         (i64.load
          (get_local $6)
         )
         (i64.const 0)
        )
       )
       (call $_ZN8exchange4saveERKNS_7AccountE
        (i32.add
         (get_local $9)
         (i32.const 32)
        )
       )
       (call $_ZN8exchange4saveERKNS_7AccountE
        (i32.add
         (get_local $9)
         (i32.const 112)
        )
       )
       (drop
        (call $remove_i128i128
         (i64.const 179785961221)
         (i64.const 626978)
         (i32.add
          (get_local $9)
          (i32.const 64)
         )
        )
       )
       (br_if $label$3
        (i32.ne
         (call $front_secondary_i128i128
          (i64.const 179785961221)
          (i64.const 179785961221)
          (i64.const 626978)
          (i32.add
           (get_local $9)
           (i32.const 64)
          )
          (i32.const 48)
         )
         (i32.const 48)
        )
       )
       (call $_ZN8exchange10getAccountEy
        (get_local $9)
        (i64.load offset=64
         (get_local $9)
        )
       )
       (i32.store
        (i32.add
         (i32.add
          (get_local $9)
          (i32.const 32)
         )
         (i32.const 24)
        )
        (i32.load
         (i32.add
          (get_local $9)
          (i32.const 24)
         )
        )
       )
       (i64.store
        (i32.add
         (i32.add
          (get_local $9)
          (i32.const 32)
         )
         (i32.const 16)
        )
        (i64.load
         (i32.add
          (get_local $9)
          (i32.const 16)
         )
        )
       )
       (i64.store
        (i32.add
         (i32.add
          (get_local $9)
          (i32.const 32)
         )
         (i32.const 8)
        )
        (i64.load
         (i32.add
          (get_local $9)
          (i32.const 8)
         )
        )
       )
       (i64.store offset=32
        (get_local $9)
        (i64.load
         (get_local $9)
        )
       )
       (br_if $label$4
        (select
         (i64.le_u
          (i64.load
           (get_local $5)
          )
          (i64.load
           (get_local $4)
          )
         )
         (i64.le_u
          (tee_local $8
           (i64.load
            (i32.add
             (i32.add
              (get_local $9)
              (i32.const 64)
             )
             (i32.const 24)
            )
           )
          )
          (tee_local $7
           (i64.load
            (i32.add
             (get_local $0)
             (i32.const 24)
            )
           )
          )
         )
         (i64.eq
          (get_local $8)
          (get_local $7)
         )
        )
       )
      )
     )
     (call $prints
      (i32.const 512)
     )
     (call $_ZN8exchange4saveERKNS_7AccountE
      (i32.add
       (get_local $9)
       (i32.const 112)
      )
     )
     (call $prints
      (i32.const 576)
     )
     (br_if $label$1
      (i64.eqz
       (tee_local $8
        (i64.load
         (i32.add
          (get_local $0)
          (i32.const 32)
         )
        )
       )
      )
     )
     (call $printi
      (get_local $8)
     )
     (call $prints
      (i32.const 992)
     )
     (call $printn
      (i64.const 19941)
     )
     (call $prints
      (i32.const 608)
     )
     (call $assert
      (i32.eqz
       (i32.load8_u offset=44
        (get_local $0)
       )
      )
      (i32.const 400)
     )
     (call $assert
      (call $store_i128i128
       (i64.const 179785961221)
       (i64.const 626978)
       (get_local $0)
       (i32.const 48)
      )
      (i32.const 1040)
     )
     (br $label$0)
    )
    (call $prints
     (i32.const 336)
    )
    (call $assert
     (i32.eqz
      (i32.load8_u offset=44
       (get_local $0)
      )
     )
     (i32.const 400)
    )
    (call $assert
     (call $store_i128i128
      (i64.const 179785961221)
      (i64.const 626978)
      (get_local $0)
      (i32.const 48)
     )
     (i32.const 1040)
    )
    (call $_ZN8exchange4saveERKNS_7AccountE
     (i32.add
      (get_local $9)
      (i32.const 112)
     )
    )
    (br $label$0)
   )
   (call $prints
    (i32.const 624)
   )
  )
  (i32.store offset=4
   (i32.const 0)
   (i32.add
    (get_local $9)
    (i32.const 192)
   )
  )
 )
 (func $_ZN8exchange10getAccountEy (param $0 i32) (param $1 i64)
  (i32.store offset=8
   (get_local $0)
   (i32.const 0)
  )
  (i64.store
   (get_local $0)
   (get_local $1)
  )
  (i64.store align=4
   (i32.add
    (get_local $0)
    (i32.const 20)
   )
   (i64.const 0)
  )
  (i64.store align=4
   (i32.add
    (get_local $0)
    (i32.const 12)
   )
   (i64.const 0)
  )
  (drop
   (call $load_i64
    (i64.const 179785961221)
    (i64.const 179785961221)
    (i64.const 21967113313)
    (get_local $1)
    (get_local $0)
    (i32.const 32)
   )
  )
 )
 (func $_ZN8exchange4saveERKNS_7AccountE (param $0 i32)
  (block $label$0
   (br_if $label$0
    (i32.eqz
     (i32.or
      (i64.ne
       (i64.or
        (i64.load offset=16
         (get_local $0)
        )
        (i64.load offset=8
         (get_local $0)
        )
       )
       (i64.const 0)
      )
      (i32.load offset=24
       (get_local $0)
      )
     )
    )
   )
   (call $prints
    (i32.const 832)
   )
   (drop
    (call $store_i64
     (i64.const 179785961221)
     (i64.const 21967113313)
     (i64.load
      (get_local $0)
     )
     (get_local $0)
     (i32.const 32)
    )
   )
   (return)
  )
  (call $prints
   (i32.const 816)
  )
  (drop
   (call $remove_i64
    (i64.const 179785961221)
    (i64.const 21967113313)
    (i64.load
     (get_local $0)
    )
   )
  )
 )
 (func $_ZN8exchange19apply_exchange_sellENS_9SellOrderE (param $0 i32)
  (local $1 i64)
  (local $2 i64)
  (local $3 i32)
  (local $4 i32)
  (local $5 i32)
  (local $6 i32)
  (local $7 i64)
  (local $8 i64)
  (local $9 i32)
  (i32.store offset=4
   (i32.const 0)
   (tee_local $9
    (i32.sub
     (i32.load offset=4
      (i32.const 0)
     )
     (i32.const 192)
    )
   )
  )
  (call $requireAuth
   (tee_local $7
    (i64.load
     (get_local $0)
    )
   )
  )
  (call $assert
   (i64.ne
    (tee_local $8
     (i64.load offset=32
      (get_local $0)
     )
    )
    (i64.const 0)
   )
   (i32.const 144)
  )
  (call $assert
   (i32.gt_u
    (i32.load offset=40
     (get_local $0)
    )
    (call $now)
   )
   (i32.const 176)
  )
  (set_local $1
   (i64.load
    (tee_local $6
     (i32.add
      (get_local $0)
      (i32.const 24)
     )
    )
   )
  )
  (set_local $2
   (i64.load offset=16
    (get_local $0)
   )
  )
  (call $prints
   (i32.const 128)
  )
  (call $printn
   (get_local $7)
  )
  (call $_ZN3eos5printIPKcJNS_5tokenIyLy862690298531EEES2_NS_5priceINS3_IyLy19941EEES4_EES2_EEEvT_DpT0_
   (i32.const 640)
   (get_local $8)
   (i32.const 224)
   (get_local $2)
   (get_local $1)
   (i32.const 256)
  )
  (i64.store
   (i32.add
    (i32.add
     (get_local $9)
     (i32.const 144)
    )
    (i32.const 24)
   )
   (i64.const 0)
  )
  (i64.store offset=160
   (get_local $9)
   (i64.const 1)
  )
  (i64.store offset=176
   (get_local $9)
   (i64.const 0)
  )
  (i32.store offset=156
   (get_local $9)
   (i32.load
    (i32.add
     (get_local $0)
     (i32.const 12)
    )
   )
  )
  (i32.store offset=152
   (get_local $9)
   (i32.load
    (i32.add
     (get_local $0)
     (i32.const 8)
    )
   )
  )
  (i32.store offset=148
   (get_local $9)
   (i32.load
    (i32.add
     (get_local $0)
     (i32.const 4)
    )
   )
  )
  (i32.store offset=144
   (get_local $9)
   (i32.load
    (get_local $0)
   )
  )
  (call $assert
   (i32.ne
    (call $load_primary_i128i128
     (i64.const 179785961221)
     (i64.const 179785961221)
     (i64.const 626978)
     (i32.add
      (get_local $9)
      (i32.const 144)
     )
     (i32.const 48)
    )
    (i32.const 48)
   )
   (i32.const 272)
  )
  (call $_ZN8exchange10getAccountEy
   (i32.add
    (get_local $9)
    (i32.const 112)
   )
   (get_local $7)
  )
  (call $assert
   (i64.ge_u
    (i64.load offset=128
     (get_local $9)
    )
    (get_local $8)
   )
   (i32.const 896)
  )
  (i64.store offset=128
   (get_local $9)
   (i64.sub
    (i64.load offset=128
     (get_local $9)
    )
    (get_local $8)
   )
  )
  (i64.store
   (tee_local $3
    (i32.add
     (i32.add
      (get_local $9)
      (i32.const 64)
     )
     (i32.const 24)
    )
   )
   (i64.const 0)
  )
  (i64.store offset=80
   (get_local $9)
   (i64.const 1)
  )
  (i64.store offset=72
   (get_local $9)
   (i64.const 0)
  )
  (i64.store offset=64
   (get_local $9)
   (i64.const 0)
  )
  (i64.store offset=96
   (get_local $9)
   (i64.const 0)
  )
  (block $label$0
   (block $label$1
    (br_if $label$1
     (i32.ne
      (call $back_secondary_i128i128
       (i64.const 179785961221)
       (i64.const 179785961221)
       (i64.const 626978)
       (i32.add
        (get_local $9)
        (i32.const 64)
       )
       (i32.const 48)
      )
      (i32.const 48)
     )
    )
    (call $prints
     (i32.const 736)
    )
    (call $_ZN8exchange10getAccountEy
     (i32.add
      (get_local $9)
      (i32.const 32)
     )
     (i64.load offset=64
      (get_local $9)
     )
    )
    (block $label$2
     (br_if $label$2
      (select
       (i64.lt_u
        (i64.load
         (tee_local $5
          (i32.add
           (i32.add
            (get_local $9)
            (i32.const 64)
           )
           (i32.const 16)
          )
         )
        )
        (i64.load
         (tee_local $4
          (i32.add
           (get_local $0)
           (i32.const 16)
          )
         )
        )
       )
       (i64.lt_u
        (tee_local $8
         (i64.load
          (get_local $3)
         )
        )
        (tee_local $7
         (i64.load
          (get_local $6)
         )
        )
       )
       (i64.eq
        (get_local $8)
        (get_local $7)
       )
      )
     )
     (set_local $6
      (i32.add
       (get_local $9)
       (i32.const 96)
      )
     )
     (loop $label$3
      (call $_ZN8exchange5matchERNS_3BidERNS_7AccountERNS_3AskES3_
       (i32.add
        (get_local $9)
        (i32.const 64)
       )
       (i32.add
        (get_local $9)
        (i32.const 32)
       )
       (get_local $0)
       (i32.add
        (get_local $9)
        (i32.const 112)
       )
      )
      (br_if $label$2
       (i64.ne
        (i64.load
         (get_local $6)
        )
        (i64.const 0)
       )
      )
      (call $_ZN8exchange4saveERKNS_7AccountE
       (i32.add
        (get_local $9)
        (i32.const 112)
       )
      )
      (call $_ZN8exchange4saveERKNS_7AccountE
       (i32.add
        (get_local $9)
        (i32.const 32)
       )
      )
      (drop
       (call $remove_i128i128
        (i64.const 179785961221)
        (i64.const 626978)
        (i32.add
         (get_local $9)
         (i32.const 64)
        )
       )
      )
      (br_if $label$2
       (i32.ne
        (call $back_secondary_i128i128
         (i64.const 179785961221)
         (i64.const 179785961221)
         (i64.const 626978)
         (i32.add
          (get_local $9)
          (i32.const 64)
         )
         (i32.const 48)
        )
        (i32.const 48)
       )
      )
      (call $_ZN8exchange10getAccountEy
       (get_local $9)
       (i64.load offset=64
        (get_local $9)
       )
      )
      (i32.store
       (i32.add
        (i32.add
         (get_local $9)
         (i32.const 32)
        )
        (i32.const 24)
       )
       (i32.load
        (i32.add
         (get_local $9)
         (i32.const 24)
        )
       )
      )
      (i64.store
       (i32.add
        (i32.add
         (get_local $9)
         (i32.const 32)
        )
        (i32.const 16)
       )
       (i64.load
        (i32.add
         (get_local $9)
         (i32.const 16)
        )
       )
      )
      (i64.store
       (i32.add
        (i32.add
         (get_local $9)
         (i32.const 32)
        )
        (i32.const 8)
       )
       (i64.load
        (i32.add
         (get_local $9)
         (i32.const 8)
        )
       )
      )
      (i64.store offset=32
       (get_local $9)
       (i64.load
        (get_local $9)
       )
      )
      (br_if $label$3
       (select
        (i64.ge_u
         (i64.load
          (get_local $5)
         )
         (i64.load
          (get_local $4)
         )
        )
        (i64.ge_u
         (tee_local $8
          (i64.load
           (i32.add
            (i32.add
             (get_local $9)
             (i32.const 64)
            )
            (i32.const 24)
           )
          )
         )
         (tee_local $7
          (i64.load
           (i32.add
            (get_local $0)
            (i32.const 24)
           )
          )
         )
        )
        (i64.eq
         (get_local $8)
         (get_local $7)
        )
       )
      )
     )
    )
    (call $_ZN8exchange4saveERKNS_7AccountE
     (i32.add
      (get_local $9)
      (i32.const 112)
     )
    )
    (br_if $label$0
     (i64.eqz
      (i64.load
       (i32.add
        (get_local $0)
        (i32.const 32)
       )
      )
     )
    )
    (call $assert
     (i32.eqz
      (i32.load8_u offset=44
       (get_local $0)
      )
     )
     (i32.const 400)
    )
    (call $prints
     (i32.const 784)
    )
    (call $assert
     (call $store_i128i128
      (i64.const 179785961221)
      (i64.const 626978)
      (get_local $0)
      (i32.const 48)
     )
     (i32.const 1040)
    )
    (br $label$0)
   )
   (call $assert
    (i32.eqz
     (i32.load8_u offset=44
      (get_local $0)
     )
    )
    (i32.const 400)
   )
   (call $prints
    (i32.const 672)
   )
   (call $assert
    (call $store_i128i128
     (i64.const 179785961221)
     (i64.const 626978)
     (get_local $0)
     (i32.const 48)
    )
    (i32.const 1040)
   )
   (call $_ZN8exchange4saveERKNS_7AccountE
    (i32.add
     (get_local $9)
     (i32.const 112)
    )
   )
  )
  (i32.store offset=4
   (i32.const 0)
   (i32.add
    (get_local $9)
    (i32.const 192)
   )
  )
 )
 (func $init
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
     (i32.const 224)
    )
   )
  )
  (block $label$0
   (block $label$1
    (block $label$2
     (block $label$3
      (br_if $label$3
       (i64.ne
        (get_local $0)
        (i64.const 179785961221)
       )
      )
      (br_if $label$2
       (i64.eq
        (get_local $1)
        (i64.const 405683)
       )
      )
      (br_if $label$1
       (i64.ne
        (get_local $1)
        (i64.const 26274)
       )
      )
      (i64.store
       (i32.add
        (get_local $2)
        (i32.const 200)
       )
       (i64.const 0)
      )
      (i64.store offset=192
       (get_local $2)
       (i64.const 1)
      )
      (i64.store offset=184
       (get_local $2)
       (i64.const 0)
      )
      (i64.store offset=176
       (get_local $2)
       (i64.const 0)
      )
      (i64.store offset=208
       (get_local $2)
       (i64.const 0)
      )
      (i32.store8 offset=220
       (get_local $2)
       (i32.const 0)
      )
      (drop
       (call $readMessage
        (i32.add
         (get_local $2)
         (i32.const 176)
        )
        (i32.const 48)
       )
      )
      (call $_ZN8exchange18apply_exchange_buyENS_8BuyOrderE
       (call $memcpy
        (get_local $2)
        (i32.add
         (get_local $2)
         (i32.const 176)
        )
        (i32.const 48)
       )
      )
      (br $label$0)
     )
     (block $label$4
      (br_if $label$4
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
      (i64.store offset=120
       (get_local $2)
       (i64.const 0)
      )
      (drop
       (call $readMessage
        (i32.add
         (get_local $2)
         (i32.const 104)
        )
        (i32.const 24)
       )
      )
      (call $_ZN8exchange23apply_currency_transferERKN8currency8TransferE
       (i32.add
        (get_local $2)
        (i32.const 104)
       )
      )
      (br $label$0)
     )
     (br_if $label$0
      (i64.ne
       (get_local $0)
       (i64.const 19941)
      )
     )
     (br_if $label$0
      (i64.ne
       (get_local $1)
       (i64.const 624065709652)
      )
     )
     (i64.store offset=120
      (get_local $2)
      (i64.const 0)
     )
     (drop
      (call $readMessage
       (i32.add
        (get_local $2)
        (i32.const 104)
       )
       (i32.const 24)
      )
     )
     (call $_ZN8exchange18apply_eos_transferERKN3eos8TransferE
      (i32.add
       (get_local $2)
       (i32.const 104)
      )
     )
     (br $label$0)
    )
    (i64.store
     (i32.add
      (get_local $2)
      (i32.const 152)
     )
     (i64.const 0)
    )
    (i64.store offset=144
     (get_local $2)
     (i64.const 1)
    )
    (i64.store offset=136
     (get_local $2)
     (i64.const 0)
    )
    (i64.store offset=128
     (get_local $2)
     (i64.const 0)
    )
    (i64.store offset=160
     (get_local $2)
     (i64.const 0)
    )
    (i32.store8 offset=172
     (get_local $2)
     (i32.const 0)
    )
    (drop
     (call $readMessage
      (i32.add
       (get_local $2)
       (i32.const 128)
      )
      (i32.const 48)
     )
    )
    (drop
     (call $memcpy
      (i32.add
       (get_local $2)
       (i32.const 48)
      )
      (i32.add
       (get_local $2)
       (i32.const 128)
      )
      (i32.const 48)
     )
    )
    (call $_ZN8exchange19apply_exchange_sellENS_9SellOrderE
     (i32.add
      (get_local $2)
      (i32.const 48)
     )
    )
    (br $label$0)
   )
   (call $assert
    (i32.const 0)
    (i32.const 800)
   )
  )
  (i32.store offset=4
   (i32.const 0)
   (i32.add
    (get_local $2)
    (i32.const 224)
   )
  )
 )
 (func $_ZN3eos5printIPKcJNS_5tokenIyLy19941EEES2_NS_5priceIS4_NS3_IyLy862690298531EEEEES2_EEEvT_DpT0_ (param $0 i32) (param $1 i64) (param $2 i32) (param $3 i64) (param $4 i64) (param $5 i32)
  (local $6 i32)
  (i32.store offset=4
   (i32.const 0)
   (tee_local $6
    (i32.sub
     (i32.load offset=4
      (i32.const 0)
     )
     (i32.const 16)
    )
   )
  )
  (call $prints
   (get_local $0)
  )
  (call $printi
   (get_local $1)
  )
  (call $prints
   (i32.const 992)
  )
  (call $printn
   (i64.const 19941)
  )
  (call $prints
   (get_local $2)
  )
  (i64.store offset=8
   (get_local $6)
   (get_local $4)
  )
  (i64.store
   (get_local $6)
   (get_local $3)
  )
  (call $printi128
   (get_local $6)
  )
  (call $prints
   (i32.const 1008)
  )
  (call $prints
   (i32.const 992)
  )
  (call $printn
   (i64.const 19941)
  )
  (call $prints
   (i32.const 1024)
  )
  (call $printn
   (i64.const 862690298531)
  )
  (call $prints
   (get_local $5)
  )
  (i32.store offset=4
   (i32.const 0)
   (i32.add
    (get_local $6)
    (i32.const 16)
   )
  )
 )
 (func $_ZN3eos5printIPKcJNS_5tokenIyLy862690298531EEES2_NS_5priceINS3_IyLy19941EEES4_EES2_EEEvT_DpT0_ (param $0 i32) (param $1 i64) (param $2 i32) (param $3 i64) (param $4 i64) (param $5 i32)
  (local $6 i32)
  (i32.store offset=4
   (i32.const 0)
   (tee_local $6
    (i32.sub
     (i32.load offset=4
      (i32.const 0)
     )
     (i32.const 16)
    )
   )
  )
  (call $prints
   (get_local $0)
  )
  (call $printi
   (get_local $1)
  )
  (call $prints
   (i32.const 992)
  )
  (call $printn
   (i64.const 862690298531)
  )
  (call $prints
   (get_local $2)
  )
  (i64.store offset=8
   (get_local $6)
   (get_local $4)
  )
  (i64.store
   (get_local $6)
   (get_local $3)
  )
  (call $printi128
   (get_local $6)
  )
  (call $prints
   (i32.const 1008)
  )
  (call $prints
   (i32.const 992)
  )
  (call $printn
   (i64.const 19941)
  )
  (call $prints
   (i32.const 1024)
  )
  (call $printn
   (i64.const 862690298531)
  )
  (call $prints
   (get_local $5)
  )
  (i32.store offset=4
   (i32.const 0)
   (i32.add
    (get_local $6)
    (i32.const 16)
   )
  )
 )
)
)=====";
