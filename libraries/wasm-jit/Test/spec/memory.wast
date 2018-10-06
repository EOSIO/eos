;; Test memory section structure
(module (memory 0 0))
(module (memory 0 1))
(module (memory 1 256))
(module (memory 0 65536))
(module (memory 0 0) (data (i32.const 0)))
(module (memory 0 0) (data (i32.const 0) ""))
(module (memory 1 1) (data (i32.const 0) "a"))
(module (memory 1 2) (data (i32.const 0) "a") (data (i32.const 65535) "b"))
(module (memory 1 2)
  (data (i32.const 0) "a") (data (i32.const 1) "b") (data (i32.const 2) "c")
)
(module (global (import "spectest" "global") i32) (memory 1) (data (get_global 0) "a"))
(module (global $g (import "spectest" "global") i32) (memory 1) (data (get_global $g) "a"))
;; Use of internal globals in constant expressions is not allowed in MVP.
;; (module (memory 1) (data (get_global 0) "a") (global i32 (i32.const 0)))
;; (module (memory 1) (data (get_global $g) "a") (global $g i32 (i32.const 0)))

(assert_invalid (module (memory 0) (memory 0)) "multiple memories")
(assert_invalid (module (memory (import "spectest" "memory") 0) (memory 0)) "multiple memories")

(module (memory (data)) (func (export "memsize") (result i32) (current_memory)))
(assert_return (invoke "memsize") (i32.const 0))
(module (memory (data "")) (func (export "memsize") (result i32) (current_memory)))
(assert_return (invoke "memsize") (i32.const 0))
(module (memory (data "x")) (func (export "memsize") (result i32) (current_memory)))
(assert_return (invoke "memsize") (i32.const 1))

(assert_invalid (module (data (i32.const 0))) "unknown memory")
(assert_invalid (module (data (i32.const 0) "")) "unknown memory")
(assert_invalid (module (data (i32.const 0) "x")) "unknown memory")

(assert_invalid
  (module (func (drop (f32.load (i32.const 0)))))
  "unknown memory"
)
(assert_invalid
  (module (func (f32.store (f32.const 0) (i32.const 0))))
  "unknown memory"
)
(assert_invalid
  (module (func (drop (i32.load8_s (i32.const 0)))))
  "unknown memory"
)
(assert_invalid
  (module (func (i32.store8 (i32.const 0) (i32.const 0))))
  "unknown memory"
)
(assert_invalid
  (module (func (drop (current_memory))))
  "unknown memory"
)
(assert_invalid
  (module (func (drop (grow_memory (i32.const 0)))))
  "unknown memory"
)

(assert_invalid
  (module (memory 1) (data (i64.const 0)))
  "type mismatch"
)
(assert_invalid
  (module (memory 1) (data (i32.ctz (i32.const 0))))
  "constant expression required"
)
(assert_invalid
  (module (memory 1) (data (nop)))
  "constant expression required"
)
;; Use of internal globals in constant expressions is not allowed in MVP.
;; (assert_invalid
;;   (module (memory 1) (data (get_global $g)) (global $g (mut i32) (i32.const 0)))
;;   "constant expression required"
;; )

(assert_unlinkable
  (module (memory 0 0) (data (i32.const 0) "a"))
  "data segment does not fit"
)
(assert_unlinkable
  (module (memory 0 1) (data (i32.const 0) "a"))
  "data segment does not fit"
)
(assert_unlinkable
  (module (memory 1 2) (data (i32.const -1) "a"))
  "data segment does not fit"
)
(assert_unlinkable
  (module (memory 1 2) (data (i32.const -1000) "a"))
  "data segment does not fit"
)
(assert_unlinkable
  (module (memory 1 2) (data (i32.const 0) "a") (data (i32.const 98304) "b"))
  "data segment does not fit"
)
(assert_unlinkable
  (module (memory 0 0) (data (i32.const 1) ""))
  "data segment does not fit"
)
(assert_unlinkable
  (module (memory 1) (data (i32.const 0x12000) ""))
  "data segment does not fit"
)
(assert_unlinkable
  (module (memory 1 2) (data (i32.const -1) ""))
  "data segment does not fit"
)
;; This seems to cause a time-out on Travis.
(;assert_unlinkable
  (module (memory 0x10000) (data (i32.const 0xffffffff) "ab"))
  ""  ;; either out of memory or segment does not fit
;)
(assert_unlinkable
  (module
    (global (import "spectest" "global") i32)
    (memory 0) (data (get_global 0) "a")
  )
  "data segment does not fit"
)

(module (memory 0 0) (data (i32.const 0) ""))
(module (memory 1 1) (data (i32.const 0x10000) ""))
(module (memory 1 2) (data (i32.const 0) "abc") (data (i32.const 0) "def"))
(module (memory 1 2) (data (i32.const 3) "ab") (data (i32.const 0) "de"))
(module
  (memory 1 2)
  (data (i32.const 0) "a") (data (i32.const 2) "b") (data (i32.const 1) "c")
)

(assert_invalid
  (module (memory 1 0))
  "size minimum must not be greater than maximum"
)
(assert_invalid
  (module (memory 65537))
  "memory size must be at most 65536 pages (4GiB)"
)
(assert_invalid
  (module (memory 2147483648))
  "memory size must be at most 65536 pages (4GiB)"
)
(assert_invalid
  (module (memory 4294967295))
  "memory size must be at most 65536 pages (4GiB)"
)
(assert_invalid
  (module (memory 0 65537))
  "memory size must be at most 65536 pages (4GiB)"
)
(assert_invalid
  (module (memory 0 2147483648))
  "memory size must be at most 65536 pages (4GiB)"
)
(assert_invalid
  (module (memory 0 4294967295))
  "memory size must be at most 65536 pages (4GiB)"
)

;; Test alignment annotation rules
(module (memory 0) (func (drop (i32.load8_u align=1 (i32.const 0)))))
(module (memory 0) (func (drop (i32.load16_u align=2 (i32.const 0)))))
(module (memory 0) (func (drop (i32.load align=4 (i32.const 0)))))
(module (memory 0) (func (drop (f32.load align=4 (i32.const 0)))))

(assert_invalid
  (module (memory 0) (func (drop (i64.load align=16 (i32.const 0)))))
  "alignment must not be larger than natural"
)
(assert_invalid
  (module (memory 0) (func (drop (i64.load align=32 (i32.const 0)))))
  "alignment must not be larger than natural"
)
(assert_invalid
  (module (memory 0) (func (drop (i32.load align=8 (i32.const 0)))))
  "alignment must not be larger than natural"
)
(assert_invalid
  (module (memory 0) (func (drop (i32.load16_u align=4 (i32.const 0)))))
  "alignment must not be larger than natural"
)
(assert_invalid
  (module (memory 0) (func (drop (i32.load8_u align=2 (i32.const 0)))))
  "alignment must not be larger than natural"
)
(assert_invalid
  (module (memory 0) (func (i32.store8 align=2 (i32.const 0) (i32.const 0))))
  "alignment must not be larger than natural"
)
(assert_invalid
  (module (memory 0) (func (i32.load16_u align=4 (i32.const 0))))
  "alignment must not be larger than natural"
)
(assert_invalid
  (module (memory 0) (func (i32.load8_u align=2 (i32.const 0))))
  "alignment must not be larger than natural"
)
(assert_invalid
  (module (memory 0) (func (i32.store8 align=2 (i32.const 0) (i32.const 0))))
  "alignment must not be larger than natural"
)

(module
  (memory 1)
  (data (i32.const 0) "ABC\a7D") (data (i32.const 20) "WASM")

  ;; Data section
  (func (export "data") (result i32)
    (i32.and
      (i32.and
        (i32.and
          (i32.eq (i32.load8_u (i32.const 0)) (i32.const 65))
          (i32.eq (i32.load8_u (i32.const 3)) (i32.const 167))
        )
        (i32.and
          (i32.eq (i32.load8_u (i32.const 6)) (i32.const 0))
          (i32.eq (i32.load8_u (i32.const 19)) (i32.const 0))
        )
      )
      (i32.and
        (i32.and
          (i32.eq (i32.load8_u (i32.const 20)) (i32.const 87))
          (i32.eq (i32.load8_u (i32.const 23)) (i32.const 77))
        )
        (i32.and
          (i32.eq (i32.load8_u (i32.const 24)) (i32.const 0))
          (i32.eq (i32.load8_u (i32.const 1023)) (i32.const 0))
        )
      )
    )
  )

  ;; Aligned read/write
  (func (export "aligned") (result i32)
    (local i32 i32 i32)
    (set_local 0 (i32.const 10))
    (block
      (loop
        (if
          (i32.eq (get_local 0) (i32.const 0))
          (then (br 2))
        )
        (set_local 2 (i32.mul (get_local 0) (i32.const 4)))
        (i32.store (get_local 2) (get_local 0))
        (set_local 1 (i32.load (get_local 2)))
        (if
          (i32.ne (get_local 0) (get_local 1))
          (then (return (i32.const 0)))
        )
        (set_local 0 (i32.sub (get_local 0) (i32.const 1)))
        (br 0)
      )
    )
    (i32.const 1)
  )

  ;; Unaligned read/write
  (func (export "unaligned") (result i32)
    (local i32 f64 f64)
    (set_local 0 (i32.const 10))
    (block
      (loop
        (if
          (i32.eq (get_local 0) (i32.const 0))
          (then (br 2))
        )
        (set_local 2 (f64.convert_s/i32 (get_local 0)))
        (f64.store align=1 (get_local 0) (get_local 2))
        (set_local 1 (f64.load align=1 (get_local 0)))
        (if
          (f64.ne (get_local 2) (get_local 1))
          (then (return (i32.const 0)))
        )
        (set_local 0 (i32.sub (get_local 0) (i32.const 1)))
        (br 0)
      )
    )
    (i32.const 1)
  )

  ;; Memory cast
  (func (export "cast") (result f64)
    (i64.store (i32.const 8) (i64.const -12345))
    (if
      (f64.eq
        (f64.load (i32.const 8))
        (f64.reinterpret/i64 (i64.const -12345))
      )
      (then (return (f64.const 0)))
    )
    (i64.store align=1 (i32.const 9) (i64.const 0))
    (i32.store16 align=1 (i32.const 15) (i32.const 16453))
    (f64.load align=1 (i32.const 9))
  )

  ;; Sign and zero extending memory loads
  (func (export "i32_load8_s") (param $i i32) (result i32)
	(i32.store8 (i32.const 8) (get_local $i))
	(i32.load8_s (i32.const 8))
  )
  (func (export "i32_load8_u") (param $i i32) (result i32)
	(i32.store8 (i32.const 8) (get_local $i))
	(i32.load8_u (i32.const 8))
  )
  (func (export "i32_load16_s") (param $i i32) (result i32)
	(i32.store16 (i32.const 8) (get_local $i))
	(i32.load16_s (i32.const 8))
  )
  (func (export "i32_load16_u") (param $i i32) (result i32)
	(i32.store16 (i32.const 8) (get_local $i))
	(i32.load16_u (i32.const 8))
  )
  (func (export "i64_load8_s") (param $i i64) (result i64)
	(i64.store8 (i32.const 8) (get_local $i))
	(i64.load8_s (i32.const 8))
  )
  (func (export "i64_load8_u") (param $i i64) (result i64)
	(i64.store8 (i32.const 8) (get_local $i))
	(i64.load8_u (i32.const 8))
  )
  (func (export "i64_load16_s") (param $i i64) (result i64)
	(i64.store16 (i32.const 8) (get_local $i))
	(i64.load16_s (i32.const 8))
  )
  (func (export "i64_load16_u") (param $i i64) (result i64)
	(i64.store16 (i32.const 8) (get_local $i))
	(i64.load16_u (i32.const 8))
  )
  (func (export "i64_load32_s") (param $i i64) (result i64)
	(i64.store32 (i32.const 8) (get_local $i))
	(i64.load32_s (i32.const 8))
  )
  (func (export "i64_load32_u") (param $i i64) (result i64)
	(i64.store32 (i32.const 8) (get_local $i))
	(i64.load32_u (i32.const 8))
  )
)

(assert_return (invoke "data") (i32.const 1))
(assert_return (invoke "aligned") (i32.const 1))
(assert_return (invoke "unaligned") (i32.const 1))
(assert_return (invoke "cast") (f64.const 42.0))

(assert_return (invoke "i32_load8_s" (i32.const -1)) (i32.const -1))
(assert_return (invoke "i32_load8_u" (i32.const -1)) (i32.const 255))
(assert_return (invoke "i32_load16_s" (i32.const -1)) (i32.const -1))
(assert_return (invoke "i32_load16_u" (i32.const -1)) (i32.const 65535))

(assert_return (invoke "i32_load8_s" (i32.const 100)) (i32.const 100))
(assert_return (invoke "i32_load8_u" (i32.const 200)) (i32.const 200))
(assert_return (invoke "i32_load16_s" (i32.const 20000)) (i32.const 20000))
(assert_return (invoke "i32_load16_u" (i32.const 40000)) (i32.const 40000))

(assert_return (invoke "i32_load8_s" (i32.const 0xfedc6543)) (i32.const 0x43))
(assert_return (invoke "i32_load8_s" (i32.const 0x3456cdef)) (i32.const 0xffffffef))
(assert_return (invoke "i32_load8_u" (i32.const 0xfedc6543)) (i32.const 0x43))
(assert_return (invoke "i32_load8_u" (i32.const 0x3456cdef)) (i32.const 0xef))
(assert_return (invoke "i32_load16_s" (i32.const 0xfedc6543)) (i32.const 0x6543))
(assert_return (invoke "i32_load16_s" (i32.const 0x3456cdef)) (i32.const 0xffffcdef))
(assert_return (invoke "i32_load16_u" (i32.const 0xfedc6543)) (i32.const 0x6543))
(assert_return (invoke "i32_load16_u" (i32.const 0x3456cdef)) (i32.const 0xcdef))

(assert_return (invoke "i64_load8_s" (i64.const -1)) (i64.const -1))
(assert_return (invoke "i64_load8_u" (i64.const -1)) (i64.const 255))
(assert_return (invoke "i64_load16_s" (i64.const -1)) (i64.const -1))
(assert_return (invoke "i64_load16_u" (i64.const -1)) (i64.const 65535))
(assert_return (invoke "i64_load32_s" (i64.const -1)) (i64.const -1))
(assert_return (invoke "i64_load32_u" (i64.const -1)) (i64.const 4294967295))

(assert_return (invoke "i64_load8_s" (i64.const 100)) (i64.const 100))
(assert_return (invoke "i64_load8_u" (i64.const 200)) (i64.const 200))
(assert_return (invoke "i64_load16_s" (i64.const 20000)) (i64.const 20000))
(assert_return (invoke "i64_load16_u" (i64.const 40000)) (i64.const 40000))
(assert_return (invoke "i64_load32_s" (i64.const 20000)) (i64.const 20000))
(assert_return (invoke "i64_load32_u" (i64.const 40000)) (i64.const 40000))

(assert_return (invoke "i64_load8_s" (i64.const 0xfedcba9856346543)) (i64.const 0x43))
(assert_return (invoke "i64_load8_s" (i64.const 0x3456436598bacdef)) (i64.const 0xffffffffffffffef))
(assert_return (invoke "i64_load8_u" (i64.const 0xfedcba9856346543)) (i64.const 0x43))
(assert_return (invoke "i64_load8_u" (i64.const 0x3456436598bacdef)) (i64.const 0xef))
(assert_return (invoke "i64_load16_s" (i64.const 0xfedcba9856346543)) (i64.const 0x6543))
(assert_return (invoke "i64_load16_s" (i64.const 0x3456436598bacdef)) (i64.const 0xffffffffffffcdef))
(assert_return (invoke "i64_load16_u" (i64.const 0xfedcba9856346543)) (i64.const 0x6543))
(assert_return (invoke "i64_load16_u" (i64.const 0x3456436598bacdef)) (i64.const 0xcdef))
(assert_return (invoke "i64_load32_s" (i64.const 0xfedcba9856346543)) (i64.const 0x56346543))
(assert_return (invoke "i64_load32_s" (i64.const 0x3456436598bacdef)) (i64.const 0xffffffff98bacdef))
(assert_return (invoke "i64_load32_u" (i64.const 0xfedcba9856346543)) (i64.const 0x56346543))
(assert_return (invoke "i64_load32_u" (i64.const 0x3456436598bacdef)) (i64.const 0x98bacdef))

(assert_malformed
  (module quote
    "(memory 1)"
    "(func (param i32) (result i32) (i32.load32 (get_local 0)))"
  )
  "unknown operator"
)
(assert_malformed
  (module quote
    "(memory 1)"
    "(func (param i32) (result i32) (i32.load32_u (get_local 0)))"
  )
  "unknown operator"
)
(assert_malformed
  (module quote
    "(memory 1)"
    "(func (param i32) (result i32) (i32.load32_s (get_local 0)))"
  )
  "unknown operator"
)
(assert_malformed
  (module quote
    "(memory 1)"
    "(func (param i32) (result i32) (i32.load64 (get_local 0)))"
  )
  "unknown operator"
)
(assert_malformed
  (module quote
    "(memory 1)"
    "(func (param i32) (result i32) (i32.load64_u (get_local 0)))"
  )
  "unknown operator"
)
(assert_malformed
  (module quote
    "(memory 1)"
    "(func (param i32) (result i32) (i32.load64_s (get_local 0)))"
  )
  "unknown operator"
)
(assert_malformed
  (module quote
    "(memory 1)"
    "(func (param i32) (i32.store32 (get_local 0) (i32.const 0)))"
  )
  "unknown operator"
)
(assert_malformed
  (module quote
    "(memory 1)"
    "(func (param i32) (i32.store64 (get_local 0) (i64.const 0)))"
  )
  "unknown operator"
)

(assert_malformed
  (module quote
    "(memory 1)"
    "(func (param i32) (result i64) (i64.load64 (get_local 0)))"
  )
  "unknown operator"
)
(assert_malformed
  (module quote
    "(memory 1)"
    "(func (param i32) (result i64) (i64.load64_u (get_local 0)))"
  )
  "unknown operator"
)
(assert_malformed
  (module quote
    "(memory 1)"
    "(func (param i32) (result i64) (i64.load64_s (get_local 0)))"
  )
  "unknown operator"
)
(assert_malformed
  (module quote
    "(memory 1)"
    "(func (param i32) (i64.store64 (get_local 0) (i64.const 0)))"
  )
  "unknown operator"
)

(assert_malformed
  (module quote
    "(memory 1)"
    "(func (param i32) (result f32) (f32.load32 (get_local 0)))"
  )
  "unknown operator"
)
(assert_malformed
  (module quote
    "(memory 1)"
    "(func (param i32) (result f32) (f32.load64 (get_local 0)))"
  )
  "unknown operator"
)
(assert_malformed
  (module quote
    "(memory 1)"
    "(func (param i32) (f32.store32 (get_local 0) (f32.const 0)))"
  )
  "unknown operator"
)
(assert_malformed
  (module quote
    "(memory 1)"
    "(func (param i32) (f32.store64 (get_local 0) (f64.const 0)))"
  )
  "unknown operator"
)

(assert_malformed
  (module quote
    "(memory 1)"
    "(func (param i32) (result f64) (f64.load32 (get_local 0)))"
  )
  "unknown operator"
)
(assert_malformed
  (module quote
    "(memory 1)"
    "(func (param i32) (result f64) (f64.load64 (get_local 0)))"
  )
  "unknown operator"
)
(assert_malformed
  (module quote
    "(memory 1)"
    "(func (param i32) (f64.store32 (get_local 0) (f32.const 0)))"
  )
  "unknown operator"
)
(assert_malformed
  (module quote
    "(memory 1)"
    "(func (param i32) (f64.store64 (get_local 0) (f64.const 0)))"
  )
  "unknown operator"
)
