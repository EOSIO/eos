;; Auxiliary module to import from

(module
  (func (export "func"))
  (func (export "func-i32") (param i32))
  (func (export "func-f32") (param f32))
  (func (export "func->i32") (result i32) (i32.const 22))
  (func (export "func->f32") (result f32) (f32.const 11))
  (func (export "func-i32->i32") (param i32) (result i32) (get_local 0))
  (func (export "func-i64->i64") (param i64) (result i64) (get_local 0))
  (global (export "global-i32") i32 (i32.const 55))
  (global (export "global-f32") f32 (f32.const 44))
  (table (export "table-10-inf") 10 anyfunc)
  ;; (table (export "table-10-20") 10 20 anyfunc)
  (memory (export "memory-2-inf") 2)
  ;; (memory (export "memory-2-4") 2 4)
)

(register "test")


;; Functions

(module
  (type $func_i32 (func (param i32)))
  (type $func_i64 (func (param i64)))
  (type $func_f32 (func (param f32)))
  (type $func_f64 (func (param f64)))

  (import "spectest" "print" (func (param i32)))
  ;; JavaScript can't handle i64 yet.
  ;; (func (import "spectest" "print") (param i64))
  (import "spectest" "print" (func $print_i32 (param i32)))
  ;; JavaScript can't handle i64 yet.
  ;; (import "spectest" "print" (func $print_i64 (param i64)))
  (import "spectest" "print" (func $print_f32 (param f32)))
  (import "spectest" "print" (func $print_f64 (param f64)))
  (import "spectest" "print" (func $print_i32_f32 (param i32 f32)))
  (import "spectest" "print" (func $print_f64_f64 (param f64 f64)))
  (func $print_i32-2 (import "spectest" "print") (param i32))
  (func $print_f64-2 (import "spectest" "print") (param f64))
  (import "test" "func-i64->i64" (func $i64->i64 (param i64) (result i64)))

  (func (export "p1") (import "spectest" "print") (param i32))
  (func $p (export "p2") (import "spectest" "print") (param i32))
  (func (export "p3") (export "p4") (import "spectest" "print") (param i32))
  (func (export "p5") (import "spectest" "print") (type 0))
  (func (export "p6") (import "spectest" "print") (type 0) (param i32) (result))

  (import "spectest" "print" (func (type $forward)))
  (func (import "spectest" "print") (type $forward))
  (type $forward (func (param i32)))

  (table anyfunc (elem $print_i32 $print_f64))

  (func (export "print32") (param $i i32)
    (local $x f32)
    (set_local $x (f32.convert_s/i32 (get_local $i)))
    (call 0 (get_local $i))
    (call $print_i32_f32
      (i32.add (get_local $i) (i32.const 1))
      (f32.const 42)
    )
    (call $print_i32 (get_local $i))
    (call $print_i32-2 (get_local $i))
    (call $print_f32 (get_local $x))
    (call_indirect $func_i32 (get_local $i) (i32.const 0))
  )

  (func (export "print64") (param $i i64)
    (local $x f64)
    (set_local $x (f64.convert_s/i64 (call $i64->i64 (get_local $i))))
    ;; JavaScript can't handle i64 yet.
    ;; (call 1 (get_local $i))
    (call $print_f64_f64
      (f64.add (get_local $x) (f64.const 1))
      (f64.const 53)
    )
    ;; JavaScript can't handle i64 yet.
    ;; (call $print_i64 (get_local $i))
    (call $print_f64 (get_local $x))
    (call $print_f64-2 (get_local $x))
    (call_indirect $func_f64 (get_local $x) (i32.const 1))
  )
)

(assert_return (invoke "print32" (i32.const 13)))
(assert_return (invoke "print64" (i64.const 24)))

(module (import "test" "func" (func)))
(module (import "test" "func-i32" (func (param i32))))
(module (import "test" "func-f32" (func (param f32))))
(module (import "test" "func->i32" (func (result i32))))
(module (import "test" "func->f32" (func (result f32))))
(module (import "test" "func-i32->i32" (func (param i32) (result i32))))
(module (import "test" "func-i64->i64" (func (param i64) (result i64))))

(assert_unlinkable
  (module (import "test" "unknown" (func)))
  "unknown import"
)
(assert_unlinkable
  (module (import "spectest" "unknown" (func)))
  "unknown import"
)

(assert_unlinkable
  (module (import "test" "func" (func (param i32))))
  "incompatible import type"
)
(assert_unlinkable
  (module (import "test" "func" (func (result i32))))
  "incompatible import type"
)
(assert_unlinkable
  (module (import "test" "func" (func (param i32) (result i32))))
  "incompatible import type"
)
(assert_unlinkable
  (module (import "test" "func-i32" (func)))
  "incompatible import type"
)
(assert_unlinkable
  (module (import "test" "func-i32" (func (result i32))))
  "incompatible import type"
)
(assert_unlinkable
  (module (import "test" "func-i32" (func (param f32))))
  "incompatible import type"
)
(assert_unlinkable
  (module (import "test" "func-i32" (func (param i64))))
  "incompatible import type"
)
(assert_unlinkable
  (module (import "test" "func-i32" (func (param i32) (result i32))))
  "incompatible import type"
)
(assert_unlinkable
  (module (import "test" "func->i32" (func)))
  "incompatible import type"
)
(assert_unlinkable
  (module (import "test" "func->i32" (func (param i32))))
  "incompatible import type"
)
(assert_unlinkable
  (module (import "test" "func->i32" (func (result f32))))
  "incompatible import type"
)
(assert_unlinkable
  (module (import "test" "func->i32" (func (result i64))))
  "incompatible import type"
)
(assert_unlinkable
  (module (import "test" "func->i32" (func (param i32) (result i32))))
  "incompatible import type"
)
(assert_unlinkable
  (module (import "test" "func-i32->i32" (func)))
  "incompatible import type"
)
(assert_unlinkable
  (module (import "test" "func-i32->i32" (func (param i32))))
  "incompatible import type"
)
(assert_unlinkable
  (module (import "test" "func-i32->i32" (func (result i32))))
  "incompatible import type"
)

(assert_unlinkable
  (module (import "test" "global-i32" (func (result i32))))
  "incompatible import type"
)
(assert_unlinkable
  (module (import "test" "table-10-inf" (func)))
  "incompatible import type"
)
(assert_unlinkable
  (module (import "test" "memory-2-inf" (func)))
  "incompatible import type"
)
(assert_unlinkable
  (module (import "spectest" "global" (func)))
  "incompatible import type"
)
(assert_unlinkable
  (module (import "spectest" "table" (func)))
  "incompatible import type"
)
(assert_unlinkable
  (module (import "spectest" "memory" (func)))
  "incompatible import type"
)


;; Globals

(module
  (import "spectest" "global" (global i32))
  (global (import "spectest" "global") i32)

  (import "spectest" "global" (global $x i32))
  (global $y (import "spectest" "global") i32)

  ;; JavaScript can't handle i64 yet.
  ;; (import "spectest" "global" (global i64))
  (import "spectest" "global" (global f32))
  (import "spectest" "global" (global f64))

  (func (export "get-0") (result i32) (get_global 0))
  (func (export "get-1") (result i32) (get_global 1))
  (func (export "get-x") (result i32) (get_global $x))
  (func (export "get-y") (result i32) (get_global $y))
)

(assert_return (invoke "get-0") (i32.const 666))
(assert_return (invoke "get-1") (i32.const 666))
(assert_return (invoke "get-x") (i32.const 666))
(assert_return (invoke "get-y") (i32.const 666))

(module (import "test" "global-i32" (global i32)))
(module (import "test" "global-f32" (global f32)))

(assert_unlinkable
  (module (import "test" "unknown" (global i32)))
  "unknown import"
)
(assert_unlinkable
  (module (import "spectest" "unknown" (global i32)))
  "unknown import"
)

(assert_unlinkable
  (module (import "test" "func" (global i32)))
  "incompatible import type"
)
(assert_unlinkable
  (module (import "test" "table-10-inf" (global i32)))
  "incompatible import type"
)
(assert_unlinkable
  (module (import "test" "memory-2-inf" (global i32)))
  "incompatible import type"
)
(assert_unlinkable
  (module (import "spectest" "print" (global i32)))
  "incompatible import type"
)
(assert_unlinkable
  (module (import "spectest" "table" (global i32)))
  "incompatible import type"
)
(assert_unlinkable
  (module (import "spectest" "memory" (global i32)))
  "incompatible import type"
)


;; Tables

(module
  (type (func (result i32)))
  (import "spectest" "table" (table 10 20 anyfunc))
  (elem 0 (i32.const 1) $f $g)

  (func (export "call") (param i32) (result i32) (call_indirect 0 (get_local 0)))
  (func $f (result i32) (i32.const 11))
  (func $g (result i32) (i32.const 22))
)

(assert_trap (invoke "call" (i32.const 0)) "uninitialized element")
(assert_return (invoke "call" (i32.const 1)) (i32.const 11))
(assert_return (invoke "call" (i32.const 2)) (i32.const 22))
(assert_trap (invoke "call" (i32.const 3)) "uninitialized element")
(assert_trap (invoke "call" (i32.const 100)) "undefined element")


(module
  (type (func (result i32)))
  (table (import "spectest" "table") 10 20 anyfunc)
  (elem 0 (i32.const 1) $f $g)

  (func (export "call") (param i32) (result i32) (call_indirect 0 (get_local 0)))
  (func $f (result i32) (i32.const 11))
  (func $g (result i32) (i32.const 22))
)

(assert_trap (invoke "call" (i32.const 0)) "uninitialized element")
(assert_return (invoke "call" (i32.const 1)) (i32.const 11))
(assert_return (invoke "call" (i32.const 2)) (i32.const 22))
(assert_trap (invoke "call" (i32.const 3)) "uninitialized element")
(assert_trap (invoke "call" (i32.const 100)) "undefined element")


(assert_invalid
  (module (import "" "" (table 10 anyfunc)) (import "" "" (table 10 anyfunc)))
  "multiple tables"
)
(assert_invalid
  (module (import "" "" (table 10 anyfunc)) (table 10 anyfunc))
  "multiple tables"
)
(assert_invalid
  (module (table 10 anyfunc) (table 10 anyfunc))
  "multiple tables"
)

(module (import "test" "table-10-inf" (table 10 anyfunc)))
(module (import "test" "table-10-inf" (table 5 anyfunc)))
(module (import "test" "table-10-inf" (table 0 anyfunc)))
(module (import "spectest" "table" (table 10 anyfunc)))
(module (import "spectest" "table" (table 5 anyfunc)))
(module (import "spectest" "table" (table 0 anyfunc)))
(module (import "spectest" "table" (table 10 20 anyfunc)))
(module (import "spectest" "table" (table 5 20 anyfunc)))
(module (import "spectest" "table" (table 0 20 anyfunc)))
(module (import "spectest" "table" (table 10 25 anyfunc)))
(module (import "spectest" "table" (table 5 25 anyfunc)))

(assert_unlinkable
  (module (import "test" "unknown" (table 10 anyfunc)))
  "unknown import"
)
(assert_unlinkable
  (module (import "spectest" "unknown" (table 10 anyfunc)))
  "unknown import"
)

(assert_unlinkable
  (module (import "test" "table-10-inf" (table 12 anyfunc)))
  "incompatible import type"
)
(assert_unlinkable
  (module (import "test" "table-10-inf" (table 10 20 anyfunc)))
  "incompatible import type"
)
(assert_unlinkable
  (module (import "spectest" "table" (table 12 anyfunc)))
  "incompatible import type"
)
(assert_unlinkable
  (module (import "spectest" "table" (table 10 15 anyfunc)))
  "incompatible import type"
)

(assert_unlinkable
  (module (import "test" "func" (table 10 anyfunc)))
  "incompatible import type"
)
(assert_unlinkable
  (module (import "test" "global-i32" (table 10 anyfunc)))
  "incompatible import type"
)
(assert_unlinkable
  (module (import "test" "memory-2-inf" (table 10 anyfunc)))
  "incompatible import type"
)
(assert_unlinkable
  (module (import "spectest" "print" (table 10 anyfunc)))
  "incompatible import type"
)



;; Memories

(module
  (import "spectest" "memory" (memory 1 2))
  (data 0 (i32.const 10) "\10")

  (func (export "load") (param i32) (result i32) (i32.load (get_local 0)))
)

(assert_return (invoke "load" (i32.const 0)) (i32.const 0))
(assert_return (invoke "load" (i32.const 10)) (i32.const 16))
(assert_return (invoke "load" (i32.const 8)) (i32.const 0x100000))
(assert_trap (invoke "load" (i32.const 1000000)) "out of bounds memory access")

(module
  (memory (import "spectest" "memory") 1 2)
  (data 0 (i32.const 10) "\10")

  (func (export "load") (param i32) (result i32) (i32.load (get_local 0)))
)
(assert_return (invoke "load" (i32.const 0)) (i32.const 0))
(assert_return (invoke "load" (i32.const 10)) (i32.const 16))
(assert_return (invoke "load" (i32.const 8)) (i32.const 0x100000))
(assert_trap (invoke "load" (i32.const 1000000)) "out of bounds memory access")

(assert_invalid
  (module (import "" "" (memory 1)) (import "" "" (memory 1)))
  "multiple memories"
)
(assert_invalid
  (module (import "" "" (memory 1)) (memory 0))
  "multiple memories"
)
(assert_invalid
  (module (memory 0) (memory 0))
  "multiple memories"
)

(module (import "test" "memory-2-inf" (memory 2)))
(module (import "test" "memory-2-inf" (memory 1)))
(module (import "test" "memory-2-inf" (memory 0)))
(module (import "spectest" "memory" (memory 1)))
(module (import "spectest" "memory" (memory 0)))
(module (import "spectest" "memory" (memory 1 2)))
(module (import "spectest" "memory" (memory 0 2)))
(module (import "spectest" "memory" (memory 1 3)))
(module (import "spectest" "memory" (memory 0 3)))

(assert_unlinkable
  (module (import "test" "unknown" (memory 1)))
  "unknown import"
)
(assert_unlinkable
  (module (import "spectest" "unknown" (memory 1)))
  "unknown import"
)

(assert_unlinkable
  (module (import "test" "memory-2-inf" (memory 3)))
  "incompatible import type"
)
(assert_unlinkable
  (module (import "test" "memory-2-inf" (memory 2 3)))
  "incompatible import type"
)
(assert_unlinkable
  (module (import "spectest" "memory" (memory 2)))
  "incompatible import type"
)
(assert_unlinkable
  (module (import "spectest" "memory" (memory 1 1)))
  "incompatible import type"
)

(assert_unlinkable
  (module (import "test" "func-i32" (memory 1)))
  "incompatible import type"
)
(assert_unlinkable
  (module (import "test" "global-i32" (memory 1)))
  "incompatible import type"
)
(assert_unlinkable
  (module (import "test" "table-10-inf" (memory 1)))
  "incompatible import type"
)
(assert_unlinkable
  (module (import "spectest" "print" (memory 1)))
  "incompatible import type"
)
(assert_unlinkable
  (module (import "spectest" "global" (memory 1)))
  "incompatible import type"
)
(assert_unlinkable
  (module (import "spectest" "table" (memory 1)))
  "incompatible import type"
)

(assert_unlinkable
  (module (import "spectest" "memory" (memory 2)))
  "incompatible import type"
)
(assert_unlinkable
  (module (import "spectest" "memory" (memory 1 1)))
  "incompatible import type"
)

(module
  (import "spectest" "memory" (memory 0 3))  ;; actual has max size 2
  (func (export "grow") (param i32) (result i32) (grow_memory (get_local 0)))
)
(assert_return (invoke "grow" (i32.const 0)) (i32.const 1))
(assert_return (invoke "grow" (i32.const 1)) (i32.const 1))
(assert_return (invoke "grow" (i32.const 0)) (i32.const 2))
(assert_return (invoke "grow" (i32.const 1)) (i32.const -1))
(assert_return (invoke "grow" (i32.const 0)) (i32.const 2))


;; Syntax errors

(assert_malformed
  (module quote "(func) (import \"\" \"\" (func))")
  "import after function"
)
(assert_malformed
  (module quote "(func) (import \"\" \"\" (global i64))")
  "import after function"
)
(assert_malformed
  (module quote "(func) (import \"\" \"\" (table 0 anyfunc))")
  "import after function"
)
(assert_malformed
  (module quote "(func) (import \"\" \"\" (memory 0))")
  "import after function"
)

(assert_malformed
  (module quote "(global i64 (i64.const 0)) (import \"\" \"\" (func))")
  "import after global"
)
(assert_malformed
  (module quote "(global i64 (i64.const 0)) (import \"\" \"\" (global f32))")
  "import after global"
)
(assert_malformed
  (module quote "(global i64 (i64.const 0)) (import \"\" \"\" (table 0 anyfunc))")
  "import after global"
)
(assert_malformed
  (module quote "(global i64 (i64.const 0)) (import \"\" \"\" (memory 0))")
  "import after global"
)

(assert_malformed
  (module quote "(table 0 anyfunc) (import \"\" \"\" (func))")
  "import after table"
)
(assert_malformed
  (module quote "(table 0 anyfunc) (import \"\" \"\" (global i32))")
  "import after table"
)
(assert_malformed
  (module quote "(table 0 anyfunc) (import \"\" \"\" (table 0 anyfunc))")
  "import after table"
)
(assert_malformed
  (module quote "(table 0 anyfunc) (import \"\" \"\" (memory 0))")
  "import after table"
)

(assert_malformed
  (module quote "(memory 0) (import \"\" \"\" (func))")
  "import after memory"
)
(assert_malformed
  (module quote "(memory 0) (import \"\" \"\" (global i32))")
  "import after memory"
)
(assert_malformed
  (module quote "(memory 0) (import \"\" \"\" (table 1 3 anyfunc))")
  "import after memory"
)
(assert_malformed
  (module quote "(memory 0) (import \"\" \"\" (memory 1 2))")
  "import after memory"
)
