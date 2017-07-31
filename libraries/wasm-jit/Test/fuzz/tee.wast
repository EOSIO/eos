;; poor man's tee
;; Outputs to stderr and stdout whatever comes in stdin

(module

  (import "env" "memory" (memory 1))
  (import "env" "_fwrite" (func $__fwrite (param i32 i32 i32 i32) (result i32)))
  (import "env" "_fread" (func $__fread (param i32 i32 i32 i32) (result i32)))
  (import "env" "_stdin" (global $stdinPtr (mut i32)))
  (import "env" "_stdout" (global $stdoutPtr (mut i32)))
  (import "env" "_stderr" (global $stderrPtr (mut i32)))
  (export "main" (func $main))

  (func $main
    (local $stdin i32)
    (local $stdout i32)
    (local $stderr i32)
    (local $nmemb i32)
    (set_local $stdin (i32.load align=4 (get_global $stdinPtr)))
    (set_local $stdout (i32.load align=4 (get_global $stdoutPtr)))
    (set_local $stderr (i32.load align=4 (get_global $stderrPtr)))

    (loop $loop
      (block $done
        (set_local $nmemb (call $__fread (i32.const 0) (i32.const 1) (i32.const 32) (get_local $stdin)))
        (br_if $done (i32.eq (i32.const 0) (get_local $nmemb)))
        (drop (call $__fwrite (i32.const 0) (i32.const 1) (get_local $nmemb) (get_local $stdout)))
        (drop (call $__fwrite (i32.const 0) (i32.const 1) (get_local $nmemb) (get_local $stderr)))
        (br $loop)
      )
    )
  )
)
