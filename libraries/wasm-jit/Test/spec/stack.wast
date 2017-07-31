(module
  (func (export "fac-expr") (param $n i64) (result i64)
    (local $i i64)
    (local $res i64)
    (set_local $i (get_local $n))
    (set_local $res (i64.const 1))
    (block $done
      (loop $loop
        (if
          (i64.eq (get_local $i) (i64.const 0))
          (then (br $done))
          (else
            (set_local $res (i64.mul (get_local $i) (get_local $res)))
            (set_local $i (i64.sub (get_local $i) (i64.const 1)))
          )
        )
        (br $loop)
      )
    )
    (get_local $res)
  )

  (func (export "fac-stack") (param $n i64) (result i64)
    (local $i i64)
    (local $res i64)
    (get_local $n)
    (set_local $i)
    (i64.const 1)
    (set_local $res)
    (block $done
      (loop $loop
        (get_local $i)
        (i64.const 0)
        (i64.eq)
        (if
          (then (br $done))
          (else
            (get_local $i)
            (get_local $res)
            (i64.mul)
            (set_local $res)
            (get_local $i)
            (i64.const 1)
            (i64.sub)
            (set_local $i)
          )
        )
        (br $loop)
      )
    )
    (get_local $res)
  )

  (func (export "fac-stack-raw") (param $n i64) (result i64)
    (local $i i64)
    (local $res i64)
    get_local $n
    set_local $i
    i64.const 1
    set_local $res
    block $done
      loop $loop
        get_local $i
        i64.const 0
        i64.eq
        if $body
          br $done
        else $body
          get_local $i
          get_local $res
          i64.mul
          set_local $res
          get_local $i
          i64.const 1
          i64.sub
          set_local $i
        end $body
        br $loop
      end $loop
    end $done
    get_local $res
  )

  (func (export "fac-mixed") (param $n i64) (result i64)
    (local $i i64)
    (local $res i64)
    (set_local $i (get_local $n))
    (set_local $res (i64.const 1))
    (block $done
      (loop $loop
        (i64.eq (get_local $i) (i64.const 0))
        (if
          (then (br $done))
          (else
            (i64.mul (get_local $i) (get_local $res))
            (set_local $res)
            (i64.sub (get_local $i) (i64.const 1))
            (set_local $i)
          )
        )
        (br $loop)
      )
    )
    (get_local $res)
  )

  (func (export "fac-mixed-raw") (param $n i64) (result i64)
    (local $i i64)
    (local $res i64)
    (set_local $i (get_local $n))
    (set_local $res (i64.const 1))
    block $done
      loop $loop
        (i64.eq (get_local $i) (i64.const 0))
        if
          br $done
        else
          (i64.mul (get_local $i) (get_local $res))
          set_local $res
          (i64.sub (get_local $i) (i64.const 1))
          set_local $i
        end
        br $loop
      end
    end
    get_local $res
  )
)

(assert_return (invoke "fac-expr" (i64.const 25)) (i64.const 7034535277573963776))
(assert_return (invoke "fac-stack" (i64.const 25)) (i64.const 7034535277573963776))
(assert_return (invoke "fac-mixed" (i64.const 25)) (i64.const 7034535277573963776))
