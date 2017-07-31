(module
  (memory 1)

  ;; Stores an i16 value in little-endian-format
  (func $i16_store_little (param $address i32) (param $value i32)
    (i32.store8 (get_local $address) (get_local $value))
    (i32.store8 (i32.add (get_local $address) (i32.const 1)) (i32.shr_u (get_local $value) (i32.const 8)))
  )

  ;; Stores an i32 value in little-endian format
  (func $i32_store_little (param $address i32) (param $value i32)
    (call $i16_store_little (get_local $address) (get_local $value))
    (call $i16_store_little (i32.add (get_local $address) (i32.const 2)) (i32.shr_u (get_local $value) (i32.const 16)))
  )

  ;; Stores an i64 value in little-endian format
  (func $i64_store_little (param $address i32) (param $value i64)
    (call $i32_store_little (get_local $address) (i32.wrap/i64 (get_local $value)))
    (call $i32_store_little (i32.add (get_local $address) (i32.const 4)) (i32.wrap/i64 (i64.shr_u (get_local $value) (i64.const 32))))
  )

  ;; Loads an i16 value in little-endian format
  (func $i16_load_little (param $address i32) (result i32)
    (i32.or
      (i32.load8_u (get_local $address))
      (i32.shl (i32.load8_u (i32.add (get_local $address) (i32.const 1))) (i32.const 8))
    )
  )

  ;; Loads an i32 value in little-endian format
  (func $i32_load_little (param $address i32) (result i32)
    (i32.or
      (call $i16_load_little (get_local $address))
      (i32.shl (call $i16_load_little (i32.add (get_local $address) (i32.const 2))) (i32.const 16))
    )
  )

  ;; Loads an i64 value in little-endian format
  (func $i64_load_little (param $address i32) (result i64)
    (i64.or
      (i64.extend_u/i32 (call $i32_load_little (get_local $address)))
      (i64.shl (i64.extend_u/i32 (call $i32_load_little (i32.add (get_local $address) (i32.const 4)))) (i64.const 32))
    )
  )

  (func (export "i32_load16_s") (param $value i32) (result i32)
    (call $i16_store_little (i32.const 0) (get_local $value))
    (i32.load16_s (i32.const 0))
  )

  (func (export "i32_load16_u") (param $value i32) (result i32)
    (call $i16_store_little (i32.const 0) (get_local $value))
    (i32.load16_u (i32.const 0))
  )

  (func (export "i32_load") (param $value i32) (result i32)
    (call $i32_store_little (i32.const 0) (get_local $value))
    (i32.load (i32.const 0))
  )

  (func (export "i64_load16_s") (param $value i64) (result i64)
    (call $i16_store_little (i32.const 0) (i32.wrap/i64 (get_local $value)))
    (i64.load16_s (i32.const 0))
  )

  (func (export "i64_load16_u") (param $value i64) (result i64)
    (call $i16_store_little (i32.const 0) (i32.wrap/i64 (get_local $value)))
    (i64.load16_u (i32.const 0))
  )

  (func (export "i64_load32_s") (param $value i64) (result i64)
    (call $i32_store_little (i32.const 0) (i32.wrap/i64 (get_local $value)))
    (i64.load32_s (i32.const 0))
  )

  (func (export "i64_load32_u") (param $value i64) (result i64)
    (call $i32_store_little (i32.const 0) (i32.wrap/i64 (get_local $value)))
    (i64.load32_u (i32.const 0))
  )

  (func (export "i64_load") (param $value i64) (result i64)
    (call $i64_store_little (i32.const 0) (get_local $value))
    (i64.load (i32.const 0))
  )

  (func (export "f32_load") (param $value f32) (result f32)
    (call $i32_store_little (i32.const 0) (i32.reinterpret/f32 (get_local $value)))
    (f32.load (i32.const 0))
  )

  (func (export "f64_load") (param $value f64) (result f64)
    (call $i64_store_little (i32.const 0) (i64.reinterpret/f64 (get_local $value)))
    (f64.load (i32.const 0))
  )


  (func (export "i32_store16") (param $value i32) (result i32)
    (i32.store16 (i32.const 0) (get_local $value))
    (call $i16_load_little (i32.const 0))
  )

  (func (export "i32_store") (param $value i32) (result i32)
    (i32.store (i32.const 0) (get_local $value))
    (call $i32_load_little (i32.const 0))
  )

  (func (export "i64_store16") (param $value i64) (result i64)
    (i64.store16 (i32.const 0) (get_local $value))
    (i64.extend_u/i32 (call $i16_load_little (i32.const 0)))
  )

  (func (export "i64_store32") (param $value i64) (result i64)
    (i64.store32 (i32.const 0) (get_local $value))
    (i64.extend_u/i32 (call $i32_load_little (i32.const 0)))
  )

  (func (export "i64_store") (param $value i64) (result i64)
    (i64.store (i32.const 0) (get_local $value))
    (call $i64_load_little (i32.const 0))
  )

  (func (export "f32_store") (param $value f32) (result f32)
    (f32.store (i32.const 0) (get_local $value))
    (f32.reinterpret/i32 (call $i32_load_little (i32.const 0)))
  )

  (func (export "f64_store") (param $value f64) (result f64)
    (f64.store (i32.const 0) (get_local $value))
    (f64.reinterpret/i64 (call $i64_load_little (i32.const 0)))
  )
)

(assert_return (invoke "i32_load16_s" (i32.const -1)) (i32.const -1))
(assert_return (invoke "i32_load16_s" (i32.const -4242)) (i32.const -4242))
(assert_return (invoke "i32_load16_s" (i32.const 42)) (i32.const 42))
(assert_return (invoke "i32_load16_s" (i32.const 0x3210)) (i32.const 0x3210))

(assert_return (invoke "i32_load16_u" (i32.const -1)) (i32.const 0xFFFF))
(assert_return (invoke "i32_load16_u" (i32.const -4242)) (i32.const 61294))
(assert_return (invoke "i32_load16_u" (i32.const 42)) (i32.const 42))
(assert_return (invoke "i32_load16_u" (i32.const 0xCAFE)) (i32.const 0xCAFE))

(assert_return (invoke "i32_load" (i32.const -1)) (i32.const -1))
(assert_return (invoke "i32_load" (i32.const -42424242)) (i32.const -42424242))
(assert_return (invoke "i32_load" (i32.const 42424242)) (i32.const 42424242))
(assert_return (invoke "i32_load" (i32.const 0xABAD1DEA)) (i32.const 0xABAD1DEA))

(assert_return (invoke "i64_load16_s" (i64.const -1)) (i64.const -1))
(assert_return (invoke "i64_load16_s" (i64.const -4242)) (i64.const -4242))
(assert_return (invoke "i64_load16_s" (i64.const 42)) (i64.const 42))
(assert_return (invoke "i64_load16_s" (i64.const 0x3210)) (i64.const 0x3210))

(assert_return (invoke "i64_load16_u" (i64.const -1)) (i64.const 0xFFFF))
(assert_return (invoke "i64_load16_u" (i64.const -4242)) (i64.const 61294))
(assert_return (invoke "i64_load16_u" (i64.const 42)) (i64.const 42))
(assert_return (invoke "i64_load16_u" (i64.const 0xCAFE)) (i64.const 0xCAFE))

(assert_return (invoke "i64_load32_s" (i64.const -1)) (i64.const -1))
(assert_return (invoke "i64_load32_s" (i64.const -42424242)) (i64.const -42424242))
(assert_return (invoke "i64_load32_s" (i64.const 42424242)) (i64.const 42424242))
(assert_return (invoke "i64_load32_s" (i64.const 0x12345678)) (i64.const 0x12345678))

(assert_return (invoke "i64_load32_u" (i64.const -1)) (i64.const 0xFFFFFFFF))
(assert_return (invoke "i64_load32_u" (i64.const -42424242)) (i64.const 4252543054))
(assert_return (invoke "i64_load32_u" (i64.const 42424242)) (i64.const 42424242))
(assert_return (invoke "i64_load32_u" (i64.const 0xABAD1DEA)) (i64.const 0xABAD1DEA))

(assert_return (invoke "i64_load" (i64.const -1)) (i64.const -1))
(assert_return (invoke "i64_load" (i64.const -42424242)) (i64.const -42424242))
(assert_return (invoke "i64_load" (i64.const 0xABAD1DEA)) (i64.const 0xABAD1DEA))
(assert_return (invoke "i64_load" (i64.const 0xABADCAFEDEAD1DEA)) (i64.const 0xABADCAFEDEAD1DEA))

(assert_return (invoke "f32_load" (f32.const -1)) (f32.const -1))
(assert_return (invoke "f32_load" (f32.const 1234e-5)) (f32.const 1234e-5))
(assert_return (invoke "f32_load" (f32.const 4242.4242)) (f32.const 4242.4242))
(assert_return (invoke "f32_load" (f32.const 0x1.fffffep+127)) (f32.const 0x1.fffffep+127))

(assert_return (invoke "f64_load" (f64.const -1)) (f64.const -1))
(assert_return (invoke "f64_load" (f64.const 123456789e-5)) (f64.const 123456789e-5))
(assert_return (invoke "f64_load" (f64.const 424242.424242)) (f64.const 424242.424242))
(assert_return (invoke "f64_load" (f64.const 0x1.fffffffffffffp+1023)) (f64.const 0x1.fffffffffffffp+1023))


(assert_return (invoke "i32_store16" (i32.const -1)) (i32.const 0xFFFF))
(assert_return (invoke "i32_store16" (i32.const -4242)) (i32.const 61294))
(assert_return (invoke "i32_store16" (i32.const 42)) (i32.const 42))
(assert_return (invoke "i32_store16" (i32.const 0xCAFE)) (i32.const 0xCAFE))

(assert_return (invoke "i32_store" (i32.const -1)) (i32.const -1))
(assert_return (invoke "i32_store" (i32.const -4242)) (i32.const -4242))
(assert_return (invoke "i32_store" (i32.const 42424242)) (i32.const 42424242))
(assert_return (invoke "i32_store" (i32.const 0xDEADCAFE)) (i32.const 0xDEADCAFE))

(assert_return (invoke "i64_store16" (i64.const -1)) (i64.const 0xFFFF))
(assert_return (invoke "i64_store16" (i64.const -4242)) (i64.const 61294))
(assert_return (invoke "i64_store16" (i64.const 42)) (i64.const 42))
(assert_return (invoke "i64_store16" (i64.const 0xCAFE)) (i64.const 0xCAFE))

(assert_return (invoke "i64_store32" (i64.const -1)) (i64.const 0xFFFFFFFF))
(assert_return (invoke "i64_store32" (i64.const -4242)) (i64.const 4294963054))
(assert_return (invoke "i64_store32" (i64.const 42424242)) (i64.const 42424242))
(assert_return (invoke "i64_store32" (i64.const 0xDEADCAFE)) (i64.const 0xDEADCAFE))

(assert_return (invoke "i64_store" (i64.const -1)) (i64.const -1))
(assert_return (invoke "i64_store" (i64.const -42424242)) (i64.const -42424242))
(assert_return (invoke "i64_store" (i64.const 0xABAD1DEA)) (i64.const 0xABAD1DEA))
(assert_return (invoke "i64_store" (i64.const 0xABADCAFEDEAD1DEA)) (i64.const 0xABADCAFEDEAD1DEA))

(assert_return (invoke "f32_store" (f32.const -1)) (f32.const -1))
(assert_return (invoke "f32_store" (f32.const 1234e-5)) (f32.const 1234e-5))
(assert_return (invoke "f32_store" (f32.const 4242.4242)) (f32.const 4242.4242))
(assert_return (invoke "f32_store" (f32.const 0x1.fffffep+127)) (f32.const 0x1.fffffep+127))

(assert_return (invoke "f64_store" (f64.const -1)) (f64.const -1))
(assert_return (invoke "f64_store" (f64.const 123456789e-5)) (f64.const 123456789e-5))
(assert_return (invoke "f64_store" (f64.const 424242.424242)) (f64.const 424242.424242))
(assert_return (invoke "f64_store" (f64.const 0x1.fffffffffffffp+1023)) (f64.const 0x1.fffffffffffffp+1023))
