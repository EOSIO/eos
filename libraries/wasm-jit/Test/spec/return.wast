;; Test `return` operator

(module
  ;; Auxiliary definition
  (func $dummy)

  (func (export "type-i32") (drop (i32.ctz (return))))
  (func (export "type-i64") (drop (i64.ctz (return))))
  (func (export "type-f32") (drop (f32.neg (return))))
  (func (export "type-f64") (drop (f64.neg (return))))

  (func (export "nullary") (return))
  (func (export "unary") (result f64) (return (f64.const 3)))

  (func (export "as-func-first") (result i32)
    (return (i32.const 1)) (i32.const 2)
  )
  (func (export "as-func-mid") (result i32)
    (call $dummy) (return (i32.const 2)) (i32.const 3)
  )
  (func (export "as-func-last")
    (nop) (call $dummy) (return)
  )
  (func (export "as-func-value") (result i32)
    (nop) (call $dummy) (return (i32.const 3))
  )

  (func (export "as-block-first")
    (block (return) (call $dummy))
  )
  (func (export "as-block-mid")
    (block (call $dummy) (return) (call $dummy))
  )
  (func (export "as-block-last")
    (block (nop) (call $dummy) (return))
  )
  (func (export "as-block-value") (result i32)
    (block (result i32) (nop) (call $dummy) (return (i32.const 2)))
  )

  (func (export "as-loop-first") (result i32)
    (loop (result i32) (return (i32.const 3)) (i32.const 2))
  )
  (func (export "as-loop-mid") (result i32)
    (loop (result i32) (call $dummy) (return (i32.const 4)) (i32.const 2))
  )
  (func (export "as-loop-last") (result i32)
    (loop (result i32) (nop) (call $dummy) (return (i32.const 5)))
  )

  (func (export "as-br-value") (result i32)
    (block (result i32) (br 0 (return (i32.const 9))))
  )

  (func (export "as-br_if-cond")
    (block (br_if 0 (return)))
  )
  (func (export "as-br_if-value") (result i32)
    (block (result i32)
      (drop (br_if 0 (return (i32.const 8)) (i32.const 1))) (i32.const 7)
    )
  )
  (func (export "as-br_if-value-cond") (result i32)
    (block (result i32)
      (drop (br_if 0 (i32.const 6) (return (i32.const 9)))) (i32.const 7)
    )
  )

  (func (export "as-br_table-index") (result i64)
    (block (br_table 0 0 0 (return (i64.const 9)))) (i64.const -1)
  )
  (func (export "as-br_table-value") (result i32)
    (block (result i32)
      (br_table 0 0 0 (return (i32.const 10)) (i32.const 1)) (i32.const 7)
    )
  )
  (func (export "as-br_table-value-index") (result i32)
    (block (result i32)
      (br_table 0 0 (i32.const 6) (return (i32.const 11))) (i32.const 7)
    )
  )

  (func (export "as-return-value") (result i64)
    (return (return (i64.const 7)))
  )

  (func (export "as-if-cond") (result i32)
    (if (result i32)
      (return (i32.const 2)) (then (i32.const 0)) (else (i32.const 1))
    )
  )
  (func (export "as-if-then") (param i32 i32) (result i32)
    (if (result i32)
      (get_local 0) (then (return (i32.const 3))) (else (get_local 1))
    )
  )
  (func (export "as-if-else") (param i32 i32) (result i32)
    (if (result i32)
      (get_local 0) (then (get_local 1)) (else (return (i32.const 4)))
    )
  )

  (func (export "as-select-first") (param i32 i32) (result i32)
    (select (return (i32.const 5)) (get_local 0) (get_local 1))
  )
  (func (export "as-select-second") (param i32 i32) (result i32)
    (select (get_local 0) (return (i32.const 6)) (get_local 1))
  )
  (func (export "as-select-cond") (result i32)
    (select (i32.const 0) (i32.const 1) (return (i32.const 7)))
  )

  (func $f (param i32 i32 i32) (result i32) (i32.const -1))
  (func (export "as-call-first") (result i32)
    (call $f (return (i32.const 12)) (i32.const 2) (i32.const 3))
  )
  (func (export "as-call-mid") (result i32)
    (call $f (i32.const 1) (return (i32.const 13)) (i32.const 3))
  )
  (func (export "as-call-last") (result i32)
    (call $f (i32.const 1) (i32.const 2) (return (i32.const 14)))
  )

  (type $sig (func (param i32 i32 i32) (result i32)))
  (table anyfunc (elem $f))
  (func (export "as-call_indirect-func") (result i32)
    (call_indirect $sig (return (i32.const 20)) (i32.const 1) (i32.const 2) (i32.const 3))
  )
  (func (export "as-call_indirect-first") (result i32)
    (call_indirect $sig (i32.const 0) (return (i32.const 21)) (i32.const 2) (i32.const 3))
  )
  (func (export "as-call_indirect-mid") (result i32)
    (call_indirect $sig (i32.const 0) (i32.const 1) (return (i32.const 22)) (i32.const 3))
  )
  (func (export "as-call_indirect-last") (result i32)
    (call_indirect $sig (i32.const 0) (i32.const 1) (i32.const 2) (return (i32.const 23)))
  )

  (func (export "as-set_local-value") (result i32) (local f32)
    (set_local 0 (return (i32.const 17))) (i32.const -1)
  )

  (memory 1)
  (func (export "as-load-address") (result f32)
    (f32.load (return (f32.const 1.7)))
  )
  (func (export "as-loadN-address") (result i64)
    (i64.load8_s (return (i64.const 30)))
  )

  (func (export "as-store-address") (result i32)
    (f64.store (return (i32.const 30)) (f64.const 7)) (i32.const -1)
  )
  (func (export "as-store-value") (result i32)
    (i64.store (i32.const 2) (return (i32.const 31))) (i32.const -1)
  )

  (func (export "as-storeN-address") (result i32)
    (i32.store8 (return (i32.const 32)) (i32.const 7)) (i32.const -1)
  )
  (func (export "as-storeN-value") (result i32)
    (i64.store16 (i32.const 2) (return (i32.const 33))) (i32.const -1)
  )

  (func (export "as-unary-operand") (result f32)
    (f32.neg (return (f32.const 3.4)))
  )

  (func (export "as-binary-left") (result i32)
    (i32.add (return (i32.const 3)) (i32.const 10))
  )
  (func (export "as-binary-right") (result i64)
    (i64.sub (i64.const 10) (return (i64.const 45)))
  )

  (func (export "as-test-operand") (result i32)
    (i32.eqz (return (i32.const 44)))
  )

  (func (export "as-compare-left") (result i32)
    (f64.le (return (i32.const 43)) (f64.const 10))
  )
  (func (export "as-compare-right") (result i32)
    (f32.ne (f32.const 10) (return (i32.const 42)))
  )

  (func (export "as-convert-operand") (result i32)
    (i32.wrap/i64 (return (i32.const 41)))
  )

  (func (export "as-grow_memory-size") (result i32)
    (grow_memory (return (i32.const 40)))
  )
)

(assert_return (invoke "type-i32"))
(assert_return (invoke "type-i64"))
(assert_return (invoke "type-f32"))
(assert_return (invoke "type-f64"))

(assert_return (invoke "nullary"))
(assert_return (invoke "unary") (f64.const 3))

(assert_return (invoke "as-func-first") (i32.const 1))
(assert_return (invoke "as-func-mid") (i32.const 2))
(assert_return (invoke "as-func-last"))
(assert_return (invoke "as-func-value") (i32.const 3))

(assert_return (invoke "as-block-first"))
(assert_return (invoke "as-block-mid"))
(assert_return (invoke "as-block-last"))
(assert_return (invoke "as-block-value") (i32.const 2))

(assert_return (invoke "as-loop-first") (i32.const 3))
(assert_return (invoke "as-loop-mid") (i32.const 4))
(assert_return (invoke "as-loop-last") (i32.const 5))

(assert_return (invoke "as-br-value") (i32.const 9))

(assert_return (invoke "as-br_if-cond"))
(assert_return (invoke "as-br_if-value") (i32.const 8))
(assert_return (invoke "as-br_if-value-cond") (i32.const 9))

(assert_return (invoke "as-br_table-index") (i64.const 9))
(assert_return (invoke "as-br_table-value") (i32.const 10))
(assert_return (invoke "as-br_table-value-index") (i32.const 11))

(assert_return (invoke "as-return-value") (i64.const 7))

(assert_return (invoke "as-if-cond") (i32.const 2))
(assert_return (invoke "as-if-then" (i32.const 1) (i32.const 6)) (i32.const 3))
(assert_return (invoke "as-if-then" (i32.const 0) (i32.const 6)) (i32.const 6))
(assert_return (invoke "as-if-else" (i32.const 0) (i32.const 6)) (i32.const 4))
(assert_return (invoke "as-if-else" (i32.const 1) (i32.const 6)) (i32.const 6))

(assert_return (invoke "as-select-first" (i32.const 0) (i32.const 6)) (i32.const 5))
(assert_return (invoke "as-select-first" (i32.const 1) (i32.const 6)) (i32.const 5))
(assert_return (invoke "as-select-second" (i32.const 0) (i32.const 6)) (i32.const 6))
(assert_return (invoke "as-select-second" (i32.const 1) (i32.const 6)) (i32.const 6))
(assert_return (invoke "as-select-cond") (i32.const 7))

(assert_return (invoke "as-call-first") (i32.const 12))
(assert_return (invoke "as-call-mid") (i32.const 13))
(assert_return (invoke "as-call-last") (i32.const 14))

(assert_return (invoke "as-call_indirect-func") (i32.const 20))
(assert_return (invoke "as-call_indirect-first") (i32.const 21))
(assert_return (invoke "as-call_indirect-mid") (i32.const 22))
(assert_return (invoke "as-call_indirect-last") (i32.const 23))

(assert_return (invoke "as-set_local-value") (i32.const 17))

(assert_return (invoke "as-load-address") (f32.const 1.7))
(assert_return (invoke "as-loadN-address") (i64.const 30))

(assert_return (invoke "as-store-address") (i32.const 30))
(assert_return (invoke "as-store-value") (i32.const 31))
(assert_return (invoke "as-storeN-address") (i32.const 32))
(assert_return (invoke "as-storeN-value") (i32.const 33))

(assert_return (invoke "as-unary-operand") (f32.const 3.4))

(assert_return (invoke "as-binary-left") (i32.const 3))
(assert_return (invoke "as-binary-right") (i64.const 45))

(assert_return (invoke "as-test-operand") (i32.const 44))

(assert_return (invoke "as-compare-left") (i32.const 43))
(assert_return (invoke "as-compare-right") (i32.const 42))

(assert_return (invoke "as-convert-operand") (i32.const 41))

(assert_return (invoke "as-grow_memory-size") (i32.const 40))

(assert_invalid
  (module (func $type-value-empty-vs-num (result f64) (return)))
  "type mismatch"
)
(assert_invalid
  (module (func $type-value-void-vs-num (result f64) (return (nop))))
  "type mismatch"
)
(assert_invalid
  (module (func $type-value-num-vs-num (result f64) (return (i64.const 1))))
  "type mismatch"
)

