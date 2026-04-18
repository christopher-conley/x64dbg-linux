# issue3803

Regression test for x64dbg issue #3803.

This test covers two things:

- `_K0=5` no longer crashes the debugger when the assignment goes through the scalar expression path.
- `_ZMM0=5` still writes whole-register data correctly by materializing a temporary buffer and then verifying the lower 256 bits through `vmovdqu`.

The ZMM verification works even on hosts without AVX-512 because TitanEngine falls back to AVX state for `GetAVX512Context`/`SetAVX512Context`.
