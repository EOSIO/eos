;; Failures in unreachable code.

(assert_invalid
  (module (func $local-index (unreachable) (drop (get_local 0))))
  "unknown local"
)
(assert_invalid
  (module (func $global-index (unreachable) (drop (get_global 0))))
  "unknown global"
)
(assert_invalid
  (module (func $func-index (unreachable) (call 1)))
  "unknown function"
)
(assert_invalid
  (module (func $label-index (unreachable) (br 1)))
  "unknown label"
)

(assert_invalid
  (module (func $type-num-vs-num
    (unreachable) (drop (i64.eqz (i32.const 0))))
  )
  "type mismatch"
)
(assert_invalid
  (module (func $type-poly-num-vs-num (result i32)
    (unreachable) (i64.const 0) (i32.const 0) (select)
  ))
  "type mismatch"
)
(assert_invalid
  (module (func $type-poly-transitive-num-vs-num (result i32)
    (unreachable)
    (i64.const 0) (i32.const 0) (select)
    (i32.const 0) (i32.const 0) (select)
  ))
  "type mismatch"
)

(assert_invalid
  (module (func $type-unconsumed-const (unreachable) (i32.const 0)))
  "type mismatch"
)
(assert_invalid
  (module (func $type-unconsumed-result (unreachable) (i32.eqz)))
  "type mismatch"
)
(assert_invalid
  (module (func $type-unconsumed-result2
    (unreachable) (i32.const 0) (i32.add)
  ))
  "type mismatch"
)
(assert_invalid
  (module (func $type-unconsumed-poly0 (unreachable) (select)))
  "type mismatch"
)
(assert_invalid
  (module (func $type-unconsumed-poly1 (unreachable) (i32.const 0) (select)))
  "type mismatch"
)
(assert_invalid
  (module (func $type-unconsumed-poly2
    (unreachable) (i32.const 0) (i32.const 0) (select)
  ))
  "type mismatch"
)

(assert_invalid
  (module (func $type-unary-num-vs-void-after-break
    (block (br 0) (block (drop (i32.eqz (nop)))))
  ))
  "type mismatch"
)
(assert_invalid
  (module (func $type-unary-num-vs-num-after-break
    (block (br 0) (drop (i32.eqz (f32.const 1))))
  ))
  "type mismatch"
)
(assert_invalid
  (module (func $type-binary-num-vs-void-after-break
    (block (br 0) (block (drop (f32.eq (i32.const 1)))))
  ))
  "type mismatch"
)
(assert_invalid
  (module (func $type-binary-num-vs-num-after-break
    (block (br 0) (drop (f32.eq (i32.const 1) (f32.const 0))))
  ))
  "type mismatch"
)
(assert_invalid
  (module (func $type-block-value-num-vs-void-after-break
    (block (br 0) (i32.const 1))
  ))
  "type mismatch"
)
(assert_invalid
  (module (func $type-block-value-num-vs-num-after-break (result i32)
    (block (result i32) (i32.const 1) (br 0) (f32.const 0))
  ))
  "type mismatch"
)
(assert_invalid
  (module (func $type-loop-value-num-vs-void-after-break
    (block (loop (br 1) (i32.const 1)))
  ))
  "type mismatch"
)
(assert_invalid
  (module (func $type-loop-value-num-vs-num-after-break (result i32)
    (loop (result i32) (br 1 (i32.const 1)) (f32.const 0))
  ))
  "type mismatch"
)
(assert_invalid
  (module (func $type-func-value-num-vs-void-after-break
    (br 0) (i32.const 1)
  ))
  "type mismatch"
)
(assert_invalid
  (module (func $type-func-value-num-vs-num-after-break (result i32)
    (br 0 (i32.const 1)) (f32.const 0)
  ))
  "type mismatch"
)

(assert_invalid
  (module (func $type-unary-num-vs-void-after-return
    (return) (block (drop (i32.eqz (nop))))
  ))
  "type mismatch"
)
(assert_invalid
  (module (func $type-unary-num-vs-num-after-return
    (return) (drop (i32.eqz (f32.const 1)))
  ))
  "type mismatch"
)
(assert_invalid
  (module (func $type-binary-num-vs-void-after-return
    (return) (block (drop (f32.eq (i32.const 1))))
  ))
  "type mismatch"
)
(assert_invalid
  (module (func $type-binary-num-vs-num-after-return
    (return) (drop (f32.eq (i32.const 1) (f32.const 0)))
  ))
  "type mismatch"
)
(assert_invalid
  (module (func $type-block-value-num-vs-void-after-return
    (block (return) (i32.const 1))
  ))
  "type mismatch"
)
(assert_invalid
  (module (func $type-block-value-num-vs-num-after-return (result i32)
    (block (result i32) (i32.const 1) (return (i32.const 0)) (f32.const 0))
  ))
  "type mismatch"
)
(assert_invalid
  (module (func $type-loop-value-num-vs-void-after-return
    (block (loop (return) (i32.const 1)))
  ))
  "type mismatch"
)
(assert_invalid
  (module (func $type-loop-value-num-vs-num-after-return (result i32)
    (loop (result i32) (return (i32.const 1)) (f32.const 0))
  ))
  "type mismatch"
)
(assert_invalid
  (module (func $type-func-value-num-vs-void-after-return
    (return) (i32.const 1)
  ))
  "type mismatch"
)
(assert_invalid
  (module (func $type-func-value-num-vs-num-after-return (result i32)
    (return (i32.const 1)) (f32.const 0)
  ))
  "type mismatch"
)

(assert_invalid
  (module (func $type-unary-num-vs-void-after-unreachable
    (unreachable) (block (drop (i32.eqz (nop))))
  ))
  "type mismatch"
)
(assert_invalid
  (module (func $type-unary-num-vs-num-after-unreachable
    (unreachable) (drop (i32.eqz (f32.const 1)))
  ))
  "type mismatch"
)
(assert_invalid
  (module (func $type-binary-num-vs-void-after-unreachable
    (unreachable) (block (drop (f32.eq (i32.const 1))))
  ))
  "type mismatch"
)
(assert_invalid
  (module (func $type-binary-num-vs-num-after-unreachable
    (unreachable) (drop (f32.eq (i32.const 1) (f32.const 0)))
  ))
  "type mismatch"
)
(assert_invalid
  (module (func $type-block-value-num-vs-void-after-unreachable
    (block (unreachable) (i32.const 1))
  ))
  "type mismatch"
)
(assert_invalid
  (module (func $type-block-value-num-vs-num-after-unreachable (result i32)
    (block (result i32) (i32.const 1) (unreachable) (f32.const 0))
  ))
  "type mismatch"
)
(assert_invalid
  (module (func $type-loop-value-num-vs-void-after-unreachable
    (block (loop (unreachable) (i32.const 1)))
  ))
  "type mismatch"
)
(assert_invalid
  (module (func $type-loop-value-num-vs-num-after-unreachable (result i32)
    (loop (result i32) (unreachable) (f32.const 0))
  ))
  "type mismatch"
)
(assert_invalid
  (module (func $type-func-value-num-vs-void-after-unreachable
    (unreachable) (i32.const 1)
  ))
  "type mismatch"
)
(assert_invalid
  (module (func $type-func-value-num-vs-num-after-unreachable (result i32)
    (unreachable) (f32.const 0)
  ))
  "type mismatch"
)

(assert_invalid
  (module (func $type-unary-num-vs-void-after-nested-unreachable
    (block (unreachable)) (block (drop (i32.eqz (nop))))
  ))
  "type mismatch"
)
(assert_invalid
  (module (func $type-unary-num-vs-num-after-nested-unreachable
    (block (unreachable)) (drop (i32.eqz (f32.const 1)))
  ))
  "type mismatch"
)
(assert_invalid
  (module (func $type-binary-num-vs-void-after-nested-unreachable
    (block (unreachable)) (block (drop (f32.eq (i32.const 1))))
  ))
  "type mismatch"
)
(assert_invalid
  (module (func $type-binary-num-vs-num-after-nested-unreachable
    (block (unreachable)) (drop (f32.eq (i32.const 1) (f32.const 0)))
  ))
  "type mismatch"
)
(assert_invalid
  (module (func $type-block-value-num-vs-void-after-nested-unreachable
    (block (block (unreachable)) (i32.const 1))
  ))
  "type mismatch"
)
(assert_invalid
  (module (func $type-block-value-num-vs-num-after-nested-unreachable
    (result i32)
    (block (result i32) (i32.const 1) (block (unreachable)) (f32.const 0))
  ))
  "type mismatch"
)
(assert_invalid
  (module (func $type-loop-value-num-vs-void-after-nested-unreachable
    (block (loop (block (unreachable)) (i32.const 1)))
  ))
  "type mismatch"
)
(assert_invalid
  (module (func $type-loop-value-num-vs-num-after-nested-unreachable
    (result i32)
    (loop (result i32) (block (unreachable)) (f32.const 0))
  ))
  "type mismatch"
)
(assert_invalid
  (module (func $type-func-value-num-vs-void-after-nested-unreachable
    (block (unreachable)) (i32.const 1)
  ))
  "type mismatch"
)
(assert_invalid
  (module (func $type-func-value-num-vs-num-after-nested-unreachable
    (result i32)
    (block (unreachable)) (f32.const 0)
  ))
  "type mismatch"
)

(assert_invalid
  (module (func $type-unary-num-vs-void-after-infinite-loop
    (loop (br 0)) (block (drop (i32.eqz (nop))))
  ))
  "type mismatch"
)
(assert_invalid
  (module (func $type-unary-num-vs-num-after-infinite-loop
    (loop (br 0)) (drop (i32.eqz (f32.const 1)))
  ))
  "type mismatch"
)
(assert_invalid
  (module (func $type-binary-num-vs-void-after-infinite-loop
    (loop (br 0)) (block (drop (f32.eq (i32.const 1))))
  ))
  "type mismatch"
)
(assert_invalid
  (module (func $type-binary-num-vs-num-after-infinite-loop
    (loop (br 0)) (drop (f32.eq (i32.const 1) (f32.const 0)))
  ))
  "type mismatch"
)
(assert_invalid
  (module (func $type-block-value-num-vs-void-after-infinite-loop
    (block (loop (br 0)) (i32.const 1))
  ))
  "type mismatch"
)
(assert_invalid
  (module (func $type-block-value-num-vs-num-after-infinite-loop (result i32)
    (block (result i32) (i32.const 1) (loop (br 0)) (f32.const 0))
  ))
  "type mismatch"
)
(assert_invalid
  (module (func $type-loop-value-num-vs-void-after-infinite-loop
    (block (loop (loop (br 0)) (i32.const 1)))
  ))
  "type mismatch"
)
(assert_invalid
  (module (func $type-loop-value-num-vs-num-after-infinite-loop (result i32)
    (loop (result i32) (loop (br 0)) (f32.const 0))
  ))
  "type mismatch"
)
(assert_invalid
  (module (func $type-func-value-num-vs-void-after-infinite-loop
    (loop (br 0)) (i32.const 1)
  ))
  "type mismatch"
)
(assert_invalid
  (module (func $type-func-value-num-vs-num-after-infinite-loop (result i32)
    (loop (br 0)) (f32.const 0)
  ))
  "type mismatch"
)

(assert_invalid
  (module (func $type-unary-num-vs-void-in-dead-body
    (if (i32.const 0) (then (drop (i32.eqz (nop)))))
  ))
  "type mismatch"
)
(assert_invalid
  (module (func $type-unary-num-vs-num-in-dead-body
    (if (i32.const 0) (then (drop (i32.eqz (f32.const 1)))))
  ))
  "type mismatch"
)
(assert_invalid
  (module (func $type-binary-num-vs-void-in-dead-body
    (if (i32.const 0) (then (drop (f32.eq (i32.const 1)))))
  ))
  "type mismatch"
)
(assert_invalid
  (module (func $type-binary-num-vs-num-in-dead-body
    (if (i32.const 0) (then (drop (f32.eq (i32.const 1) (f32.const 0)))))
  ))
  "type mismatch"
)
(assert_invalid
  (module (func $type-if-value-num-vs-void-in-dead-body
    (if (i32.const 0) (then (i32.const 1)))
  ))
  "type mismatch"
)
(assert_invalid
  (module (func $type-if-value-num-vs-num-in-dead-body (result i32)
    (if (result i32) (i32.const 0) (then (f32.const 0)))
  ))
  "type mismatch"
)
(assert_invalid
  (module (func $type-block-value-num-vs-void-in-dead-body
    (if (i32.const 0) (then (block (i32.const 1))))
  ))
  "type mismatch"
)
(assert_invalid
  (module (func $type-block-value-num-vs-num-in-dead-body (result i32)
    (if (result i32) (i32.const 0) (then (block (result i32) (f32.const 0))))
  ))
  "type mismatch"
)
(assert_invalid
  (module (func $type-block-value-num-vs-void-in-dead-body
    (if (i32.const 0) (then (loop (i32.const 1))))
  ))
  "type mismatch"
)
(assert_invalid
  (module (func $type-block-value-num-vs-num-in-dead-body (result i32)
    (if (result i32) (i32.const 0) (then (loop (result i32) (f32.const 0))))
  ))
  "type mismatch"
)

(assert_invalid
  (module (func $type-return-second-num-vs-num (result i32)
    (return (i32.const 1)) (return (f64.const 1))
  ))
  "type mismatch"
)

(assert_invalid
  (module (func $type-br-second-num-vs-num (result i32)
    (block (result i32) (br 0 (i32.const 1)) (br 0 (f64.const 1)))
  ))
  "type mismatch"
)

(assert_invalid
  (module (func $type-br_if-cond-num-vs-num-after-unreachable
    (block (br_if 0 (unreachable) (f32.const 0)))
  ))
  "type mismatch"
)
(assert_invalid
  (module (func $type-br_table-num-vs-num-after-unreachable
    (block (br_table 0 (unreachable) (f32.const 1)))
  ))
  "type mismatch"
)
(assert_invalid
  (module (func $type-br_table-label-num-vs-num-after-unreachable (result i32)
    (block (result i32) (unreachable) (br_table 0 (f32.const 0) (i32.const 1)))
  ))
  "type mismatch"
)
(assert_invalid
  (module (func $type-br_table-label-num-vs-label-void-after-unreachable
    (block
      (block (result f32)
        (unreachable)
        (br_table 0 1 0 (i32.const 1))
      )
      (drop)
    )
  ))
  "type mismatch"
)
(assert_invalid
  (module (func $type-br_table-label-num-vs-label-num-after-unreachable
    (block (result f64)
      (block (result f32)
        (unreachable)
        (br_table 0 1 1 (i32.const 1))
      )
      (drop)
      (f64.const 0)
    )
    (drop)
  ))
  "type mismatch"
)

(assert_invalid
  (module (func $type-block-value-nested-unreachable-num-vs-void
    (block (i32.const 3) (block (unreachable)))
  ))
  "type mismatch"
)
(assert_invalid
  (module (func $type-block-value-nested-unreachable-void-vs-num (result i32)
    (block (block (unreachable)))
  ))
  "type mismatch"
)
(assert_invalid
  (module (func $type-block-value-nested-unreachable-num-vs-num (result i32)
    (block (result i64) (i64.const 0) (block (unreachable)))
  ))
  "type mismatch"
)
(assert_invalid
  (module (func $type-block-value-nested-unreachable-num2-vs-void (result i32)
    (block (i32.const 3) (block (i64.const 1) (unreachable))) (i32.const 9)
  ))
  "type mismatch"
)

(assert_invalid
  (module (func $type-block-value-nested-br-num-vs-void
    (block (i32.const 3) (block (br 1)))
  ))
  "type mismatch"
)
(assert_invalid
  (module (func $type-block-value-nested-br-void-vs-num (result i32)
    (block (result i32) (block (br 1 (i32.const 0))))
  ))
  "type mismatch"
)
(assert_invalid
  (module (func $type-block-value-nested-br-num-vs-num (result i32)
    (block (result i32) (i64.const 0) (block (br 1 (i32.const 0))))
  ))
  "type mismatch"
)

(assert_invalid
  (module (func $type-block-value-nested2-br-num-vs-void
    (block (block (i32.const 3) (block (br 2))))
  ))
  "type mismatch"
)
(assert_invalid
  (module (func $type-block-value-nested2-br-void-vs-num (result i32)
    (block (result i32) (block (block (br 2 (i32.const 0)))))
  ))
  "type mismatch"
)
(assert_invalid
  (module (func $type-block-value-nested2-br-num-vs-num (result i32)
    (block (result i32)
      (block (result i64) (i64.const 0) (block (br 2 (i32.const 0))))
    )
  ))
  "type mismatch"
)
(assert_invalid
  (module (func $type-block-value-nested2-br-num2-vs-void (result i32)
    (block (i32.const 3) (block (i64.const 1) (br 1))) (i32.const 9)
  ))
  "type mismatch"
)

(assert_invalid
  (module (func $type-block-value-nested-return-num-vs-void
    (block (i32.const 3) (block (return)))
  ))
  "type mismatch"
)
(assert_invalid
  (module (func $type-block-value-nested-return-void-vs-num (result i32)
    (block (block (return (i32.const 0))))
  ))
  "type mismatch"
)
(assert_invalid
  (module (func $type-block-value-nested-return-num-vs-num (result i32)
    (block (result i64) (i64.const 0) (block (return (i32.const 0))))
  ))
  "type mismatch"
)
(assert_invalid
  (module (func $type-block-value-nested-return-num2-vs-void (result i32)
    (block (i32.const 3) (block (i64.const 1) (return (i32.const 0))))
    (i32.const 9)
  ))
  "type mismatch"
)

(assert_invalid
  (module (func $type-loop-value-nested-unreachable-num-vs-void
    (loop (i32.const 3) (block (unreachable)))
  ))
  "type mismatch"
)
(assert_invalid
  (module (func $type-loop-value-nested-unreachable-void-vs-num (result i32)
    (loop (block (unreachable)))
  ))
  "type mismatch"
)
(assert_invalid
  (module (func $type-loop-value-nested-unreachable-num-vs-num (result i32)
    (loop (result i64) (i64.const 0) (block (unreachable)))
  ))
  "type mismatch"
)

(assert_invalid
  (module (func $type-cont-last-void-vs-empty (result i32)
    (loop (br 0 (nop)))
  ))
  "type mismatch"
)
(assert_invalid
  (module (func $type-cont-last-num-vs-empty (result i32)
    (loop (br 0 (i32.const 0)))
  ))
  "type mismatch"
)
