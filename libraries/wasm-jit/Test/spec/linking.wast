;; Functions

(module $Mf
  (func (export "call") (result i32) (call $g))
  (func $g (result i32) (i32.const 2))
)
(register "Mf" $Mf)

(module $Nf
  (func $f (import "Mf" "call") (result i32))
  (export "Mf.call" (func $f))
  (func (export "call Mf.call") (result i32) (call $f))
  (func (export "call") (result i32) (call $g))
  (func $g (result i32) (i32.const 3))
)

(assert_return (invoke $Mf "call") (i32.const 2))
(assert_return (invoke $Nf "Mf.call") (i32.const 2))
(assert_return (invoke $Nf "call") (i32.const 3))
(assert_return (invoke $Nf "call Mf.call") (i32.const 2))

(module
  (import "spectest" "print" (func $f (param i32)))
  (export "print" (func $f))
)
(register "reexport_f")
(assert_unlinkable
  (module (import "reexport_f" "print" (func (param i64))))
  "type mismatch"
)
(assert_unlinkable
  (module (import "reexport_f" "print" (func (param i32) (result i32))))
  "type mismatch"
)


;; Globals

(module $Mg
  (global $glob (export "glob") i32 (i32.const 42))
  (func (export "get") (result i32) (get_global $glob))
)
(register "Mg" $Mg)

(module $Ng
  (global $x (import "Mg" "glob") i32)
  (func $f (import "Mg" "get") (result i32))
  (export "Mg.glob" (global $x))
  (export "Mg.get" (func $f))
  (global $glob (export "glob") i32 (i32.const 43))
  (func (export "get") (result i32) (get_global $glob))
)

(assert_return (get $Mg "glob") (i32.const 42))
(assert_return (get $Ng "Mg.glob") (i32.const 42))
(assert_return (get $Ng "glob") (i32.const 43))
(assert_return (invoke $Mg "get") (i32.const 42))
(assert_return (invoke $Ng "Mg.get") (i32.const 42))
(assert_return (invoke $Ng "get") (i32.const 43))


;; Tables

(module $Mt
  (type (func (result i32)))
  (type (func))

  (table (export "tab") 10 anyfunc)
  (elem (i32.const 2) $g $g $g $g)
  (func $g (result i32) (i32.const 4))
  (func (export "h") (result i32) (i32.const -4))

  (func (export "call") (param i32) (result i32)
    (call_indirect 0 (get_local 0))
  )
)
(register "Mt" $Mt)

(module $Nt
  (type (func))
  (type (func (result i32)))

  (func $f (import "Mt" "call") (param i32) (result i32))
  (func $h (import "Mt" "h") (result i32))

  (table anyfunc (elem $g $g $g $h $f))
  (func $g (result i32) (i32.const 5))

  (export "Mt.call" (func $f))
  (func (export "call Mt.call") (param i32) (result i32)
    (call $f (get_local 0))
  )
  (func (export "call") (param i32) (result i32)
    (call_indirect 1 (get_local 0))
  )
)

(assert_return (invoke $Mt "call" (i32.const 2)) (i32.const 4))
(assert_return (invoke $Nt "Mt.call" (i32.const 2)) (i32.const 4))
(assert_return (invoke $Nt "call" (i32.const 2)) (i32.const 5))
(assert_return (invoke $Nt "call Mt.call" (i32.const 2)) (i32.const 4))

(assert_trap (invoke $Mt "call" (i32.const 1)) "uninitialized")
(assert_trap (invoke $Nt "Mt.call" (i32.const 1)) "uninitialized")
(assert_return (invoke $Nt "call" (i32.const 1)) (i32.const 5))
(assert_trap (invoke $Nt "call Mt.call" (i32.const 1)) "uninitialized")

(assert_trap (invoke $Mt "call" (i32.const 0)) "uninitialized")
(assert_trap (invoke $Nt "Mt.call" (i32.const 0)) "uninitialized")
(assert_return (invoke $Nt "call" (i32.const 0)) (i32.const 5))
(assert_trap (invoke $Nt "call Mt.call" (i32.const 0)) "uninitialized")

(assert_trap (invoke $Mt "call" (i32.const 20)) "undefined")
(assert_trap (invoke $Nt "Mt.call" (i32.const 20)) "undefined")
(assert_trap (invoke $Nt "call" (i32.const 7)) "undefined")
(assert_trap (invoke $Nt "call Mt.call" (i32.const 20)) "undefined")

(assert_return (invoke $Nt "call" (i32.const 3)) (i32.const -4))
(assert_trap (invoke $Nt "call" (i32.const 4)) "indirect call")

(module $Ot
  (type (func (result i32)))

  (func $h (import "Mt" "h") (result i32))
  (table (import "Mt" "tab") 5 anyfunc)
  (elem (i32.const 1) $i $h)
  (func $i (result i32) (i32.const 6))

  (func (export "call") (param i32) (result i32)
    (call_indirect 0 (get_local 0))
  )
)

(assert_return (invoke $Mt "call" (i32.const 3)) (i32.const 4))
(assert_return (invoke $Nt "Mt.call" (i32.const 3)) (i32.const 4))
(assert_return (invoke $Nt "call Mt.call" (i32.const 3)) (i32.const 4))
(assert_return (invoke $Ot "call" (i32.const 3)) (i32.const 4))

(assert_return (invoke $Mt "call" (i32.const 2)) (i32.const -4))
(assert_return (invoke $Nt "Mt.call" (i32.const 2)) (i32.const -4))
(assert_return (invoke $Nt "call" (i32.const 2)) (i32.const 5))
(assert_return (invoke $Nt "call Mt.call" (i32.const 2)) (i32.const -4))
(assert_return (invoke $Ot "call" (i32.const 2)) (i32.const -4))

(assert_return (invoke $Mt "call" (i32.const 1)) (i32.const 6))
(assert_return (invoke $Nt "Mt.call" (i32.const 1)) (i32.const 6))
(assert_return (invoke $Nt "call" (i32.const 1)) (i32.const 5))
(assert_return (invoke $Nt "call Mt.call" (i32.const 1)) (i32.const 6))
(assert_return (invoke $Ot "call" (i32.const 1)) (i32.const 6))

(assert_trap (invoke $Mt "call" (i32.const 0)) "uninitialized")
(assert_trap (invoke $Nt "Mt.call" (i32.const 0)) "uninitialized")
(assert_return (invoke $Nt "call" (i32.const 0)) (i32.const 5))
(assert_trap (invoke $Nt "call Mt.call" (i32.const 0)) "uninitialized")
(assert_trap (invoke $Ot "call" (i32.const 0)) "uninitialized")

(assert_trap (invoke $Ot "call" (i32.const 20)) "undefined")

(module
  (table (import "Mt" "tab") 0 anyfunc)
  (elem (i32.const 9) $f)
  (func $f)
)

(assert_unlinkable
  (module
    (table (import "Mt" "tab") 0 anyfunc)
    (elem (i32.const 10) $f)
    (func $f)
  )
  "elements segment does not fit"
)

(assert_unlinkable
  (module
    (table (import "Mt" "tab") 10 anyfunc)
    (memory (import "Mt" "mem") 1)  ;; does not exist
    (func $f (result i32) (i32.const 0))
    (elem (i32.const 7) $f)
    (elem (i32.const 9) $f)
  )
  "unknown import"
)
(assert_trap (invoke $Mt "call" (i32.const 7)) "uninitialized")

(assert_unlinkable
  (module
    (table (import "Mt" "tab") 10 anyfunc)
    (func $f (result i32) (i32.const 0))
    (elem (i32.const 7) $f)
    (elem (i32.const 12) $f)  ;; out of bounds
  )
  "elements segment does not fit"
)
(assert_trap (invoke $Mt "call" (i32.const 7)) "uninitialized")

(assert_unlinkable
  (module
    (table (import "Mt" "tab") 10 anyfunc)
    (func $f (result i32) (i32.const 0))
    (elem (i32.const 7) $f)
    (memory 1)
    (data (i32.const 0x10000) "d") ;; out of bounds
  )
  "data segment does not fit"
)
(assert_trap (invoke $Mt "call" (i32.const 7)) "uninitialized")


;; Memories

(module $Mm
  (memory (export "mem") 1 5)
  (data (i32.const 10) "\00\01\02\03\04\05\06\07\08\09")

  (func (export "load") (param $a i32) (result i32)
    (i32.load8_u (get_local 0))
  )
)
(register "Mm" $Mm)

(module $Nm
  (func $loadM (import "Mm" "load") (param i32) (result i32))

  (memory 1)
  (data (i32.const 10) "\f0\f1\f2\f3\f4\f5")

  (export "Mm.load" (func $loadM))
  (func (export "load") (param $a i32) (result i32)
    (i32.load8_u (get_local 0))
  )
)

(assert_return (invoke $Mm "load" (i32.const 12)) (i32.const 2))
(assert_return (invoke $Nm "Mm.load" (i32.const 12)) (i32.const 2))
(assert_return (invoke $Nm "load" (i32.const 12)) (i32.const 0xf2))

(module $Om
  (memory (import "Mm" "mem") 1)
  (data (i32.const 5) "\a0\a1\a2\a3\a4\a5\a6\a7")

  (func (export "load") (param $a i32) (result i32)
    (i32.load8_u (get_local 0))
  )
)

(assert_return (invoke $Mm "load" (i32.const 12)) (i32.const 0xa7))
(assert_return (invoke $Nm "Mm.load" (i32.const 12)) (i32.const 0xa7))
(assert_return (invoke $Nm "load" (i32.const 12)) (i32.const 0xf2))
(assert_return (invoke $Om "load" (i32.const 12)) (i32.const 0xa7))

(module
  (memory (import "Mm" "mem") 0)
  (data (i32.const 0xffff) "a")
)

(assert_unlinkable
  (module
    (memory (import "Mm" "mem") 0)
    (data (i32.const 0x10000) "a")
  )
  "data segment does not fit"
)

(module $Pm
  (memory (import "Mm" "mem") 1 8)

  (func (export "grow") (param $a i32) (result i32)
    (grow_memory (get_local 0))
  )
)

(assert_return (invoke $Pm "grow" (i32.const 0)) (i32.const 1))
(assert_return (invoke $Pm "grow" (i32.const 2)) (i32.const 1))
(assert_return (invoke $Pm "grow" (i32.const 0)) (i32.const 3))
(assert_return (invoke $Pm "grow" (i32.const 1)) (i32.const 3))
(assert_return (invoke $Pm "grow" (i32.const 1)) (i32.const 4))
(assert_return (invoke $Pm "grow" (i32.const 0)) (i32.const 5))
(assert_return (invoke $Pm "grow" (i32.const 1)) (i32.const -1))
(assert_return (invoke $Pm "grow" (i32.const 0)) (i32.const 5))

(assert_unlinkable
  (module
    (func $host (import "spectest" "print"))
    (memory (import "Mm" "mem") 1)
    (table (import "Mm" "tab") 0 anyfunc)  ;; does not exist
    (data (i32.const 0) "abc")
  )
  "unknown import"
)
(assert_return (invoke $Mm "load" (i32.const 0)) (i32.const 0))

(assert_unlinkable
  (module
    (memory (import "Mm" "mem") 1)
    (data (i32.const 0) "abc")
    (data (i32.const 0x50000) "d") ;; out of bounds
  )
  "data segment does not fit"
)
(assert_return (invoke $Mm "load" (i32.const 0)) (i32.const 0))

(assert_unlinkable
  (module
    (memory (import "Mm" "mem") 1)
    (data (i32.const 0) "abc")
    (table 0 anyfunc)
    (func)
    (elem (i32.const 0) 0) ;; out of bounds
  )
  "elements segment does not fit"
)
(assert_return (invoke $Mm "load" (i32.const 0)) (i32.const 0))
