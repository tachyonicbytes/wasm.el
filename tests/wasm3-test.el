;;; wasm3-test.el --- wasm3 tests -*- lexical-binding: t; -*-

(require 'ert)
(require 'wasm3)

(ert-deftest wasm3--sanity-test ()
  (should (eq (wasm3-test) 5)))
