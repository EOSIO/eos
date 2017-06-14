;; Test t.const instructions

;; Syntax error

(module (func (i32.const 0xffffffff) drop))
(module (func (i32.const -0x80000000) drop))
(assert_malformed
  (module quote "(func (i32.const 0x100000000) drop)")
  "constant out of range"
)
(assert_malformed
  (module quote "(func (i32.const -0x80000001) drop)")
  "constant out of range"
)

(module (func (i32.const 4294967295) drop))
(module (func (i32.const -2147483648) drop))
(assert_malformed
  (module quote "(func (i32.const 4294967296) drop)")
  "constant out of range"
)
(assert_malformed
  (module quote "(func (i32.const -2147483649) drop)")
  "constant out of range"
)

(module (func (i64.const 0xffffffffffffffff) drop))
(module (func (i64.const -0x8000000000000000) drop))
(assert_malformed
  (module quote "(func (i64.const 0x10000000000000000) drop)")
  "constant out of range"
)
(assert_malformed
  (module quote "(func (i64.const -0x8000000000000001) drop)")
  "constant out of range"
)

(module (func (i64.const 18446744073709551615) drop))
(module (func (i64.const -9223372036854775808) drop))
(assert_malformed
  (module quote "(func (i64.const 18446744073709551616) drop)")
  "constant out of range"
)
(assert_malformed
  (module quote "(func (i64.const -9223372036854775809) drop)")
  "constant out of range"
)
