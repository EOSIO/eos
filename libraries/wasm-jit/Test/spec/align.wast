(assert_malformed
  (module quote
    "(module (memory 0) (func (drop (i64.load align=0 (i32.const 0)))))"
  )
  "alignment"
)
(assert_malformed
  (module quote
    "(module (memory 0) (func (drop (i64.load align=7 (i32.const 0)))))"
  )
  "alignment"
)
(assert_invalid
  (module (memory 0) (func (drop (i64.load align=16 (i32.const 0)))))
  "alignment"
)

(assert_malformed
  (module quote
    "(module (memory 0) (func (i64.store align=0 (i32.const 0) (i64.const 0))))"
  )
  "alignment"
)
(assert_malformed
  (module quote
    "(module (memory 0) (func (i64.store align=5 (i32.const 0) (i64.const 0))))"
  )
  "alignment"
)
(assert_invalid
  (module (memory 0) (func (i64.store align=16 (i32.const 0) (i64.const 0))))
  "alignment"
)
