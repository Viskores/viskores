## `cppyy` Investigation Notes

Date: April 22, 2026

Scope tested:
- `viskores::cont::DataSetBuilderUniform`
- `viskores::cont::DataSet`
- `viskores::filter::vector_analysis::VectorMagnitude`
- minimal result-field inspection

### Summary

`cppyy` remains conceptually interesting for low-maintenance header-driven bindings, but
the current evaluation on this macOS Apple Silicon machine is blocked by Cling/rootcling
toolchain issues before we can fairly judge the Viskores API fit.

The packaged `cppyy` stack was not usable. A source-built `cppyy-cling` stack got much
farther, but still failed in the final `rootcling_stage1` dictionary-generation phase.

### Findings

1. Packaged `cppyy` was broken before any Viskores-specific test.

- The installed backend referenced a nonexistent runtime dependency:
  - `/opt/local/lib/libzstd.1.dylib`
- After patching that dependency to Homebrew `zstd`, `import cppyy` still failed with
  Cling/macOS SDK/libc++ startup errors.

2. Source-built `cppyy-cling` is materially healthier than the packaged backend.

- Building `cppyy-cling` from source in `/tmp` succeeded through the heavy LLVM/Cling
  compile and linked `libCling.so` successfully.
- This shows the earlier packaged-backend failure was not just "cppyy cannot work here."

3. The remaining blocker is `rootcling_stage1`, not Viskores.

- The final failure occurs in the dictionary-generation step, even for a tiny standalone
  test header unrelated to Viskores.
- A minimal direct `rootcling_stage1` test reproduces the same problem without any Viskores
  headers involved.

4. The failure class is Apple Silicon/macOS SDK/Cling header setup.

Observed symptoms include:
- `assert.h` not found in the default setup
- libc++ parse failures such as `_Tp/_Up does not refer to a value`
- `Unsupported architecture`
- duplicate SDK typedefs after forcing additional SDK paths

5. Homebrew `cling` was also broken and removed.

- `/opt/homebrew/bin/cling` reported a missing resource directory in a temporary build path:
  - `resource directory .../cling-1.3/build/lib/clang/20 not found`
- That package was uninstalled to avoid confusion.
- The source-built `cppyy-cling` tests did not rely on the Homebrew `cling` binary.

### Online signals

The failure pattern is consistent with known upstream pain points:

- `cppyy` issue `#150`: Apple Silicon startup/header failures
  - https://github.com/wlav/cppyy/issues/150
- ROOT issue `#18306`: recent macOS `rootcling_stage1` breakage
  - https://github.com/root-project/root/issues/18306

These are not exact duplicates of the final local failure, but they strongly suggest this
is part of a real upstream issue class rather than a Viskores-specific problem.

### Recommendation

For Viskores Python bindings, do not continue a deep `cppyy` rescue effort right now unless
runtime/reflection-based bindings are strategically important enough to justify Cling/rootcling
debugging on Apple Silicon.

Practical direction:
- Treat `cppyy` as promising in principle but currently blocked on this platform/toolchain.
- Prefer continuing the Binder evaluation using curated adapter headers for the actual
  Python-facing Viskores slice.

### If revisiting `cppyy`

The next debugging step would be to compare `rootcling_stage1` predefined target/compiler
macros against system clang for the same `arm64 + isysroot` setup and determine which
builtin macros are missing or inconsistent at startup.
