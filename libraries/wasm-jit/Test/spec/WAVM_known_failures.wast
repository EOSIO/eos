;;
;; From float_exprs.wast
;;

;; Test that x - 0.0 is not folded to x.

(module
  (func (export "f32.no_fold_sub_zero") (param $x f32) (result f32)
    (f32.sub (get_local $x) (f32.const 0.0)))
  (func (export "f64.no_fold_sub_zero") (param $x f64) (result f64)
    (f64.sub (get_local $x) (f64.const 0.0)))
)

;; should be (assert_return (invoke "f32.no_fold_sub_zero" (f32.const nan:0x200000)) (f32.const nan:0x600000))
(assert_return (invoke "f32.no_fold_sub_zero" (f32.const nan:0x200000)) (f32.const nan:0x200000))

;; should be (assert_return (invoke "f64.no_fold_sub_zero" (f64.const nan:0x4000000000000)) (f64.const nan:0xc000000000000))
(assert_return (invoke "f64.no_fold_sub_zero" (f64.const nan:0x4000000000000)) (f64.const nan:0x4000000000000))

;; Test that x*1.0 is not folded to x.
;; See IEEE 754-2008 10.4 "Literal meaning and value-changing optimizations".

(module
  (func (export "f32.no_fold_mul_one") (param $x f32) (result f32)
    (f32.mul (get_local $x) (f32.const 1.0)))
  (func (export "f64.no_fold_mul_one") (param $x f64) (result f64)
    (f64.mul (get_local $x) (f64.const 1.0)))
)

;; should be (assert_return (invoke "f32.no_fold_mul_one" (f32.const nan:0x200000)) (f32.const nan:0x600000))
(assert_return (invoke "f32.no_fold_mul_one" (f32.const nan:0x200000)) (f32.const nan:0x200000))

;; should be (assert_return (invoke "f64.no_fold_mul_one" (f64.const nan:0x4000000000000)) (f64.const nan:0xc000000000000))
(assert_return (invoke "f64.no_fold_mul_one" (f64.const nan:0x4000000000000)) (f64.const nan:0x4000000000000))

;; Test that x/1.0 is not folded to x.

(module
  (func (export "f32.no_fold_div_one") (param $x f32) (result f32)
    (f32.div (get_local $x) (f32.const 1.0)))
  (func (export "f64.no_fold_div_one") (param $x f64) (result f64)
    (f64.div (get_local $x) (f64.const 1.0)))
)

;; should be (assert_return (invoke "f32.no_fold_div_one" (f32.const nan:0x200000)) (f32.const nan:0x600000))
(assert_return (invoke "f32.no_fold_div_one" (f32.const nan:0x200000)) (f32.const nan:0x200000))

;; should be (assert_return (invoke "f64.no_fold_div_one" (f64.const nan:0x4000000000000)) (f64.const nan:0xc000000000000))
(assert_return (invoke "f64.no_fold_div_one" (f64.const nan:0x4000000000000)) (f64.const nan:0x4000000000000))


;; Test that x/-1.0 is not folded to -x.

(module
  (func (export "f32.no_fold_div_neg1") (param $x f32) (result f32)
    (f32.div (get_local $x) (f32.const -1.0)))
  (func (export "f64.no_fold_div_neg1") (param $x f64) (result f64)
    (f64.div (get_local $x) (f64.const -1.0)))
)

;; should be (assert_return (invoke "f32.no_fold_div_neg1" (f32.const nan:0x200000)) (f32.const nan:0x600000))
(assert_return (invoke "f32.no_fold_div_neg1" (f32.const nan:0x200000)) (f32.const -nan:0x200000))

;; should be (assert_return (invoke "f64.no_fold_div_neg1" (f64.const nan:0x4000000000000)) (f64.const nan:0xc000000000000))
(assert_return (invoke "f64.no_fold_div_neg1" (f64.const nan:0x4000000000000)) (f64.const -nan:0x4000000000000))

;; Test that -0.0 - x is not folded to -x.

(module
  (func (export "f32.no_fold_neg0_sub") (param $x f32) (result f32)
    (f32.sub (f32.const -0.0) (get_local $x)))
  (func (export "f64.no_fold_neg0_sub") (param $x f64) (result f64)
    (f64.sub (f64.const -0.0) (get_local $x)))
)

;; should be (assert_return (invoke "f32.no_fold_neg0_sub" (f32.const nan:0x200000)) (f32.const nan:0x600000))
(assert_return (invoke "f32.no_fold_neg0_sub" (f32.const nan:0x200000)) (f32.const -nan:0x200000))

;; should be (assert_return (invoke "f64.no_fold_neg0_sub" (f64.const nan:0x4000000000000)) (f64.const nan:0xc000000000000))
(assert_return (invoke "f64.no_fold_neg0_sub" (f64.const nan:0x4000000000000)) (f64.const -nan:0x4000000000000))

;; Test that -1.0 * x is not folded to -x.

(module
  (func (export "f32.no_fold_neg1_mul") (param $x f32) (result f32)
    (f32.mul (f32.const -1.0) (get_local $x)))
  (func (export "f64.no_fold_neg1_mul") (param $x f64) (result f64)
    (f64.mul (f64.const -1.0) (get_local $x)))
)

;; should be (assert_return (invoke "f32.no_fold_neg1_mul" (f32.const nan:0x200000)) (f32.const nan:0x600000))
(assert_return (invoke "f32.no_fold_neg1_mul" (f32.const nan:0x200000)) (f32.const -nan:0x200000))

;; should be (assert_return (invoke "f64.no_fold_neg1_mul" (f64.const nan:0x4000000000000)) (f64.const nan:0xc000000000000))
(assert_return (invoke "f64.no_fold_neg1_mul" (f64.const nan:0x4000000000000)) (f64.const -nan:0x4000000000000))

;; Test that demote(promote(x)) is not folded to x, and aside from NaN is
;; bit-preserving.

(module
  (func (export "no_fold_promote_demote") (param $x f32) (result f32)
    (f32.demote/f64 (f64.promote/f32 (get_local $x))))
)

;; should be (assert_return (invoke "no_fold_promote_demote" (f32.const nan:0x200000)) (f32.const nan:0x600000))
(assert_return (invoke "no_fold_promote_demote" (f32.const nan:0x200000)) (f32.const nan:0x200000))

;; Test that 0 - (-0 - x) is not optimized to x.
;; https://llvm.org/bugs/show_bug.cgi?id=26746

(module
  (func (export "llvm_pr26746") (param $x f32) (result f32)
    (f32.sub (f32.const 0.0) (f32.sub (f32.const -0.0) (get_local $x)))
  )
)

;; Test for improperly reassociating an addition and a conversion.
;; https://llvm.org/bugs/show_bug.cgi?id=27153

(module
  (func (export "llvm_pr27153") (param $x i32) (result f32)
    (f32.add (f32.convert_s/i32 (i32.and (get_local $x) (i32.const 268435455))) (f32.const -8388608.0))
  )
)

;; should be (assert_return (invoke "llvm_pr27153" (i32.const 33554434)) (f32.const 25165824.000000))
(assert_return (invoke "llvm_pr27153" (i32.const 33554434)) (f32.const 0x1.800002p+24))

;; Test that (float)x + (float)y is not optimized to (float)(x + y) when unsafe.
;; https://llvm.org/bugs/show_bug.cgi?id=27036

(module
  (func (export "llvm_pr27036") (param $x i32) (param $y i32) (result f32)
    (f32.add (f32.convert_s/i32 (i32.or (get_local $x) (i32.const -25034805)))
             (f32.convert_s/i32 (i32.and (get_local $y) (i32.const 14942208))))
  )
)

;; should be (assert_return (invoke "llvm_pr27036" (i32.const -25034805) (i32.const 14942208)) (f32.const -0x1.340068p+23))
(assert_return (invoke "llvm_pr27036" (i32.const -25034805) (i32.const 14942208)) (f32.const -0x1.34006ap+23))

;; test that various operations with no effect on non-NaN floats have the correct effect when operating on a NaN.
(module
  (func (export "f32.no_fold_sub_zero") (param $x i32) (result i32)
    (i32.and (i32.reinterpret/f32 (f32.sub (f32.reinterpret/i32 (get_local $x)) (f32.const 0.0)))
             (i32.const 0x7fc00000)))
  (func (export "f32.no_fold_neg0_sub") (param $x i32) (result i32)
    (i32.and (i32.reinterpret/f32 (f32.sub (f32.const -0.0) (f32.reinterpret/i32 (get_local $x))))
             (i32.const 0x7fc00000)))
  (func (export "f32.no_fold_mul_one") (param $x i32) (result i32)
    (i32.and (i32.reinterpret/f32 (f32.mul (f32.reinterpret/i32 (get_local $x)) (f32.const 1.0)))
             (i32.const 0x7fc00000)))
  (func (export "f32.no_fold_neg1_mul") (param $x i32) (result i32)
    (i32.and (i32.reinterpret/f32 (f32.mul (f32.const -1.0) (f32.reinterpret/i32 (get_local $x))))
             (i32.const 0x7fc00000)))
  (func (export "f32.no_fold_div_one") (param $x i32) (result i32)
    (i32.and (i32.reinterpret/f32 (f32.div (f32.reinterpret/i32 (get_local $x)) (f32.const 1.0)))
             (i32.const 0x7fc00000)))
  (func (export "f32.no_fold_div_neg1") (param $x i32) (result i32)
    (i32.and (i32.reinterpret/f32 (f32.div (f32.reinterpret/i32 (get_local $x)) (f32.const -1.0)))
             (i32.const 0x7fc00000)))
  (func (export "f64.no_fold_sub_zero") (param $x i64) (result i64)
    (i64.and (i64.reinterpret/f64 (f64.sub (f64.reinterpret/i64 (get_local $x)) (f64.const 0.0)))
             (i64.const 0x7ff8000000000000)))
  (func (export "f64.no_fold_neg0_sub") (param $x i64) (result i64)
    (i64.and (i64.reinterpret/f64 (f64.sub (f64.const -0.0) (f64.reinterpret/i64 (get_local $x))))
             (i64.const 0x7ff8000000000000)))
  (func (export "f64.no_fold_mul_one") (param $x i64) (result i64)
    (i64.and (i64.reinterpret/f64 (f64.mul (f64.reinterpret/i64 (get_local $x)) (f64.const 1.0)))
             (i64.const 0x7ff8000000000000)))
  (func (export "f64.no_fold_neg1_mul") (param $x i64) (result i64)
    (i64.and (i64.reinterpret/f64 (f64.mul (f64.const -1.0) (f64.reinterpret/i64 (get_local $x))))
             (i64.const 0x7ff8000000000000)))
  (func (export "f64.no_fold_div_one") (param $x i64) (result i64)
    (i64.and (i64.reinterpret/f64 (f64.div (f64.reinterpret/i64 (get_local $x)) (f64.const 1.0)))
             (i64.const 0x7ff8000000000000)))
  (func (export "f64.no_fold_div_neg1") (param $x i64) (result i64)
    (i64.and (i64.reinterpret/f64 (f64.div (f64.reinterpret/i64 (get_local $x)) (f64.const -1.0)))
             (i64.const 0x7ff8000000000000)))
  (func (export "no_fold_promote_demote") (param $x i32) (result i32)
    (i32.and (i32.reinterpret/f32 (f32.demote/f64 (f64.promote/f32 (f32.reinterpret/i32 (get_local $x)))))
             (i32.const 0x7fc00000)))
)
(assert_return (invoke "f32.no_fold_sub_zero" (i32.const 0x7fa00000)) (i32.const 0x7f800000))
(assert_return (invoke "f32.no_fold_neg0_sub" (i32.const 0x7fa00000)) (i32.const 0x7f800000))
(assert_return (invoke "f32.no_fold_mul_one" (i32.const 0x7fa00000)) (i32.const 0x7f800000))
(assert_return (invoke "f32.no_fold_neg1_mul" (i32.const 0x7fa00000)) (i32.const 0x7f800000))
(assert_return (invoke "f32.no_fold_div_one" (i32.const 0x7fa00000)) (i32.const 0x7f800000))
(assert_return (invoke "f32.no_fold_div_neg1" (i32.const 0x7fa00000)) (i32.const 0x7f800000))
(assert_return (invoke "f64.no_fold_sub_zero" (i64.const 0x7ff4000000000000)) (i64.const 0x7ff0000000000000))
(assert_return (invoke "f64.no_fold_neg0_sub" (i64.const 0x7ff4000000000000)) (i64.const 0x7ff0000000000000))
(assert_return (invoke "f64.no_fold_mul_one" (i64.const 0x7ff4000000000000)) (i64.const 0x7ff0000000000000))
(assert_return (invoke "f64.no_fold_neg1_mul" (i64.const 0x7ff4000000000000)) (i64.const 0x7ff0000000000000))
(assert_return (invoke "f64.no_fold_div_one" (i64.const 0x7ff4000000000000)) (i64.const 0x7ff0000000000000))
(assert_return (invoke "f64.no_fold_div_neg1" (i64.const 0x7ff4000000000000)) (i64.const 0x7ff0000000000000))
(assert_return (invoke "no_fold_promote_demote" (i32.const 0x7fa00000)) (i32.const 0x7f800000))

;;
;; from int_exprs.wast
;;

(module
  (func (export "i32.no_fold_div_neg1") (param $x i32) (result i32)
    (i32.div_s (get_local $x) (i32.const -1)))

  (func (export "i64.no_fold_div_neg1") (param $x i64) (result i64)
    (i64.div_s (get_local $x) (i64.const -1)))
)

(assert_return (invoke "i32.no_fold_div_neg1" (i32.const 0x80000000)) (i32.const 0x80000000))
(assert_return (invoke "i64.no_fold_div_neg1" (i64.const 0x8000000000000000)) (i64.const 0x8000000000000000))
;; should be:
;;(assert_trap (invoke "i32.no_fold_div_neg1" (i32.const 0x80000000)) "integer overflow")
;;(assert_trap (invoke "i64.no_fold_div_neg1" (i64.const 0x8000000000000000)) "integer overflow")