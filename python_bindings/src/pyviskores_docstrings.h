//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

#ifndef PYVISKORES_DOCSTRINGS_H
#define PYVISKORES_DOCSTRINGS_H

#include <string_view>

namespace viskores::python::bindings
{
namespace doc
{

inline bool StartsWith(std::string_view text, std::string_view prefix)
{
  return text.size() >= prefix.size() && text.substr(0, prefix.size()) == prefix;
}

inline bool EndsWith(std::string_view text, std::string_view suffix)
{
  return text.size() >= suffix.size() &&
    text.substr(text.size() - suffix.size(), suffix.size()) == suffix;
}

inline const char* ClassDoc(std::string_view name)
{
  if (name == "UnknownArrayHandle")
  {
    return R"doc(Type-erased Viskores array handle.

UnknownArrayHandle stores an array without fixing the value or storage type in
the Python signature. It is commonly used for fields and coordinate-system data
that may contain any supported Viskores scalar or vector value type.)doc";
  }
  if (name == "ArrayHandleSOAVec3f" || StartsWith(name, "ArrayHandleSOA"))
  {
    return R"doc(Viskores ArrayHandleSOA wrapper.

ArrayHandleSOA stores vector components in separate arrays. The Python binding
exposes NumPy interoperation helpers for supported scalar and vector value
types.)doc";
  }
  if (StartsWith(name, "ArrayHandleRecombineVec"))
  {
    return R"doc(Viskores ArrayHandleRecombineVec wrapper.

This array handle recombines separate component arrays into vector values for
use by Viskores algorithms while retaining Python/NumPy interoperation.)doc";
  }
  if (name == "CellSet")
  {
    return R"doc(Type-erased Viskores cell set.

A cell set describes the topology of a data set: the cells, points, cell
shapes, and point ids that connect cells to their incident points.)doc";
  }
  if (name == "Field")
  {
    return R"doc(Named data array associated with a Viskores data set.

A field has a name, an association such as points or cells, and an array of
values. Fields are the primary way scalar, vector, and other data values are
attached to a DataSet.)doc";
  }
  if (name == "CoordinateSystem")
  {
    return R"doc(Field subclass used as a Viskores coordinate system.

A coordinate system stores point coordinates either as explicit coordinate
arrays or as generated structured coordinates derived from dimensions, origin,
and spacing.)doc";
  }
  if (name == "DataSet")
  {
    return R"doc(Viskores data-set container.

A DataSet holds the cell-set topology, coordinate systems, and associated
fields that Viskores filters and worklets operate on.)doc";
  }
  if (name == "DataSetBuilderExplicit")
  {
    return R"doc(Builder for explicit-cell Viskores data sets.

Explicit data sets describe each cell by a shape id, component count, and
connectivity into a coordinate array.)doc";
  }
  if (name == "DataSetBuilderCurvilinear")
  {
    return R"doc(Builder for curvilinear structured Viskores data sets.

Curvilinear data sets have structured topology and explicit point coordinates.)doc";
  }
  if (name == "DataSetBuilderUniform")
  {
    return R"doc(Builder for uniform structured Viskores data sets.

Uniform data sets use point dimensions plus optional origin and spacing to
define a regular coordinate system.)doc";
  }
  if (name == "DataSetBuilderRectilinear")
  {
    return R"doc(Builder for rectilinear structured Viskores data sets.

Rectilinear data sets use independent coordinate arrays for each logical axis.)doc";
  }
  if (name == "DataSetBuilderExplicitIterative")
  {
    return R"doc(Iterative builder for explicit-cell Viskores data sets.

Use Begin, AddPoint, AddCell, and Create to construct an explicit mesh one
point and cell at a time.)doc";
  }
  if (name == "PartitionedDataSet")
  {
    return R"doc(Collection of Viskores DataSet partitions.

PartitionedDataSet stores multiple DataSet objects for multi-block or
distributed-style processing.)doc";
  }
  if (name == "DeviceAdapterId")
  {
    return R"doc(Identifier for a Viskores device adapter.

DeviceAdapterId names or numbers the runtime backend used by Viskores, such as
Serial, OpenMP, CUDA, or Any when those adapters are available.)doc";
  }
  if (name == "InitializeResult")
  {
    return R"doc(Result returned by viskores.cont.Initialize.

The result reports the selected DeviceAdapterId and any usage/help text
generated while processing initialization options.)doc";
  }
  if (name == "Range")
  {
    return R"doc(Numeric interval with minimum and maximum values.

Range is used for scalar field ranges and supports containment, length, center,
union, and intersection operations.)doc";
  }
  if (name == "Box")
  {
    return R"doc(Axis-aligned box implicit function.

The box is defined by minimum and maximum points and can be used by extraction
and clipping filters.)doc";
  }
  if (name == "Sphere")
  {
    return R"doc(Spherical implicit function.

The sphere is defined by a center point and radius and can be used by
extraction and clipping filters.)doc";
  }
  if (name == "Cylinder")
  {
    return R"doc(Cylindrical implicit function.

The cylinder is defined by an axis and radius, with an optional center point in
the Python binding.)doc";
  }
  if (name == "Plane")
  {
    return R"doc(Planar implicit function.

The plane is defined by an origin point and normal vector and can be used by
extraction and clipping filters.)doc";
  }
  if (name == "MakeTestDataSet")
  {
    return R"doc(Factory for Viskores test data sets.

This class exposes the Viskores testing data-set generators for unit tests,
examples, and smoke tests.)doc";
  }
  if (name == "BufferState")
  {
    return R"doc(OpenGL buffer state used by Viskores interop helpers.

BufferState stores the buffer handle, buffer type, size, and capacity used when
transferring Viskores data to OpenGL.)doc";
  }
  if (name == "ColorTable")
  {
    return R"doc(Viskores color table.

ColorTable maps scalar values to colors and opacity for rendering and field
color conversion.)doc";
  }
  if (name == "Camera")
  {
    return R"doc(Viskores rendering camera.

Camera stores view position, look-at point, view-up vector, clipping range,
projection settings, and related rendering controls.)doc";
  }
  if (name == "Actor")
  {
    return R"doc(Renderable data-set actor.

An Actor combines a DataSet, coordinate system, field, and color table for use
in a Viskores rendering Scene.)doc";
  }
  if (name == "Scene")
  {
    return R"doc(Collection of rendering actors.

Scene stores the actors that are rendered together by a Viskores View.)doc";
  }
  if (name == "Canvas" || name == "CanvasRayTracer")
  {
    return R"doc(Rendering canvas.

A canvas owns the color and depth buffers used by Viskores rendering. Concrete
canvas subclasses provide specific rendering backends.)doc";
  }
  if (StartsWith(name, "Mapper"))
  {
    return R"doc(Viskores rendering mapper.

A mapper converts data-set geometry and fields into rendered primitives for a
Viskores scene.)doc";
  }
  if (name == "ScalarRenderer" || name == "ScalarRendererResult")
  {
    return R"doc(Utility for rendering scalar fields.

ScalarRenderer renders a scalar field from a DataSet and reports output image
information through ScalarRendererResult.)doc";
  }
  if (name == "View3D")
  {
    return R"doc(Three-dimensional Viskores rendering view.

View3D combines a Scene, Mapper, Canvas, Camera, and background color to render
3D data.)doc";
  }
  if (name == "Tangle")
  {
    return R"doc(Source that generates a structured tangle data set.

The tangle source creates a uniform grid with a synthetic scalar field useful
for examples and tests.)doc";
  }
  if (name == "PerlinNoise")
  {
    return R"doc(Source that generates a structured Perlin-noise data set.

The source creates a uniform grid and a synthetic noise field controlled by
dimensions, origin, and seed.)doc";
  }
  if (name == "Oscillator")
  {
    return R"doc(Source that generates an oscillator field on a structured grid.

Oscillator combines periodic, damped, and decaying wave contributions over
time.)doc";
  }
  if (name == "Wavelet")
  {
    return R"doc(Source that generates the Viskores wavelet data set.

The source creates a structured grid containing the commonly used wavelet
scalar field.)doc";
  }
  if (EndsWith(name, "Reader") || StartsWith(name, "ImageReader") || name == "BOVDataSetReader" ||
      name == "VTKVisItFileReader")
  {
    return R"doc(Viskores data-set reader.

Reader classes load supported Viskores data-set or partitioned-data-set file
formats from disk.)doc";
  }
  if (EndsWith(name, "Writer") || StartsWith(name, "ImageWriter"))
  {
    return R"doc(Viskores data-set writer.

Writer classes save Viskores data sets to supported file formats.)doc";
  }
  if (name == "ContourTreeMesh2D" || name == "ContourTreeMesh3D")
  {
    return R"doc(Contour-tree filter for structured meshes.

This filter computes contour-tree data for 2D or 3D mesh inputs using the
selected active scalar field.)doc";
  }
  if (name == "ContourTreeAugmented")
  {
    return R"doc(Augmented contour-tree filter.

This filter computes augmented contour-tree structures and related arrays from
a scalar field on a Viskores data set.)doc";
  }
  if (name == "ContourTreeUniformDistributed")
  {
    return R"doc(Distributed contour-tree filter for uniform grids.

This filter computes contour-tree data over a PartitionedDataSet representing a
partitioned uniform grid.)doc";
  }
  if (name == "DistributedBranchDecompositionFilter" || name == "SelectTopVolumeBranchesFilter" ||
      name == "ExtractTopVolumeContoursFilter")
  {
    return R"doc(Scalar-topology post-processing filter.

This filter operates on contour-tree branch decomposition or selected contour
volume data produced by the scalar-topology workflow.)doc";
  }
  if (name == "Contour")
  {
    return R"doc(Generate isosurfaces from a scalar field.

Contour extracts surfaces at one or more scalar values from a Viskores data
set.)doc";
  }
  if (name == "ContourMarchingCells")
  {
    return R"doc(Generate contours using the marching-cells algorithm.

This filter extracts contour geometry from a selected scalar field.)doc";
  }
  if (name == "ClipWithImplicitFunction")
  {
    return R"doc(Clip a data set with an implicit function.

The filter removes or keeps regions of the input according to the selected
implicit function and invert setting.)doc";
  }
  if (name == "ClipWithField")
  {
    return R"doc(Clip a data set using scalar field values.

The filter removes or keeps regions according to a selected field and clip
value.)doc";
  }
  if (name == "Slice" || name == "SliceMultiple")
  {
    return R"doc(Slice a data set with one or more implicit functions.

The filter extracts the intersection of the input cells with the selected
implicit function surfaces.)doc";
  }
  if (name == "MIRFilter")
  {
    return R"doc(Material interface reconstruction filter.

MIRFilter reconstructs material interfaces from volume-fraction-style input
fields.)doc";
  }
  if (name == "CellAverage")
  {
    return R"doc(Map point fields to cell fields by averaging incident point values.)doc";
  }
  if (name == "PointAverage")
  {
    return R"doc(Map cell fields to point fields by averaging incident cell values.)doc";
  }
  if (name == "VectorMagnitude")
  {
    return R"doc(Compute magnitudes of vector-valued field values.)doc";
  }
  if (name == "Gradient")
  {
    return R"doc(Compute gradients and optional divergence, vorticity, and Q-criterion fields.)doc";
  }
  if (name == "CrossProduct")
  {
    return R"doc(Compute the cross product of two vector fields.)doc";
  }
  if (name == "DotProduct")
  {
    return R"doc(Compute the dot product of two vector fields.)doc";
  }
  if (name == "SurfaceNormals")
  {
    return R"doc(Generate or orient normals for surface geometry.)doc";
  }
  if (name == "Threshold" || name == "ThresholdPoints")
  {
    return R"doc(Extract cells or points whose field values satisfy threshold criteria.)doc";
  }
  if (name == "Mask" || name == "MaskPoints")
  {
    return R"doc(Select a subset of cells or points from a data set using stride or mask settings.)doc";
  }
  if (name == "ExternalFaces")
  {
    return R"doc(Extract external faces from a data set.)doc";
  }
  if (name == "ExtractStructured")
  {
    return R"doc(Extract a structured extent or sample from a structured data set.)doc";
  }
  if (name == "ExtractPoints")
  {
    return R"doc(Extract points selected by an implicit function.)doc";
  }
  if (name == "ExtractGeometry")
  {
    return R"doc(Extract cells selected by an implicit function.)doc";
  }
  if (name == "GhostCellRemove")
  {
    return R"doc(Remove cells marked as ghost cells from a data set.)doc";
  }
  if (name == "CleanGrid")
  {
    return R"doc(Clean unused points, duplicate points, or degenerate cells from a data set.)doc";
  }
  if (name == "GhostCellClassify")
  {
    return R"doc(Classify ghost cells in a structured data set.)doc";
  }
  if (name == "CellMeasures")
  {
    return R"doc(Compute measures such as length, area, or volume for cells.)doc";
  }
  if (name == "MeshQuality")
  {
    return R"doc(Compute mesh-quality metrics for cells.)doc";
  }
  if (name == "CellSetConnectivity" || name == "ImageConnectivity")
  {
    return R"doc(Find connected components in cell or image data.)doc";
  }
  if (name == "MergeDataSets")
  {
    return R"doc(Merge partitions or data sets into a combined data set.)doc";
  }
  if (name == "Tetrahedralize")
  {
    return R"doc(Decompose cells into tetrahedra.)doc";
  }
  if (name == "Triangulate")
  {
    return R"doc(Decompose cells into triangles.)doc";
  }
  if (name == "Shrink")
  {
    return R"doc(Shrink each input cell toward its center.)doc";
  }
  if (name == "ConvertToPointCloud")
  {
    return R"doc(Convert input cells to point-cloud geometry.)doc";
  }
  if (name == "VertexClustering")
  {
    return R"doc(Simplify geometry using vertex clustering.)doc";
  }
  if (name == "SplitSharpEdges")
  {
    return R"doc(Split vertices along feature edges sharper than a configured angle.)doc";
  }
  if (name == "Tube")
  {
    return R"doc(Generate tube geometry around input polylines.)doc";
  }
  if (name == "Probe")
  {
    return R"doc(Sample fields from one data set onto the geometry of another.)doc";
  }
  if (name == "HistSampling")
  {
    return R"doc(Sample data using histogram-driven resampling.)doc";
  }
  if (name == "GenerateIds")
  {
    return R"doc(Generate point or cell id fields.)doc";
  }
  if (name == "CompositeVectors")
  {
    return R"doc(Combine scalar component fields into vector-valued fields.)doc";
  }
  if (name == "LogValues")
  {
    return R"doc(Apply logarithmic transformation to field values.)doc";
  }
  if (name == "PointElevation")
  {
    return R"doc(Generate scalar values from point positions along a line.)doc";
  }
  if (name == "PointTransform")
  {
    return R"doc(Apply a transform to point coordinates.)doc";
  }
  if (name == "CylindricalCoordinateTransform" || name == "SphericalCoordinateTransform")
  {
    return R"doc(Transform point coordinates between Cartesian and curvilinear coordinate systems.)doc";
  }
  if (name == "FieldToColors")
  {
    return R"doc(Map scalar field values to color values using a color table.)doc";
  }
  if (name == "Warp")
  {
    return R"doc(Displace point coordinates using a vector field and scale factor.)doc";
  }
  if (name == "ComputeMoments")
  {
    return R"doc(Compute image moments for selected field data.)doc";
  }
  if (name == "ImageDifference")
  {
    return R"doc(Compute per-pixel image differences between two fields.)doc";
  }
  if (name == "ImageMedian")
  {
    return R"doc(Apply a median filter to image data.)doc";
  }
  if (name == "ContinuousScatterPlot")
  {
    return R"doc(Compute a continuous scatter plot from two scalar fields.)doc";
  }
  if (name == "Entropy" || name == "NDEntropy")
  {
    return R"doc(Compute entropy from scalar or multi-dimensional field distributions.)doc";
  }
  if (name == "Histogram" || name == "NDHistogram")
  {
    return R"doc(Compute scalar or multi-dimensional histograms from field data.)doc";
  }
  if (name == "Statistics")
  {
    return R"doc(Compute descriptive statistics for selected field data.)doc";
  }
  if (name == "ParticleDensityNearestGridPoint" || name == "ParticleDensityCloudInCell")
  {
    return R"doc(Estimate particle density on a structured grid.)doc";
  }

  return R"doc(Viskores Python binding.

This object wraps a Viskores C++ class for use from Python.)doc";
}

inline constexpr const char* Initialize =
  R"doc(Initialize the Viskores runtime and select a device adapter.

Call this once early in a process or notebook kernel when you need to choose a
specific backend. The argv overload accepts Viskores command-line options and
updates the list in place with options consumed by Viskores removed.)doc";

inline constexpr const char* IsInitialized =
  R"doc(Return True if the Viskores runtime has already been initialized.)doc";

inline constexpr const char* MakeDeviceAdapterId =
  R"doc(Create a DeviceAdapterId from a device adapter name or numeric id.)doc";

inline constexpr const char* ArrayFromNumPy =
  R"doc(Create a Viskores UnknownArrayHandle from a NumPy-compatible array.

When copy is False and the input layout is compatible, the Viskores array can
share ownership with the Python array object.)doc";

inline constexpr const char* AsNumPy =
  R"doc(Return a NumPy view or copy for a Viskores array-like object.

The object may be an UnknownArrayHandle, a supported concrete Viskores array, a
Field, or any object accepted by numpy.asarray.)doc";

inline constexpr const char* ArrayCopy =
  R"doc(Copy values between a Viskores array-like source and a Python destination.)doc";

inline constexpr const char* MakeField =
  R"doc(Create a Viskores Field from a name, association, and Python array-like values.)doc";

inline constexpr const char* MakeFieldPoint =
  R"doc(Create a point-associated Viskores Field from Python array-like values.)doc";

inline constexpr const char* MakeFieldCell =
  R"doc(Create a cell-associated Viskores Field from Python array-like values.)doc";

inline constexpr const char* MakeFieldWholeDataSet =
  R"doc(Create a whole-data-set-associated Viskores Field from Python array-like values.)doc";

inline constexpr const char* CreateUniformDataSet =
  R"doc(Create a uniform structured DataSet from dimensions, origin, and spacing.)doc";

inline constexpr const char* FieldRange =
  R"doc(Return the scalar range of a named field in a DataSet as (min, max).)doc";

inline constexpr const char* CreateTangleDataSet =
  R"doc(Create a synthetic Tangle source DataSet, optionally with custom point dimensions.)doc";

inline constexpr const char* PartitionUniformDataSet =
  R"doc(Partition a uniform DataSet for scalar-topology distributed examples and tests.)doc";

inline constexpr const char* ExecuteFilter =
  R"doc(Execute the filter on a DataSet or PartitionedDataSet and return the matching result type.)doc";

inline constexpr const char* ExecuteSource = R"doc(Generate and return the source DataSet.)doc";

inline constexpr const char* SetActiveField =
  R"doc(Select the active input field by name and optional association.)doc";

inline constexpr const char* SetFieldsToPass =
  R"doc(Select which input fields are passed through to the output.)doc";

inline constexpr const char* CompatibilityCellAverage =
  R"doc(Run CellAverage on a DataSet or PartitionedDataSet and return the result.)doc";

inline constexpr const char* CompatibilityPointAverage =
  R"doc(Run PointAverage on a DataSet or PartitionedDataSet and return the result.)doc";

inline constexpr const char* CompatibilityVectorMagnitude =
  R"doc(Run VectorMagnitude on a DataSet or PartitionedDataSet and return the result.)doc";

inline constexpr const char* CompatibilityGradient =
  R"doc(Run Gradient on a DataSet or PartitionedDataSet with common Python keyword options.)doc";

inline constexpr const char* CompatibilityContour =
  R"doc(Run Contour on a DataSet or PartitionedDataSet for one or more isovalues.)doc";

inline constexpr const char* TransferToOpenGL =
  R"doc(Transfer Viskores data into an OpenGL buffer state for interoperation.)doc";

inline constexpr const char* MakeGhostCellDataSet =
  R"doc(Create a DataSet with ghost-cell data for tests and examples.)doc";

inline constexpr const char* CompileDistributedContourTreeSuperarcs =
  R"doc(Compile superarcs from distributed contour-tree partitions for testing.)doc";

inline constexpr const char* CanonicalizeDistributedBranchDecomposition =
  R"doc(Return a canonical text representation of distributed branch decomposition results.)doc";

inline constexpr const char* CanonicalizeDistributedAugmentedTreeVolumes =
  R"doc(Return canonical augmented-tree volume text for distributed contour-tree tests.)doc";

} // namespace doc
} // namespace viskores::python::bindings

#endif
