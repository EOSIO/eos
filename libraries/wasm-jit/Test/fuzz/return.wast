;; Test `return` operator

(module
  ;; Auxiliary definition
  (func $dummy)

  (func $type-i32 (drop (i32.ctz (return))))
  (func $type-i64 (drop (i64.ctz (return))))
  (func $type-f32 (drop (f32.neg (return))))
  (func $type-f64 (drop (f64.neg (return))))

  (func $nullary (return))
  (func $unary (result f64) (return (f64.const 3)))

  (func $as-func-first (result i32)
    (return (i32.const 1)) (i32.const 2)
  )
  (func $as-func-mid (result i32)
    (call $dummy) (return (i32.const 2)) (i32.const 3)
  )
  (func $as-func-last
    (nop) (call $dummy) (return)
  )
  (func $as-func-value (result i32)
    (nop) (call $dummy) (return (i32.const 3))
  )

  (func $as-block-first
    (block (return) (call $dummy))
  )
  (func $as-block-mid
    (block (call $dummy) (return) (call $dummy))
  )
  (func $as-block-last
    (block (nop) (call $dummy) (return))
  )
  (func $as-block-value (result i32)
    (block i32 (nop) (call $dummy) (return (i32.const 2)))
  )

  (func $as-loop-first (result i32)
    (loop i32 (return (i32.const 3)) (i32.const 2))
  )
  (func $as-loop-mid (result i32)
    (loop i32 (call $dummy) (return (i32.const 4)) (i32.const 2))
  )
  (func $as-loop-last (result i32)
    (loop i32 (nop) (call $dummy) (return (i32.const 5)))
  )

  (func $as-br-value (result i32)
    (block i32 (br 0 (return (i32.const 9))))
  )

  (func $as-br_if-cond
    (block (br_if 0 (return)))
  )
  (func $as-br_if-value (result i32)
    (block i32 (br_if 0 (return (i32.const 8)) (i32.const 1)) (i32.const 7))
  )
  (func $as-br_if-value-cond (result i32)
    (block i32 (drop (br_if 0 (i32.const 6) (return (i32.const 9)))) (i32.const 7))
  )

  (func $as-br_table-index (result i64)
    (block (br_table 0 0 0 (return (i64.const 9)))) (i64.const -1)
  )
  (func $as-br_table-value (result i32)
    (block i32
      (br_table 0 0 0 (return (i32.const 10)) (i32.const 1)) (i32.const 7)
    )
  )
  (func $as-br_table-value-index (result i32)
    (block i32
      (br_table 0 0 (i32.const 6) (return (i32.const 11))) (i32.const 7)
    )
  )

  (func $as-return-value (result i64)
    (return (return (i64.const 7)))
  )

  (func $as-if-cond (result i32)
    (if (return (i32.const 2)) (i32.const 0) (i32.const 1))
  )
  (func $as-if-then (param i32 i32) (result i32)
    (if i32 (get_local 0) (return (i32.const 3)) (get_local 1))
  )
  (func $as-if-else (param i32 i32) (result i32)
    (if i32 (get_local 0) (get_local 1) (return (i32.const 4)))
  )

  (func $as-select-first (param i32 i32) (result i32)
    (select (return (i32.const 5)) (get_local 0) (get_local 1))
  )
  (func $as-select-second (param i32 i32) (result i32)
    (select (get_local 0) (return (i32.const 6)) (get_local 1))
  )
  (func $as-select-cond (result i32)
    (select (i32.const 0) (i32.const 1) (return (i32.const 7)))
  )

  (func $f (param i32 i32 i32) (result i32) (i32.const -1))
  (func $as-call-first (result i32)
    (call $f (return (i32.const 12)) (i32.const 2) (i32.const 3))
  )
  (func $as-call-mid (result i32)
    (call $f (i32.const 1) (return (i32.const 13)) (i32.const 3))
  )
  (func $as-call-last (result i32)
    (call $f (i32.const 1) (i32.const 2) (return (i32.const 14)))
  )

  (type $sig (func (param i32 i32 i32) (result i32)))
  (table anyfunc (elem $f))
  (func $as-call_indirect-func (result i32)
    (call_indirect $sig (return (i32.const 20)) (i32.const 1) (i32.const 2) (i32.const 3))
  )
  (func $as-call_indirect-first (result i32)
    (call_indirect $sig (i32.const 0) (return (i32.const 21)) (i32.const 2) (i32.const 3))
  )
  (func $as-call_indirect-mid (result i32)
    (call_indirect $sig (i32.const 0) (i32.const 1) (return (i32.const 22)) (i32.const 3))
  )
  (func $as-call_indirect-last (result i32)
    (call_indirect $sig (i32.const 0) (i32.const 1) (i32.const 2) (return (i32.const 23)))
  )

  (func $as-set_local-value (result i32) (local f32)
    (set_local 0 (return (i32.const 17))) (i32.const -1)
  )

  (memory 1)
  (func $as-load-address (result f32)
    (f32.load (return (f32.const 1.7)))
  )
  (func $as-loadN-address (result i64)
    (i64.load8_s (return (i64.const 30)))
  )

  (func $as-store-address (result i32)
    (f64.store (return (i32.const 30)) (f64.const 7)) (i32.const -1)
  )
  (func $as-store-value (result i32)
    (i64.store (i32.const 2) (return (i32.const 31))) (i32.const -1)
  )

  (func $as-storeN-address (result i32)
    (i32.store8 (return (i32.const 32)) (i32.const 7)) (i32.const -1)
  )
  (func $as-storeN-value (result i32)
    (i64.store16 (i32.const 2) (return (i32.const 33))) (i32.const -1)
  )

  (func $as-unary-operand (result f32)
    (f32.neg (return (f32.const 3.4)))
  )

  (func $as-binary-left (result i32)
    (i32.add (return (i32.const 3)) (i32.const 10))
  )
  (func $as-binary-right (result i64)
    (i64.sub (i64.const 10) (return (i64.const 45)))
  )

  (func $as-test-operand (result i32)
    (i32.eqz (return (i32.const 44)))
  )

  (func $as-compare-left (result i32)
    (f64.le (return (i32.const 43)) (f64.const 10))
  )
  (func $as-compare-right (result i32)
    (f32.ne (f32.const 10) (return (i32.const 42)))
  )

  (func $as-convert-operand (result i32)
    (i32.wrap/i64 (return (i32.const 41)))
  )

  (func $as-grow_memory-size (result i32)
    (grow_memory (return (i32.const 40)))
  )

  (func (export "main")
    (call $type-i32)
    (call $type-i64)
    (call $type-f32)
    (call $type-f64)

    (call $nullary)
    (drop (call $unary))

    (drop (call $as-func-first))
    (drop (call $as-func-mid))
    (call $as-func-last)
    (drop (call $as-func-value))

    (call $as-block-first)
    (call $as-block-mid)
    (call $as-block-last)
    (drop (call $as-block-value))

    (drop (call $as-loop-first))
    (drop (call $as-loop-mid))
    (drop (call $as-loop-last))

    (drop (call $as-br-value))

    (call $as-br_if-cond)
    (drop (call $as-br_if-value))
    (drop (call $as-br_if-value-cond))

    (drop (call $as-br_table-index))
    (drop (call $as-br_table-value))
    (drop (call $as-br_table-value-index))

    (drop (call $as-return-value))

    (drop (call $as-if-cond))
    (drop (call $as-if-then (i32.const 1) (i32.const 6)))
    (drop (call $as-if-then (i32.const 0) (i32.const 6)))
    (drop (call $as-if-else (i32.const 0) (i32.const 6)))
    (drop (call $as-if-else (i32.const 1) (i32.const 6)))

    (drop (call $as-select-first (i32.const 0) (i32.const 6)))
    (drop (call $as-select-first (i32.const 1) (i32.const 6)))
    (drop (call $as-select-second (i32.const 0) (i32.const 6)))
    (drop (call $as-select-second (i32.const 1) (i32.const 6)))
    (drop (call $as-select-cond))

    (drop (call $as-call-first))
    (drop (call $as-call-mid))
    (drop (call $as-call-last))

    (drop (call $as-call_indirect-func))
    (drop (call $as-call_indirect-first))
    (drop (call $as-call_indirect-mid))
    (drop (call $as-call_indirect-last))

    (drop (call $as-set_local-value))

    (drop (call $as-load-address) )
    (drop (call $as-loadN-address))

    (drop (call $as-store-address))
    (drop (call $as-store-value))
    (drop (call $as-storeN-address))
    (drop (call $as-storeN-value))

    (drop (call $as-unary-operand) )

    (drop (call $as-binary-left))
    (drop (call $as-binary-right))

    (drop (call $as-test-operand))

    (drop (call $as-compare-left))
    (drop (call $as-compare-right))

    (drop (call $as-convert-operand))

    (drop (call $as-grow_memory-size))
    )
)
