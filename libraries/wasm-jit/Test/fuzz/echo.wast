;; Classic Unix echo program in WebAssembly WASM AST

(module
  (data (i32.const 8) "\n\00")
  (data (i32.const 12) " \00")

  (import "env" "memory" (memory 1))
  (import "env" "_fwrite" (func $__fwrite (param i32 i32 i32 i32) (result i32)))
  (import "env" "_stdout" (global $stdoutPtr (mut i32)))
  (export "main" (func $main))

  (func $strlen (param $s i32) (result i32)
    (local $head i32)
    (set_local $head (get_local $s))
    (loop $loop
	  (block $done
        (br_if $done (i32.eq (i32.const 0) (i32.load8_u offset=0 (get_local $head))))
        (set_local $head (i32.add (get_local $head) (i32.const 1)))
        ;; Would it be worth unrolling offset 1,2,3?
        (br $loop)
      )
    )
    (return (i32.sub (get_local $head) (get_local $s)))
  )

  (func $fputs (param $s i32) (param $stream i32) (result i32)
    (local $len i32)
    (set_local $len (call $strlen (get_local $s)))
    (return (call $__fwrite
      (get_local $s)        ;; ptr
      (i32.const 1)         ;; size_t size  => Data size
      (get_local $len)      ;; size_t nmemb => Length of our string
      (get_local $stream))) ;; stream
  )

  (func $main (param $argc i32) (param $argv i32) (result i32)
    (local $stdout i32)
    (local $s i32)
    (local $space i32)
    (set_local $space (i32.const 0))
    (set_local $stdout (i32.load align=4 (get_global $stdoutPtr)))

    (loop $loop
      (block $done
        (set_local $argv (i32.add (get_local $argv) (i32.const 4)))
        (set_local $s (i32.load (get_local $argv)))
        (br_if $done (i32.eq (i32.const 0) (get_local $s)))

        (if (i32.eq (i32.const 1) (get_local $space))
          (drop (call $fputs (i32.const 12) (get_local $stdout))) ;; ' '
        )
        (set_local $space (i32.const 1))

        (drop (call $fputs (get_local $s) (get_local $stdout)))
        (br $loop)
      )
    )

    (drop (call $fputs (i32.const 8) (get_local $stdout))) ;; \n

    (return (i32.const 0))
  )
)
