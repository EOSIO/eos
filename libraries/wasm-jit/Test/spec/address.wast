(module
  (import "spectest" "print" (func $print (param i32)))

  (memory 1)
  (data (i32.const 0) "abcdefghijklmnopqrstuvwxyz")

  (func (export "good") (param $i i32)
    (call $print (i32.load8_u offset=0 (get_local $i)))  ;; 97 'a'
    (call $print (i32.load8_u offset=1 (get_local $i)))  ;; 98 'b'
    (call $print (i32.load8_u offset=2 (get_local $i)))  ;; 99 'c'
    (call $print (i32.load8_u offset=25 (get_local $i))) ;; 122 'z'

    (call $print (i32.load16_u offset=0 (get_local $i)))          ;; 25185 'ab'
    (call $print (i32.load16_u align=1 (get_local $i)))           ;; 25185 'ab'
    (call $print (i32.load16_u offset=1 align=1 (get_local $i)))  ;; 25442 'bc'
    (call $print (i32.load16_u offset=2 (get_local $i)))          ;; 25699 'cd'
    (call $print (i32.load16_u offset=25 align=1 (get_local $i))) ;; 122 'z\0'

    (call $print (i32.load offset=0 (get_local $i)))          ;; 1684234849 'abcd'
    (call $print (i32.load offset=1 align=1 (get_local $i)))  ;; 1701077858 'bcde'
    (call $print (i32.load offset=2 align=2 (get_local $i)))  ;; 1717920867 'cdef'
    (call $print (i32.load offset=25 align=1 (get_local $i))) ;; 122 'z\0\0\0'
  )

  (func (export "bad") (param $i i32)
    (drop (i32.load offset=4294967295 (get_local $i)))
  )
)

(invoke "good" (i32.const 0))
(invoke "good" (i32.const 65507))
(assert_trap (invoke "good" (i32.const 65508)) "out of bounds memory access")
(assert_trap (invoke "bad" (i32.const 0)) "out of bounds memory access")
(assert_trap (invoke "bad" (i32.const 1)) "out of bounds memory access")

(assert_malformed
  (module quote
    "(memory 1)"
    "(func (drop (i32.load offset=4294967296 (i32.const 0))))"
  )
  "i32 constant"
)
