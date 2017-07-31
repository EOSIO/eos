;; Blake2b hash function

(module
	(import "env" "_fwrite" (func $__fwrite (param i32 i32 i32 i32) (result i32)))
	(import "env" "_stdout" (global $stdoutPtr i32))
	(import "env" "memory" (memory 1))
	(import "env" "_sbrk" (func $sbrk (param i32) (result i32)))
	(export "main" (func $main))
	
	(table anyfunc (elem $threadEntry $threadError))

	(global $numThreads i32 (i32.const 8))
	(global $numIterationsPerThread i32 (i32.const 10))

	;; IV array
	(global $ivAddress i32 (i32.const 0))	;; 64 byte IV array
	(data (i32.const 0)
		"\08\c9\bc\f3\67\e6\09\6a"
		"\3b\a7\ca\84\85\ae\67\bb"
		"\2b\f8\94\fe\72\f3\6e\3c"
		"\f1\36\1d\5f\3a\f5\4f\a5"
		"\d1\82\e6\ad\7f\52\0e\51"
		"\1f\6c\3e\2b\8c\68\05\9b"
		"\6b\bd\41\fb\ab\d9\83\1f"
		"\79\21\7e\13\19\cd\e0\5b"
	)

	(global $pAddress i32 (i32.const 64))	;; 64 byte Param struct
	(data (i32.const 64)
		"\40" ;; digest_length
		"\00" ;; key_length
		"\01" ;; fanout
		"\01" ;; depth
		"\00\00\00\00" ;; leaf_length
		"\00\00\00\00" ;; node_offset
		"\00\00\00\00" ;; xof_length
		"\00" ;; node_depth
		"\00" ;; inner_length
		"\00\00\00\00\00\00\00\00\00\00\00\00\00\00" ;; reserved
		"\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00" ;; salt
		"\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00" ;; personal
		)

	;; lookup table for converting a nibble to a hexit
	(global $hexitTable i32 (i32.const 128))
	(data (i32.const 128) "0123456789abcdef")
	
	(global $dataAddress (mut i32) (i32.const 0))
	(global $dataNumBytes (mut i32) (i32.const 134217728))
	
	(global $numPendingThreadsAddress i32 (i32.const 144)) ;; 4 bytes
	(global $outputStringAddress i32 (i32.const 148)) ;; 129 bytes
	
	(global $statesArrayAddress i32 (i32.const 1024))	;; 128*N per-thread state blocks.

	(func $compress
		(param $threadStateAddress i32)
		(param $blockAddress i32)

		(local $row1l v128)
		(local $row1h v128)
		(local $row2l v128)
		(local $row2h v128)
		(local $row3l v128)
		(local $row3h v128)
		(local $row4l v128)
		(local $row4h v128)
		(local $b0 v128)
		(local $b1 v128)
		(local $t0 v128)
		(local $t1 v128)

		;; Initialize the state

		(set_local $row1l (v128.load offset=0 (get_local $threadStateAddress)))
		(set_local $row1h (v128.load offset=16 (get_local $threadStateAddress)))
		(set_local $row2l (v128.load offset=32 (get_local $threadStateAddress)))
		(set_local $row2h (v128.load offset=48 (get_local $threadStateAddress)))
		(set_local $row3l (v128.load offset=0 (get_global $ivAddress)))
		(set_local $row3h (v128.load offset=16 (get_global $ivAddress)))
		(set_local $row4l (v128.xor (v128.load offset=32 (get_global $ivAddress)) (v128.load offset=64 (get_local $threadStateAddress))))
		(set_local $row4h (v128.xor (v128.load offset=48 (get_global $ivAddress)) (v128.load offset=80 (get_local $threadStateAddress))))
		
		;; Round 0

		(set_local $b0 (v8x16.shuffle (0 1 2 3 4 5 6 7 16 17 18 19 20 21 22 23) (v128.load offset=0 (get_local $blockAddress)) (v128.load offset=16 (get_local $blockAddress))))
		(set_local $b1 (v8x16.shuffle (0 1 2 3 4 5 6 7 16 17 18 19 20 21 22 23) (v128.load offset=32 (get_local $blockAddress)) (v128.load offset=48 (get_local $blockAddress))))

		(set_local $row1l (i64x2.add (i64x2.add (get_local $row1l) (get_local $b0)) (get_local $row2l)))
		(set_local $row1h (i64x2.add (i64x2.add (get_local $row1h) (get_local $b1)) (get_local $row2h)))
		(set_local $row4l (v128.xor (get_local $row4l) (get_local $row1l)))
		(set_local $row4h (v128.xor (get_local $row4h) (get_local $row1h)))
		(set_local $row4l (v8x16.shuffle (4 5 6 7 0 1 2 3 12 13 14 15 8 9 10 11) (get_local $row4l) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row4h (v8x16.shuffle (4 5 6 7 0 1 2 3 12 13 14 15 8 9 10 11) (get_local $row4h) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row3l (i64x2.add (get_local $row3l) (get_local $row4l)))
		(set_local $row3h (i64x2.add (get_local $row3h) (get_local $row4h)))
		(set_local $row2l (v128.xor (get_local $row2l) (get_local $row3l)))
		(set_local $row2h (v128.xor (get_local $row2h) (get_local $row3h)))
		(set_local $row2l (v8x16.shuffle (3 4 5 6 7 0 1 2 11 12 13 14 15 8 9 10) (get_local $row2l) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row2h (v8x16.shuffle (3 4 5 6 7 0 1 2 11 12 13 14 15 8 9 10) (get_local $row2h) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))

		(set_local $b0 (v8x16.shuffle (8 9 10 11 12 13 14 15 24 25 26 27 28 29 30 31) (v128.load offset=0 (get_local $blockAddress)) (v128.load offset=16 (get_local $blockAddress))))
		(set_local $b1 (v8x16.shuffle (8 9 10 11 12 13 14 15 24 25 26 27 28 29 30 31) (v128.load offset=32 (get_local $blockAddress)) (v128.load offset=48 (get_local $blockAddress))))

		(set_local $row1l (i64x2.add (i64x2.add (get_local $row1l) (get_local $b0)) (get_local $row2l)))
		(set_local $row1h (i64x2.add (i64x2.add (get_local $row1h) (get_local $b1)) (get_local $row2h)))
		(set_local $row4l (v128.xor (get_local $row4l) (get_local $row1l)))
		(set_local $row4h (v128.xor (get_local $row4h) (get_local $row1h)))
		(set_local $row4l (v8x16.shuffle (2 3 4 5 6 7 0 1 10 11 12 13 14 15 8 9) (get_local $row4l) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row4h (v8x16.shuffle (2 3 4 5 6 7 0 1 10 11 12 13 14 15 8 9) (get_local $row4h) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row3l (i64x2.add (get_local $row3l) (get_local $row4l)))
		(set_local $row3h (i64x2.add (get_local $row3h) (get_local $row4h)))
		(set_local $row2l (v128.xor (get_local $row2l) (get_local $row3l)))
		(set_local $row2h (v128.xor (get_local $row2h) (get_local $row3h)))
		(set_local $row2l (v128.xor (i64x2.shr_u (get_local $row2l) (i64x2.splat (i64.const 63))) (i64x2.add (get_local $row2l) (get_local $row2l))))
		(set_local $row2h (v128.xor (i64x2.shr_u (get_local $row2h) (i64x2.splat (i64.const 63))) (i64x2.add (get_local $row2h) (get_local $row2h))))

		(set_local $t0 (v8x16.shuffle (24 25 26 27 28 29 30 31 0 1 2 3 4 5 6 7) (get_local $row2h) (get_local $row2l)))
		(set_local $t1 (v8x16.shuffle (24 25 26 27 28 29 30 31 0 1 2 3 4 5 6 7) (get_local $row2l) (get_local $row2h)))
		(set_local $row2l (get_local $t0))
		(set_local $row2h (get_local $t1))
		(set_local $t0 (get_local $row3l))
		(set_local $row3l (get_local $row3h))
		(set_local $row3h (get_local $t0))
		(set_local $t0 (v8x16.shuffle (24 25 26 27 28 29 30 31 0 1 2 3 4 5 6 7) (get_local $row4h) (get_local $row4l)))
		(set_local $t1 (v8x16.shuffle (24 25 26 27 28 29 30 31 0 1 2 3 4 5 6 7) (get_local $row4l) (get_local $row4h)))
		(set_local $row4l (get_local $t1))
		(set_local $row4h (get_local $t0))

		(set_local $b0 (v8x16.shuffle (0 1 2 3 4 5 6 7 16 17 18 19 20 21 22 23) (v128.load offset=64 (get_local $blockAddress)) (v128.load offset=80 (get_local $blockAddress))))
		(set_local $b1 (v8x16.shuffle (0 1 2 3 4 5 6 7 16 17 18 19 20 21 22 23) (v128.load offset=96 (get_local $blockAddress)) (v128.load offset=112 (get_local $blockAddress))))

		(set_local $row1l (i64x2.add (i64x2.add (get_local $row1l) (get_local $b0)) (get_local $row2l)))
		(set_local $row1h (i64x2.add (i64x2.add (get_local $row1h) (get_local $b1)) (get_local $row2h)))
		(set_local $row4l (v128.xor (get_local $row4l) (get_local $row1l)))
		(set_local $row4h (v128.xor (get_local $row4h) (get_local $row1h)))
		(set_local $row4l (v8x16.shuffle (4 5 6 7 0 1 2 3 12 13 14 15 8 9 10 11) (get_local $row4l) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row4h (v8x16.shuffle (4 5 6 7 0 1 2 3 12 13 14 15 8 9 10 11) (get_local $row4h) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row3l (i64x2.add (get_local $row3l) (get_local $row4l)))
		(set_local $row3h (i64x2.add (get_local $row3h) (get_local $row4h)))
		(set_local $row2l (v128.xor (get_local $row2l) (get_local $row3l)))
		(set_local $row2h (v128.xor (get_local $row2h) (get_local $row3h)))
		(set_local $row2l (v8x16.shuffle (3 4 5 6 7 0 1 2 11 12 13 14 15 8 9 10) (get_local $row2l) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row2h (v8x16.shuffle (3 4 5 6 7 0 1 2 11 12 13 14 15 8 9 10) (get_local $row2h) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))

		(set_local $b0 (v8x16.shuffle (8 9 10 11 12 13 14 15 24 25 26 27 28 29 30 31) (v128.load offset=64 (get_local $blockAddress)) (v128.load offset=80 (get_local $blockAddress))))
		(set_local $b1 (v8x16.shuffle (8 9 10 11 12 13 14 15 24 25 26 27 28 29 30 31) (v128.load offset=96 (get_local $blockAddress)) (v128.load offset=112 (get_local $blockAddress))))

		(set_local $row1l (i64x2.add (i64x2.add (get_local $row1l) (get_local $b0)) (get_local $row2l)))
		(set_local $row1h (i64x2.add (i64x2.add (get_local $row1h) (get_local $b1)) (get_local $row2h)))
		(set_local $row4l (v128.xor (get_local $row4l) (get_local $row1l)))
		(set_local $row4h (v128.xor (get_local $row4h) (get_local $row1h)))
		(set_local $row4l (v8x16.shuffle (2 3 4 5 6 7 0 1 10 11 12 13 14 15 8 9) (get_local $row4l) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row4h (v8x16.shuffle (2 3 4 5 6 7 0 1 10 11 12 13 14 15 8 9) (get_local $row4h) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row3l (i64x2.add (get_local $row3l) (get_local $row4l)))
		(set_local $row3h (i64x2.add (get_local $row3h) (get_local $row4h)))
		(set_local $row2l (v128.xor (get_local $row2l) (get_local $row3l)))
		(set_local $row2h (v128.xor (get_local $row2h) (get_local $row3h)))
		(set_local $row2l (v128.xor (i64x2.shr_u (get_local $row2l) (i64x2.splat (i64.const 63))) (i64x2.add (get_local $row2l) (get_local $row2l))))
		(set_local $row2h (v128.xor (i64x2.shr_u (get_local $row2h) (i64x2.splat (i64.const 63))) (i64x2.add (get_local $row2h) (get_local $row2h))))

		(set_local $t0 (v8x16.shuffle (24 25 26 27 28 29 30 31 0 1 2 3 4 5 6 7) (get_local $row2l) (get_local $row2h)))
		(set_local $t1 (v8x16.shuffle (24 25 26 27 28 29 30 31 0 1 2 3 4 5 6 7) (get_local $row2h) (get_local $row2l)))
		(set_local $row2l (get_local $t0))
		(set_local $row2h (get_local $t1))
		(set_local $t0 (get_local $row3l))
		(set_local $row3l (get_local $row3h))
		(set_local $row3h (get_local $t0))
		(set_local $t0 (v8x16.shuffle (24 25 26 27 28 29 30 31 0 1 2 3 4 5 6 7) (get_local $row4l) (get_local $row4h)))
		(set_local $t1 (v8x16.shuffle (24 25 26 27 28 29 30 31 0 1 2 3 4 5 6 7) (get_local $row4h) (get_local $row4l)))
		(set_local $row4l (get_local $t1))
		(set_local $row4h (get_local $t0))

		;; Round 1

		(set_local $b0 (v8x16.shuffle (0 1 2 3 4 5 6 7 16 17 18 19 20 21 22 23) (v128.load offset=112 (get_local $blockAddress)) (v128.load offset=32 (get_local $blockAddress))))
		(set_local $b1 (v8x16.shuffle (8 9 10 11 12 13 14 15 24 25 26 27 28 29 30 31) (v128.load offset=64 (get_local $blockAddress)) (v128.load offset=96 (get_local $blockAddress))))

		(set_local $row1l (i64x2.add (i64x2.add (get_local $row1l) (get_local $b0)) (get_local $row2l)))
		(set_local $row1h (i64x2.add (i64x2.add (get_local $row1h) (get_local $b1)) (get_local $row2h)))
		(set_local $row4l (v128.xor (get_local $row4l) (get_local $row1l)))
		(set_local $row4h (v128.xor (get_local $row4h) (get_local $row1h)))
		(set_local $row4l (v8x16.shuffle (4 5 6 7 0 1 2 3 12 13 14 15 8 9 10 11) (get_local $row4l) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row4h (v8x16.shuffle (4 5 6 7 0 1 2 3 12 13 14 15 8 9 10 11) (get_local $row4h) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row3l (i64x2.add (get_local $row3l) (get_local $row4l)))
		(set_local $row3h (i64x2.add (get_local $row3h) (get_local $row4h)))
		(set_local $row2l (v128.xor (get_local $row2l) (get_local $row3l)))
		(set_local $row2h (v128.xor (get_local $row2h) (get_local $row3h)))
		(set_local $row2l (v8x16.shuffle (3 4 5 6 7 0 1 2 11 12 13 14 15 8 9 10) (get_local $row2l) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row2h (v8x16.shuffle (3 4 5 6 7 0 1 2 11 12 13 14 15 8 9 10) (get_local $row2h) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))

		(set_local $b0 (v8x16.shuffle (0 1 2 3 4 5 6 7 16 17 18 19 20 21 22 23) (v128.load offset=80 (get_local $blockAddress)) (v128.load offset=64 (get_local $blockAddress))))
		(set_local $b1 (v8x16.shuffle (24 25 26 27 28 29 30 31 0 1 2 3 4 5 6 7) (v128.load offset=48 (get_local $blockAddress)) (v128.load offset=112 (get_local $blockAddress))))

		(set_local $row1l (i64x2.add (i64x2.add (get_local $row1l) (get_local $b0)) (get_local $row2l)))
		(set_local $row1h (i64x2.add (i64x2.add (get_local $row1h) (get_local $b1)) (get_local $row2h)))
		(set_local $row4l (v128.xor (get_local $row4l) (get_local $row1l)))
		(set_local $row4h (v128.xor (get_local $row4h) (get_local $row1h)))
		(set_local $row4l (v8x16.shuffle (2 3 4 5 6 7 0 1 10 11 12 13 14 15 8 9) (get_local $row4l) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row4h (v8x16.shuffle (2 3 4 5 6 7 0 1 10 11 12 13 14 15 8 9) (get_local $row4h) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row3l (i64x2.add (get_local $row3l) (get_local $row4l)))
		(set_local $row3h (i64x2.add (get_local $row3h) (get_local $row4h)))
		(set_local $row2l (v128.xor (get_local $row2l) (get_local $row3l)))
		(set_local $row2h (v128.xor (get_local $row2h) (get_local $row3h)))
		(set_local $row2l (v128.xor (i64x2.shr_u (get_local $row2l) (i64x2.splat (i64.const 63))) (i64x2.add (get_local $row2l) (get_local $row2l))))
		(set_local $row2h (v128.xor (i64x2.shr_u (get_local $row2h) (i64x2.splat (i64.const 63))) (i64x2.add (get_local $row2h) (get_local $row2h))))

		(set_local $t0 (v8x16.shuffle (24 25 26 27 28 29 30 31 0 1 2 3 4 5 6 7) (get_local $row2h) (get_local $row2l)))
		(set_local $t1 (v8x16.shuffle (24 25 26 27 28 29 30 31 0 1 2 3 4 5 6 7) (get_local $row2l) (get_local $row2h)))
		(set_local $row2l (get_local $t0))
		(set_local $row2h (get_local $t1))
		(set_local $t0 (get_local $row3l))
		(set_local $row3l (get_local $row3h))
		(set_local $row3h (get_local $t0))
		(set_local $t0 (v8x16.shuffle (24 25 26 27 28 29 30 31 0 1 2 3 4 5 6 7) (get_local $row4h) (get_local $row4l)))
		(set_local $t1 (v8x16.shuffle (24 25 26 27 28 29 30 31 0 1 2 3 4 5 6 7) (get_local $row4l) (get_local $row4h)))
		(set_local $row4l (get_local $t1))
		(set_local $row4h (get_local $t0))

		(set_local $b0 (v8x16.shuffle (8 9 10 11 12 13 14 15 0 1 2 3 4 5 6 7) (v128.load offset=0 (get_local $blockAddress)) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $b1 (v8x16.shuffle (8 9 10 11 12 13 14 15 24 25 26 27 28 29 30 31) (v128.load offset=80 (get_local $blockAddress)) (v128.load offset=32 (get_local $blockAddress))))

		(set_local $row1l (i64x2.add (i64x2.add (get_local $row1l) (get_local $b0)) (get_local $row2l)))
		(set_local $row1h (i64x2.add (i64x2.add (get_local $row1h) (get_local $b1)) (get_local $row2h)))
		(set_local $row4l (v128.xor (get_local $row4l) (get_local $row1l)))
		(set_local $row4h (v128.xor (get_local $row4h) (get_local $row1h)))
		(set_local $row4l (v8x16.shuffle (4 5 6 7 0 1 2 3 12 13 14 15 8 9 10 11) (get_local $row4l) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row4h (v8x16.shuffle (4 5 6 7 0 1 2 3 12 13 14 15 8 9 10 11) (get_local $row4h) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row3l (i64x2.add (get_local $row3l) (get_local $row4l)))
		(set_local $row3h (i64x2.add (get_local $row3h) (get_local $row4h)))
		(set_local $row2l (v128.xor (get_local $row2l) (get_local $row3l)))
		(set_local $row2h (v128.xor (get_local $row2h) (get_local $row3h)))
		(set_local $row2l (v8x16.shuffle (3 4 5 6 7 0 1 2 11 12 13 14 15 8 9 10) (get_local $row2l) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row2h (v8x16.shuffle (3 4 5 6 7 0 1 2 11 12 13 14 15 8 9 10) (get_local $row2h) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		
		(set_local $b0 (v8x16.shuffle (0 1 2 3 4 5 6 7 16 17 18 19 20 21 22 23) (v128.load offset=96 (get_local $blockAddress)) (v128.load offset=16 (get_local $blockAddress))))
		(set_local $b1 (v8x16.shuffle (8 9 10 11 12 13 14 15 24 25 26 27 28 29 30 31) (v128.load offset=48 (get_local $blockAddress)) (v128.load offset=16 (get_local $blockAddress))))
		
		(set_local $row1l (i64x2.add (i64x2.add (get_local $row1l) (get_local $b0)) (get_local $row2l)))
		(set_local $row1h (i64x2.add (i64x2.add (get_local $row1h) (get_local $b1)) (get_local $row2h)))
		(set_local $row4l (v128.xor (get_local $row4l) (get_local $row1l)))
		(set_local $row4h (v128.xor (get_local $row4h) (get_local $row1h)))
		(set_local $row4l (v8x16.shuffle (2 3 4 5 6 7 0 1 10 11 12 13 14 15 8 9) (get_local $row4l) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row4h (v8x16.shuffle (2 3 4 5 6 7 0 1 10 11 12 13 14 15 8 9) (get_local $row4h) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row3l (i64x2.add (get_local $row3l) (get_local $row4l)))
		(set_local $row3h (i64x2.add (get_local $row3h) (get_local $row4h)))
		(set_local $row2l (v128.xor (get_local $row2l) (get_local $row3l)))
		(set_local $row2h (v128.xor (get_local $row2h) (get_local $row3h)))
		(set_local $row2l (v128.xor (i64x2.shr_u (get_local $row2l) (i64x2.splat (i64.const 63))) (i64x2.add (get_local $row2l) (get_local $row2l))))
		(set_local $row2h (v128.xor (i64x2.shr_u (get_local $row2h) (i64x2.splat (i64.const 63))) (i64x2.add (get_local $row2h) (get_local $row2h))))

		(set_local $t0 (v8x16.shuffle (24 25 26 27 28 29 30 31 0 1 2 3 4 5 6 7) (get_local $row2l) (get_local $row2h)))
		(set_local $t1 (v8x16.shuffle (24 25 26 27 28 29 30 31 0 1 2 3 4 5 6 7) (get_local $row2h) (get_local $row2l)))
		(set_local $row2l (get_local $t0))
		(set_local $row2h (get_local $t1))
		(set_local $t0 (get_local $row3l))
		(set_local $row3l (get_local $row3h))
		(set_local $row3h (get_local $t0))
		(set_local $t0 (v8x16.shuffle (24 25 26 27 28 29 30 31 0 1 2 3 4 5 6 7) (get_local $row4l) (get_local $row4h)))
		(set_local $t1 (v8x16.shuffle (24 25 26 27 28 29 30 31 0 1 2 3 4 5 6 7) (get_local $row4h) (get_local $row4l)))
		(set_local $row4l (get_local $t1))
		(set_local $row4h (get_local $t0))

		;; Round 2

		(set_local $b0 (v8x16.shuffle (24 25 26 27 28 29 30 31 0 1 2 3 4 5 6 7) (v128.load offset=96 (get_local $blockAddress)) (v128.load offset=80 (get_local $blockAddress))))
		(set_local $b1 (v8x16.shuffle (8 9 10 11 12 13 14 15 24 25 26 27 28 29 30 31) (v128.load offset=32 (get_local $blockAddress)) (v128.load offset=112 (get_local $blockAddress))))

		(set_local $row1l (i64x2.add (i64x2.add (get_local $row1l) (get_local $b0)) (get_local $row2l)))
		(set_local $row1h (i64x2.add (i64x2.add (get_local $row1h) (get_local $b1)) (get_local $row2h)))
		(set_local $row4l (v128.xor (get_local $row4l) (get_local $row1l)))
		(set_local $row4h (v128.xor (get_local $row4h) (get_local $row1h)))
		(set_local $row4l (v8x16.shuffle (4 5 6 7 0 1 2 3 12 13 14 15 8 9 10 11) (get_local $row4l) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row4h (v8x16.shuffle (4 5 6 7 0 1 2 3 12 13 14 15 8 9 10 11) (get_local $row4h) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row3l (i64x2.add (get_local $row3l) (get_local $row4l)))
		(set_local $row3h (i64x2.add (get_local $row3h) (get_local $row4h)))
		(set_local $row2l (v128.xor (get_local $row2l) (get_local $row3l)))
		(set_local $row2h (v128.xor (get_local $row2h) (get_local $row3h)))
		(set_local $row2l (v8x16.shuffle (3 4 5 6 7 0 1 2 11 12 13 14 15 8 9 10) (get_local $row2l) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row2h (v8x16.shuffle (3 4 5 6 7 0 1 2 11 12 13 14 15 8 9 10) (get_local $row2h) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))

		(set_local $b0 (v8x16.shuffle (0 1 2 3 4 5 6 7 16 17 18 19 20 21 22 23) (v128.load offset=64 (get_local $blockAddress)) (v128.load offset=0 (get_local $blockAddress))))
		(set_local $b1 (v128.bitselect (v128.load offset=16 (get_local $blockAddress)) (v128.load offset=96 (get_local $blockAddress)) (v128.const 0xff 0xff 0xff 0xff 0xff 0xff 0xff 0xff 0 0 0 0 0 0 0 0)))

		(set_local $row1l (i64x2.add (i64x2.add (get_local $row1l) (get_local $b0)) (get_local $row2l)))
		(set_local $row1h (i64x2.add (i64x2.add (get_local $row1h) (get_local $b1)) (get_local $row2h)))
		(set_local $row4l (v128.xor (get_local $row4l) (get_local $row1l)))
		(set_local $row4h (v128.xor (get_local $row4h) (get_local $row1h)))
		(set_local $row4l (v8x16.shuffle (2 3 4 5 6 7 0 1 10 11 12 13 14 15 8 9) (get_local $row4l) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row4h (v8x16.shuffle (2 3 4 5 6 7 0 1 10 11 12 13 14 15 8 9) (get_local $row4h) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row3l (i64x2.add (get_local $row3l) (get_local $row4l)))
		(set_local $row3h (i64x2.add (get_local $row3h) (get_local $row4h)))
		(set_local $row2l (v128.xor (get_local $row2l) (get_local $row3l)))
		(set_local $row2h (v128.xor (get_local $row2h) (get_local $row3h)))
		(set_local $row2l (v128.xor (i64x2.shr_u (get_local $row2l) (i64x2.splat (i64.const 63))) (i64x2.add (get_local $row2l) (get_local $row2l))))
		(set_local $row2h (v128.xor (i64x2.shr_u (get_local $row2h) (i64x2.splat (i64.const 63))) (i64x2.add (get_local $row2h) (get_local $row2h))))

		(set_local $t0 (v8x16.shuffle (24 25 26 27 28 29 30 31 0 1 2 3 4 5 6 7) (get_local $row2h) (get_local $row2l)))
		(set_local $t1 (v8x16.shuffle (24 25 26 27 28 29 30 31 0 1 2 3 4 5 6 7) (get_local $row2l) (get_local $row2h)))
		(set_local $row2l (get_local $t0))
		(set_local $row2h (get_local $t1))
		(set_local $t0 (get_local $row3l))
		(set_local $row3l (get_local $row3h))
		(set_local $row3h (get_local $t0))
		(set_local $t0 (v8x16.shuffle (24 25 26 27 28 29 30 31 0 1 2 3 4 5 6 7) (get_local $row4h) (get_local $row4l)))
		(set_local $t1 (v8x16.shuffle (24 25 26 27 28 29 30 31 0 1 2 3 4 5 6 7) (get_local $row4l) (get_local $row4h)))
		(set_local $row4l (get_local $t1))
		(set_local $row4h (get_local $t0))

		(set_local $b0 (v128.bitselect (v128.load offset=80 (get_local $blockAddress)) (v128.load offset=16 (get_local $blockAddress)) (v128.const 0xff 0xff 0xff 0xff 0xff 0xff 0xff 0xff 0 0 0 0 0 0 0 0)))
		(set_local $b1 (v8x16.shuffle (8 9 10 11 12 13 14 15 24 25 26 27 28 29 30 31) (v128.load offset=48 (get_local $blockAddress)) (v128.load offset=64 (get_local $blockAddress))))

		(set_local $row1l (i64x2.add (i64x2.add (get_local $row1l) (get_local $b0)) (get_local $row2l)))
		(set_local $row1h (i64x2.add (i64x2.add (get_local $row1h) (get_local $b1)) (get_local $row2h)))
		(set_local $row4l (v128.xor (get_local $row4l) (get_local $row1l)))
		(set_local $row4h (v128.xor (get_local $row4h) (get_local $row1h)))
		(set_local $row4l (v8x16.shuffle (4 5 6 7 0 1 2 3 12 13 14 15 8 9 10 11) (get_local $row4l) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row4h (v8x16.shuffle (4 5 6 7 0 1 2 3 12 13 14 15 8 9 10 11) (get_local $row4h) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row3l (i64x2.add (get_local $row3l) (get_local $row4l)))
		(set_local $row3h (i64x2.add (get_local $row3h) (get_local $row4h)))
		(set_local $row2l (v128.xor (get_local $row2l) (get_local $row3l)))
		(set_local $row2h (v128.xor (get_local $row2h) (get_local $row3h)))
		(set_local $row2l (v8x16.shuffle (3 4 5 6 7 0 1 2 11 12 13 14 15 8 9 10) (get_local $row2l) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row2h (v8x16.shuffle (3 4 5 6 7 0 1 2 11 12 13 14 15 8 9 10) (get_local $row2h) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))

		(set_local $b0 (v8x16.shuffle (0 1 2 3 4 5 6 7 16 17 18 19 20 21 22 23) (v128.load offset=112 (get_local $blockAddress)) (v128.load offset=48 (get_local $blockAddress))))
		(set_local $b1 (v8x16.shuffle (24 25 26 27 28 29 30 31 0 1 2 3 4 5 6 7) (v128.load offset=32 (get_local $blockAddress)) (v128.load offset=0 (get_local $blockAddress))))
		
		(set_local $row1l (i64x2.add (i64x2.add (get_local $row1l) (get_local $b0)) (get_local $row2l)))
		(set_local $row1h (i64x2.add (i64x2.add (get_local $row1h) (get_local $b1)) (get_local $row2h)))
		(set_local $row4l (v128.xor (get_local $row4l) (get_local $row1l)))
		(set_local $row4h (v128.xor (get_local $row4h) (get_local $row1h)))
		(set_local $row4l (v8x16.shuffle (2 3 4 5 6 7 0 1 10 11 12 13 14 15 8 9) (get_local $row4l) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row4h (v8x16.shuffle (2 3 4 5 6 7 0 1 10 11 12 13 14 15 8 9) (get_local $row4h) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row3l (i64x2.add (get_local $row3l) (get_local $row4l)))
		(set_local $row3h (i64x2.add (get_local $row3h) (get_local $row4h)))
		(set_local $row2l (v128.xor (get_local $row2l) (get_local $row3l)))
		(set_local $row2h (v128.xor (get_local $row2h) (get_local $row3h)))
		(set_local $row2l (v128.xor (i64x2.shr_u (get_local $row2l) (i64x2.splat (i64.const 63))) (i64x2.add (get_local $row2l) (get_local $row2l))))
		(set_local $row2h (v128.xor (i64x2.shr_u (get_local $row2h) (i64x2.splat (i64.const 63))) (i64x2.add (get_local $row2h) (get_local $row2h))))

		(set_local $t0 (v8x16.shuffle (24 25 26 27 28 29 30 31 0 1 2 3 4 5 6 7) (get_local $row2l) (get_local $row2h)))
		(set_local $t1 (v8x16.shuffle (24 25 26 27 28 29 30 31 0 1 2 3 4 5 6 7) (get_local $row2h) (get_local $row2l)))
		(set_local $row2l (get_local $t0))
		(set_local $row2h (get_local $t1))
		(set_local $t0 (get_local $row3l))
		(set_local $row3l (get_local $row3h))
		(set_local $row3h (get_local $t0))
		(set_local $t0 (v8x16.shuffle (24 25 26 27 28 29 30 31 0 1 2 3 4 5 6 7) (get_local $row4l) (get_local $row4h)))
		(set_local $t1 (v8x16.shuffle (24 25 26 27 28 29 30 31 0 1 2 3 4 5 6 7) (get_local $row4h) (get_local $row4l)))
		(set_local $row4l (get_local $t1))
		(set_local $row4h (get_local $t0))
			
		;; Round 3

		(set_local $b0 (v8x16.shuffle (8 9 10 11 12 13 14 15 24 25 26 27 28 29 30 31) (v128.load offset=48 (get_local $blockAddress)) (v128.load offset=16 (get_local $blockAddress))))
		(set_local $b1 (v8x16.shuffle (8 9 10 11 12 13 14 15 24 25 26 27 28 29 30 31) (v128.load offset=96 (get_local $blockAddress)) (v128.load offset=80 (get_local $blockAddress))))

		(set_local $row1l (i64x2.add (i64x2.add (get_local $row1l) (get_local $b0)) (get_local $row2l)))
		(set_local $row1h (i64x2.add (i64x2.add (get_local $row1h) (get_local $b1)) (get_local $row2h)))
		(set_local $row4l (v128.xor (get_local $row4l) (get_local $row1l)))
		(set_local $row4h (v128.xor (get_local $row4h) (get_local $row1h)))
		(set_local $row4l (v8x16.shuffle (4 5 6 7 0 1 2 3 12 13 14 15 8 9 10 11) (get_local $row4l) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row4h (v8x16.shuffle (4 5 6 7 0 1 2 3 12 13 14 15 8 9 10 11) (get_local $row4h) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row3l (i64x2.add (get_local $row3l) (get_local $row4l)))
		(set_local $row3h (i64x2.add (get_local $row3h) (get_local $row4h)))
		(set_local $row2l (v128.xor (get_local $row2l) (get_local $row3l)))
		(set_local $row2h (v128.xor (get_local $row2h) (get_local $row3h)))
		(set_local $row2l (v8x16.shuffle (3 4 5 6 7 0 1 2 11 12 13 14 15 8 9 10) (get_local $row2l) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row2h (v8x16.shuffle (3 4 5 6 7 0 1 2 11 12 13 14 15 8 9 10) (get_local $row2h) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))

		(set_local $b0 (v8x16.shuffle (8 9 10 11 12 13 14 15 24 25 26 27 28 29 30 31) (v128.load offset=64 (get_local $blockAddress)) (v128.load offset=0 (get_local $blockAddress))))
		(set_local $b1 (v8x16.shuffle (0 1 2 3 4 5 6 7 16 17 18 19 20 21 22 23) (v128.load offset=96 (get_local $blockAddress)) (v128.load offset=112 (get_local $blockAddress))))

		(set_local $row1l (i64x2.add (i64x2.add (get_local $row1l) (get_local $b0)) (get_local $row2l)))
		(set_local $row1h (i64x2.add (i64x2.add (get_local $row1h) (get_local $b1)) (get_local $row2h)))
		(set_local $row4l (v128.xor (get_local $row4l) (get_local $row1l)))
		(set_local $row4h (v128.xor (get_local $row4h) (get_local $row1h)))
		(set_local $row4l (v8x16.shuffle (2 3 4 5 6 7 0 1 10 11 12 13 14 15 8 9) (get_local $row4l) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row4h (v8x16.shuffle (2 3 4 5 6 7 0 1 10 11 12 13 14 15 8 9) (get_local $row4h) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row3l (i64x2.add (get_local $row3l) (get_local $row4l)))
		(set_local $row3h (i64x2.add (get_local $row3h) (get_local $row4h)))
		(set_local $row2l (v128.xor (get_local $row2l) (get_local $row3l)))
		(set_local $row2h (v128.xor (get_local $row2h) (get_local $row3h)))
		(set_local $row2l (v128.xor (i64x2.shr_u (get_local $row2l) (i64x2.splat (i64.const 63))) (i64x2.add (get_local $row2l) (get_local $row2l))))
		(set_local $row2h (v128.xor (i64x2.shr_u (get_local $row2h) (i64x2.splat (i64.const 63))) (i64x2.add (get_local $row2h) (get_local $row2h))))

		(set_local $t0 (v8x16.shuffle (24 25 26 27 28 29 30 31 0 1 2 3 4 5 6 7) (get_local $row2h) (get_local $row2l)))
		(set_local $t1 (v8x16.shuffle (24 25 26 27 28 29 30 31 0 1 2 3 4 5 6 7) (get_local $row2l) (get_local $row2h)))
		(set_local $row2l (get_local $t0))
		(set_local $row2h (get_local $t1))
		(set_local $t0 (get_local $row3l))
		(set_local $row3l (get_local $row3h))
		(set_local $row3h (get_local $t0))
		(set_local $t0 (v8x16.shuffle (24 25 26 27 28 29 30 31 0 1 2 3 4 5 6 7) (get_local $row4h) (get_local $row4l)))
		(set_local $t1 (v8x16.shuffle (24 25 26 27 28 29 30 31 0 1 2 3 4 5 6 7) (get_local $row4l) (get_local $row4h)))
		(set_local $row4l (get_local $t1))
		(set_local $row4h (get_local $t0))

		(set_local $b0 (v128.bitselect (v128.load offset=16 (get_local $blockAddress)) (v128.load offset=32 (get_local $blockAddress))  (v128.const 0xff 0xff 0xff 0xff 0xff 0xff 0xff 0xff 0 0 0 0 0 0 0 0)))
		(set_local $b1 (v128.bitselect (v128.load offset=32 (get_local $blockAddress)) (v128.load offset=112 (get_local $blockAddress))  (v128.const 0xff 0xff 0xff 0xff 0xff 0xff 0xff 0xff 0 0 0 0 0 0 0 0)))

		(set_local $row1l (i64x2.add (i64x2.add (get_local $row1l) (get_local $b0)) (get_local $row2l)))
		(set_local $row1h (i64x2.add (i64x2.add (get_local $row1h) (get_local $b1)) (get_local $row2h)))
		(set_local $row4l (v128.xor (get_local $row4l) (get_local $row1l)))
		(set_local $row4h (v128.xor (get_local $row4h) (get_local $row1h)))
		(set_local $row4l (v8x16.shuffle (4 5 6 7 0 1 2 3 12 13 14 15 8 9 10 11) (get_local $row4l) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row4h (v8x16.shuffle (4 5 6 7 0 1 2 3 12 13 14 15 8 9 10 11) (get_local $row4h) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row3l (i64x2.add (get_local $row3l) (get_local $row4l)))
		(set_local $row3h (i64x2.add (get_local $row3h) (get_local $row4h)))
		(set_local $row2l (v128.xor (get_local $row2l) (get_local $row3l)))
		(set_local $row2h (v128.xor (get_local $row2h) (get_local $row3h)))
		(set_local $row2l (v8x16.shuffle (3 4 5 6 7 0 1 2 11 12 13 14 15 8 9 10) (get_local $row2l) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row2h (v8x16.shuffle (3 4 5 6 7 0 1 2 11 12 13 14 15 8 9 10) (get_local $row2h) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))

		(set_local $b0 (v8x16.shuffle (0 1 2 3 4 5 6 7 16 17 18 19 20 21 22 23) (v128.load offset=48 (get_local $blockAddress)) (v128.load offset=80 (get_local $blockAddress))))
		(set_local $b1 (v8x16.shuffle (0 1 2 3 4 5 6 7 16 17 18 19 20 21 22 23) (v128.load offset=0 (get_local $blockAddress)) (v128.load offset=64 (get_local $blockAddress))))
		
		(set_local $row1l (i64x2.add (i64x2.add (get_local $row1l) (get_local $b0)) (get_local $row2l)))
		(set_local $row1h (i64x2.add (i64x2.add (get_local $row1h) (get_local $b1)) (get_local $row2h)))
		(set_local $row4l (v128.xor (get_local $row4l) (get_local $row1l)))
		(set_local $row4h (v128.xor (get_local $row4h) (get_local $row1h)))
		(set_local $row4l (v8x16.shuffle (2 3 4 5 6 7 0 1 10 11 12 13 14 15 8 9) (get_local $row4l) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row4h (v8x16.shuffle (2 3 4 5 6 7 0 1 10 11 12 13 14 15 8 9) (get_local $row4h) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row3l (i64x2.add (get_local $row3l) (get_local $row4l)))
		(set_local $row3h (i64x2.add (get_local $row3h) (get_local $row4h)))
		(set_local $row2l (v128.xor (get_local $row2l) (get_local $row3l)))
		(set_local $row2h (v128.xor (get_local $row2h) (get_local $row3h)))
		(set_local $row2l (v128.xor (i64x2.shr_u (get_local $row2l) (i64x2.splat (i64.const 63))) (i64x2.add (get_local $row2l) (get_local $row2l))))
		(set_local $row2h (v128.xor (i64x2.shr_u (get_local $row2h) (i64x2.splat (i64.const 63))) (i64x2.add (get_local $row2h) (get_local $row2h))))

		(set_local $t0 (v8x16.shuffle (24 25 26 27 28 29 30 31 0 1 2 3 4 5 6 7) (get_local $row2l) (get_local $row2h)))
		(set_local $t1 (v8x16.shuffle (24 25 26 27 28 29 30 31 0 1 2 3 4 5 6 7) (get_local $row2h) (get_local $row2l)))
		(set_local $row2l (get_local $t0))
		(set_local $row2h (get_local $t1))
		(set_local $t0 (get_local $row3l))
		(set_local $row3l (get_local $row3h))
		(set_local $row3h (get_local $t0))
		(set_local $t0 (v8x16.shuffle (24 25 26 27 28 29 30 31 0 1 2 3 4 5 6 7) (get_local $row4l) (get_local $row4h)))
		(set_local $t1 (v8x16.shuffle (24 25 26 27 28 29 30 31 0 1 2 3 4 5 6 7) (get_local $row4h) (get_local $row4l)))
		(set_local $row4l (get_local $t1))
		(set_local $row4h (get_local $t0))
			
		;; Round 4

		(set_local $b0 (v8x16.shuffle (8 9 10 11 12 13 14 15 24 25 26 27 28 29 30 31) (v128.load offset=64 (get_local $blockAddress)) (v128.load offset=32 (get_local $blockAddress))))
		(set_local $b1 (v8x16.shuffle (0 1 2 3 4 5 6 7 16 17 18 19 20 21 22 23) (v128.load offset=16 (get_local $blockAddress)) (v128.load offset=80 (get_local $blockAddress))))

		(set_local $row1l (i64x2.add (i64x2.add (get_local $row1l) (get_local $b0)) (get_local $row2l)))
		(set_local $row1h (i64x2.add (i64x2.add (get_local $row1h) (get_local $b1)) (get_local $row2h)))
		(set_local $row4l (v128.xor (get_local $row4l) (get_local $row1l)))
		(set_local $row4h (v128.xor (get_local $row4h) (get_local $row1h)))
		(set_local $row4l (v8x16.shuffle (4 5 6 7 0 1 2 3 12 13 14 15 8 9 10 11) (get_local $row4l) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row4h (v8x16.shuffle (4 5 6 7 0 1 2 3 12 13 14 15 8 9 10 11) (get_local $row4h) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row3l (i64x2.add (get_local $row3l) (get_local $row4l)))
		(set_local $row3h (i64x2.add (get_local $row3h) (get_local $row4h)))
		(set_local $row2l (v128.xor (get_local $row2l) (get_local $row3l)))
		(set_local $row2h (v128.xor (get_local $row2h) (get_local $row3h)))
		(set_local $row2l (v8x16.shuffle (3 4 5 6 7 0 1 2 11 12 13 14 15 8 9 10) (get_local $row2l) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row2h (v8x16.shuffle (3 4 5 6 7 0 1 2 11 12 13 14 15 8 9 10) (get_local $row2h) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))

		(set_local $b0 (v128.bitselect (v128.load offset=0 (get_local $blockAddress)) (v128.load offset=48 (get_local $blockAddress))  (v128.const 0xff 0xff 0xff 0xff 0xff 0xff 0xff 0xff 0 0 0 0 0 0 0 0)))
		(set_local $b1 (v128.bitselect (v128.load offset=32 (get_local $blockAddress)) (v128.load offset=112 (get_local $blockAddress))  (v128.const 0xff 0xff 0xff 0xff 0xff 0xff 0xff 0xff 0 0 0 0 0 0 0 0)))

		(set_local $row1l (i64x2.add (i64x2.add (get_local $row1l) (get_local $b0)) (get_local $row2l)))
		(set_local $row1h (i64x2.add (i64x2.add (get_local $row1h) (get_local $b1)) (get_local $row2h)))
		(set_local $row4l (v128.xor (get_local $row4l) (get_local $row1l)))
		(set_local $row4h (v128.xor (get_local $row4h) (get_local $row1h)))
		(set_local $row4l (v8x16.shuffle (2 3 4 5 6 7 0 1 10 11 12 13 14 15 8 9) (get_local $row4l) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row4h (v8x16.shuffle (2 3 4 5 6 7 0 1 10 11 12 13 14 15 8 9) (get_local $row4h) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row3l (i64x2.add (get_local $row3l) (get_local $row4l)))
		(set_local $row3h (i64x2.add (get_local $row3h) (get_local $row4h)))
		(set_local $row2l (v128.xor (get_local $row2l) (get_local $row3l)))
		(set_local $row2h (v128.xor (get_local $row2h) (get_local $row3h)))
		(set_local $row2l (v128.xor (i64x2.shr_u (get_local $row2l) (i64x2.splat (i64.const 63))) (i64x2.add (get_local $row2l) (get_local $row2l))))
		(set_local $row2h (v128.xor (i64x2.shr_u (get_local $row2h) (i64x2.splat (i64.const 63))) (i64x2.add (get_local $row2h) (get_local $row2h))))

		(set_local $t0 (v8x16.shuffle (24 25 26 27 28 29 30 31 0 1 2 3 4 5 6 7) (get_local $row2h) (get_local $row2l)))
		(set_local $t1 (v8x16.shuffle (24 25 26 27 28 29 30 31 0 1 2 3 4 5 6 7) (get_local $row2l) (get_local $row2h)))
		(set_local $row2l (get_local $t0))
		(set_local $row2h (get_local $t1))
		(set_local $t0 (get_local $row3l))
		(set_local $row3l (get_local $row3h))
		(set_local $row3h (get_local $t0))
		(set_local $t0 (v8x16.shuffle (24 25 26 27 28 29 30 31 0 1 2 3 4 5 6 7) (get_local $row4h) (get_local $row4l)))
		(set_local $t1 (v8x16.shuffle (24 25 26 27 28 29 30 31 0 1 2 3 4 5 6 7) (get_local $row4l) (get_local $row4h)))
		(set_local $row4l (get_local $t1))
		(set_local $row4h (get_local $t0))

		(set_local $b0 (v128.bitselect (v128.load offset=112 (get_local $blockAddress)) (v128.load offset=80 (get_local $blockAddress))  (v128.const 0xff 0xff 0xff 0xff 0xff 0xff 0xff 0xff 0 0 0 0 0 0 0 0)))
		(set_local $b1 (v128.bitselect (v128.load offset=48 (get_local $blockAddress)) (v128.load offset=16 (get_local $blockAddress))  (v128.const 0xff 0xff 0xff 0xff 0xff 0xff 0xff 0xff 0 0 0 0 0 0 0 0)))

		(set_local $row1l (i64x2.add (i64x2.add (get_local $row1l) (get_local $b0)) (get_local $row2l)))
		(set_local $row1h (i64x2.add (i64x2.add (get_local $row1h) (get_local $b1)) (get_local $row2h)))
		(set_local $row4l (v128.xor (get_local $row4l) (get_local $row1l)))
		(set_local $row4h (v128.xor (get_local $row4h) (get_local $row1h)))
		(set_local $row4l (v8x16.shuffle (4 5 6 7 0 1 2 3 12 13 14 15 8 9 10 11) (get_local $row4l) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row4h (v8x16.shuffle (4 5 6 7 0 1 2 3 12 13 14 15 8 9 10 11) (get_local $row4h) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row3l (i64x2.add (get_local $row3l) (get_local $row4l)))
		(set_local $row3h (i64x2.add (get_local $row3h) (get_local $row4h)))
		(set_local $row2l (v128.xor (get_local $row2l) (get_local $row3l)))
		(set_local $row2h (v128.xor (get_local $row2h) (get_local $row3h)))
		(set_local $row2l (v8x16.shuffle (3 4 5 6 7 0 1 2 11 12 13 14 15 8 9 10) (get_local $row2l) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row2h (v8x16.shuffle (3 4 5 6 7 0 1 2 11 12 13 14 15 8 9 10) (get_local $row2h) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))

		(set_local $b0 (v8x16.shuffle (24 25 26 27 28 29 30 31 0 1 2 3 4 5 6 7) (v128.load offset=96 (get_local $blockAddress)) (v128.load offset=0 (get_local $blockAddress))))
		(set_local $b1 (v128.bitselect (v128.load offset=64 (get_local $blockAddress)) (v128.load offset=96 (get_local $blockAddress))  (v128.const 0xff 0xff 0xff 0xff 0xff 0xff 0xff 0xff 0 0 0 0 0 0 0 0)))

		(set_local $row1l (i64x2.add (i64x2.add (get_local $row1l) (get_local $b0)) (get_local $row2l)))
		(set_local $row1h (i64x2.add (i64x2.add (get_local $row1h) (get_local $b1)) (get_local $row2h)))
		(set_local $row4l (v128.xor (get_local $row4l) (get_local $row1l)))
		(set_local $row4h (v128.xor (get_local $row4h) (get_local $row1h)))
		(set_local $row4l (v8x16.shuffle (2 3 4 5 6 7 0 1 10 11 12 13 14 15 8 9) (get_local $row4l) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row4h (v8x16.shuffle (2 3 4 5 6 7 0 1 10 11 12 13 14 15 8 9) (get_local $row4h) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row3l (i64x2.add (get_local $row3l) (get_local $row4l)))
		(set_local $row3h (i64x2.add (get_local $row3h) (get_local $row4h)))
		(set_local $row2l (v128.xor (get_local $row2l) (get_local $row3l)))
		(set_local $row2h (v128.xor (get_local $row2h) (get_local $row3h)))
		(set_local $row2l (v128.xor (i64x2.shr_u (get_local $row2l) (i64x2.splat (i64.const 63))) (i64x2.add (get_local $row2l) (get_local $row2l))))
		(set_local $row2h (v128.xor (i64x2.shr_u (get_local $row2h) (i64x2.splat (i64.const 63))) (i64x2.add (get_local $row2h) (get_local $row2h))))

		(set_local $t0 (v8x16.shuffle (24 25 26 27 28 29 30 31 0 1 2 3 4 5 6 7) (get_local $row2l) (get_local $row2h)))
		(set_local $t1 (v8x16.shuffle (24 25 26 27 28 29 30 31 0 1 2 3 4 5 6 7) (get_local $row2h) (get_local $row2l)))
		(set_local $row2l (get_local $t0))
		(set_local $row2h (get_local $t1))
		(set_local $t0 (get_local $row3l))
		(set_local $row3l (get_local $row3h))
		(set_local $row3h (get_local $t0))
		(set_local $t0 (v8x16.shuffle (24 25 26 27 28 29 30 31 0 1 2 3 4 5 6 7) (get_local $row4l) (get_local $row4h)))
		(set_local $t1 (v8x16.shuffle (24 25 26 27 28 29 30 31 0 1 2 3 4 5 6 7) (get_local $row4h) (get_local $row4l)))
		(set_local $row4l (get_local $t1))
		(set_local $row4h (get_local $t0))
		
		;; Round 5

		(set_local $b0 (v8x16.shuffle (0 1 2 3 4 5 6 7 16 17 18 19 20 21 22 23) (v128.load offset=16 (get_local $blockAddress)) (v128.load offset=48 (get_local $blockAddress))))
		(set_local $b1 (v8x16.shuffle (0 1 2 3 4 5 6 7 16 17 18 19 20 21 22 23) (v128.load offset=0 (get_local $blockAddress)) (v128.load offset=64 (get_local $blockAddress))))

		(set_local $row1l (i64x2.add (i64x2.add (get_local $row1l) (get_local $b0)) (get_local $row2l)))
		(set_local $row1h (i64x2.add (i64x2.add (get_local $row1h) (get_local $b1)) (get_local $row2h)))
		(set_local $row4l (v128.xor (get_local $row4l) (get_local $row1l)))
		(set_local $row4h (v128.xor (get_local $row4h) (get_local $row1h)))
		(set_local $row4l (v8x16.shuffle (4 5 6 7 0 1 2 3 12 13 14 15 8 9 10 11) (get_local $row4l) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row4h (v8x16.shuffle (4 5 6 7 0 1 2 3 12 13 14 15 8 9 10 11) (get_local $row4h) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row3l (i64x2.add (get_local $row3l) (get_local $row4l)))
		(set_local $row3h (i64x2.add (get_local $row3h) (get_local $row4h)))
		(set_local $row2l (v128.xor (get_local $row2l) (get_local $row3l)))
		(set_local $row2h (v128.xor (get_local $row2h) (get_local $row3h)))
		(set_local $row2l (v8x16.shuffle (3 4 5 6 7 0 1 2 11 12 13 14 15 8 9 10) (get_local $row2l) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row2h (v8x16.shuffle (3 4 5 6 7 0 1 2 11 12 13 14 15 8 9 10) (get_local $row2h) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))

		(set_local $b0 (v8x16.shuffle (0 1 2 3 4 5 6 7 16 17 18 19 20 21 22 23) (v128.load offset=96 (get_local $blockAddress)) (v128.load offset=80 (get_local $blockAddress))))
		(set_local $b1 (v8x16.shuffle (8 9 10 11 12 13 14 15 24 25 26 27 28 29 30 31) (v128.load offset=80 (get_local $blockAddress)) (v128.load offset=16 (get_local $blockAddress))))

		(set_local $row1l (i64x2.add (i64x2.add (get_local $row1l) (get_local $b0)) (get_local $row2l)))
		(set_local $row1h (i64x2.add (i64x2.add (get_local $row1h) (get_local $b1)) (get_local $row2h)))
		(set_local $row4l (v128.xor (get_local $row4l) (get_local $row1l)))
		(set_local $row4h (v128.xor (get_local $row4h) (get_local $row1h)))
		(set_local $row4l (v8x16.shuffle (2 3 4 5 6 7 0 1 10 11 12 13 14 15 8 9) (get_local $row4l) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row4h (v8x16.shuffle (2 3 4 5 6 7 0 1 10 11 12 13 14 15 8 9) (get_local $row4h) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row3l (i64x2.add (get_local $row3l) (get_local $row4l)))
		(set_local $row3h (i64x2.add (get_local $row3h) (get_local $row4h)))
		(set_local $row2l (v128.xor (get_local $row2l) (get_local $row3l)))
		(set_local $row2h (v128.xor (get_local $row2h) (get_local $row3h)))
		(set_local $row2l (v128.xor (i64x2.shr_u (get_local $row2l) (i64x2.splat (i64.const 63))) (i64x2.add (get_local $row2l) (get_local $row2l))))
		(set_local $row2h (v128.xor (i64x2.shr_u (get_local $row2h) (i64x2.splat (i64.const 63))) (i64x2.add (get_local $row2h) (get_local $row2h))))

		(set_local $t0 (v8x16.shuffle (24 25 26 27 28 29 30 31 0 1 2 3 4 5 6 7) (get_local $row2h) (get_local $row2l)))
		(set_local $t1 (v8x16.shuffle (24 25 26 27 28 29 30 31 0 1 2 3 4 5 6 7) (get_local $row2l) (get_local $row2h)))
		(set_local $row2l (get_local $t0))
		(set_local $row2h (get_local $t1))
		(set_local $t0 (get_local $row3l))
		(set_local $row3l (get_local $row3h))
		(set_local $row3h (get_local $t0))
		(set_local $t0 (v8x16.shuffle (24 25 26 27 28 29 30 31 0 1 2 3 4 5 6 7) (get_local $row4h) (get_local $row4l)))
		(set_local $t1 (v8x16.shuffle (24 25 26 27 28 29 30 31 0 1 2 3 4 5 6 7) (get_local $row4l) (get_local $row4h)))
		(set_local $row4l (get_local $t1))
		(set_local $row4h (get_local $t0))

		(set_local $b0 (v128.bitselect (v128.load offset=32 (get_local $blockAddress)) (v128.load offset=48 (get_local $blockAddress))  (v128.const 0xff 0xff 0xff 0xff 0xff 0xff 0xff 0xff 0 0 0 0 0 0 0 0)))
		(set_local $b1 (v8x16.shuffle (8 9 10 11 12 13 14 15 24 25 26 27 28 29 30 31) (v128.load offset=112 (get_local $blockAddress)) (v128.load offset=0 (get_local $blockAddress))))

		(set_local $row1l (i64x2.add (i64x2.add (get_local $row1l) (get_local $b0)) (get_local $row2l)))
		(set_local $row1h (i64x2.add (i64x2.add (get_local $row1h) (get_local $b1)) (get_local $row2h)))
		(set_local $row4l (v128.xor (get_local $row4l) (get_local $row1l)))
		(set_local $row4h (v128.xor (get_local $row4h) (get_local $row1h)))
		(set_local $row4l (v8x16.shuffle (4 5 6 7 0 1 2 3 12 13 14 15 8 9 10 11) (get_local $row4l) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row4h (v8x16.shuffle (4 5 6 7 0 1 2 3 12 13 14 15 8 9 10 11) (get_local $row4h) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row3l (i64x2.add (get_local $row3l) (get_local $row4l)))
		(set_local $row3h (i64x2.add (get_local $row3h) (get_local $row4h)))
		(set_local $row2l (v128.xor (get_local $row2l) (get_local $row3l)))
		(set_local $row2h (v128.xor (get_local $row2h) (get_local $row3h)))
		(set_local $row2l (v8x16.shuffle (3 4 5 6 7 0 1 2 11 12 13 14 15 8 9 10) (get_local $row2l) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row2h (v8x16.shuffle (3 4 5 6 7 0 1 2 11 12 13 14 15 8 9 10) (get_local $row2h) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))

		(set_local $b0 (v8x16.shuffle (8 9 10 11 12 13 14 15 24 25 26 27 28 29 30 31) (v128.load offset=96 (get_local $blockAddress)) (v128.load offset=32 (get_local $blockAddress))))
		(set_local $b1 (v128.bitselect (v128.load offset=112 (get_local $blockAddress)) (v128.load offset=64 (get_local $blockAddress))  (v128.const 0xff 0xff 0xff 0xff 0xff 0xff 0xff 0xff 0 0 0 0 0 0 0 0)))
		
		(set_local $row1l (i64x2.add (i64x2.add (get_local $row1l) (get_local $b0)) (get_local $row2l)))
		(set_local $row1h (i64x2.add (i64x2.add (get_local $row1h) (get_local $b1)) (get_local $row2h)))
		(set_local $row4l (v128.xor (get_local $row4l) (get_local $row1l)))
		(set_local $row4h (v128.xor (get_local $row4h) (get_local $row1h)))
		(set_local $row4l (v8x16.shuffle (2 3 4 5 6 7 0 1 10 11 12 13 14 15 8 9) (get_local $row4l) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row4h (v8x16.shuffle (2 3 4 5 6 7 0 1 10 11 12 13 14 15 8 9) (get_local $row4h) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row3l (i64x2.add (get_local $row3l) (get_local $row4l)))
		(set_local $row3h (i64x2.add (get_local $row3h) (get_local $row4h)))
		(set_local $row2l (v128.xor (get_local $row2l) (get_local $row3l)))
		(set_local $row2h (v128.xor (get_local $row2h) (get_local $row3h)))
		(set_local $row2l (v128.xor (i64x2.shr_u (get_local $row2l) (i64x2.splat (i64.const 63))) (i64x2.add (get_local $row2l) (get_local $row2l))))
		(set_local $row2h (v128.xor (i64x2.shr_u (get_local $row2h) (i64x2.splat (i64.const 63))) (i64x2.add (get_local $row2h) (get_local $row2h))))

		(set_local $t0 (v8x16.shuffle (24 25 26 27 28 29 30 31 0 1 2 3 4 5 6 7) (get_local $row2l) (get_local $row2h)))
		(set_local $t1 (v8x16.shuffle (24 25 26 27 28 29 30 31 0 1 2 3 4 5 6 7) (get_local $row2h) (get_local $row2l)))
		(set_local $row2l (get_local $t0))
		(set_local $row2h (get_local $t1))
		(set_local $t0 (get_local $row3l))
		(set_local $row3l (get_local $row3h))
		(set_local $row3h (get_local $t0))
		(set_local $t0 (v8x16.shuffle (24 25 26 27 28 29 30 31 0 1 2 3 4 5 6 7) (get_local $row4l) (get_local $row4h)))
		(set_local $t1 (v8x16.shuffle (24 25 26 27 28 29 30 31 0 1 2 3 4 5 6 7) (get_local $row4h) (get_local $row4l)))
		(set_local $row4l (get_local $t1))
		(set_local $row4h (get_local $t0))
			
		;; Round 6

		(set_local $b0 (v128.bitselect (v128.load offset=96 (get_local $blockAddress)) (v128.load offset=0 (get_local $blockAddress))  (v128.const 0xff 0xff 0xff 0xff 0xff 0xff 0xff 0xff 0 0 0 0 0 0 0 0)))
		(set_local $b1 (v8x16.shuffle (0 1 2 3 4 5 6 7 16 17 18 19 20 21 22 23) (v128.load offset=112 (get_local $blockAddress)) (v128.load offset=32 (get_local $blockAddress))))

		(set_local $row1l (i64x2.add (i64x2.add (get_local $row1l) (get_local $b0)) (get_local $row2l)))
		(set_local $row1h (i64x2.add (i64x2.add (get_local $row1h) (get_local $b1)) (get_local $row2h)))
		(set_local $row4l (v128.xor (get_local $row4l) (get_local $row1l)))
		(set_local $row4h (v128.xor (get_local $row4h) (get_local $row1h)))
		(set_local $row4l (v8x16.shuffle (4 5 6 7 0 1 2 3 12 13 14 15 8 9 10 11) (get_local $row4l) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row4h (v8x16.shuffle (4 5 6 7 0 1 2 3 12 13 14 15 8 9 10 11) (get_local $row4h) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row3l (i64x2.add (get_local $row3l) (get_local $row4l)))
		(set_local $row3h (i64x2.add (get_local $row3h) (get_local $row4h)))
		(set_local $row2l (v128.xor (get_local $row2l) (get_local $row3l)))
		(set_local $row2h (v128.xor (get_local $row2h) (get_local $row3h)))
		(set_local $row2l (v8x16.shuffle (3 4 5 6 7 0 1 2 11 12 13 14 15 8 9 10) (get_local $row2l) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row2h (v8x16.shuffle (3 4 5 6 7 0 1 2 11 12 13 14 15 8 9 10) (get_local $row2h) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))

		(set_local $b0 (v8x16.shuffle (8 9 10 11 12 13 14 15 24 25 26 27 28 29 30 31) (v128.load offset=32 (get_local $blockAddress)) (v128.load offset=112 (get_local $blockAddress))))
		(set_local $b1 (v8x16.shuffle (24 25 26 27 28 29 30 31 0 1 2 3 4 5 6 7) (v128.load offset=80 (get_local $blockAddress)) (v128.load offset=96 (get_local $blockAddress))))

		(set_local $row1l (i64x2.add (i64x2.add (get_local $row1l) (get_local $b0)) (get_local $row2l)))
		(set_local $row1h (i64x2.add (i64x2.add (get_local $row1h) (get_local $b1)) (get_local $row2h)))
		(set_local $row4l (v128.xor (get_local $row4l) (get_local $row1l)))
		(set_local $row4h (v128.xor (get_local $row4h) (get_local $row1h)))
		(set_local $row4l (v8x16.shuffle (2 3 4 5 6 7 0 1 10 11 12 13 14 15 8 9) (get_local $row4l) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row4h (v8x16.shuffle (2 3 4 5 6 7 0 1 10 11 12 13 14 15 8 9) (get_local $row4h) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row3l (i64x2.add (get_local $row3l) (get_local $row4l)))
		(set_local $row3h (i64x2.add (get_local $row3h) (get_local $row4h)))
		(set_local $row2l (v128.xor (get_local $row2l) (get_local $row3l)))
		(set_local $row2h (v128.xor (get_local $row2h) (get_local $row3h)))
		(set_local $row2l (v128.xor (i64x2.shr_u (get_local $row2l) (i64x2.splat (i64.const 63))) (i64x2.add (get_local $row2l) (get_local $row2l))))
		(set_local $row2h (v128.xor (i64x2.shr_u (get_local $row2h) (i64x2.splat (i64.const 63))) (i64x2.add (get_local $row2h) (get_local $row2h))))

		(set_local $t0 (v8x16.shuffle (24 25 26 27 28 29 30 31 0 1 2 3 4 5 6 7) (get_local $row2h) (get_local $row2l)))
		(set_local $t1 (v8x16.shuffle (24 25 26 27 28 29 30 31 0 1 2 3 4 5 6 7) (get_local $row2l) (get_local $row2h)))
		(set_local $row2l (get_local $t0))
		(set_local $row2h (get_local $t1))
		(set_local $t0 (get_local $row3l))
		(set_local $row3l (get_local $row3h))
		(set_local $row3h (get_local $t0))
		(set_local $t0 (v8x16.shuffle (24 25 26 27 28 29 30 31 0 1 2 3 4 5 6 7) (get_local $row4h) (get_local $row4l)))
		(set_local $t1 (v8x16.shuffle (24 25 26 27 28 29 30 31 0 1 2 3 4 5 6 7) (get_local $row4l) (get_local $row4h)))
		(set_local $row4l (get_local $t1))
		(set_local $row4h (get_local $t0))

		(set_local $b0 (v8x16.shuffle (0 1 2 3 4 5 6 7 16 17 18 19 20 21 22 23) (v128.load offset=0 (get_local $blockAddress)) (v128.load offset=48 (get_local $blockAddress))))
		(set_local $b1 (v8x16.shuffle (8 9 10 11 12 13 14 15 0 1 2 3 4 5 6 7) (v128.load offset=64 (get_local $blockAddress)) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))

		(set_local $row1l (i64x2.add (i64x2.add (get_local $row1l) (get_local $b0)) (get_local $row2l)))
		(set_local $row1h (i64x2.add (i64x2.add (get_local $row1h) (get_local $b1)) (get_local $row2h)))
		(set_local $row4l (v128.xor (get_local $row4l) (get_local $row1l)))
		(set_local $row4h (v128.xor (get_local $row4h) (get_local $row1h)))
		(set_local $row4l (v8x16.shuffle (4 5 6 7 0 1 2 3 12 13 14 15 8 9 10 11) (get_local $row4l) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row4h (v8x16.shuffle (4 5 6 7 0 1 2 3 12 13 14 15 8 9 10 11) (get_local $row4h) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row3l (i64x2.add (get_local $row3l) (get_local $row4l)))
		(set_local $row3h (i64x2.add (get_local $row3h) (get_local $row4h)))
		(set_local $row2l (v128.xor (get_local $row2l) (get_local $row3l)))
		(set_local $row2h (v128.xor (get_local $row2h) (get_local $row3h)))
		(set_local $row2l (v8x16.shuffle (3 4 5 6 7 0 1 2 11 12 13 14 15 8 9 10) (get_local $row2l) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row2h (v8x16.shuffle (3 4 5 6 7 0 1 2 11 12 13 14 15 8 9 10) (get_local $row2h) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))

		(set_local $b0 (v8x16.shuffle (8 9 10 11 12 13 14 15 24 25 26 27 28 29 30 31) (v128.load offset=48 (get_local $blockAddress)) (v128.load offset=16 (get_local $blockAddress))))
		(set_local $b1 (v128.bitselect (v128.load offset=16 (get_local $blockAddress)) (v128.load offset=80 (get_local $blockAddress))  (v128.const 0xff 0xff 0xff 0xff 0xff 0xff 0xff 0xff 0 0 0 0 0 0 0 0)))
		
		(set_local $row1l (i64x2.add (i64x2.add (get_local $row1l) (get_local $b0)) (get_local $row2l)))
		(set_local $row1h (i64x2.add (i64x2.add (get_local $row1h) (get_local $b1)) (get_local $row2h)))
		(set_local $row4l (v128.xor (get_local $row4l) (get_local $row1l)))
		(set_local $row4h (v128.xor (get_local $row4h) (get_local $row1h)))
		(set_local $row4l (v8x16.shuffle (2 3 4 5 6 7 0 1 10 11 12 13 14 15 8 9) (get_local $row4l) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row4h (v8x16.shuffle (2 3 4 5 6 7 0 1 10 11 12 13 14 15 8 9) (get_local $row4h) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row3l (i64x2.add (get_local $row3l) (get_local $row4l)))
		(set_local $row3h (i64x2.add (get_local $row3h) (get_local $row4h)))
		(set_local $row2l (v128.xor (get_local $row2l) (get_local $row3l)))
		(set_local $row2h (v128.xor (get_local $row2h) (get_local $row3h)))
		(set_local $row2l (v128.xor (i64x2.shr_u (get_local $row2l) (i64x2.splat (i64.const 63))) (i64x2.add (get_local $row2l) (get_local $row2l))))
		(set_local $row2h (v128.xor (i64x2.shr_u (get_local $row2h) (i64x2.splat (i64.const 63))) (i64x2.add (get_local $row2h) (get_local $row2h))))

		(set_local $t0 (v8x16.shuffle (24 25 26 27 28 29 30 31 0 1 2 3 4 5 6 7) (get_local $row2l) (get_local $row2h)))
		(set_local $t1 (v8x16.shuffle (24 25 26 27 28 29 30 31 0 1 2 3 4 5 6 7) (get_local $row2h) (get_local $row2l)))
		(set_local $row2l (get_local $t0))
		(set_local $row2h (get_local $t1))
		(set_local $t0 (get_local $row3l))
		(set_local $row3l (get_local $row3h))
		(set_local $row3h (get_local $t0))
		(set_local $t0 (v8x16.shuffle (24 25 26 27 28 29 30 31 0 1 2 3 4 5 6 7) (get_local $row4l) (get_local $row4h)))
		(set_local $t1 (v8x16.shuffle (24 25 26 27 28 29 30 31 0 1 2 3 4 5 6 7) (get_local $row4h) (get_local $row4l)))
		(set_local $row4l (get_local $t1))
		(set_local $row4h (get_local $t0))
			
		;; Round 7

		(set_local $b0 (v8x16.shuffle (8 9 10 11 12 13 14 15 24 25 26 27 28 29 30 31) (v128.load offset=96 (get_local $blockAddress)) (v128.load offset=48 (get_local $blockAddress))))
		(set_local $b1 (v128.bitselect (v128.load offset=96 (get_local $blockAddress)) (v128.load offset=16 (get_local $blockAddress))  (v128.const 0xff 0xff 0xff 0xff 0xff 0xff 0xff 0xff 0 0 0 0 0 0 0 0)))

		(set_local $row1l (i64x2.add (i64x2.add (get_local $row1l) (get_local $b0)) (get_local $row2l)))
		(set_local $row1h (i64x2.add (i64x2.add (get_local $row1h) (get_local $b1)) (get_local $row2h)))
		(set_local $row4l (v128.xor (get_local $row4l) (get_local $row1l)))
		(set_local $row4h (v128.xor (get_local $row4h) (get_local $row1h)))
		(set_local $row4l (v8x16.shuffle (4 5 6 7 0 1 2 3 12 13 14 15 8 9 10 11) (get_local $row4l) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row4h (v8x16.shuffle (4 5 6 7 0 1 2 3 12 13 14 15 8 9 10 11) (get_local $row4h) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row3l (i64x2.add (get_local $row3l) (get_local $row4l)))
		(set_local $row3h (i64x2.add (get_local $row3h) (get_local $row4h)))
		(set_local $row2l (v128.xor (get_local $row2l) (get_local $row3l)))
		(set_local $row2h (v128.xor (get_local $row2h) (get_local $row3h)))
		(set_local $row2l (v8x16.shuffle (3 4 5 6 7 0 1 2 11 12 13 14 15 8 9 10) (get_local $row2l) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row2h (v8x16.shuffle (3 4 5 6 7 0 1 2 11 12 13 14 15 8 9 10) (get_local $row2h) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))

		(set_local $b0 (v8x16.shuffle (24 25 26 27 28 29 30 31 0 1 2 3 4 5 6 7) (v128.load offset=112 (get_local $blockAddress)) (v128.load offset=80 (get_local $blockAddress))))
		(set_local $b1 (v8x16.shuffle (8 9 10 11 12 13 14 15 24 25 26 27 28 29 30 31) (v128.load offset=0 (get_local $blockAddress)) (v128.load offset=64 (get_local $blockAddress))))

		(set_local $row1l (i64x2.add (i64x2.add (get_local $row1l) (get_local $b0)) (get_local $row2l)))
		(set_local $row1h (i64x2.add (i64x2.add (get_local $row1h) (get_local $b1)) (get_local $row2h)))
		(set_local $row4l (v128.xor (get_local $row4l) (get_local $row1l)))
		(set_local $row4h (v128.xor (get_local $row4h) (get_local $row1h)))
		(set_local $row4l (v8x16.shuffle (2 3 4 5 6 7 0 1 10 11 12 13 14 15 8 9) (get_local $row4l) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row4h (v8x16.shuffle (2 3 4 5 6 7 0 1 10 11 12 13 14 15 8 9) (get_local $row4h) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row3l (i64x2.add (get_local $row3l) (get_local $row4l)))
		(set_local $row3h (i64x2.add (get_local $row3h) (get_local $row4h)))
		(set_local $row2l (v128.xor (get_local $row2l) (get_local $row3l)))
		(set_local $row2h (v128.xor (get_local $row2h) (get_local $row3h)))
		(set_local $row2l (v128.xor (i64x2.shr_u (get_local $row2l) (i64x2.splat (i64.const 63))) (i64x2.add (get_local $row2l) (get_local $row2l))))
		(set_local $row2h (v128.xor (i64x2.shr_u (get_local $row2h) (i64x2.splat (i64.const 63))) (i64x2.add (get_local $row2h) (get_local $row2h))))

		(set_local $t0 (v8x16.shuffle (24 25 26 27 28 29 30 31 0 1 2 3 4 5 6 7) (get_local $row2h) (get_local $row2l)))
		(set_local $t1 (v8x16.shuffle (24 25 26 27 28 29 30 31 0 1 2 3 4 5 6 7) (get_local $row2l) (get_local $row2h)))
		(set_local $row2l (get_local $t0))
		(set_local $row2h (get_local $t1))
		(set_local $t0 (get_local $row3l))
		(set_local $row3l (get_local $row3h))
		(set_local $row3h (get_local $t0))
		(set_local $t0 (v8x16.shuffle (24 25 26 27 28 29 30 31 0 1 2 3 4 5 6 7) (get_local $row4h) (get_local $row4l)))
		(set_local $t1 (v8x16.shuffle (24 25 26 27 28 29 30 31 0 1 2 3 4 5 6 7) (get_local $row4l) (get_local $row4h)))
		(set_local $row4l (get_local $t1))
		(set_local $row4h (get_local $t0))

		(set_local $b0 (v8x16.shuffle (8 9 10 11 12 13 14 15 24 25 26 27 28 29 30 31) (v128.load offset=32 (get_local $blockAddress)) (v128.load offset=112 (get_local $blockAddress))))
		(set_local $b1 (v8x16.shuffle (0 1 2 3 4 5 6 7 16 17 18 19 20 21 22 23) (v128.load offset=64 (get_local $blockAddress)) (v128.load offset=16 (get_local $blockAddress))))

		(set_local $row1l (i64x2.add (i64x2.add (get_local $row1l) (get_local $b0)) (get_local $row2l)))
		(set_local $row1h (i64x2.add (i64x2.add (get_local $row1h) (get_local $b1)) (get_local $row2h)))
		(set_local $row4l (v128.xor (get_local $row4l) (get_local $row1l)))
		(set_local $row4h (v128.xor (get_local $row4h) (get_local $row1h)))
		(set_local $row4l (v8x16.shuffle (4 5 6 7 0 1 2 3 12 13 14 15 8 9 10 11) (get_local $row4l) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row4h (v8x16.shuffle (4 5 6 7 0 1 2 3 12 13 14 15 8 9 10 11) (get_local $row4h) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row3l (i64x2.add (get_local $row3l) (get_local $row4l)))
		(set_local $row3h (i64x2.add (get_local $row3h) (get_local $row4h)))
		(set_local $row2l (v128.xor (get_local $row2l) (get_local $row3l)))
		(set_local $row2h (v128.xor (get_local $row2h) (get_local $row3h)))
		(set_local $row2l (v8x16.shuffle (3 4 5 6 7 0 1 2 11 12 13 14 15 8 9 10) (get_local $row2l) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row2h (v8x16.shuffle (3 4 5 6 7 0 1 2 11 12 13 14 15 8 9 10) (get_local $row2h) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))

		(set_local $b0 (v8x16.shuffle (0 1 2 3 4 5 6 7 16 17 18 19 20 21 22 23) (v128.load offset=0 (get_local $blockAddress)) (v128.load offset=32 (get_local $blockAddress))))
		(set_local $b1 (v8x16.shuffle (0 1 2 3 4 5 6 7 16 17 18 19 20 21 22 23) (v128.load offset=48 (get_local $blockAddress)) (v128.load offset=80 (get_local $blockAddress))))
		
		(set_local $row1l (i64x2.add (i64x2.add (get_local $row1l) (get_local $b0)) (get_local $row2l)))
		(set_local $row1h (i64x2.add (i64x2.add (get_local $row1h) (get_local $b1)) (get_local $row2h)))
		(set_local $row4l (v128.xor (get_local $row4l) (get_local $row1l)))
		(set_local $row4h (v128.xor (get_local $row4h) (get_local $row1h)))
		(set_local $row4l (v8x16.shuffle (2 3 4 5 6 7 0 1 10 11 12 13 14 15 8 9) (get_local $row4l) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row4h (v8x16.shuffle (2 3 4 5 6 7 0 1 10 11 12 13 14 15 8 9) (get_local $row4h) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row3l (i64x2.add (get_local $row3l) (get_local $row4l)))
		(set_local $row3h (i64x2.add (get_local $row3h) (get_local $row4h)))
		(set_local $row2l (v128.xor (get_local $row2l) (get_local $row3l)))
		(set_local $row2h (v128.xor (get_local $row2h) (get_local $row3h)))
		(set_local $row2l (v128.xor (i64x2.shr_u (get_local $row2l) (i64x2.splat (i64.const 63))) (i64x2.add (get_local $row2l) (get_local $row2l))))
		(set_local $row2h (v128.xor (i64x2.shr_u (get_local $row2h) (i64x2.splat (i64.const 63))) (i64x2.add (get_local $row2h) (get_local $row2h))))

		(set_local $t0 (v8x16.shuffle (24 25 26 27 28 29 30 31 0 1 2 3 4 5 6 7) (get_local $row2l) (get_local $row2h)))
		(set_local $t1 (v8x16.shuffle (24 25 26 27 28 29 30 31 0 1 2 3 4 5 6 7) (get_local $row2h) (get_local $row2l)))
		(set_local $row2l (get_local $t0))
		(set_local $row2h (get_local $t1))
		(set_local $t0 (get_local $row3l))
		(set_local $row3l (get_local $row3h))
		(set_local $row3h (get_local $t0))
		(set_local $t0 (v8x16.shuffle (24 25 26 27 28 29 30 31 0 1 2 3 4 5 6 7) (get_local $row4l) (get_local $row4h)))
		(set_local $t1 (v8x16.shuffle (24 25 26 27 28 29 30 31 0 1 2 3 4 5 6 7) (get_local $row4h) (get_local $row4l)))
		(set_local $row4l (get_local $t1))
		(set_local $row4h (get_local $t0))
			
		;; Round 8

		(set_local $b0 (v8x16.shuffle (0 1 2 3 4 5 6 7 16 17 18 19 20 21 22 23) (v128.load offset=48 (get_local $blockAddress)) (v128.load offset=112 (get_local $blockAddress))))
		(set_local $b1 (v8x16.shuffle (24 25 26 27 28 29 30 31 0 1 2 3 4 5 6 7) (v128.load offset=0 (get_local $blockAddress)) (v128.load offset=80 (get_local $blockAddress))))

		(set_local $row1l (i64x2.add (i64x2.add (get_local $row1l) (get_local $b0)) (get_local $row2l)))
		(set_local $row1h (i64x2.add (i64x2.add (get_local $row1h) (get_local $b1)) (get_local $row2h)))
		(set_local $row4l (v128.xor (get_local $row4l) (get_local $row1l)))
		(set_local $row4h (v128.xor (get_local $row4h) (get_local $row1h)))
		(set_local $row4l (v8x16.shuffle (4 5 6 7 0 1 2 3 12 13 14 15 8 9 10 11) (get_local $row4l) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row4h (v8x16.shuffle (4 5 6 7 0 1 2 3 12 13 14 15 8 9 10 11) (get_local $row4h) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row3l (i64x2.add (get_local $row3l) (get_local $row4l)))
		(set_local $row3h (i64x2.add (get_local $row3h) (get_local $row4h)))
		(set_local $row2l (v128.xor (get_local $row2l) (get_local $row3l)))
		(set_local $row2h (v128.xor (get_local $row2h) (get_local $row3h)))
		(set_local $row2l (v8x16.shuffle (3 4 5 6 7 0 1 2 11 12 13 14 15 8 9 10) (get_local $row2l) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row2h (v8x16.shuffle (3 4 5 6 7 0 1 2 11 12 13 14 15 8 9 10) (get_local $row2h) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))

		(set_local $b0 (v8x16.shuffle (8 9 10 11 12 13 14 15 24 25 26 27 28 29 30 31) (v128.load offset=112 (get_local $blockAddress)) (v128.load offset=64 (get_local $blockAddress))))
		(set_local $b1 (v8x16.shuffle (24 25 26 27 28 29 30 31 0 1 2 3 4 5 6 7) (v128.load offset=64 (get_local $blockAddress)) (v128.load offset=16 (get_local $blockAddress))))

		(set_local $row1l (i64x2.add (i64x2.add (get_local $row1l) (get_local $b0)) (get_local $row2l)))
		(set_local $row1h (i64x2.add (i64x2.add (get_local $row1h) (get_local $b1)) (get_local $row2h)))
		(set_local $row4l (v128.xor (get_local $row4l) (get_local $row1l)))
		(set_local $row4h (v128.xor (get_local $row4h) (get_local $row1h)))
		(set_local $row4l (v8x16.shuffle (2 3 4 5 6 7 0 1 10 11 12 13 14 15 8 9) (get_local $row4l) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row4h (v8x16.shuffle (2 3 4 5 6 7 0 1 10 11 12 13 14 15 8 9) (get_local $row4h) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row3l (i64x2.add (get_local $row3l) (get_local $row4l)))
		(set_local $row3h (i64x2.add (get_local $row3h) (get_local $row4h)))
		(set_local $row2l (v128.xor (get_local $row2l) (get_local $row3l)))
		(set_local $row2h (v128.xor (get_local $row2h) (get_local $row3h)))
		(set_local $row2l (v128.xor (i64x2.shr_u (get_local $row2l) (i64x2.splat (i64.const 63))) (i64x2.add (get_local $row2l) (get_local $row2l))))
		(set_local $row2h (v128.xor (i64x2.shr_u (get_local $row2h) (i64x2.splat (i64.const 63))) (i64x2.add (get_local $row2h) (get_local $row2h))))

		(set_local $t0 (v8x16.shuffle (24 25 26 27 28 29 30 31 0 1 2 3 4 5 6 7) (get_local $row2h) (get_local $row2l)))
		(set_local $t1 (v8x16.shuffle (24 25 26 27 28 29 30 31 0 1 2 3 4 5 6 7) (get_local $row2l) (get_local $row2h)))
		(set_local $row2l (get_local $t0))
		(set_local $row2h (get_local $t1))
		(set_local $t0 (get_local $row3l))
		(set_local $row3l (get_local $row3h))
		(set_local $row3h (get_local $t0))
		(set_local $t0 (v8x16.shuffle (24 25 26 27 28 29 30 31 0 1 2 3 4 5 6 7) (get_local $row4h) (get_local $row4l)))
		(set_local $t1 (v8x16.shuffle (24 25 26 27 28 29 30 31 0 1 2 3 4 5 6 7) (get_local $row4l) (get_local $row4h)))
		(set_local $row4l (get_local $t1))
		(set_local $row4h (get_local $t0))

		(set_local $b0 (v128.load offset=96 (get_local $blockAddress)))
		(set_local $b1 (v8x16.shuffle (24 25 26 27 28 29 30 31 0 1 2 3 4 5 6 7) (v128.load offset=80 (get_local $blockAddress)) (v128.load offset=0 (get_local $blockAddress))))

		(set_local $row1l (i64x2.add (i64x2.add (get_local $row1l) (get_local $b0)) (get_local $row2l)))
		(set_local $row1h (i64x2.add (i64x2.add (get_local $row1h) (get_local $b1)) (get_local $row2h)))
		(set_local $row4l (v128.xor (get_local $row4l) (get_local $row1l)))
		(set_local $row4h (v128.xor (get_local $row4h) (get_local $row1h)))
		(set_local $row4l (v8x16.shuffle (4 5 6 7 0 1 2 3 12 13 14 15 8 9 10 11) (get_local $row4l) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row4h (v8x16.shuffle (4 5 6 7 0 1 2 3 12 13 14 15 8 9 10 11) (get_local $row4h) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row3l (i64x2.add (get_local $row3l) (get_local $row4l)))
		(set_local $row3h (i64x2.add (get_local $row3h) (get_local $row4h)))
		(set_local $row2l (v128.xor (get_local $row2l) (get_local $row3l)))
		(set_local $row2h (v128.xor (get_local $row2h) (get_local $row3h)))
		(set_local $row2l (v8x16.shuffle (3 4 5 6 7 0 1 2 11 12 13 14 15 8 9 10) (get_local $row2l) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row2h (v8x16.shuffle (3 4 5 6 7 0 1 2 11 12 13 14 15 8 9 10) (get_local $row2h) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))

		(set_local $b0 (v128.bitselect (v128.load offset=16 (get_local $blockAddress)) (v128.load offset=48 (get_local $blockAddress))  (v128.const 0xff 0xff 0xff 0xff 0xff 0xff 0xff 0xff 0 0 0 0 0 0 0 0)))
		(set_local $b1 (v128.load offset=32 (get_local $blockAddress)))
		
		(set_local $row1l (i64x2.add (i64x2.add (get_local $row1l) (get_local $b0)) (get_local $row2l)))
		(set_local $row1h (i64x2.add (i64x2.add (get_local $row1h) (get_local $b1)) (get_local $row2h)))
		(set_local $row4l (v128.xor (get_local $row4l) (get_local $row1l)))
		(set_local $row4h (v128.xor (get_local $row4h) (get_local $row1h)))
		(set_local $row4l (v8x16.shuffle (2 3 4 5 6 7 0 1 10 11 12 13 14 15 8 9) (get_local $row4l) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row4h (v8x16.shuffle (2 3 4 5 6 7 0 1 10 11 12 13 14 15 8 9) (get_local $row4h) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row3l (i64x2.add (get_local $row3l) (get_local $row4l)))
		(set_local $row3h (i64x2.add (get_local $row3h) (get_local $row4h)))
		(set_local $row2l (v128.xor (get_local $row2l) (get_local $row3l)))
		(set_local $row2h (v128.xor (get_local $row2h) (get_local $row3h)))
		(set_local $row2l (v128.xor (i64x2.shr_u (get_local $row2l) (i64x2.splat (i64.const 63))) (i64x2.add (get_local $row2l) (get_local $row2l))))
		(set_local $row2h (v128.xor (i64x2.shr_u (get_local $row2h) (i64x2.splat (i64.const 63))) (i64x2.add (get_local $row2h) (get_local $row2h))))

		(set_local $t0 (v8x16.shuffle (24 25 26 27 28 29 30 31 0 1 2 3 4 5 6 7) (get_local $row2l) (get_local $row2h)))
		(set_local $t1 (v8x16.shuffle (24 25 26 27 28 29 30 31 0 1 2 3 4 5 6 7) (get_local $row2h) (get_local $row2l)))
		(set_local $row2l (get_local $t0))
		(set_local $row2h (get_local $t1))
		(set_local $t0 (get_local $row3l))
		(set_local $row3l (get_local $row3h))
		(set_local $row3h (get_local $t0))
		(set_local $t0 (v8x16.shuffle (24 25 26 27 28 29 30 31 0 1 2 3 4 5 6 7) (get_local $row4l) (get_local $row4h)))
		(set_local $t1 (v8x16.shuffle (24 25 26 27 28 29 30 31 0 1 2 3 4 5 6 7) (get_local $row4h) (get_local $row4l)))
		(set_local $row4l (get_local $t1))
		(set_local $row4h (get_local $t0))
			
		;; Round 9

		(set_local $b0 (v8x16.shuffle (0 1 2 3 4 5 6 7 16 17 18 19 20 21 22 23) (v128.load offset=80 (get_local $blockAddress)) (v128.load offset=64 (get_local $blockAddress))))
		(set_local $b1 (v8x16.shuffle (8 9 10 11 12 13 14 15 24 25 26 27 28 29 30 31) (v128.load offset=48 (get_local $blockAddress)) (v128.load offset=0 (get_local $blockAddress))))

		(set_local $row1l (i64x2.add (i64x2.add (get_local $row1l) (get_local $b0)) (get_local $row2l)))
		(set_local $row1h (i64x2.add (i64x2.add (get_local $row1h) (get_local $b1)) (get_local $row2h)))
		(set_local $row4l (v128.xor (get_local $row4l) (get_local $row1l)))
		(set_local $row4h (v128.xor (get_local $row4h) (get_local $row1h)))
		(set_local $row4l (v8x16.shuffle (4 5 6 7 0 1 2 3 12 13 14 15 8 9 10 11) (get_local $row4l) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row4h (v8x16.shuffle (4 5 6 7 0 1 2 3 12 13 14 15 8 9 10 11) (get_local $row4h) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row3l (i64x2.add (get_local $row3l) (get_local $row4l)))
		(set_local $row3h (i64x2.add (get_local $row3h) (get_local $row4h)))
		(set_local $row2l (v128.xor (get_local $row2l) (get_local $row3l)))
		(set_local $row2h (v128.xor (get_local $row2h) (get_local $row3h)))
		(set_local $row2l (v8x16.shuffle (3 4 5 6 7 0 1 2 11 12 13 14 15 8 9 10) (get_local $row2l) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row2h (v8x16.shuffle (3 4 5 6 7 0 1 2 11 12 13 14 15 8 9 10) (get_local $row2h) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))

		(set_local $b0 (v8x16.shuffle (0 1 2 3 4 5 6 7 16 17 18 19 20 21 22 23) (v128.load offset=16 (get_local $blockAddress)) (v128.load offset=32 (get_local $blockAddress))))
		(set_local $b1 (v128.bitselect (v128.load offset=48 (get_local $blockAddress)) (v128.load offset=32 (get_local $blockAddress))  (v128.const 0xff 0xff 0xff 0xff 0xff 0xff 0xff 0xff 0 0 0 0 0 0 0 0)))

		(set_local $row1l (i64x2.add (i64x2.add (get_local $row1l) (get_local $b0)) (get_local $row2l)))
		(set_local $row1h (i64x2.add (i64x2.add (get_local $row1h) (get_local $b1)) (get_local $row2h)))
		(set_local $row4l (v128.xor (get_local $row4l) (get_local $row1l)))
		(set_local $row4h (v128.xor (get_local $row4h) (get_local $row1h)))
		(set_local $row4l (v8x16.shuffle (2 3 4 5 6 7 0 1 10 11 12 13 14 15 8 9) (get_local $row4l) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row4h (v8x16.shuffle (2 3 4 5 6 7 0 1 10 11 12 13 14 15 8 9) (get_local $row4h) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row3l (i64x2.add (get_local $row3l) (get_local $row4l)))
		(set_local $row3h (i64x2.add (get_local $row3h) (get_local $row4h)))
		(set_local $row2l (v128.xor (get_local $row2l) (get_local $row3l)))
		(set_local $row2h (v128.xor (get_local $row2h) (get_local $row3h)))
		(set_local $row2l (v128.xor (i64x2.shr_u (get_local $row2l) (i64x2.splat (i64.const 63))) (i64x2.add (get_local $row2l) (get_local $row2l))))
		(set_local $row2h (v128.xor (i64x2.shr_u (get_local $row2h) (i64x2.splat (i64.const 63))) (i64x2.add (get_local $row2h) (get_local $row2h))))

		(set_local $t0 (v8x16.shuffle (24 25 26 27 28 29 30 31 0 1 2 3 4 5 6 7) (get_local $row2h) (get_local $row2l)))
		(set_local $t1 (v8x16.shuffle (24 25 26 27 28 29 30 31 0 1 2 3 4 5 6 7) (get_local $row2l) (get_local $row2h)))
		(set_local $row2l (get_local $t0))
		(set_local $row2h (get_local $t1))
		(set_local $t0 (get_local $row3l))
		(set_local $row3l (get_local $row3h))
		(set_local $row3h (get_local $t0))
		(set_local $t0 (v8x16.shuffle (24 25 26 27 28 29 30 31 0 1 2 3 4 5 6 7) (get_local $row4h) (get_local $row4l)))
		(set_local $t1 (v8x16.shuffle (24 25 26 27 28 29 30 31 0 1 2 3 4 5 6 7) (get_local $row4l) (get_local $row4h)))
		(set_local $row4l (get_local $t1))
		(set_local $row4h (get_local $t0))

		(set_local $b0 (v8x16.shuffle (8 9 10 11 12 13 14 15 24 25 26 27 28 29 30 31) (v128.load offset=112 (get_local $blockAddress)) (v128.load offset=64 (get_local $blockAddress))))
		(set_local $b1 (v8x16.shuffle (8 9 10 11 12 13 14 15 24 25 26 27 28 29 30 31) (v128.load offset=16 (get_local $blockAddress)) (v128.load offset=96 (get_local $blockAddress))))

		(set_local $row1l (i64x2.add (i64x2.add (get_local $row1l) (get_local $b0)) (get_local $row2l)))
		(set_local $row1h (i64x2.add (i64x2.add (get_local $row1h) (get_local $b1)) (get_local $row2h)))
		(set_local $row4l (v128.xor (get_local $row4l) (get_local $row1l)))
		(set_local $row4h (v128.xor (get_local $row4h) (get_local $row1h)))
		(set_local $row4l (v8x16.shuffle (4 5 6 7 0 1 2 3 12 13 14 15 8 9 10 11) (get_local $row4l) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row4h (v8x16.shuffle (4 5 6 7 0 1 2 3 12 13 14 15 8 9 10 11) (get_local $row4h) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row3l (i64x2.add (get_local $row3l) (get_local $row4l)))
		(set_local $row3h (i64x2.add (get_local $row3h) (get_local $row4h)))
		(set_local $row2l (v128.xor (get_local $row2l) (get_local $row3l)))
		(set_local $row2h (v128.xor (get_local $row2h) (get_local $row3h)))
		(set_local $row2l (v8x16.shuffle (3 4 5 6 7 0 1 2 11 12 13 14 15 8 9 10) (get_local $row2l) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row2h (v8x16.shuffle (3 4 5 6 7 0 1 2 11 12 13 14 15 8 9 10) (get_local $row2h) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))

		(set_local $b0 (v8x16.shuffle (24 25 26 27 28 29 30 31 0 1 2 3 4 5 6 7) (v128.load offset=112 (get_local $blockAddress)) (v128.load offset=80 (get_local $blockAddress))))
		(set_local $b1 (v8x16.shuffle (0 1 2 3 4 5 6 7 16 17 18 19 20 21 22 23) (v128.load offset=96 (get_local $blockAddress)) (v128.load offset=0 (get_local $blockAddress))))
		
		(set_local $row1l (i64x2.add (i64x2.add (get_local $row1l) (get_local $b0)) (get_local $row2l)))
		(set_local $row1h (i64x2.add (i64x2.add (get_local $row1h) (get_local $b1)) (get_local $row2h)))
		(set_local $row4l (v128.xor (get_local $row4l) (get_local $row1l)))
		(set_local $row4h (v128.xor (get_local $row4h) (get_local $row1h)))
		(set_local $row4l (v8x16.shuffle (2 3 4 5 6 7 0 1 10 11 12 13 14 15 8 9) (get_local $row4l) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row4h (v8x16.shuffle (2 3 4 5 6 7 0 1 10 11 12 13 14 15 8 9) (get_local $row4h) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row3l (i64x2.add (get_local $row3l) (get_local $row4l)))
		(set_local $row3h (i64x2.add (get_local $row3h) (get_local $row4h)))
		(set_local $row2l (v128.xor (get_local $row2l) (get_local $row3l)))
		(set_local $row2h (v128.xor (get_local $row2h) (get_local $row3h)))
		(set_local $row2l (v128.xor (i64x2.shr_u (get_local $row2l) (i64x2.splat (i64.const 63))) (i64x2.add (get_local $row2l) (get_local $row2l))))
		(set_local $row2h (v128.xor (i64x2.shr_u (get_local $row2h) (i64x2.splat (i64.const 63))) (i64x2.add (get_local $row2h) (get_local $row2h))))

		(set_local $t0 (v8x16.shuffle (24 25 26 27 28 29 30 31 0 1 2 3 4 5 6 7) (get_local $row2l) (get_local $row2h)))
		(set_local $t1 (v8x16.shuffle (24 25 26 27 28 29 30 31 0 1 2 3 4 5 6 7) (get_local $row2h) (get_local $row2l)))
		(set_local $row2l (get_local $t0))
		(set_local $row2h (get_local $t1))
		(set_local $t0 (get_local $row3l))
		(set_local $row3l (get_local $row3h))
		(set_local $row3h (get_local $t0))
		(set_local $t0 (v8x16.shuffle (24 25 26 27 28 29 30 31 0 1 2 3 4 5 6 7) (get_local $row4l) (get_local $row4h)))
		(set_local $t1 (v8x16.shuffle (24 25 26 27 28 29 30 31 0 1 2 3 4 5 6 7) (get_local $row4h) (get_local $row4l)))
		(set_local $row4l (get_local $t1))
		(set_local $row4h (get_local $t0))
			
		;; Round 10

		(set_local $b0 (v8x16.shuffle (0 1 2 3 4 5 6 7 16 17 18 19 20 21 22 23) (v128.load offset=0 (get_local $blockAddress)) (v128.load offset=16 (get_local $blockAddress))))
		(set_local $b1 (v8x16.shuffle (0 1 2 3 4 5 6 7 16 17 18 19 20 21 22 23) (v128.load offset=32 (get_local $blockAddress)) (v128.load offset=48 (get_local $blockAddress))))

		(set_local $row1l (i64x2.add (i64x2.add (get_local $row1l) (get_local $b0)) (get_local $row2l)))
		(set_local $row1h (i64x2.add (i64x2.add (get_local $row1h) (get_local $b1)) (get_local $row2h)))
		(set_local $row4l (v128.xor (get_local $row4l) (get_local $row1l)))
		(set_local $row4h (v128.xor (get_local $row4h) (get_local $row1h)))
		(set_local $row4l (v8x16.shuffle (4 5 6 7 0 1 2 3 12 13 14 15 8 9 10 11) (get_local $row4l) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row4h (v8x16.shuffle (4 5 6 7 0 1 2 3 12 13 14 15 8 9 10 11) (get_local $row4h) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row3l (i64x2.add (get_local $row3l) (get_local $row4l)))
		(set_local $row3h (i64x2.add (get_local $row3h) (get_local $row4h)))
		(set_local $row2l (v128.xor (get_local $row2l) (get_local $row3l)))
		(set_local $row2h (v128.xor (get_local $row2h) (get_local $row3h)))
		(set_local $row2l (v8x16.shuffle (3 4 5 6 7 0 1 2 11 12 13 14 15 8 9 10) (get_local $row2l) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row2h (v8x16.shuffle (3 4 5 6 7 0 1 2 11 12 13 14 15 8 9 10) (get_local $row2h) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))

		(set_local $b0 (v8x16.shuffle (8 9 10 11 12 13 14 15 24 25 26 27 28 29 30 31) (v128.load offset=0 (get_local $blockAddress)) (v128.load offset=16 (get_local $blockAddress))))
		(set_local $b1 (v8x16.shuffle (8 9 10 11 12 13 14 15 24 25 26 27 28 29 30 31) (v128.load offset=32 (get_local $blockAddress)) (v128.load offset=48 (get_local $blockAddress))))

		(set_local $row1l (i64x2.add (i64x2.add (get_local $row1l) (get_local $b0)) (get_local $row2l)))
		(set_local $row1h (i64x2.add (i64x2.add (get_local $row1h) (get_local $b1)) (get_local $row2h)))
		(set_local $row4l (v128.xor (get_local $row4l) (get_local $row1l)))
		(set_local $row4h (v128.xor (get_local $row4h) (get_local $row1h)))
		(set_local $row4l (v8x16.shuffle (2 3 4 5 6 7 0 1 10 11 12 13 14 15 8 9) (get_local $row4l) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row4h (v8x16.shuffle (2 3 4 5 6 7 0 1 10 11 12 13 14 15 8 9) (get_local $row4h) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row3l (i64x2.add (get_local $row3l) (get_local $row4l)))
		(set_local $row3h (i64x2.add (get_local $row3h) (get_local $row4h)))
		(set_local $row2l (v128.xor (get_local $row2l) (get_local $row3l)))
		(set_local $row2h (v128.xor (get_local $row2h) (get_local $row3h)))
		(set_local $row2l (v128.xor (i64x2.shr_u (get_local $row2l) (i64x2.splat (i64.const 63))) (i64x2.add (get_local $row2l) (get_local $row2l))))
		(set_local $row2h (v128.xor (i64x2.shr_u (get_local $row2h) (i64x2.splat (i64.const 63))) (i64x2.add (get_local $row2h) (get_local $row2h))))

		(set_local $t0 (v8x16.shuffle (24 25 26 27 28 29 30 31 0 1 2 3 4 5 6 7) (get_local $row2h) (get_local $row2l)))
		(set_local $t1 (v8x16.shuffle (24 25 26 27 28 29 30 31 0 1 2 3 4 5 6 7) (get_local $row2l) (get_local $row2h)))
		(set_local $row2l (get_local $t0))
		(set_local $row2h (get_local $t1))
		(set_local $t0 (get_local $row3l))
		(set_local $row3l (get_local $row3h))
		(set_local $row3h (get_local $t0))
		(set_local $t0 (v8x16.shuffle (24 25 26 27 28 29 30 31 0 1 2 3 4 5 6 7) (get_local $row4h) (get_local $row4l)))
		(set_local $t1 (v8x16.shuffle (24 25 26 27 28 29 30 31 0 1 2 3 4 5 6 7) (get_local $row4l) (get_local $row4h)))
		(set_local $row4l (get_local $t1))
		(set_local $row4h (get_local $t0))

		(set_local $b0 (v8x16.shuffle (0 1 2 3 4 5 6 7 16 17 18 19 20 21 22 23) (v128.load offset=64 (get_local $blockAddress)) (v128.load offset=80 (get_local $blockAddress))))
		(set_local $b1 (v8x16.shuffle (0 1 2 3 4 5 6 7 16 17 18 19 20 21 22 23) (v128.load offset=96 (get_local $blockAddress)) (v128.load offset=112 (get_local $blockAddress))))

		(set_local $row1l (i64x2.add (i64x2.add (get_local $row1l) (get_local $b0)) (get_local $row2l)))
		(set_local $row1h (i64x2.add (i64x2.add (get_local $row1h) (get_local $b1)) (get_local $row2h)))
		(set_local $row4l (v128.xor (get_local $row4l) (get_local $row1l)))
		(set_local $row4h (v128.xor (get_local $row4h) (get_local $row1h)))
		(set_local $row4l (v8x16.shuffle (4 5 6 7 0 1 2 3 12 13 14 15 8 9 10 11) (get_local $row4l) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row4h (v8x16.shuffle (4 5 6 7 0 1 2 3 12 13 14 15 8 9 10 11) (get_local $row4h) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row3l (i64x2.add (get_local $row3l) (get_local $row4l)))
		(set_local $row3h (i64x2.add (get_local $row3h) (get_local $row4h)))
		(set_local $row2l (v128.xor (get_local $row2l) (get_local $row3l)))
		(set_local $row2h (v128.xor (get_local $row2h) (get_local $row3h)))
		(set_local $row2l (v8x16.shuffle (3 4 5 6 7 0 1 2 11 12 13 14 15 8 9 10) (get_local $row2l) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row2h (v8x16.shuffle (3 4 5 6 7 0 1 2 11 12 13 14 15 8 9 10) (get_local $row2h) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))

		(set_local $b0 (v8x16.shuffle (8 9 10 11 12 13 14 15 24 25 26 27 28 29 30 31) (v128.load offset=64 (get_local $blockAddress)) (v128.load offset=80 (get_local $blockAddress))))
		(set_local $b1 (v8x16.shuffle (8 9 10 11 12 13 14 15 24 25 26 27 28 29 30 31) (v128.load offset=96 (get_local $blockAddress)) (v128.load offset=112 (get_local $blockAddress))))
		
		(set_local $row1l (i64x2.add (i64x2.add (get_local $row1l) (get_local $b0)) (get_local $row2l)))
		(set_local $row1h (i64x2.add (i64x2.add (get_local $row1h) (get_local $b1)) (get_local $row2h)))
		(set_local $row4l (v128.xor (get_local $row4l) (get_local $row1l)))
		(set_local $row4h (v128.xor (get_local $row4h) (get_local $row1h)))
		(set_local $row4l (v8x16.shuffle (2 3 4 5 6 7 0 1 10 11 12 13 14 15 8 9) (get_local $row4l) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row4h (v8x16.shuffle (2 3 4 5 6 7 0 1 10 11 12 13 14 15 8 9) (get_local $row4h) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row3l (i64x2.add (get_local $row3l) (get_local $row4l)))
		(set_local $row3h (i64x2.add (get_local $row3h) (get_local $row4h)))
		(set_local $row2l (v128.xor (get_local $row2l) (get_local $row3l)))
		(set_local $row2h (v128.xor (get_local $row2h) (get_local $row3h)))
		(set_local $row2l (v128.xor (i64x2.shr_u (get_local $row2l) (i64x2.splat (i64.const 63))) (i64x2.add (get_local $row2l) (get_local $row2l))))
		(set_local $row2h (v128.xor (i64x2.shr_u (get_local $row2h) (i64x2.splat (i64.const 63))) (i64x2.add (get_local $row2h) (get_local $row2h))))

		(set_local $t0 (v8x16.shuffle (24 25 26 27 28 29 30 31 0 1 2 3 4 5 6 7) (get_local $row2l) (get_local $row2h)))
		(set_local $t1 (v8x16.shuffle (24 25 26 27 28 29 30 31 0 1 2 3 4 5 6 7) (get_local $row2h) (get_local $row2l)))
		(set_local $row2l (get_local $t0))
		(set_local $row2h (get_local $t1))
		(set_local $t0 (get_local $row3l))
		(set_local $row3l (get_local $row3h))
		(set_local $row3h (get_local $t0))
		(set_local $t0 (v8x16.shuffle (24 25 26 27 28 29 30 31 0 1 2 3 4 5 6 7) (get_local $row4l) (get_local $row4h)))
		(set_local $t1 (v8x16.shuffle (24 25 26 27 28 29 30 31 0 1 2 3 4 5 6 7) (get_local $row4h) (get_local $row4l)))
		(set_local $row4l (get_local $t1))
		(set_local $row4h (get_local $t0))
			
		;; Round 11

		(set_local $b0 (v8x16.shuffle (0 1 2 3 4 5 6 7 16 17 18 19 20 21 22 23) (v128.load offset=112 (get_local $blockAddress)) (v128.load offset=32 (get_local $blockAddress))))
		(set_local $b1 (v8x16.shuffle (8 9 10 11 12 13 14 15 24 25 26 27 28 29 30 31) (v128.load offset=64 (get_local $blockAddress)) (v128.load offset=96 (get_local $blockAddress))))

		(set_local $row1l (i64x2.add (i64x2.add (get_local $row1l) (get_local $b0)) (get_local $row2l)))
		(set_local $row1h (i64x2.add (i64x2.add (get_local $row1h) (get_local $b1)) (get_local $row2h)))
		(set_local $row4l (v128.xor (get_local $row4l) (get_local $row1l)))
		(set_local $row4h (v128.xor (get_local $row4h) (get_local $row1h)))
		(set_local $row4l (v8x16.shuffle (4 5 6 7 0 1 2 3 12 13 14 15 8 9 10 11) (get_local $row4l) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row4h (v8x16.shuffle (4 5 6 7 0 1 2 3 12 13 14 15 8 9 10 11) (get_local $row4h) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row3l (i64x2.add (get_local $row3l) (get_local $row4l)))
		(set_local $row3h (i64x2.add (get_local $row3h) (get_local $row4h)))
		(set_local $row2l (v128.xor (get_local $row2l) (get_local $row3l)))
		(set_local $row2h (v128.xor (get_local $row2h) (get_local $row3h)))
		(set_local $row2l (v8x16.shuffle (3 4 5 6 7 0 1 2 11 12 13 14 15 8 9 10) (get_local $row2l) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row2h (v8x16.shuffle (3 4 5 6 7 0 1 2 11 12 13 14 15 8 9 10) (get_local $row2h) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))

		(set_local $b0 (v8x16.shuffle (0 1 2 3 4 5 6 7 16 17 18 19 20 21 22 23) (v128.load offset=80 (get_local $blockAddress)) (v128.load offset=64 (get_local $blockAddress))))
		(set_local $b1 (v8x16.shuffle (24 25 26 27 28 29 30 31 0 1 2 3 4 5 6 7) (v128.load offset=48 (get_local $blockAddress)) (v128.load offset=112 (get_local $blockAddress))))

		(set_local $row1l (i64x2.add (i64x2.add (get_local $row1l) (get_local $b0)) (get_local $row2l)))
		(set_local $row1h (i64x2.add (i64x2.add (get_local $row1h) (get_local $b1)) (get_local $row2h)))
		(set_local $row4l (v128.xor (get_local $row4l) (get_local $row1l)))
		(set_local $row4h (v128.xor (get_local $row4h) (get_local $row1h)))
		(set_local $row4l (v8x16.shuffle (2 3 4 5 6 7 0 1 10 11 12 13 14 15 8 9) (get_local $row4l) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row4h (v8x16.shuffle (2 3 4 5 6 7 0 1 10 11 12 13 14 15 8 9) (get_local $row4h) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row3l (i64x2.add (get_local $row3l) (get_local $row4l)))
		(set_local $row3h (i64x2.add (get_local $row3h) (get_local $row4h)))
		(set_local $row2l (v128.xor (get_local $row2l) (get_local $row3l)))
		(set_local $row2h (v128.xor (get_local $row2h) (get_local $row3h)))
		(set_local $row2l (v128.xor (i64x2.shr_u (get_local $row2l) (i64x2.splat (i64.const 63))) (i64x2.add (get_local $row2l) (get_local $row2l))))
		(set_local $row2h (v128.xor (i64x2.shr_u (get_local $row2h) (i64x2.splat (i64.const 63))) (i64x2.add (get_local $row2h) (get_local $row2h))))

		(set_local $t0 (v8x16.shuffle (24 25 26 27 28 29 30 31 0 1 2 3 4 5 6 7) (get_local $row2h) (get_local $row2l)))
		(set_local $t1 (v8x16.shuffle (24 25 26 27 28 29 30 31 0 1 2 3 4 5 6 7) (get_local $row2l) (get_local $row2h)))
		(set_local $row2l (get_local $t0))
		(set_local $row2h (get_local $t1))
		(set_local $t0 (get_local $row3l))
		(set_local $row3l (get_local $row3h))
		(set_local $row3h (get_local $t0))
		(set_local $t0 (v8x16.shuffle (24 25 26 27 28 29 30 31 0 1 2 3 4 5 6 7) (get_local $row4h) (get_local $row4l)))
		(set_local $t1 (v8x16.shuffle (24 25 26 27 28 29 30 31 0 1 2 3 4 5 6 7) (get_local $row4l) (get_local $row4h)))
		(set_local $row4l (get_local $t1))
		(set_local $row4h (get_local $t0))

		(set_local $b0 (v8x16.shuffle (8 9 10 11 12 13 14 15 0 1 2 3 4 5 6 7) (v128.load offset=0 (get_local $blockAddress)) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $b1 (v8x16.shuffle (8 9 10 11 12 13 14 15 24 25 26 27 28 29 30 31) (v128.load offset=80 (get_local $blockAddress)) (v128.load offset=32 (get_local $blockAddress))))

		(set_local $row1l (i64x2.add (i64x2.add (get_local $row1l) (get_local $b0)) (get_local $row2l)))
		(set_local $row1h (i64x2.add (i64x2.add (get_local $row1h) (get_local $b1)) (get_local $row2h)))
		(set_local $row4l (v128.xor (get_local $row4l) (get_local $row1l)))
		(set_local $row4h (v128.xor (get_local $row4h) (get_local $row1h)))
		(set_local $row4l (v8x16.shuffle (4 5 6 7 0 1 2 3 12 13 14 15 8 9 10 11) (get_local $row4l) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row4h (v8x16.shuffle (4 5 6 7 0 1 2 3 12 13 14 15 8 9 10 11) (get_local $row4h) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row3l (i64x2.add (get_local $row3l) (get_local $row4l)))
		(set_local $row3h (i64x2.add (get_local $row3h) (get_local $row4h)))
		(set_local $row2l (v128.xor (get_local $row2l) (get_local $row3l)))
		(set_local $row2h (v128.xor (get_local $row2h) (get_local $row3h)))
		(set_local $row2l (v8x16.shuffle (3 4 5 6 7 0 1 2 11 12 13 14 15 8 9 10) (get_local $row2l) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row2h (v8x16.shuffle (3 4 5 6 7 0 1 2 11 12 13 14 15 8 9 10) (get_local $row2h) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))

		(set_local $b0 (v8x16.shuffle (0 1 2 3 4 5 6 7 16 17 18 19 20 21 22 23) (v128.load offset=96 (get_local $blockAddress)) (v128.load offset=16 (get_local $blockAddress))))
		(set_local $b1 (v8x16.shuffle (8 9 10 11 12 13 14 15 24 25 26 27 28 29 30 31) (v128.load offset=48 (get_local $blockAddress)) (v128.load offset=16 (get_local $blockAddress))))

		(set_local $row1l (i64x2.add (i64x2.add (get_local $row1l) (get_local $b0)) (get_local $row2l)))
		(set_local $row1h (i64x2.add (i64x2.add (get_local $row1h) (get_local $b1)) (get_local $row2h)))
		(set_local $row4l (v128.xor (get_local $row4l) (get_local $row1l)))
		(set_local $row4h (v128.xor (get_local $row4h) (get_local $row1h)))
		(set_local $row4l (v8x16.shuffle (2 3 4 5 6 7 0 1 10 11 12 13 14 15 8 9) (get_local $row4l) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row4h (v8x16.shuffle (2 3 4 5 6 7 0 1 10 11 12 13 14 15 8 9) (get_local $row4h) (v128.const 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
		(set_local $row3l (i64x2.add (get_local $row3l) (get_local $row4l)))
		(set_local $row3h (i64x2.add (get_local $row3h) (get_local $row4h)))
		(set_local $row2l (v128.xor (get_local $row2l) (get_local $row3l)))
		(set_local $row2h (v128.xor (get_local $row2h) (get_local $row3h)))
		(set_local $row2l (v128.xor (i64x2.shr_u (get_local $row2l) (i64x2.splat (i64.const 63))) (i64x2.add (get_local $row2l) (get_local $row2l))))
		(set_local $row2h (v128.xor (i64x2.shr_u (get_local $row2h) (i64x2.splat (i64.const 63))) (i64x2.add (get_local $row2h) (get_local $row2h))))

		(set_local $t0 (v8x16.shuffle (24 25 26 27 28 29 30 31 0 1 2 3 4 5 6 7) (get_local $row2l) (get_local $row2h)))
		(set_local $t1 (v8x16.shuffle (24 25 26 27 28 29 30 31 0 1 2 3 4 5 6 7) (get_local $row2h) (get_local $row2l)))
		(set_local $row2l (get_local $t0))
		(set_local $row2h (get_local $t1))
		(set_local $t0 (get_local $row3l))
		(set_local $row3l (get_local $row3h))
		(set_local $row3h (get_local $t0))
		(set_local $t0 (v8x16.shuffle (24 25 26 27 28 29 30 31 0 1 2 3 4 5 6 7) (get_local $row4l) (get_local $row4h)))
		(set_local $t1 (v8x16.shuffle (24 25 26 27 28 29 30 31 0 1 2 3 4 5 6 7) (get_local $row4h) (get_local $row4l)))
		(set_local $row4l (get_local $t1))
		(set_local $row4h (get_local $t0))
			
		;; Update the hash

		(set_local $row1l (v128.xor (get_local $row3l) (get_local $row1l)))
		(set_local $row1h (v128.xor (get_local $row3h) (get_local $row1h)))
		(v128.store offset=0 (get_local $threadStateAddress) (v128.xor (v128.load offset=0 (get_local $threadStateAddress)) (get_local $row1l)))
		(v128.store offset=16 (get_local $threadStateAddress) (v128.xor (v128.load offset=16 (get_local $threadStateAddress)) (get_local $row1h)))
		(set_local $row2l (v128.xor (get_local $row4l) (get_local $row2l)))
		(set_local $row2h (v128.xor (get_local $row4h) (get_local $row2h)))
		(v128.store offset=32 (get_local $threadStateAddress) (v128.xor (v128.load offset=32 (get_local $threadStateAddress)) (get_local $row2l)))
		(v128.store offset=48 (get_local $threadStateAddress) (v128.xor (v128.load offset=48 (get_local $threadStateAddress)) (get_local $row2h)))
	)

	(func $memsetAligned
		(param $address i32)
		(param $value i32)
		(param $numBytes i32)

		(local $value128 v128)
		(local $endAddress i32)
		(local $endAddress128 i32)

		(set_local $value128 (i8x16.splat (get_local $value)))
		(set_local $endAddress (i32.add (get_local $address) (get_local $numBytes)))
		(set_local $endAddress128 (i32.sub (get_local $endAddress) (i32.const 16)))

		block $loop128End
			loop $loop128
				(br_if $loop128End (i32.gt_u (get_local $address) (get_local $endAddress128)))
				(v128.store align=16 (get_local $address) (get_local $value128))
				(set_local $address (i32.add (get_local $address) (i32.const 16)))
				br $loop128
			end
		end

		block $loopEnd
			loop $loop
				(br_if $loopEnd (i32.ge_u (get_local $address) (get_local $endAddress)))
				(i32.store8 (get_local $address) (get_local $value))
				(set_local $address (i32.add (get_local $address) (i32.const 1)))
				br $loop
			end
		end
	)

	(func $blake2b
		(param $threadIndex i32)
		(param $inputAddress i32)
		(param $numBytes i32)

		(local $threadStateAddress i32)
		(local $temp i64)
		
		(set_local $threadStateAddress (i32.add (get_global $statesArrayAddress) (i32.mul (get_local $threadIndex) (i32.const 128))))
		
		;; zero the state
		(call $memsetAligned (get_local $threadStateAddress) (i32.const 0) (i32.const 128))

		;; initialize the hash to the first 64 bytes of the param struct XORed with the contents of IV
		(v128.store offset=0 (get_local $threadStateAddress) (v128.xor
			(v128.load offset=0 (get_global $pAddress))
			(v128.load offset=0 (get_global $ivAddress))))
		(v128.store offset=16 (get_local $threadStateAddress) (v128.xor
			(v128.load offset=16 (get_global $pAddress))
			(v128.load offset=16 (get_global $ivAddress))))
		(v128.store offset=32 (get_local $threadStateAddress) (v128.xor
			(v128.load offset=32 (get_global $pAddress))
			(v128.load offset=32 (get_global $ivAddress))))
		(v128.store offset=48 (get_local $threadStateAddress) (v128.xor
			(v128.load offset=48 (get_global $pAddress))
			(v128.load offset=48 (get_global $ivAddress))))

		block $loopEnd
			loop $loop
				(br_if $loopEnd (i32.lt_u (get_local $numBytes) (i32.const 128)))

				(set_local $temp (i64.add (i64.load offset=64 (get_local $threadStateAddress)) (i64.const 128)))
				(i64.store offset=64 (get_local $threadStateAddress) (get_local $temp))
				(i64.store offset=72 (get_local $threadStateAddress) (i64.add
					(i64.load offset=72 (get_local $threadStateAddress))
					(i64.extend_u/i32 (i64.lt_u (get_local $temp) (i64.const 128)))))

				(if (i32.eq (get_local $numBytes) (i32.const 128))
					(i64.store offset=80 (get_local $threadStateAddress) (i64.const 0xffffffffffffffff)))

				(call $compress (get_local $threadStateAddress) (get_local $inputAddress))

				(set_local $inputAddress (i32.add (get_local $inputAddress) (i32.const 128)))
				(set_local $numBytes (i32.sub (get_local $numBytes) (i32.const 128)))
				br $loop
			end
		end
	)

	(func $threadEntry
		(param $threadIndex i32)
		
		(local $i i32)

		;; Hash the test data enough times to dilute all the non-hash components of the timing.
		(set_local $i (i32.const 0))
		loop $iterLoop
			(call $blake2b (get_local $threadIndex) (get_global $dataAddress) (get_global $dataNumBytes))
			(set_local $i (i32.add (get_local $i) (i32.const 1)))
			(br_if $iterLoop (i32.lt_u (get_local $i) (get_global $numIterationsPerThread)))
		end

		(i32.eq (i32.const 1) (i32.atomic.rmw.sub (get_global $numPendingThreadsAddress) (i32.const 1)))
		if
			(drop (wake (get_global $numPendingThreadsAddress) (i32.const 1)))
		end
	)

	(func $threadError
		(param $threadIndex i32)

		;; Set the thread error flag and wake the main thread.
		(i32.atomic.store (get_global $numPendingThreadsAddress) (i32.const -1))
		(drop (wake (get_global $numPendingThreadsAddress) (i32.const 1)))
	)

	(func $main
		(result i32)
		(local $stdout i32)
		(local $i i32)
		(local $byte i32)		

		;; Initialize the test data.
		(set_global $dataAddress (call $sbrk (get_global $dataNumBytes)))
		(set_local $i (i32.const 0))
		loop $initDataLoop
			(i32.store (i32.add (get_global $dataAddress) (get_local $i)) (get_local $i))
			(set_local $i (i32.add (get_local $i) (i32.const 4)))
			(br_if $initDataLoop (i32.lt_u (get_local $i) (get_global $dataNumBytes)))
		end
		
		;; Launch the threads.
		(i32.atomic.store (get_global $numPendingThreadsAddress) (get_global $numThreads))
		(set_local $i (i32.const 0))
		loop $threadLoop
			(launch_thread (i32.const 0) (get_local $i) (i32.const 1))
			(set_local $i (i32.add (get_local $i) (i32.const 1)))
			(br_if $threadLoop (i32.lt_u (get_local $i) (get_global $numThreads)))
		end

		;; Wait for the threads to finish.
		block $waitLoopEnd
			loop $waitLoop
				(set_local $i (i32.atomic.load (get_global $numPendingThreadsAddress)))
				(br_if $waitLoopEnd (i32.le_s (get_local $i) (i32.const 0)))
				(drop (i32.wait (get_global $numPendingThreadsAddress) (get_local $i) (f64.const +inf)))
				(br $waitLoop)
			end
		end

		(i32.eq (i32.atomic.load (get_global $numPendingThreadsAddress)) (i32.const 0))
		if
			;; Create a hexadecimal string from the hash.
			(set_local $i (i32.const 0))
			loop $loop
				(set_local $byte (i32.load8_u (i32.add (get_global $statesArrayAddress) (get_local $i))))
				(i32.store8 offset=0
					(i32.add (get_global $outputStringAddress) (i32.shl (get_local $i) (i32.const 1)))
					(i32.load8_u (i32.add (get_global $hexitTable) (i32.and (get_local $byte) (i32.const 0x0f)))))
				(i32.store8 offset=1
					(i32.add (get_global $outputStringAddress) (i32.shl (get_local $i) (i32.const 1)))
					(i32.load8_u (i32.add (get_global $hexitTable) (i32.shr_u (get_local $byte) (i32.const 4)))))
				(set_local $i (i32.add (get_local $i) (i32.const 1)))
				(br_if $loop (i32.lt_u (get_local $i) (i32.const 64)))
			end
			(i32.store8 offset=128 (get_global $outputStringAddress) (i32.const 10))

			;; Print the string to the output.
			(set_local $stdout (i32.load align=4 (get_global $stdoutPtr)))
			(return (call $__fwrite (get_global $outputStringAddress) (i32.const 1) (i32.const 129) (get_local $stdout)))
		end

		(return (i32.const 1))
	)

	(func (export "establishStackSpace") (param i32 i32) (nop))
)