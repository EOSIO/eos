;; atomic operations

(module
  (memory 1 1)

  (func (export "init") (param $value i64) (i64.store (i32.const 0) (get_local $value)))

  (func (export "i32.atomic.load") (param $addr i32) (result i32) (i32.atomic.load (get_local $addr)))
  (func (export "i64.atomic.load") (param $addr i32) (result i64) (i64.atomic.load (get_local $addr)))
  (func (export "i32.atomic.load8_u") (param $addr i32) (result i32) (i32.atomic.load8_u (get_local $addr)))
  (func (export "i32.atomic.load16_u") (param $addr i32) (result i32) (i32.atomic.load16_u (get_local $addr)))
  (func (export "i64.atomic.load8_u") (param $addr i32) (result i64) (i64.atomic.load8_u (get_local $addr)))
  (func (export "i64.atomic.load16_u") (param $addr i32) (result i64) (i64.atomic.load16_u (get_local $addr)))
  (func (export "i64.atomic.load32_u") (param $addr i32) (result i64) (i64.atomic.load32_u (get_local $addr)))

  (func (export "i32.atomic.store") (param $addr i32) (param $value i32) (i32.atomic.store (get_local $addr) (get_local $value)))
  (func (export "i64.atomic.store") (param $addr i32) (param $value i64) (i64.atomic.store (get_local $addr) (get_local $value)))
  (func (export "i32.atomic.store8") (param $addr i32) (param $value i32) (i32.atomic.store8 (get_local $addr) (get_local $value)))
  (func (export "i32.atomic.store16") (param $addr i32) (param $value i32) (i32.atomic.store16 (get_local $addr) (get_local $value)))
  (func (export "i64.atomic.store8") (param $addr i32) (param $value i64) (i64.atomic.store8 (get_local $addr) (get_local $value)))
  (func (export "i64.atomic.store16") (param $addr i32) (param $value i64) (i64.atomic.store16 (get_local $addr) (get_local $value)))
  (func (export "i64.atomic.store32") (param $addr i32) (param $value i64) (i64.atomic.store32 (get_local $addr) (get_local $value)))

  (func (export "i32.atomic.rmw.add") (param $addr i32) (param $value i32) (result i32) (i32.atomic.rmw.add (get_local $addr) (get_local $value)))
  (func (export "i64.atomic.rmw.add") (param $addr i32) (param $value i64) (result i64) (i64.atomic.rmw.add (get_local $addr) (get_local $value)))
  (func (export "i32.atomic.rmw8_u.add") (param $addr i32) (param $value i32) (result i32) (i32.atomic.rmw8_u.add (get_local $addr) (get_local $value)))
  (func (export "i32.atomic.rmw16_u.add") (param $addr i32) (param $value i32) (result i32) (i32.atomic.rmw16_u.add (get_local $addr) (get_local $value)))
  (func (export "i64.atomic.rmw8_u.add") (param $addr i32) (param $value i64) (result i64) (i64.atomic.rmw8_u.add (get_local $addr) (get_local $value)))
  (func (export "i64.atomic.rmw16_u.add") (param $addr i32) (param $value i64) (result i64) (i64.atomic.rmw16_u.add (get_local $addr) (get_local $value)))
  (func (export "i64.atomic.rmw32_u.add") (param $addr i32) (param $value i64) (result i64) (i64.atomic.rmw32_u.add (get_local $addr) (get_local $value)))

  (func (export "i32.atomic.rmw.sub") (param $addr i32) (param $value i32) (result i32) (i32.atomic.rmw.sub (get_local $addr) (get_local $value)))
  (func (export "i64.atomic.rmw.sub") (param $addr i32) (param $value i64) (result i64) (i64.atomic.rmw.sub (get_local $addr) (get_local $value)))
  (func (export "i32.atomic.rmw8_u.sub") (param $addr i32) (param $value i32) (result i32) (i32.atomic.rmw8_u.sub (get_local $addr) (get_local $value)))
  (func (export "i32.atomic.rmw16_u.sub") (param $addr i32) (param $value i32) (result i32) (i32.atomic.rmw16_u.sub (get_local $addr) (get_local $value)))
  (func (export "i64.atomic.rmw8_u.sub") (param $addr i32) (param $value i64) (result i64) (i64.atomic.rmw8_u.sub (get_local $addr) (get_local $value)))
  (func (export "i64.atomic.rmw16_u.sub") (param $addr i32) (param $value i64) (result i64) (i64.atomic.rmw16_u.sub (get_local $addr) (get_local $value)))
  (func (export "i64.atomic.rmw32_u.sub") (param $addr i32) (param $value i64) (result i64) (i64.atomic.rmw32_u.sub (get_local $addr) (get_local $value)))

  (func (export "i32.atomic.rmw.and") (param $addr i32) (param $value i32) (result i32) (i32.atomic.rmw.and (get_local $addr) (get_local $value)))
  (func (export "i64.atomic.rmw.and") (param $addr i32) (param $value i64) (result i64) (i64.atomic.rmw.and (get_local $addr) (get_local $value)))
  (func (export "i32.atomic.rmw8_u.and") (param $addr i32) (param $value i32) (result i32) (i32.atomic.rmw8_u.and (get_local $addr) (get_local $value)))
  (func (export "i32.atomic.rmw16_u.and") (param $addr i32) (param $value i32) (result i32) (i32.atomic.rmw16_u.and (get_local $addr) (get_local $value)))
  (func (export "i64.atomic.rmw8_u.and") (param $addr i32) (param $value i64) (result i64) (i64.atomic.rmw8_u.and (get_local $addr) (get_local $value)))
  (func (export "i64.atomic.rmw16_u.and") (param $addr i32) (param $value i64) (result i64) (i64.atomic.rmw16_u.and (get_local $addr) (get_local $value)))
  (func (export "i64.atomic.rmw32_u.and") (param $addr i32) (param $value i64) (result i64) (i64.atomic.rmw32_u.and (get_local $addr) (get_local $value)))

  (func (export "i32.atomic.rmw.or") (param $addr i32) (param $value i32) (result i32) (i32.atomic.rmw.or (get_local $addr) (get_local $value)))
  (func (export "i64.atomic.rmw.or") (param $addr i32) (param $value i64) (result i64) (i64.atomic.rmw.or (get_local $addr) (get_local $value)))
  (func (export "i32.atomic.rmw8_u.or") (param $addr i32) (param $value i32) (result i32) (i32.atomic.rmw8_u.or (get_local $addr) (get_local $value)))
  (func (export "i32.atomic.rmw16_u.or") (param $addr i32) (param $value i32) (result i32) (i32.atomic.rmw16_u.or (get_local $addr) (get_local $value)))
  (func (export "i64.atomic.rmw8_u.or") (param $addr i32) (param $value i64) (result i64) (i64.atomic.rmw8_u.or (get_local $addr) (get_local $value)))
  (func (export "i64.atomic.rmw16_u.or") (param $addr i32) (param $value i64) (result i64) (i64.atomic.rmw16_u.or (get_local $addr) (get_local $value)))
  (func (export "i64.atomic.rmw32_u.or") (param $addr i32) (param $value i64) (result i64) (i64.atomic.rmw32_u.or (get_local $addr) (get_local $value)))

  (func (export "i32.atomic.rmw.xor") (param $addr i32) (param $value i32) (result i32) (i32.atomic.rmw.xor (get_local $addr) (get_local $value)))
  (func (export "i64.atomic.rmw.xor") (param $addr i32) (param $value i64) (result i64) (i64.atomic.rmw.xor (get_local $addr) (get_local $value)))
  (func (export "i32.atomic.rmw8_u.xor") (param $addr i32) (param $value i32) (result i32) (i32.atomic.rmw8_u.xor (get_local $addr) (get_local $value)))
  (func (export "i32.atomic.rmw16_u.xor") (param $addr i32) (param $value i32) (result i32) (i32.atomic.rmw16_u.xor (get_local $addr) (get_local $value)))
  (func (export "i64.atomic.rmw8_u.xor") (param $addr i32) (param $value i64) (result i64) (i64.atomic.rmw8_u.xor (get_local $addr) (get_local $value)))
  (func (export "i64.atomic.rmw16_u.xor") (param $addr i32) (param $value i64) (result i64) (i64.atomic.rmw16_u.xor (get_local $addr) (get_local $value)))
  (func (export "i64.atomic.rmw32_u.xor") (param $addr i32) (param $value i64) (result i64) (i64.atomic.rmw32_u.xor (get_local $addr) (get_local $value)))

  (func (export "i32.atomic.rmw.xchg") (param $addr i32) (param $value i32) (result i32) (i32.atomic.rmw.xchg (get_local $addr) (get_local $value)))
  (func (export "i64.atomic.rmw.xchg") (param $addr i32) (param $value i64) (result i64) (i64.atomic.rmw.xchg (get_local $addr) (get_local $value)))
  (func (export "i32.atomic.rmw8_u.xchg") (param $addr i32) (param $value i32) (result i32) (i32.atomic.rmw8_u.xchg (get_local $addr) (get_local $value)))
  (func (export "i32.atomic.rmw16_u.xchg") (param $addr i32) (param $value i32) (result i32) (i32.atomic.rmw16_u.xchg (get_local $addr) (get_local $value)))
  (func (export "i64.atomic.rmw8_u.xchg") (param $addr i32) (param $value i64) (result i64) (i64.atomic.rmw8_u.xchg (get_local $addr) (get_local $value)))
  (func (export "i64.atomic.rmw16_u.xchg") (param $addr i32) (param $value i64) (result i64) (i64.atomic.rmw16_u.xchg (get_local $addr) (get_local $value)))
  (func (export "i64.atomic.rmw32_u.xchg") (param $addr i32) (param $value i64) (result i64) (i64.atomic.rmw32_u.xchg (get_local $addr) (get_local $value)))

  (func (export "i32.atomic.rmw.cmpxchg") (param $addr i32) (param $expected i32) (param $value i32) (result i32) (i32.atomic.rmw.cmpxchg (get_local $addr) (get_local $expected) (get_local $value)))
  (func (export "i64.atomic.rmw.cmpxchg") (param $addr i32) (param $expected i64)  (param $value i64) (result i64) (i64.atomic.rmw.cmpxchg (get_local $addr) (get_local $expected) (get_local $value)))
  (func (export "i32.atomic.rmw8_u.cmpxchg") (param $addr i32) (param $expected i32)  (param $value i32) (result i32) (i32.atomic.rmw8_u.cmpxchg (get_local $addr) (get_local $expected) (get_local $value)))
  (func (export "i32.atomic.rmw16_u.cmpxchg") (param $addr i32) (param $expected i32)  (param $value i32) (result i32) (i32.atomic.rmw16_u.cmpxchg (get_local $addr) (get_local $expected) (get_local $value)))
  (func (export "i64.atomic.rmw8_u.cmpxchg") (param $addr i32) (param $expected i64)  (param $value i64) (result i64) (i64.atomic.rmw8_u.cmpxchg (get_local $addr) (get_local $expected) (get_local $value)))
  (func (export "i64.atomic.rmw16_u.cmpxchg") (param $addr i32) (param $expected i64)  (param $value i64) (result i64) (i64.atomic.rmw16_u.cmpxchg (get_local $addr) (get_local $expected) (get_local $value)))
  (func (export "i64.atomic.rmw32_u.cmpxchg") (param $addr i32) (param $expected i64)  (param $value i64) (result i64) (i64.atomic.rmw32_u.cmpxchg (get_local $addr) (get_local $expected) (get_local $value)))

)

;; *.atomic.load*

(invoke "init" (i64.const 0x0706050403020100))

(assert_return (invoke "i32.atomic.load" (i32.const 0)) (i32.const 0x03020100))
(assert_return (invoke "i32.atomic.load" (i32.const 4)) (i32.const 0x07060504))

(assert_return (invoke "i64.atomic.load" (i32.const 0)) (i64.const 0x0706050403020100))

(assert_return (invoke "i32.atomic.load8_u" (i32.const 0)) (i32.const 0x00))
(assert_return (invoke "i32.atomic.load8_u" (i32.const 5)) (i32.const 0x05))

(assert_return (invoke "i32.atomic.load16_u" (i32.const 0)) (i32.const 0x0100))
(assert_return (invoke "i32.atomic.load16_u" (i32.const 6)) (i32.const 0x0706))

(assert_return (invoke "i64.atomic.load8_u" (i32.const 0)) (i64.const 0x00))
(assert_return (invoke "i64.atomic.load8_u" (i32.const 5)) (i64.const 0x05))

(assert_return (invoke "i64.atomic.load16_u" (i32.const 0)) (i64.const 0x0100))
(assert_return (invoke "i64.atomic.load16_u" (i32.const 6)) (i64.const 0x0706))

(assert_return (invoke "i64.atomic.load32_u" (i32.const 0)) (i64.const 0x03020100))
(assert_return (invoke "i64.atomic.load32_u" (i32.const 4)) (i64.const 0x07060504))

;; *.atomic.store*

(invoke "init" (i64.const 0x0000000000000000))

(assert_return (invoke "i32.atomic.store" (i32.const 0) (i32.const 0xffeeddcc)))
(assert_return (invoke "i64.atomic.load" (i32.const 0)) (i64.const 0x00000000ffeeddcc))

(assert_return (invoke "i64.atomic.store" (i32.const 0) (i64.const 0x0123456789abcdef)))
(assert_return (invoke "i64.atomic.load" (i32.const 0)) (i64.const 0x0123456789abcdef))

(assert_return (invoke "i32.atomic.store8" (i32.const 1) (i32.const 0x42)))
(assert_return (invoke "i64.atomic.load" (i32.const 0)) (i64.const 0x0123456789ab42ef))

(assert_return (invoke "i32.atomic.store16" (i32.const 4) (i32.const 0x8844)))
(assert_return (invoke "i64.atomic.load" (i32.const 0)) (i64.const 0x0123884489ab42ef))

(assert_return (invoke "i64.atomic.store8" (i32.const 1) (i64.const 0x99)))
(assert_return (invoke "i64.atomic.load" (i32.const 0)) (i64.const 0x0123884489ab99ef))

(assert_return (invoke "i64.atomic.store16" (i32.const 4) (i64.const 0xcafe)))
(assert_return (invoke "i64.atomic.load" (i32.const 0)) (i64.const 0x0123cafe89ab99ef))

(assert_return (invoke "i64.atomic.store32" (i32.const 4) (i64.const 0xdeadbeef)))
(assert_return (invoke "i64.atomic.load" (i32.const 0)) (i64.const 0xdeadbeef89ab99ef))

;; *.atomic.rmw*.add

(invoke "init" (i64.const 0x1111111111111111))
(assert_return (invoke "i32.atomic.rmw.add" (i32.const 0) (i32.const 0x12345678)) (i32.const 0x11111111))
(assert_return (invoke "i64.atomic.load" (i32.const 0)) (i64.const 0x1111111123456789))

(invoke "init" (i64.const 0x1111111111111111))
(assert_return (invoke "i64.atomic.rmw.add" (i32.const 0) (i64.const 0x0101010102020202)) (i64.const 0x1111111111111111))
(assert_return (invoke "i64.atomic.load" (i32.const 0)) (i64.const 0x1212121213131313))

(invoke "init" (i64.const 0x1111111111111111))
(assert_return (invoke "i32.atomic.rmw8_u.add" (i32.const 0) (i32.const 0xcdcdcdcd)) (i32.const 0x11))
(assert_return (invoke "i64.atomic.load" (i32.const 0)) (i64.const 0x11111111111111de))

(invoke "init" (i64.const 0x1111111111111111))
(assert_return (invoke "i32.atomic.rmw16_u.add" (i32.const 0) (i32.const 0xcafecafe)) (i32.const 0x1111))
(assert_return (invoke "i64.atomic.load" (i32.const 0)) (i64.const 0x111111111111dc0f))

(invoke "init" (i64.const 0x1111111111111111))
(assert_return (invoke "i64.atomic.rmw8_u.add" (i32.const 0) (i64.const 0x4242424242424242)) (i64.const 0x11))
(assert_return (invoke "i64.atomic.load" (i32.const 0)) (i64.const 0x1111111111111153))

(invoke "init" (i64.const 0x1111111111111111))
(assert_return (invoke "i64.atomic.rmw16_u.add" (i32.const 0) (i64.const 0xbeefbeefbeefbeef)) (i64.const 0x1111))
(assert_return (invoke "i64.atomic.load" (i32.const 0)) (i64.const 0x111111111111d000))

(invoke "init" (i64.const 0x1111111111111111))
(assert_return (invoke "i64.atomic.rmw32_u.add" (i32.const 0) (i64.const 0xcabba6e5cabba6e5)) (i64.const 0x11111111))
(assert_return (invoke "i64.atomic.load" (i32.const 0)) (i64.const 0x11111111dbccb7f6))

;; *.atomic.rmw*.sub

(invoke "init" (i64.const 0x1111111111111111))
(assert_return (invoke "i32.atomic.rmw.sub" (i32.const 0) (i32.const 0x12345678)) (i32.const 0x11111111))
(assert_return (invoke "i64.atomic.load" (i32.const 0)) (i64.const 0x11111111fedcba99))

(invoke "init" (i64.const 0x1111111111111111))
(assert_return (invoke "i64.atomic.rmw.sub" (i32.const 0) (i64.const 0x0101010102020202)) (i64.const 0x1111111111111111))
(assert_return (invoke "i64.atomic.load" (i32.const 0)) (i64.const 0x101010100f0f0f0f))

(invoke "init" (i64.const 0x1111111111111111))
(assert_return (invoke "i32.atomic.rmw8_u.sub" (i32.const 0) (i32.const 0xcdcdcdcd)) (i32.const 0x11))
(assert_return (invoke "i64.atomic.load" (i32.const 0)) (i64.const 0x1111111111111144))

(invoke "init" (i64.const 0x1111111111111111))
(assert_return (invoke "i32.atomic.rmw16_u.sub" (i32.const 0) (i32.const 0xcafecafe)) (i32.const 0x1111))
(assert_return (invoke "i64.atomic.load" (i32.const 0)) (i64.const 0x1111111111114613))

(invoke "init" (i64.const 0x1111111111111111))
(assert_return (invoke "i64.atomic.rmw8_u.sub" (i32.const 0) (i64.const 0x4242424242424242)) (i64.const 0x11))
(assert_return (invoke "i64.atomic.load" (i32.const 0)) (i64.const 0x11111111111111cf))

(invoke "init" (i64.const 0x1111111111111111))
(assert_return (invoke "i64.atomic.rmw16_u.sub" (i32.const 0) (i64.const 0xbeefbeefbeefbeef)) (i64.const 0x1111))
(assert_return (invoke "i64.atomic.load" (i32.const 0)) (i64.const 0x1111111111115222))

(invoke "init" (i64.const 0x1111111111111111))
(assert_return (invoke "i64.atomic.rmw32_u.sub" (i32.const 0) (i64.const 0xcabba6e5cabba6e5)) (i64.const 0x11111111))
(assert_return (invoke "i64.atomic.load" (i32.const 0)) (i64.const 0x1111111146556a2c))

;; *.atomic.rmw*.and

(invoke "init" (i64.const 0x1111111111111111))
(assert_return (invoke "i32.atomic.rmw.and" (i32.const 0) (i32.const 0x12345678)) (i32.const 0x11111111))
(assert_return (invoke "i64.atomic.load" (i32.const 0)) (i64.const 0x1111111110101010))

(invoke "init" (i64.const 0x1111111111111111))
(assert_return (invoke "i64.atomic.rmw.and" (i32.const 0) (i64.const 0x0101010102020202)) (i64.const 0x1111111111111111))
(assert_return (invoke "i64.atomic.load" (i32.const 0)) (i64.const 0x0101010100000000))

(invoke "init" (i64.const 0x1111111111111111))
(assert_return (invoke "i32.atomic.rmw8_u.and" (i32.const 0) (i32.const 0xcdcdcdcd)) (i32.const 0x11))
(assert_return (invoke "i64.atomic.load" (i32.const 0)) (i64.const 0x1111111111111101))

(invoke "init" (i64.const 0x1111111111111111))
(assert_return (invoke "i32.atomic.rmw16_u.and" (i32.const 0) (i32.const 0xcafecafe)) (i32.const 0x1111))
(assert_return (invoke "i64.atomic.load" (i32.const 0)) (i64.const 0x1111111111110010))

(invoke "init" (i64.const 0x1111111111111111))
(assert_return (invoke "i64.atomic.rmw8_u.and" (i32.const 0) (i64.const 0x4242424242424242)) (i64.const 0x11))
(assert_return (invoke "i64.atomic.load" (i32.const 0)) (i64.const 0x1111111111111100))

(invoke "init" (i64.const 0x1111111111111111))
(assert_return (invoke "i64.atomic.rmw16_u.and" (i32.const 0) (i64.const 0xbeefbeefbeefbeef)) (i64.const 0x1111))
(assert_return (invoke "i64.atomic.load" (i32.const 0)) (i64.const 0x1111111111111001))

(invoke "init" (i64.const 0x1111111111111111))
(assert_return (invoke "i64.atomic.rmw32_u.and" (i32.const 0) (i64.const 0xcabba6e5cabba6e5)) (i64.const 0x11111111))
(assert_return (invoke "i64.atomic.load" (i32.const 0)) (i64.const 0x1111111100110001))

;; *.atomic.rmw*.or

(invoke "init" (i64.const 0x1111111111111111))
(assert_return (invoke "i32.atomic.rmw.or" (i32.const 0) (i32.const 0x12345678)) (i32.const 0x11111111))
(assert_return (invoke "i64.atomic.load" (i32.const 0)) (i64.const 0x1111111113355779))

(invoke "init" (i64.const 0x1111111111111111))
(assert_return (invoke "i64.atomic.rmw.or" (i32.const 0) (i64.const 0x0101010102020202)) (i64.const 0x1111111111111111))
(assert_return (invoke "i64.atomic.load" (i32.const 0)) (i64.const 0x1111111113131313))

(invoke "init" (i64.const 0x1111111111111111))
(assert_return (invoke "i32.atomic.rmw8_u.or" (i32.const 0) (i32.const 0xcdcdcdcd)) (i32.const 0x11))
(assert_return (invoke "i64.atomic.load" (i32.const 0)) (i64.const 0x11111111111111dd))

(invoke "init" (i64.const 0x1111111111111111))
(assert_return (invoke "i32.atomic.rmw16_u.or" (i32.const 0) (i32.const 0xcafecafe)) (i32.const 0x1111))
(assert_return (invoke "i64.atomic.load" (i32.const 0)) (i64.const 0x111111111111dbff))

(invoke "init" (i64.const 0x1111111111111111))
(assert_return (invoke "i64.atomic.rmw8_u.or" (i32.const 0) (i64.const 0x4242424242424242)) (i64.const 0x11))
(assert_return (invoke "i64.atomic.load" (i32.const 0)) (i64.const 0x1111111111111153))

(invoke "init" (i64.const 0x1111111111111111))
(assert_return (invoke "i64.atomic.rmw16_u.or" (i32.const 0) (i64.const 0xbeefbeefbeefbeef)) (i64.const 0x1111))
(assert_return (invoke "i64.atomic.load" (i32.const 0)) (i64.const 0x111111111111bfff))

(invoke "init" (i64.const 0x1111111111111111))
(assert_return (invoke "i64.atomic.rmw32_u.or" (i32.const 0) (i64.const 0xcabba6e5cabba6e5)) (i64.const 0x11111111))
(assert_return (invoke "i64.atomic.load" (i32.const 0)) (i64.const 0x11111111dbbbb7f5))

;; *.atomic.rmw*.xor

(invoke "init" (i64.const 0x1111111111111111))
(assert_return (invoke "i32.atomic.rmw.xor" (i32.const 0) (i32.const 0x12345678)) (i32.const 0x11111111))
(assert_return (invoke "i64.atomic.load" (i32.const 0)) (i64.const 0x1111111103254769))

(invoke "init" (i64.const 0x1111111111111111))
(assert_return (invoke "i64.atomic.rmw.xor" (i32.const 0) (i64.const 0x0101010102020202)) (i64.const 0x1111111111111111))
(assert_return (invoke "i64.atomic.load" (i32.const 0)) (i64.const 0x1010101013131313))

(invoke "init" (i64.const 0x1111111111111111))
(assert_return (invoke "i32.atomic.rmw8_u.xor" (i32.const 0) (i32.const 0xcdcdcdcd)) (i32.const 0x11))
(assert_return (invoke "i64.atomic.load" (i32.const 0)) (i64.const 0x11111111111111dc))

(invoke "init" (i64.const 0x1111111111111111))
(assert_return (invoke "i32.atomic.rmw16_u.xor" (i32.const 0) (i32.const 0xcafecafe)) (i32.const 0x1111))
(assert_return (invoke "i64.atomic.load" (i32.const 0)) (i64.const 0x111111111111dbef))

(invoke "init" (i64.const 0x1111111111111111))
(assert_return (invoke "i64.atomic.rmw8_u.xor" (i32.const 0) (i64.const 0x4242424242424242)) (i64.const 0x11))
(assert_return (invoke "i64.atomic.load" (i32.const 0)) (i64.const 0x1111111111111153))

(invoke "init" (i64.const 0x1111111111111111))
(assert_return (invoke "i64.atomic.rmw16_u.xor" (i32.const 0) (i64.const 0xbeefbeefbeefbeef)) (i64.const 0x1111))
(assert_return (invoke "i64.atomic.load" (i32.const 0)) (i64.const 0x111111111111affe))

(invoke "init" (i64.const 0x1111111111111111))
(assert_return (invoke "i64.atomic.rmw32_u.xor" (i32.const 0) (i64.const 0xcabba6e5cabba6e5)) (i64.const 0x11111111))
(assert_return (invoke "i64.atomic.load" (i32.const 0)) (i64.const 0x11111111dbaab7f4))

;; *.atomic.rmw*.xchg

(invoke "init" (i64.const 0x1111111111111111))
(assert_return (invoke "i32.atomic.rmw.xchg" (i32.const 0) (i32.const 0x12345678)) (i32.const 0x11111111))
(assert_return (invoke "i64.atomic.load" (i32.const 0)) (i64.const 0x1111111112345678))

(invoke "init" (i64.const 0x1111111111111111))
(assert_return (invoke "i64.atomic.rmw.xchg" (i32.const 0) (i64.const 0x0101010102020202)) (i64.const 0x1111111111111111))
(assert_return (invoke "i64.atomic.load" (i32.const 0)) (i64.const 0x0101010102020202))

(invoke "init" (i64.const 0x1111111111111111))
(assert_return (invoke "i32.atomic.rmw8_u.xchg" (i32.const 0) (i32.const 0xcdcdcdcd)) (i32.const 0x11))
(assert_return (invoke "i64.atomic.load" (i32.const 0)) (i64.const 0x11111111111111cd))

(invoke "init" (i64.const 0x1111111111111111))
(assert_return (invoke "i32.atomic.rmw16_u.xchg" (i32.const 0) (i32.const 0xcafecafe)) (i32.const 0x1111))
(assert_return (invoke "i64.atomic.load" (i32.const 0)) (i64.const 0x111111111111cafe))

(invoke "init" (i64.const 0x1111111111111111))
(assert_return (invoke "i64.atomic.rmw8_u.xchg" (i32.const 0) (i64.const 0x4242424242424242)) (i64.const 0x11))
(assert_return (invoke "i64.atomic.load" (i32.const 0)) (i64.const 0x1111111111111142))

(invoke "init" (i64.const 0x1111111111111111))
(assert_return (invoke "i64.atomic.rmw16_u.xchg" (i32.const 0) (i64.const 0xbeefbeefbeefbeef)) (i64.const 0x1111))
(assert_return (invoke "i64.atomic.load" (i32.const 0)) (i64.const 0x111111111111beef))

(invoke "init" (i64.const 0x1111111111111111))
(assert_return (invoke "i64.atomic.rmw32_u.xchg" (i32.const 0) (i64.const 0xcabba6e5cabba6e5)) (i64.const 0x11111111))
(assert_return (invoke "i64.atomic.load" (i32.const 0)) (i64.const 0x11111111cabba6e5))

;; *.atomic.rmw*.cmpxchg (compare false)

(invoke "init" (i64.const 0x1111111111111111))
(assert_return (invoke "i32.atomic.rmw.cmpxchg" (i32.const 0) (i32.const 0) (i32.const 0x12345678)) (i32.const 0x11111111))
(assert_return (invoke "i64.atomic.load" (i32.const 0)) (i64.const 0x1111111111111111))

(invoke "init" (i64.const 0x1111111111111111))
(assert_return (invoke "i64.atomic.rmw.cmpxchg" (i32.const 0) (i64.const 0) (i64.const 0x0101010102020202)) (i64.const 0x1111111111111111))
(assert_return (invoke "i64.atomic.load" (i32.const 0)) (i64.const 0x1111111111111111))

(invoke "init" (i64.const 0x1111111111111111))
(assert_return (invoke "i32.atomic.rmw8_u.cmpxchg" (i32.const 0) (i32.const 0) (i32.const 0xcdcdcdcd)) (i32.const 0x11))
(assert_return (invoke "i64.atomic.load" (i32.const 0)) (i64.const 0x1111111111111111))

(invoke "init" (i64.const 0x1111111111111111))
(assert_return (invoke "i32.atomic.rmw16_u.cmpxchg" (i32.const 0) (i32.const 0) (i32.const 0xcafecafe)) (i32.const 0x1111))
(assert_return (invoke "i64.atomic.load" (i32.const 0)) (i64.const 0x1111111111111111))

(invoke "init" (i64.const 0x1111111111111111))
(assert_return (invoke "i64.atomic.rmw8_u.cmpxchg" (i32.const 0) (i64.const 0) (i64.const 0x4242424242424242)) (i64.const 0x11))
(assert_return (invoke "i64.atomic.load" (i32.const 0)) (i64.const 0x1111111111111111))

(invoke "init" (i64.const 0x1111111111111111))
(assert_return (invoke "i64.atomic.rmw16_u.cmpxchg" (i32.const 0) (i64.const 0) (i64.const 0xbeefbeefbeefbeef)) (i64.const 0x1111))
(assert_return (invoke "i64.atomic.load" (i32.const 0)) (i64.const 0x1111111111111111))

(invoke "init" (i64.const 0x1111111111111111))
(assert_return (invoke "i64.atomic.rmw32_u.cmpxchg" (i32.const 0) (i64.const 0) (i64.const 0xcabba6e5cabba6e5)) (i64.const 0x11111111))
(assert_return (invoke "i64.atomic.load" (i32.const 0)) (i64.const 0x1111111111111111))

;; *.atomic.rmw*.cmpxchg (compare true)

(invoke "init" (i64.const 0x1111111111111111))
(assert_return (invoke "i32.atomic.rmw.cmpxchg" (i32.const 0) (i32.const 0x11111111) (i32.const 0x12345678)) (i32.const 0x11111111))
(assert_return (invoke "i64.atomic.load" (i32.const 0)) (i64.const 0x1111111112345678))

(invoke "init" (i64.const 0x1111111111111111))
(assert_return (invoke "i64.atomic.rmw.cmpxchg" (i32.const 0) (i64.const 0x1111111111111111) (i64.const 0x0101010102020202)) (i64.const 0x1111111111111111))
(assert_return (invoke "i64.atomic.load" (i32.const 0)) (i64.const 0x0101010102020202))

(invoke "init" (i64.const 0x1111111111111111))
(assert_return (invoke "i32.atomic.rmw8_u.cmpxchg" (i32.const 0) (i32.const 0x11) (i32.const 0xcdcdcdcd)) (i32.const 0x11))
(assert_return (invoke "i64.atomic.load" (i32.const 0)) (i64.const 0x11111111111111cd))

(invoke "init" (i64.const 0x1111111111111111))
(assert_return (invoke "i32.atomic.rmw16_u.cmpxchg" (i32.const 0) (i32.const 0x1111) (i32.const 0xcafecafe)) (i32.const 0x1111))
(assert_return (invoke "i64.atomic.load" (i32.const 0)) (i64.const 0x111111111111cafe))

(invoke "init" (i64.const 0x1111111111111111))
(assert_return (invoke "i64.atomic.rmw8_u.cmpxchg" (i32.const 0) (i64.const 0x11) (i64.const 0x4242424242424242)) (i64.const 0x11))
(assert_return (invoke "i64.atomic.load" (i32.const 0)) (i64.const 0x1111111111111142))

(invoke "init" (i64.const 0x1111111111111111))
(assert_return (invoke "i64.atomic.rmw16_u.cmpxchg" (i32.const 0) (i64.const 0x1111) (i64.const 0xbeefbeefbeefbeef)) (i64.const 0x1111))
(assert_return (invoke "i64.atomic.load" (i32.const 0)) (i64.const 0x111111111111beef))

(invoke "init" (i64.const 0x1111111111111111))
(assert_return (invoke "i64.atomic.rmw32_u.cmpxchg" (i32.const 0) (i64.const 0x11111111) (i64.const 0xcabba6e5cabba6e5)) (i64.const 0x11111111))
(assert_return (invoke "i64.atomic.load" (i32.const 0)) (i64.const 0x11111111cabba6e5))


;; unaligned accesses

(assert_trap (invoke "i32.atomic.load" (i32.const 1)) "unaligned atomic")
(assert_trap (invoke "i64.atomic.load" (i32.const 1)) "unaligned atomic")
(assert_trap (invoke "i32.atomic.load16_u" (i32.const 1)) "unaligned atomic")
(assert_trap (invoke "i64.atomic.load16_u" (i32.const 1)) "unaligned atomic")
(assert_trap (invoke "i64.atomic.load32_u" (i32.const 1)) "unaligned atomic")
(assert_trap (invoke "i32.atomic.store" (i32.const 1) (i32.const 0)) "unaligned atomic")
(assert_trap (invoke "i64.atomic.store" (i32.const 1) (i64.const 0)) "unaligned atomic")
(assert_trap (invoke "i32.atomic.store16" (i32.const 1) (i32.const 0)) "unaligned atomic")
(assert_trap (invoke "i64.atomic.store16" (i32.const 1) (i64.const 0)) "unaligned atomic")
(assert_trap (invoke "i64.atomic.store32" (i32.const 1) (i64.const 0)) "unaligned atomic")
(assert_trap (invoke "i32.atomic.rmw.add" (i32.const 1) (i32.const 0)) "unaligned atomic")
(assert_trap (invoke "i64.atomic.rmw.add" (i32.const 1) (i64.const 0)) "unaligned atomic")
(assert_trap (invoke "i32.atomic.rmw16_u.add" (i32.const 1) (i32.const 0)) "unaligned atomic")
(assert_trap (invoke "i64.atomic.rmw16_u.add" (i32.const 1) (i64.const 0)) "unaligned atomic")
(assert_trap (invoke "i64.atomic.rmw32_u.add" (i32.const 1) (i64.const 0)) "unaligned atomic")
(assert_trap (invoke "i32.atomic.rmw.sub" (i32.const 1) (i32.const 0)) "unaligned atomic")
(assert_trap (invoke "i64.atomic.rmw.sub" (i32.const 1) (i64.const 0)) "unaligned atomic")
(assert_trap (invoke "i32.atomic.rmw16_u.sub" (i32.const 1) (i32.const 0)) "unaligned atomic")
(assert_trap (invoke "i64.atomic.rmw16_u.sub" (i32.const 1) (i64.const 0)) "unaligned atomic")
(assert_trap (invoke "i64.atomic.rmw32_u.sub" (i32.const 1) (i64.const 0)) "unaligned atomic")
(assert_trap (invoke "i32.atomic.rmw.and" (i32.const 1) (i32.const 0)) "unaligned atomic")
(assert_trap (invoke "i64.atomic.rmw.and" (i32.const 1) (i64.const 0)) "unaligned atomic")
(assert_trap (invoke "i32.atomic.rmw16_u.and" (i32.const 1) (i32.const 0)) "unaligned atomic")
(assert_trap (invoke "i64.atomic.rmw16_u.and" (i32.const 1) (i64.const 0)) "unaligned atomic")
(assert_trap (invoke "i64.atomic.rmw32_u.and" (i32.const 1) (i64.const 0)) "unaligned atomic")
(assert_trap (invoke "i32.atomic.rmw.or" (i32.const 1) (i32.const 0)) "unaligned atomic")
(assert_trap (invoke "i64.atomic.rmw.or" (i32.const 1) (i64.const 0)) "unaligned atomic")
(assert_trap (invoke "i32.atomic.rmw16_u.or" (i32.const 1) (i32.const 0)) "unaligned atomic")
(assert_trap (invoke "i64.atomic.rmw16_u.or" (i32.const 1) (i64.const 0)) "unaligned atomic")
(assert_trap (invoke "i64.atomic.rmw32_u.or" (i32.const 1) (i64.const 0)) "unaligned atomic")
(assert_trap (invoke "i32.atomic.rmw.xor" (i32.const 1) (i32.const 0)) "unaligned atomic")
(assert_trap (invoke "i64.atomic.rmw.xor" (i32.const 1) (i64.const 0)) "unaligned atomic")
(assert_trap (invoke "i32.atomic.rmw16_u.xor" (i32.const 1) (i32.const 0)) "unaligned atomic")
(assert_trap (invoke "i64.atomic.rmw16_u.xor" (i32.const 1) (i64.const 0)) "unaligned atomic")
(assert_trap (invoke "i64.atomic.rmw32_u.xor" (i32.const 1) (i64.const 0)) "unaligned atomic")
(assert_trap (invoke "i32.atomic.rmw.xchg" (i32.const 1) (i32.const 0)) "unaligned atomic")
(assert_trap (invoke "i64.atomic.rmw.xchg" (i32.const 1) (i64.const 0)) "unaligned atomic")
(assert_trap (invoke "i32.atomic.rmw16_u.xchg" (i32.const 1) (i32.const 0)) "unaligned atomic")
(assert_trap (invoke "i64.atomic.rmw16_u.xchg" (i32.const 1) (i64.const 0)) "unaligned atomic")
(assert_trap (invoke "i64.atomic.rmw32_u.xchg" (i32.const 1) (i64.const 0)) "unaligned atomic")
(assert_trap (invoke "i32.atomic.rmw.cmpxchg" (i32.const 1) (i32.const 0) (i32.const 0)) "unaligned atomic")
(assert_trap (invoke "i64.atomic.rmw.cmpxchg" (i32.const 1) (i64.const 0)  (i64.const 0)) "unaligned atomic")
(assert_trap (invoke "i32.atomic.rmw16_u.cmpxchg" (i32.const 1) (i32.const 0) (i32.const 0)) "unaligned atomic")
(assert_trap (invoke "i64.atomic.rmw16_u.cmpxchg" (i32.const 1) (i64.const 0) (i64.const 0)) "unaligned atomic")
(assert_trap (invoke "i64.atomic.rmw32_u.cmpxchg" (i32.const 1) (i64.const 0) (i64.const 0)) "unaligned atomic")
