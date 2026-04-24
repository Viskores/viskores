# Remaining Python Test Porting Plan

This note tracks the gap between the current Python wrapper tests and the
non-internal C++ test surface that is relevant for the public Python bindings.
It is scoped to single-node tests for now. MPI cases stay out of scope until
the single-node surface is in better shape.

## Current Coverage

Implemented Python wrapper tests currently cover:

- core class surface
  - `viskores::Range`
  - `viskores::cont::Field`
  - `viskores::cont::CoordinateSystem`
  - `viskores::cont::UnknownCellSet` exposed as `viskores.cont.CellSet`
  - `viskores::cont::ColorTable`
  - `viskores::rendering::Camera`
- `source/testing`
  - `UnitTestTangleSource.cxx`
  - `UnitTestOscillatorSource.cxx`
  - `UnitTestWaveletSource.cxx`
  - `RenderTestPerlinNoise.cxx`
- `io/testing`
  - `UnitTestBOVDataSetReader.cxx`
  - `UnitTestHDF5Image.cxx`
  - `UnitTestImageWriter.cxx`
  - `UnitTestVTKDataSetReader.cxx`
  - `UnitTestVTKDataSetWriter.cxx`
  - `UnitTestVisItFileDataSetReader.cxx`
- `rendering/testing`
  - `RenderTestAlternateCoordinates.cxx`
  - `UnitTestCanvas.cxx`
  - `UnitTestMapperConnectivity.cxx`
  - `UnitTestMapperCylinders.cxx`
  - `UnitTestMapperGlyphScalar.cxx`
  - `UnitTestMapperGlyphVector.cxx`
  - `UnitTestMapperPoints.cxx`
  - `UnitTestMapperQuads.cxx`
  - `UnitTestMapperRayTracer.cxx`
  - `UnitTestMapperVolume.cxx`
  - `UnitTestMapperWireframer.cxx`
  - `UnitTestScalarRenderer.cxx`
  - plus `Demo.py` and `GameOfLife.py`
- `filter/field_conversion/testing`
  - `UnitTestCellAverageFilter.cxx`
  - `UnitTestPointAverageFilter.cxx`
- `filter/field_transform/testing`
  - `UnitTestCompositeVectors.cxx`
  - `UnitTestCoordinateSystemTransform.cxx`
  - `UnitTestFieldToColors.cxx`
  - `UnitTestGenerateIds.cxx`
  - `UnitTestLogValues.cxx`
  - `UnitTestPointElevationFilter.cxx`
  - `UnitTestPointTransform.cxx`
  - `UnitTestWarpFilter.cxx`
- `filter/vector_analysis/testing`
  - `UnitTestCrossProductFilter.cxx`
  - `UnitTestDotProductFilter.cxx`
  - `UnitTestGradientExplicit.cxx`
  - `UnitTestGradientUniform.cxx`
  - `UnitTestSurfaceNormalsFilter.cxx`
  - `UnitTestVectorMagnitudeFilter.cxx`
  - `RenderTestSurfaceNormals.cxx`
- `filter/contour/testing`
  - `UnitTestContourFilter.cxx`
  - `UnitTestContourFilterNormals.cxx`
  - `UnitTestClipWithFieldFilter.cxx`
  - `UnitTestClipWithImplicitFunctionFilter.cxx`
  - `UnitTestMIRFilter.cxx`
  - `UnitTestSliceMultipleFilter.cxx`
  - `RenderTestContourFilter.cxx`
  - `RenderTestSliceFilter.cxx`
- `filter/scalar_topology/testing`
  - `UnitTestContourTreeUniformFilter.cxx`
  - `UnitTestContourTreeUniformAugmentedFilter.cxx`
  - `UnitTestContourTreeUniformDistributedFilter.cxx`
  - `UnitTestDistributedBranchDecompositionFilter.cxx`
  - the contour-tree / distributed / branch decomposition slice through
    `unit_test_scalar_topology_filters.py`
- `filter/entity_extraction/testing`
  - `UnitTestExternalFacesFilter.cxx`
  - `UnitTestExtractGeometryFilter.cxx`
  - `UnitTestExtractPointsFilter.cxx`
  - `UnitTestExtractStructuredFilter.cxx`
  - `UnitTestGhostCellRemove.cxx`
  - `UnitTestMaskFilter.cxx`
  - `UnitTestMaskPointsFilter.cxx`
  - `UnitTestThresholdFilter.cxx`
  - `UnitTestThresholdPointsFilter.cxx`
- `filter/clean_grid/testing`
  - `UnitTestCleanGrid.cxx`
- `filter/mesh_info/testing`
  - `UnitTestGhostCellClassify.cxx`
  - `UnitTestCellMeasuresFilter.cxx`
  - `UnitTestMeshQualityFilter.cxx`
- `filter/connected_components/testing`
  - `UnitTestCellSetConnectivityFilter.cxx`
  - `UnitTestImageConnectivityFilter.cxx`
- `filter/multi_block/testing`
  - `UnitTestMergeDataSetsFilter.cxx`
- `filter/resampling/testing`
  - `UnitTestHistSampling.cxx`
  - `UnitTestProbe.cxx`
- `filter/density_estimate/testing`
  - `UnitTestContinuousScatterPlot.cxx`
  - `UnitTestEntropyFilter.cxx`
  - `UnitTestHistogramFilter.cxx`
  - `UnitTestNDEntropyFilter.cxx`
  - `UnitTestNDHistogramFilter.cxx`
  - `UnitTestParticleDensity.cxx`
  - `UnitTestPartitionedDataSetHistogramFilter.cxx`
  - `UnitTestStatisticsFilter.cxx`
- `filter/geometry_refinement/testing`
  - `UnitTestConvertToPointCloud.cxx`
  - `UnitTestShrinkFilter.cxx`
  - `UnitTestSplitSharpEdgesFilter.cxx`
  - `UnitTestTetrahedralizeFilter.cxx`
  - `UnitTestTubeFilter.cxx`
  - `UnitTestTriangulateFilter.cxx`
  - `UnitTestVertexClusteringFilter.cxx`
  - `RenderTestSplitSharpEdges.cxx`
- `filter/image_processing/testing`
  - `RenderTestComputeMoments.cxx`
  - `UnitTestImageDifferenceFilter.cxx`
  - `UnitTestImageMedianFilter.cxx`

## Remaining Gaps By Module

### `source/testing`

Missing C++ tests:

- none in the current single-node user-facing slice

Notes:

- `Tangle`, `Oscillator`, `Wavelet`, and the Perlin-noise render path are all
  now covered.

### `io/testing`

Missing C++ tests:

- none in the current IO reader/writer slice

Notes:

- `UnitTestFileUtils.cxx` is intentionally skipped for the Python bindings
  because it exercises low-level file utility helpers rather than core
  user-facing Viskores classes.
- `UnitTestPixelTypes.cxx` is intentionally skipped for the Python bindings
  because it targets low-level pixel-type utility coverage rather than the
  public image reader/writer API.
- `ImageReaderPNG`, `ImageReaderPNM`, `ImageReaderHDF5`, `ImageWriterPNG`,
  `ImageWriterPNM`, `ImageWriterHDF5`, and `ImageWriterBase::PixelDepth` are
  now exposed and covered.

### `rendering/testing`

Missing C++ tests:

- `UnitTestMultiMapper.cxx`

Likely missing classes:

- low-level mapper `RenderCells(...)` entry points

Notes:

- `MapperCylinders`, `MapperGlyphScalar`, `MapperGlyphVector`, `MapperVolume`,
  and `ScalarRenderer` are already exposed and covered.
- `CoordinateSystem`, `ColorTable`, `Camera`, `Field`, and `CellSet` are now
  exposed directly in Python.
- `RenderTestAlternateCoordinates.cxx` is covered through the exposed
  array-handle slice:
  - `UnknownArrayHandle`
  - `ArrayHandleSOAVec3f`
  - `ArrayHandleRecombineVecFloatDefault`
  - `ArrayCopy(...)`
- `UnitTestMultiMapper.cxx` remains deferred pending the design decision on
  whether Python should expose the low-level mapper `RenderCells(...)` surface.

### `scalar_topology/testing`

Missing C++ tests:

- worklet-specific tests such as
  `UnitTestContourTreeUniformAugmentedWorklet.cxx`
- MPI-specific tests such as
  `UnitTestContourTreeUniformDistributedFilterMPI.cxx`
- presimplification-focused cases embedded in
  `TestingContourTreeUniformDistributedFilter.h`

Notes:

- direct Python ports now cover:
  - `UnitTestContourTreeUniformFilter.cxx`
  - `UnitTestContourTreeUniformAugmentedFilter.cxx`
  - `UnitTestContourTreeUniformDistributedFilter.cxx`
  - `UnitTestDistributedBranchDecompositionFilter.cxx`
- worklet-specific scalar-topology tests are intentionally skipped because they
  cover internal implementation details rather than the public Python API.
- MPI/distributed-across-ranks scalar-topology tests are intentionally deferred
  for now; only the single-node out-of-core/distributed filter path is covered.
- a small `viskores.testing` helper layer is used to compile distributed
  contour-tree outputs for verification without exposing additional internal
  worklet classes as public bindings.

### `filter/field_conversion/testing`

Missing C++ tests:

- `UnitTestPointAverageCellSetExtrude.cxx`

Notes:

- this looks like a dataset-construction gap rather than a missing public
  filter class.

### `filter/connected_components/testing`

Missing C++ tests:

- `UnitTestGraphConnectivityWorklet.cxx`

Notes:

- `CellSetConnectivity` and `ImageConnectivity` are exposed and covered.
- `GraphConnectivityWorklet` appears too internal for the public Python
  surface and should stay skipped unless the public API grows around it.

### `filter/multi_block/testing`

Missing C++ tests:

- `RenderTestAmrArrays.cxx`

Likely missing classes:

- AMR-specific arrays and rendering helpers used by `RenderTestAmrArrays.cxx`

Notes:

- `MergeDataSets` is exposed and covered.
- the remaining gap here looks AMR/rendering-specific rather than like another
  ordinary filter binding.

### `filter/uncertainty/testing`

Missing C++ tests:

- `UnitTestContourUncertainUniform.cxx`
- `UnitTestFiberUncertainUniform.cxx`

Likely missing classes:

- uncertainty filters used by those tests

### `filter/flow/testing`

Missing single-node C++ tests:

- `RenderTestStreamline.cxx`
- `UnitTestLagrangianFilter.cxx`
- `UnitTestLagrangianStructuresFilter.cxx`
- `UnitTestStreamSurfaceFilter.cxx`
- `UnitTestStreamSurfaceWorklet.cxx`
- `UnitTestStreamlineFilter.cxx`
- `UnitTestStreamlineFilterWarpX.cxx`
- `UnitTestWorkletParticleAdvection.cxx`
- `UnitTestWorkletTemporalAdvection.cxx`

Out of scope for now:

- `UnitTestAdvectionMPI.cxx`
- `UnitTestPathlineMPI.cxx`
- `UnitTestStreamlineAMRMPI.cxx`
- `UnitTestStreamlineMPI.cxx`

Likely missing classes:

- `Streamline`
- `StreamSurface`
- `Lagrangian`
- `LagrangianStructures`
- possibly helper/advection classes depending on how public the C++ surface is

## Recommended Next Steps

1. Revisit the remaining rendering policy decision

- decide whether the low-level mapper `RenderCells(...)` surface needed by
  `UnitTestMultiMapper.cxx` should be exposed in Python

2. Continue with the next ordinary unported family

- `filter/uncertainty/testing`

3. Then move to the broader flow family

- `filter/flow/testing`

4. Revisit the deferred scalar-topology MPI slice only after the remaining
   ordinary single-node filter coverage is in better shape

## Triage Rules For Remaining C++ Tests

Use these rules before porting a test one-for-one:

- skip MPI tests for now
- skip worklet-only or clearly internal tests unless they correspond to a real
  public Python API goal
- prefer tests that exercise direct public Viskores classes over tests that are
  mostly about hidden helper utilities
- prefer one Python test per public class/filter first, then add broader render
  tests
