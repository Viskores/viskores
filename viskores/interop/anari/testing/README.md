<!--
============================================================================
  The contents of this file are covered by the Viskores license. See
  LICENSE.txt for details.

  By contributing to this file, all contributors agree to the Developer
  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
============================================================================
-->

# ANARI test coverage

The tests in this directory are divided into two CTest categories:

- `ANARI_INTEROP` exercises `viskores::interop::anari` through mapper and
  scene interfaces. Data-inspection tests are the primary contract coverage;
  they verify ANARI object subtypes, parameter names and types, array sizes and
  values, ownership, rejection of invalid input, and materialized updates.
- `ANARI_DEVICE` constructs raw ANARI objects and tests the bundled rendering
  device. These tests do not exercise mapper lowering.

`UnitTestANARIExternalDevice` also has the `ANARI_EXTERNAL` label. Set
`VISKORES_TEST_ANARI_LIBRARY` to an installed library name to run the external
device smoke test. It cleanly skips when the variable is absent or names the
bundled `viskores` device, and fails when an explicitly requested library
cannot be loaded or queried.

## Image baseline triage

The image mismatches present before this test-suite update were investigated
before accepting new baselines:

| Image | Classification | Evidence |
| --- | --- | --- |
| `points.png` | Intentional mapper change; old baseline was stale | The point shape is unchanged. The mapper now uses the supplied blue-green-red color map instead of a hard-coded red transfer function. `UnitTestANARIMapperPointsData` independently verifies sphere geometry, direct positions, radius, attributes, and array sizes. |
| `isosurface.png` | Intentional mapper change; old baseline was stale | Indexed triangles preserve the old surface shape while the configured color map and field values now affect color. `UnitTestANARIMapperTrianglesData` verifies triangle subtype, indices, point/cell associations, values, and shared array sizes. |
| `volume.png` | Intentional mapper change; old baseline was stale | The mapper now uses truthful uniform origin/spacing and the configured transfer function and opacity. Structured and unstructured contract tests verify the spatial-field representation independently of pixels. |
| `scene.png` | Backend-specific and not accepted | The bundled device does not advertise cone geometry, so the complete volume/triangle/glyph scene cannot run there. The image test skips next to an explanation; glyph conversion remains covered by `UnitTestANARIMapperGlyphsData`. |
| `quad.png` | Bundled-device change; old baseline was stale | This raw-device test does not use an interop mapper, and its source was unchanged by the mapper remediation. Five consecutive renders were byte-identical before the current baseline was accepted. |

## Lifetime and performance evidence

The bundled device reports live ANARI object counts during device destruction.
The CTest definitions treat any nonzero leak report as a failure. This gives
the focused suite deterministic ANARI-handle leak coverage in ordinary builds,
while the same ownership and repeated-update tests also run in Viskores'
standard sanitizer jobs. A local instrumented run can be configured with
`Viskores_ENABLE_SANITIZER=ON` and selected with `ctest -L ANARI_INTEROP`.

The representative point and triangle data tests publish the following CDash
measurements without imposing a timing threshold:

- `ANARI.PointLoweringMilliseconds`, `ANARI.PointArrayBytes`, and
  `ANARI.PointArrayCount` for 65,536 points.
- `ANARI.TriangleLoweringMilliseconds`, `ANARI.TriangleArrayBytes`, and
  `ANARI.TriangleArrayCount` for a 257 by 257 point grid.

The stable allocation contracts are one 786,432-byte point array and three
triangle arrays totaling 2,629,648 bytes. Timings are evidence only until
enough dashboard history exists to choose a stable regression threshold.
