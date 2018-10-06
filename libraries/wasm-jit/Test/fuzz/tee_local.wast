;; Test `tee_local` operator

(module
  ;; Typing

  (func $type-local-i32 (result i32) (local i32) (tee_local 0 (i32.const 0)))
  (func $type-local-i64 (result i64) (local i64) (tee_local 0 (i64.const 0)))
  (func $type-local-f32 (result f32) (local f32) (tee_local 0 (f32.const 0)))
  (func $type-local-f64 (result f64) (local f64) (tee_local 0 (f64.const 0)))

  (func $type-param-i32 (param i32) (result i32) (tee_local 0 (i32.const 10)))
  (func $type-param-i64 (param i64) (result i64) (tee_local 0 (i64.const 11)))
  (func $type-param-f32 (param f32) (result f32) (tee_local 0 (f32.const 11.1)))
  (func $type-param-f64 (param f64) (result f64) (tee_local 0 (f64.const 12.2)))

  (func $type-mixed (param i64 f32 f64 i32 i32) (local f32 i64 i64 f64)
    (drop (i64.eqz (tee_local 0 (i64.const 0))))
    (drop (f32.neg (tee_local 1 (f32.const 0))))
    (drop (f64.neg (tee_local 2 (f64.const 0))))
    (drop (i32.eqz (tee_local 3 (i32.const 0))))
    (drop (i32.eqz (tee_local 4 (i32.const 0))))
    (drop (f32.neg (tee_local 5 (f32.const 0))))
    (drop (i64.eqz (tee_local 6 (i64.const 0))))
    (drop (i64.eqz (tee_local 7 (i64.const 0))))
    (drop (f64.neg (tee_local 8 (f64.const 0))))
  )

  ;; Writing

  (func $write (param i64 f32 f64 i32 i32) (result i64) (local f32 i64 i64 f64)
    (drop (tee_local 1 (f32.const -0.3)))
    (drop (tee_local 3 (i32.const 40)))
    (drop (tee_local 4 (i32.const -7)))
    (drop (tee_local 5 (f32.const 5.5)))
    (drop (tee_local 6 (i64.const 6)))
    (drop (tee_local 8 (f64.const 8)))
    (i64.trunc_s/f64
      (f64.add
        (f64.convert_u/i64 (get_local 0))
        (f64.add
          (f64.promote/f32 (get_local 1))
          (f64.add
            (get_local 2)
            (f64.add
              (f64.convert_u/i32 (get_local 3))
              (f64.add
                (f64.convert_s/i32 (get_local 4))
                (f64.add
                  (f64.promote/f32 (get_local 5))
                  (f64.add
                    (f64.convert_u/i64 (get_local 6))
                    (f64.add
                      (f64.convert_u/i64 (get_local 7))
                      (get_local 8)
                    )
                  )
                )
              )
            )
          )
        )
      )
    )
  )

  ;; Result

  (func $result (param i64 f32 f64 i32 i32) (result f64)
    (local f32 i64 i64 f64)
    (f64.add
      (f64.convert_u/i64 (tee_local 0 (i64.const 1)))
      (f64.add
        (f64.promote/f32 (tee_local 1 (f32.const 2)))
        (f64.add
          (tee_local 2 (f64.const 3.3))
          (f64.add
            (f64.convert_u/i32 (tee_local 3 (i32.const 4)))
            (f64.add
              (f64.convert_s/i32 (tee_local 4 (i32.const 5)))
              (f64.add
                (f64.promote/f32 (tee_local 5 (f32.const 5.5)))
                (f64.add
                  (f64.convert_u/i64 (tee_local 6 (i64.const 6)))
                  (f64.add
                    (f64.convert_u/i64 (tee_local 7 (i64.const 0)))
                    (tee_local 8 (f64.const 8))
                  )
                )
              )
            )
          )
        )
      )
    )
  )

  (func (export "main")
    (drop (call $type-local-i32))
    (drop (call $type-local-i64))
    (drop (call $type-local-f32))
    (drop (call $type-local-f64))
    (drop (call $type-param-i32 (i32.const 2)))
    (drop (call $type-param-i64 (i64.const 3)))
    (drop (call $type-param-f32 (f32.const 4.4)))
    (drop (call $type-param-f64 (f64.const 5.5)))

    (call $type-mixed
      (i64.const 1) (f32.const 2.2) (f64.const 3.3) (i32.const 4) (i32.const 5)
    )

    (drop
      (call $write
        (i64.const 1) (f32.const 2) (f64.const 3.3) (i32.const 4) (i32.const 5)
      ))

    (drop
      (call $result
        (i64.const -1) (f32.const -2) (f64.const -3.3) (i32.const -4) (i32.const -5)
      ))
    )
)
