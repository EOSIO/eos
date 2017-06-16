(module
  (memory 0)

  (func $load_at_zero (result i32) (i32.load (i32.const 0)))
  (func $store_at_zero (i32.store (i32.const 0) (i32.const 2)))

  (func $load_at_page_size (result i32) (i32.load (i32.const 0x10000)))
  (func $store_at_page_size (i32.store (i32.const 0x10000) (i32.const 3)))

  (func $grow (param $sz i32) (result i32) (grow_memory (get_local $sz)))
  (func $size (result i32) (current_memory))

  (func (export "main")
    (drop (call $size))
    (drop (call $grow (i32.const 1)))
    (drop (call $size))
    (drop (call $load_at_zero))
    (call $store_at_zero)
    (drop (call $load_at_zero))
    (drop (call $grow (i32.const 4)))
    (drop (call $size))
    (drop (call $load_at_zero))
    (call $store_at_zero)
    (drop (call $load_at_zero))
    (drop (call $load_at_page_size))
    (call $store_at_page_size)
    (drop (call $load_at_page_size))
    )
)
