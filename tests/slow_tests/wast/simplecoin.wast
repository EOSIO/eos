const char* simplecoin_wast = R"====((module
  (type $FUNCSIG$ji (func (param i32) (result i64)))
  (type $FUNCSIG$vi (func (param i32)))
  (type $FUNCSIG$vj (func (param i64)))
  (type $FUNCSIG$vii (func (param i32 i32)))
  (type $FUNCSIG$viiii (func (param i32 i32 i32 i32)))
  (type $FUNCSIG$iii (func (param i32 i32) (result i32)))
  (type $FUNCSIG$iiiii (func (param i32 i32 i32 i32) (result i32)))
  (import "env" "assert" (func $assert (param i32 i32)))
  (import "env" "load" (func $load (param i32 i32 i32 i32) (result i32)))
  (import "env" "name_to_int64" (func $name_to_int64 (param i32) (result i64)))
  (import "env" "print" (func $print (param i32)))
  (import "env" "printi" (func $printi (param i64)))
  (import "env" "readMessage" (func $readMessage (param i32 i32) (result i32)))
  (import "env" "remove" (func $remove (param i32 i32) (result i32)))
  (import "env" "store" (func $store (param i32 i32 i32 i32)))
  (table 0 anyfunc)
  (memory $0 1)
  (data (i32.const 16) "\00\e4\0bT\02\00\00\00")
  (data (i32.const 32) "simplecoin\00")
  (data (i32.const 48) "on_init called with \00")
  (data (i32.const 80) "\n\00")
  (data (i32.const 128) "insufficient funds\00")
  (export "memory" (memory $0))
  (export "init" (func $init))
  (export "apply_simplecoin_transfer" (func $apply_simplecoin_transfer))
  (func $init
    (i64.store offset=24
      (i32.const 0)
      (call $name_to_int64
        (i32.const 32)
      )
    )
    (call $print
      (i32.const 48)
    )
    (call $printi
      (i64.load offset=16
        (i32.const 0)
      )
    )
    (call $print
      (i32.const 80)
    )
    (call $store
      (i32.const 24)
      (i32.const 8)
      (i32.const 16)
      (i32.const 8)
    )
  )
  (func $apply_simplecoin_transfer
    (local $0 i64)
    (local $1 i64)
    (i64.store offset=120
      (i32.const 0)
      (i64.const 0)
    )
    (drop
      (call $readMessage
        (i32.const 88)
        (i32.const 24)
      )
    )
    (drop
      (call $load
        (i32.const 88)
        (i32.const 8)
        (i32.const 112)
        (i32.const 8)
      )
    )
    (drop
      (call $load
        (i32.const 96)
        (i32.const 8)
        (i32.const 120)
        (i32.const 8)
      )
    )
    (call $assert
      (i64.ge_s
        (i64.load offset=112
          (i32.const 0)
        )
        (i64.load offset=104
          (i32.const 0)
        )
      )
      (i32.const 128)
    )
    (i64.store offset=112
      (i32.const 0)
      (tee_local $1
        (i64.sub
          (i64.load offset=112
            (i32.const 0)
          )
          (tee_local $0
            (i64.load offset=104
              (i32.const 0)
            )
          )
        )
      )
    )
    (i64.store offset=120
      (i32.const 0)
      (i64.add
        (get_local $0)
        (i64.load offset=120
          (i32.const 0)
        )
      )
    )
    (block $label$0
      (block $label$1
        (br_if $label$1
          (i64.eqz
            (get_local $1)
          )
        )
        (call $store
          (i32.const 88)
          (i32.const 8)
          (i32.const 112)
          (i32.const 8)
        )
        (br $label$0)
      )
      (drop
        (call $remove
          (i32.const 88)
          (i32.const 8)
        )
      )
    )
    (call $store
      (i32.const 96)
      (i32.const 8)
      (i32.const 120)
      (i32.const 8)
    )
  )
))====";
