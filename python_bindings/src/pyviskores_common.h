//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

#ifndef VISKORES_PYTHON_ENABLE_SOURCE
#define VISKORES_PYTHON_ENABLE_SOURCE 0
#endif
#ifndef VISKORES_PYTHON_ENABLE_FILTER_CORE
#define VISKORES_PYTHON_ENABLE_FILTER_CORE 0
#endif
#ifndef VISKORES_PYTHON_ENABLE_FILTER_CONTOUR
#define VISKORES_PYTHON_ENABLE_FILTER_CONTOUR 0
#endif
#ifndef VISKORES_PYTHON_ENABLE_FILTER_FIELD_CONVERSION
#define VISKORES_PYTHON_ENABLE_FILTER_FIELD_CONVERSION 0
#endif
#ifndef VISKORES_PYTHON_ENABLE_FILTER_FIELD_TRANSFORM
#define VISKORES_PYTHON_ENABLE_FILTER_FIELD_TRANSFORM 0
#endif
#ifndef VISKORES_PYTHON_ENABLE_FILTER_CLEAN_GRID
#define VISKORES_PYTHON_ENABLE_FILTER_CLEAN_GRID 0
#endif
#ifndef VISKORES_PYTHON_ENABLE_FILTER_SCALAR_TOPOLOGY
#define VISKORES_PYTHON_ENABLE_FILTER_SCALAR_TOPOLOGY 0
#endif
#ifndef VISKORES_PYTHON_ENABLE_FILTER_VECTOR_ANALYSIS
#define VISKORES_PYTHON_ENABLE_FILTER_VECTOR_ANALYSIS 0
#endif
#ifndef VISKORES_PYTHON_ENABLE_FILTER_CONNECTED_COMPONENTS
#define VISKORES_PYTHON_ENABLE_FILTER_CONNECTED_COMPONENTS 0
#endif
#ifndef VISKORES_PYTHON_ENABLE_FILTER_ENTITY_EXTRACTION
#define VISKORES_PYTHON_ENABLE_FILTER_ENTITY_EXTRACTION 0
#endif
#ifndef VISKORES_PYTHON_ENABLE_FILTER_MESH_INFO
#define VISKORES_PYTHON_ENABLE_FILTER_MESH_INFO 0
#endif
#ifndef VISKORES_PYTHON_ENABLE_FILTER_MULTI_BLOCK
#define VISKORES_PYTHON_ENABLE_FILTER_MULTI_BLOCK 0
#endif
#ifndef VISKORES_PYTHON_ENABLE_FILTER_GEOMETRY_REFINEMENT
#define VISKORES_PYTHON_ENABLE_FILTER_GEOMETRY_REFINEMENT 0
#endif
#ifndef VISKORES_PYTHON_ENABLE_FILTER_RESAMPLING
#define VISKORES_PYTHON_ENABLE_FILTER_RESAMPLING 0
#endif
#ifndef VISKORES_PYTHON_ENABLE_IO
#define VISKORES_PYTHON_ENABLE_IO 0
#endif
#ifndef VISKORES_PYTHON_ENABLE_IO_HDF5
#define VISKORES_PYTHON_ENABLE_IO_HDF5 0
#endif
#ifndef VISKORES_PYTHON_ENABLE_INTEROP
#define VISKORES_PYTHON_ENABLE_INTEROP 0
#endif
#ifndef VISKORES_PYTHON_ENABLE_RENDERING
#define VISKORES_PYTHON_ENABLE_RENDERING 0
#endif
#ifndef VISKORES_PYTHON_ENABLE_TESTING_UTILS
#define VISKORES_PYTHON_ENABLE_TESTING_UTILS 0
#endif

#include <viskores/CellClassification.h>
#include <viskores/ImplicitFunction.h>
#include <viskores/Range.h>
#include <viskores/Types.h>
#include <viskores/cont/ArrayHandleBasic.h>
#include <viskores/cont/CastAndCall.h>
#include <viskores/cont/CellSetExplicit.h>
#include <viskores/cont/CellSetStructured.h>
#include <viskores/cont/DataSet.h>
#include <viskores/cont/DataSetBuilderExplicit.h>
#include <viskores/cont/DataSetBuilderRectilinear.h>
#include <viskores/cont/DataSetBuilderUniform.h>
#include <viskores/cont/Initialize.h>
#include <viskores/cont/PartitionedDataSet.h>
#if VISKORES_PYTHON_ENABLE_TESTING_UTILS
#include <viskores/cont/testing/MakeTestDataSet.h>
#endif
#if VISKORES_PYTHON_ENABLE_RENDERING
#include <viskores/cont/ColorTable.h>
#endif
#if VISKORES_PYTHON_ENABLE_FILTER_CORE
#include <viskores/filter/MapFieldPermutation.h>
#endif
#if VISKORES_PYTHON_ENABLE_FILTER_CONTOUR
#include <viskores/filter/contour/ClipWithField.h>
#include <viskores/filter/contour/ClipWithImplicitFunction.h>
#include <viskores/filter/contour/Contour.h>
#include <viskores/filter/contour/ContourMarchingCells.h>
#include <viskores/filter/contour/MIRFilter.h>
#include <viskores/filter/contour/Slice.h>
#include <viskores/filter/contour/SliceMultiple.h>
#endif
#if VISKORES_PYTHON_ENABLE_FILTER_FIELD_CONVERSION
#include <viskores/filter/field_conversion/CellAverage.h>
#include <viskores/filter/field_conversion/PointAverage.h>
#endif
#if VISKORES_PYTHON_ENABLE_FILTER_FIELD_TRANSFORM
#include <viskores/filter/field_transform/CompositeVectors.h>
#include <viskores/filter/field_transform/CylindricalCoordinateTransform.h>
#include <viskores/filter/field_transform/FieldToColors.h>
#include <viskores/filter/field_transform/GenerateIds.h>
#include <viskores/filter/field_transform/LogValues.h>
#include <viskores/filter/field_transform/PointElevation.h>
#include <viskores/filter/field_transform/PointTransform.h>
#include <viskores/filter/field_transform/SphericalCoordinateTransform.h>
#include <viskores/filter/field_transform/Warp.h>
#endif
#if VISKORES_PYTHON_ENABLE_FILTER_DENSITY_ESTIMATE
#include <viskores/filter/density_estimate/ContinuousScatterPlot.h>
#include <viskores/filter/density_estimate/Entropy.h>
#include <viskores/filter/density_estimate/Histogram.h>
#include <viskores/filter/density_estimate/NDEntropy.h>
#include <viskores/filter/density_estimate/NDHistogram.h>
#include <viskores/filter/density_estimate/ParticleDensityCloudInCell.h>
#include <viskores/filter/density_estimate/ParticleDensityNearestGridPoint.h>
#include <viskores/filter/density_estimate/Statistics.h>
#endif
#if VISKORES_PYTHON_ENABLE_FILTER_IMAGE_PROCESSING
#include <viskores/filter/image_processing/ComputeMoments.h>
#include <viskores/filter/image_processing/ImageDifference.h>
#include <viskores/filter/image_processing/ImageMedian.h>
#endif
#if VISKORES_PYTHON_ENABLE_FILTER_CLEAN_GRID
#include <viskores/filter/clean_grid/CleanGrid.h>
#endif
#if VISKORES_PYTHON_ENABLE_FILTER_CONNECTED_COMPONENTS
#include <viskores/filter/connected_components/CellSetConnectivity.h>
#include <viskores/filter/connected_components/ImageConnectivity.h>
#endif
#if VISKORES_PYTHON_ENABLE_FILTER_ENTITY_EXTRACTION
#include <viskores/filter/entity_extraction/ExternalFaces.h>
#include <viskores/filter/entity_extraction/ExtractGeometry.h>
#include <viskores/filter/entity_extraction/ExtractPoints.h>
#include <viskores/filter/entity_extraction/ExtractStructured.h>
#include <viskores/filter/entity_extraction/GhostCellRemove.h>
#include <viskores/filter/entity_extraction/Mask.h>
#include <viskores/filter/entity_extraction/MaskPoints.h>
#include <viskores/filter/entity_extraction/Threshold.h>
#include <viskores/filter/entity_extraction/ThresholdPoints.h>
#endif
#if VISKORES_PYTHON_ENABLE_FILTER_MESH_INFO
#include <viskores/filter/mesh_info/CellMeasures.h>
#include <viskores/filter/mesh_info/GhostCellClassify.h>
#include <viskores/filter/mesh_info/MeshQuality.h>
#endif
#if VISKORES_PYTHON_ENABLE_FILTER_GEOMETRY_REFINEMENT
#include <viskores/filter/geometry_refinement/ConvertToPointCloud.h>
#include <viskores/filter/geometry_refinement/Shrink.h>
#include <viskores/filter/geometry_refinement/SplitSharpEdges.h>
#include <viskores/filter/geometry_refinement/Tetrahedralize.h>
#include <viskores/filter/geometry_refinement/Triangulate.h>
#include <viskores/filter/geometry_refinement/Tube.h>
#include <viskores/filter/geometry_refinement/VertexClustering.h>
#endif
#if VISKORES_PYTHON_ENABLE_FILTER_RESAMPLING
#include <viskores/filter/resampling/HistSampling.h>
#include <viskores/filter/resampling/Probe.h>
#endif
#if VISKORES_PYTHON_ENABLE_FILTER_MULTI_BLOCK
#include <viskores/filter/multi_block/MergeDataSets.h>
#endif
#if VISKORES_PYTHON_ENABLE_FILTER_SCALAR_TOPOLOGY
#include <viskores/filter/scalar_topology/ContourTreeUniform.h>
#include <viskores/filter/scalar_topology/ContourTreeUniformAugmented.h>
#include <viskores/filter/scalar_topology/ContourTreeUniformDistributed.h>
#include <viskores/filter/scalar_topology/DistributedBranchDecompositionFilter.h>
#include <viskores/filter/scalar_topology/ExtractTopVolumeContoursFilter.h>
#include <viskores/filter/scalar_topology/SelectTopVolumeBranchesFilter.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/ProcessContourTree.h>
#endif
#if VISKORES_PYTHON_ENABLE_FILTER_VECTOR_ANALYSIS
#include <viskores/filter/vector_analysis/CrossProduct.h>
#include <viskores/filter/vector_analysis/DotProduct.h>
#include <viskores/filter/vector_analysis/Gradient.h>
#include <viskores/filter/vector_analysis/SurfaceNormals.h>
#include <viskores/filter/vector_analysis/VectorMagnitude.h>
#endif
#if VISKORES_PYTHON_ENABLE_IO
#include <viskores/io/BOVDataSetReader.h>
#if VISKORES_PYTHON_ENABLE_IO_HDF5
#include <viskores/io/ImageReaderHDF5.h>
#include <viskores/io/ImageWriterHDF5.h>
#endif
#include <viskores/io/ImageReaderPNG.h>
#include <viskores/io/ImageReaderPNM.h>
#include <viskores/io/ImageWriterPNG.h>
#include <viskores/io/ImageWriterPNM.h>
#include <viskores/io/VTKDataSetReader.h>
#include <viskores/io/VTKDataSetWriter.h>
#include <viskores/io/VTKVisItFileReader.h>
#endif
#if VISKORES_PYTHON_ENABLE_INTEROP
#include <viskores/interop/BufferState.h>
#include <viskores/interop/TransferToOpenGL.h>
#endif
#if VISKORES_PYTHON_ENABLE_RENDERING
#include <viskores/rendering/Actor.h>
#include <viskores/rendering/Camera.h>
#include <viskores/rendering/Canvas.h>
#include <viskores/rendering/CanvasRayTracer.h>
#include <viskores/rendering/GlyphType.h>
#include <viskores/rendering/MapperConnectivity.h>
#include <viskores/rendering/MapperCylinder.h>
#include <viskores/rendering/MapperGlyphScalar.h>
#include <viskores/rendering/MapperGlyphVector.h>
#include <viskores/rendering/MapperPoint.h>
#include <viskores/rendering/MapperQuad.h>
#include <viskores/rendering/MapperRayTracer.h>
#include <viskores/rendering/MapperVolume.h>
#include <viskores/rendering/MapperWireframer.h>
#include <viskores/rendering/ScalarRenderer.h>
#include <viskores/rendering/Scene.h>
#include <viskores/rendering/View3D.h>
#endif
#if VISKORES_PYTHON_ENABLE_SOURCE
#include <viskores/source/Oscillator.h>
#include <viskores/source/PerlinNoise.h>
#include <viskores/source/Tangle.h>
#include <viskores/source/Wavelet.h>
#endif

#include <cstdint>
#include <cstring>
#include <memory>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

#if defined(__clang__)
#define VISKORES_PYTHON_CLANG_DIAGNOSTIC_PUSH _Pragma("clang diagnostic push")
#define VISKORES_PYTHON_CLANG_DIAGNOSTIC_POP _Pragma("clang diagnostic pop")
#define VISKORES_PYTHON_IGNORE_MISSING_FIELD_INITIALIZERS \
  _Pragma("clang diagnostic ignored \"-Wmissing-field-initializers\"")
#else
#define VISKORES_PYTHON_CLANG_DIAGNOSTIC_PUSH
#define VISKORES_PYTHON_CLANG_DIAGNOSTIC_POP
#define VISKORES_PYTHON_IGNORE_MISSING_FIELD_INITIALIZERS
#endif
