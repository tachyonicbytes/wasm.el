;;; wasm3-test.el --- wasm3 tests -*- lexical-binding: t; -*-

(require 'ert)
(require 'wasm)

(ert-deftest wasm3--sanity-test ()
  (should (eq (wasm-test "wasm3/test/lang/fib.c.wasm" "fib") 5)))
