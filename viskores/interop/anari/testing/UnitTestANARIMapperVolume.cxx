//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================


// viskores::anari
#include <viskores/interop/anari/ANARIMapperVolume.h>
#include <viskores/rendering/anari-device/ViskoresDevice.h>
// viskores
#include <viskores/CellShape.h>
#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/ArrayHandleUniformPointCoordinates.h>
#include <viskores/cont/CellSetExplicit.h>
#include <viskores/cont/CellSetSingleType.h>
#include <viskores/cont/CellSetStructured.h>
#include <viskores/cont/DataSetBuilderRectilinear.h>
#include <viskores/cont/DataSetBuilderUniform.h>
#include <viskores/cont/testing/Testing.h>
#include <viskores/filter/contour/Contour.h>
#include <viskores/io/EncodePNG.h>
#include <viskores/source/Tangle.h>
// std
#include <cstdlib>
#include <cstring>
#include <limits>
#include <string>
#include <vector>

#include "ANARITestCommon.h"

namespace
{

viskores::cont::DataSet MakeSingleCellDataSet(viskores::UInt8 shape,
                                              const std::vector<viskores::Vec3f_32>& points)
{
  const auto numPoints = static_cast<viskores::IdComponent>(points.size());

  std::vector<viskores::Id> connectivity(points.size());
  std::vector<viskores::Float32> values(points.size());
  for (viskores::IdComponent pointIndex = 0; pointIndex < numPoints; ++pointIndex)
  {
    connectivity[static_cast<std::size_t>(pointIndex)] = pointIndex;
    values[static_cast<std::size_t>(pointIndex)] = static_cast<viskores::Float32>(pointIndex);
  }

  viskores::cont::CellSetSingleType<> cells;
  cells.Fill(numPoints,
             shape,
             numPoints,
             viskores::cont::make_ArrayHandle(connectivity, viskores::CopyFlag::On));

  viskores::cont::DataSet dataSet;
  dataSet.SetCellSet(cells);
  dataSet.AddCoordinateSystem(viskores::cont::CoordinateSystem(
    "coords", viskores::cont::make_ArrayHandle(points, viskores::CopyFlag::On)));
  dataSet.AddPointField("values", values);
  return dataSet;
}

void CheckSentCellType(anari_cpp::Device d,
                       const viskores::cont::DataSet& dataSet,
                       viskores::UInt8 expectedCellType)
{
  viskores::interop::anari::ANARIActor actor(
    dataSet.GetCellSet(), dataSet.GetCoordinateSystem(), dataSet.GetField("values"));
  viskores::interop::anari::ANARIMapperVolume mVol(d, actor);
  mVol.GetANARISpatialField();

  auto cellTypeArray = mVol.GetANARIUnstructuredParameters().CellType;
  VISKORES_TEST_ASSERT(cellTypeArray != nullptr, "No cell.type array was sent to ANARI");

  const auto* cellTypes = anari_cpp::map<viskores::UInt8>(d, cellTypeArray);
  const viskores::UInt8 sentCellType = cellTypes[0];
  anari_cpp::unmap(d, cellTypeArray);

  VISKORES_TEST_ASSERT(sentCellType == expectedCellType,
                       "Wrong cell.type sent to ANARI: expected ",
                       static_cast<int>(expectedCellType),
                       ", got ",
                       static_cast<int>(sentCellType));
}

void UnstructuredCellTypeTests()
{
  auto d = loadANARIDevice();

  // ANARI KHR_SPATIAL_FIELD_UNSTRUCTURED cell.type: hexahedron = 12, pyramid = 14
  auto hexahedron = MakeSingleCellDataSet(viskores::CELL_SHAPE_HEXAHEDRON,
                                          { { 0, 0, 0 },
                                            { 1, 0, 0 },
                                            { 1, 1, 0 },
                                            { 0, 1, 0 },
                                            { 0, 0, 1 },
                                            { 1, 0, 1 },
                                            { 1, 1, 1 },
                                            { 0, 1, 1 } });
  CheckSentCellType(d, hexahedron, 12);

  auto pyramid = MakeSingleCellDataSet(
    viskores::CELL_SHAPE_PYRAMID,
    { { 0, 0, 0 }, { 1, 0, 0 }, { 1, 1, 0 }, { 0, 1, 0 }, { 0.5f, 0.5f, 1 } });
  CheckSentCellType(d, pyramid, 14);

  anari_cpp::release(d, d);
}

viskores::cont::DataSet MakeTwoTetrahedraDataSet()
{
  const std::vector<viskores::Vec3f_32> points = {
    { 0, 0, 0 }, { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 }, { 1, 1, 1 }
  };
  const std::vector<viskores::UInt8> shapes = { viskores::CELL_SHAPE_TETRA,
                                                viskores::CELL_SHAPE_TETRA };
  const std::vector<viskores::Id> connectivity = { 0, 1, 2, 3, 1, 2, 3, 4 };
  const std::vector<viskores::Id> offsets = { 0, 4, 8 };
  const std::vector<viskores::Float32> values = { 0, 1, 2, 3, 4 };

  viskores::cont::CellSetExplicit<> cells;
  cells.Fill(static_cast<viskores::Id>(points.size()),
             viskores::cont::make_ArrayHandle(shapes, viskores::CopyFlag::On),
             viskores::cont::make_ArrayHandle(connectivity, viskores::CopyFlag::On),
             viskores::cont::make_ArrayHandle(offsets, viskores::CopyFlag::On));

  viskores::cont::DataSet dataSet;
  dataSet.SetCellSet(cells);
  dataSet.AddCoordinateSystem(viskores::cont::CoordinateSystem(
    "coords", viskores::cont::make_ArrayHandle(points, viskores::CopyFlag::On)));
  dataSet.AddPointField("values", values);
  return dataSet;
}

void CheckUInt32Array(anari_cpp::Device d,
                      anari_cpp::Array1D array,
                      const std::vector<viskores::UInt32>& expected,
                      const char* name)
{
  VISKORES_TEST_ASSERT(array != nullptr, "No ", name, " array was sent to ANARI");

  // KHR_SPATIAL_FIELD_UNSTRUCTURED requires ANARI_UINT32 elements. Reading wider
  // elements as UInt32 gives the wrong values.
  const auto* values = anari_cpp::map<viskores::UInt32>(d, array);
  const std::vector<viskores::UInt32> sent(values, values + expected.size());
  anari_cpp::unmap(d, array);

  for (std::size_t valueIndex = 0; valueIndex < expected.size(); ++valueIndex)
  {
    VISKORES_TEST_ASSERT(sent[valueIndex] == expected[valueIndex],
                         "Wrong ",
                         name,
                         "[",
                         valueIndex,
                         "] sent to ANARI: expected ",
                         expected[valueIndex],
                         ", got ",
                         sent[valueIndex]);
  }
}

void UnstructuredIndexTypeTests()
{
  auto d = loadANARIDevice();

  auto dataSet = MakeTwoTetrahedraDataSet();
  viskores::interop::anari::ANARIActor actor(
    dataSet.GetCellSet(), dataSet.GetCoordinateSystem(), dataSet.GetField("values"));
  viskores::interop::anari::ANARIMapperVolume mVol(d, actor);
  mVol.GetANARISpatialField();

  const auto& parameters = mVol.GetANARIUnstructuredParameters();
  CheckUInt32Array(d, parameters.Index, { 0, 1, 2, 3, 1, 2, 3, 4 }, "index");
  CheckUInt32Array(d, parameters.CellIndex, { 0, 4 }, "cell.index");

  anari_cpp::release(d, d);
}

viskores::cont::DataSet MakeStructuredDataSet()
{
  const viskores::Id3 dims(3, 3, 3);
  std::vector<viskores::Float32> values(static_cast<std::size_t>(dims[0] * dims[1] * dims[2]));
  for (std::size_t valueIndex = 0; valueIndex < values.size(); ++valueIndex)
  {
    values[valueIndex] = static_cast<viskores::Float32>(valueIndex);
  }

  auto dataSet = viskores::cont::DataSetBuilderUniform::Create(dims);
  dataSet.AddPointField("values", values);
  return dataSet;
}

void CheckStructuredGeometry(const viskores::interop::anari::ANARIMapperVolume& mVol,
                             const viskores::Id3& expectedDims,
                             const viskores::Vec3f_32& expectedOrigin,
                             const viskores::Vec3f_32& expectedSpacing,
                             const char* what)
{
  const auto& parameters = mVol.GetANARIStructuredParameters();
  VISKORES_TEST_ASSERT(parameters.Data != nullptr, "No data array was sent to ANARI for ", what);
  auto checkAxis = [what](const char* name, viskores::IdComponent axis, auto sent, auto expected)
  {
    VISKORES_TEST_ASSERT(test_equal(sent, expected),
                         "Wrong ",
                         name,
                         "[",
                         axis,
                         "] for ",
                         what,
                         ": expected ",
                         expected,
                         ", got ",
                         sent);
  };
  for (viskores::IdComponent axis = 0; axis < 3; ++axis)
  {
    checkAxis("dims", axis, viskores::Id(parameters.Dims[axis]), expectedDims[axis]);
    checkAxis("origin", axis, parameters.Origin[axis], expectedOrigin[axis]);
    checkAxis("spacing", axis, parameters.Spacing[axis], expectedSpacing[axis]);
  }
}

void StructuredGeometryTests()
{
  auto d = loadANARIDevice();

  const viskores::Id3 pointDims(3, 4, 5);
  const viskores::Id3 cellDims(2, 3, 4);
  const viskores::Vec3f origin(1.f, 2.f, 3.f);
  const viskores::Vec3f spacing(0.5f, 1.f, 2.f);
  auto dataSet = viskores::cont::DataSetBuilderUniform::Create(pointDims, origin, spacing);

  // Point data: one sample at each point
  {
    std::vector<viskores::Float32> values(
      static_cast<std::size_t>(pointDims[0] * pointDims[1] * pointDims[2]), 1.f);
    dataSet.AddPointField("pointValues", values);
    viskores::interop::anari::ANARIActor actor(
      dataSet.GetCellSet(), dataSet.GetCoordinateSystem(), dataSet.GetField("pointValues"));
    viskores::interop::anari::ANARIMapperVolume mVol(d, actor);
    mVol.GetANARISpatialField();
    CheckStructuredGeometry(mVol,
                            pointDims,
                            viskores::Vec3f_32(1.f, 2.f, 3.f),
                            viskores::Vec3f_32(spacing),
                            "point data");
  }

  // Cell data: one sample at each cell center, half a spacing in from the first point
  {
    std::vector<viskores::Float32> values(
      static_cast<std::size_t>(cellDims[0] * cellDims[1] * cellDims[2]), 1.f);
    dataSet.AddCellField("cellValues", values);
    viskores::interop::anari::ANARIActor actor(
      dataSet.GetCellSet(), dataSet.GetCoordinateSystem(), dataSet.GetField("cellValues"));
    viskores::interop::anari::ANARIMapperVolume mVol(d, actor);
    mVol.GetANARISpatialField();
    CheckStructuredGeometry(mVol,
                            cellDims,
                            viskores::Vec3f_32(1.25f, 2.5f, 4.f),
                            viskores::Vec3f_32(spacing),
                            "cell data");
  }

  // One cell along an axis: the cell data spacing still comes from the grid
  {
    auto thin = viskores::cont::DataSetBuilderUniform::Create(
      viskores::Id3(2, 3, 3), viskores::Vec3f(0.f), viskores::Vec3f(0.25f));
    const std::vector<viskores::Float32> values(4, 1.f);
    thin.AddCellField("cellValues", values);
    viskores::interop::anari::ANARIActor actor(
      thin.GetCellSet(), thin.GetCoordinateSystem(), thin.GetField("cellValues"));
    viskores::interop::anari::ANARIMapperVolume mVol(d, actor);
    mVol.GetANARISpatialField();
    CheckStructuredGeometry(mVol,
                            viskores::Id3(1, 2, 2),
                            viskores::Vec3f_32(0.125f),
                            viskores::Vec3f_32(0.25f),
                            "one cell along x");
  }

  anari_cpp::release(d, d);
}

void LeakTrackingStatusFunc(const void* userData,
                            ANARIDevice,
                            ANARIObject,
                            ANARIDataType,
                            ANARIStatusSeverity severity,
                            ANARIStatusCode,
                            const char* message)
{
  // The device reports objects still alive when it is destroyed as warnings.
  if (severity == ANARI_SEVERITY_WARNING && std::strstr(message, "leaked") != nullptr)
  {
    auto* leaks = static_cast<std::vector<std::string>*>(const_cast<void*>(userData));
    leaks->emplace_back(message);
  }
}

void CheckOnlyUnstructuredParameters(const viskores::interop::anari::ANARIMapperVolume& mVol)
{
  const auto& unstructured = mVol.GetANARIUnstructuredParameters();
  VISKORES_TEST_ASSERT(unstructured.VertexPosition != nullptr, "Missing vertex.position");
  VISKORES_TEST_ASSERT(unstructured.VertexData != nullptr, "Missing vertex.data");
  VISKORES_TEST_ASSERT(unstructured.Index != nullptr, "Missing index");
  VISKORES_TEST_ASSERT(unstructured.CellIndex != nullptr, "Missing cell.index");
  VISKORES_TEST_ASSERT(unstructured.CellType != nullptr, "Missing cell.type");
  VISKORES_TEST_ASSERT(mVol.GetANARIStructuredParameters().Data == nullptr,
                       "Stale structured data left on an unstructured spatial field");
}

void CheckOnlyStructuredParameters(const viskores::interop::anari::ANARIMapperVolume& mVol)
{
  VISKORES_TEST_ASSERT(mVol.GetANARIStructuredParameters().Data != nullptr, "Missing data");
  const auto& unstructured = mVol.GetANARIUnstructuredParameters();
  VISKORES_TEST_ASSERT(unstructured.VertexPosition == nullptr, "Stale vertex.position");
  VISKORES_TEST_ASSERT(unstructured.VertexData == nullptr, "Stale vertex.data");
  VISKORES_TEST_ASSERT(unstructured.Index == nullptr, "Stale index");
  VISKORES_TEST_ASSERT(unstructured.CellIndex == nullptr, "Stale cell.index");
  VISKORES_TEST_ASSERT(unstructured.CellData == nullptr, "Stale cell.data");
  VISKORES_TEST_ASSERT(unstructured.CellType == nullptr, "Stale cell.type");
  VISKORES_TEST_ASSERT(!unstructured.IndexPrefixed, "Stale indexPrefixed");
}

void SwitchRepresentationTests()
{
  // Construct the internal device directly so its leak report reaches this test.
  std::vector<std::string> leaks;
  auto d = reinterpret_cast<anari_cpp::Device>(
    new viskores_device::ViskoresDevice(LeakTrackingStatusFunc, &leaks));

  auto unstructured = MakeSingleCellDataSet(viskores::CELL_SHAPE_HEXAHEDRON,
                                            { { 0, 0, 0 },
                                              { 1, 0, 0 },
                                              { 1, 1, 0 },
                                              { 0, 1, 0 },
                                              { 0, 0, 1 },
                                              { 1, 0, 1 },
                                              { 1, 1, 1 },
                                              { 0, 1, 1 } });
  auto structured = MakeStructuredDataSet();
  viskores::interop::anari::ANARIActor unstructuredActor(
    unstructured.GetCellSet(), unstructured.GetCoordinateSystem(), unstructured.GetField("values"));
  viskores::interop::anari::ANARIActor structuredActor(
    structured.GetCellSet(), structured.GetCoordinateSystem(), structured.GetField("values"));

  {
    viskores::interop::anari::ANARIMapperVolume mVol(d, unstructuredActor);
    mVol.GetANARIVolume();
    CheckOnlyUnstructuredParameters(mVol);

    mVol.SetActor(structuredActor);
    CheckOnlyStructuredParameters(mVol);

    mVol.SetActor(unstructuredActor);
    CheckOnlyUnstructuredParameters(mVol);
  }

  anari_cpp::release(d, d);

  std::string report;
  for (const auto& leak : leaks)
  {
    report += "\n  " + leak;
  }
  VISKORES_TEST_ASSERT(leaks.empty(), "ANARI objects leaked by the volume mapper:", report);
}

void CheckRejected(viskores::interop::anari::ANARIMapperVolume& mVol, const char* what)
{
  VISKORES_TEST_ASSERT(mVol.GetANARISpatialField() == nullptr, "Spatial field created for ", what);
  VISKORES_TEST_ASSERT(mVol.GroupIsEmpty(), "Group not empty for ", what);
  VISKORES_TEST_ASSERT(
    mVol.GetANARIStructuredParameters().Data == nullptr, "Structured data created for ", what);
  const auto& unstructured = mVol.GetANARIUnstructuredParameters();
  VISKORES_TEST_ASSERT(unstructured.VertexPosition == nullptr, "vertex.position for ", what);
  VISKORES_TEST_ASSERT(unstructured.VertexData == nullptr, "vertex.data for ", what);
  VISKORES_TEST_ASSERT(unstructured.Index == nullptr, "index for ", what);
  VISKORES_TEST_ASSERT(unstructured.CellIndex == nullptr, "cell.index for ", what);
  VISKORES_TEST_ASSERT(unstructured.CellData == nullptr, "cell.data for ", what);
  VISKORES_TEST_ASSERT(unstructured.CellType == nullptr, "cell.type for ", what);

  // Building the rest of the ANARI objects must not fail either.
  mVol.GetANARIVolume();
  mVol.GetANARIGroup();
}

void UnsupportedInputTests()
{
  auto d = loadANARIDevice();

  // Structured cells with a Vec3 point field
  {
    const viskores::Id3 dims(3, 3, 3);
    auto dataSet = viskores::cont::DataSetBuilderUniform::Create(dims);
    std::vector<viskores::Vec3f_32> vectors(static_cast<std::size_t>(dims[0] * dims[1] * dims[2]),
                                            viskores::Vec3f_32(1.f, 2.f, 3.f));
    dataSet.AddPointField("vectors", vectors);
    viskores::interop::anari::ANARIActor actor(
      dataSet.GetCellSet(), dataSet.GetCoordinateSystem(), dataSet.GetField("vectors"));
    viskores::interop::anari::ANARIMapperVolume mVol(d, actor);
    CheckRejected(mVol, "structured cells with a Vec3 point field");
  }

  // Structured cells with non-uniform (rectilinear) coordinates
  {
    auto dataSet = viskores::cont::DataSetBuilderRectilinear::Create(
      std::vector<viskores::Float32>{ 0.f, 1.f, 3.f },
      std::vector<viskores::Float32>{ 0.f, 1.f, 2.f },
      std::vector<viskores::Float32>{ 0.f, 0.5f, 4.f });
    const std::vector<viskores::Float32> values(27, 1.f);
    dataSet.AddPointField("values", values);
    viskores::interop::anari::ANARIActor actor(
      dataSet.GetCellSet(), dataSet.GetCoordinateSystem(), dataSet.GetField("values"));
    viskores::interop::anari::ANARIMapperVolume mVol(d, actor);
    CheckRejected(mVol, "structured cells with rectilinear coordinates");
  }

  // Structured cells with explicit (curvilinear) coordinates
  {
    auto dataSet = MakeStructuredDataSet();
    viskores::cont::ArrayHandle<viskores::Vec3f> points;
    viskores::cont::ArrayCopy(dataSet.GetCoordinateSystem().GetData(), points);
    viskores::interop::anari::ANARIActor actor(dataSet.GetCellSet(),
                                               viskores::cont::CoordinateSystem("coords", points),
                                               dataSet.GetField("values"));
    viskores::interop::anari::ANARIMapperVolume mVol(d, actor);
    CheckRejected(mVol, "structured cells with explicit coordinates");
  }

  // Structured cells with a single point along an axis have no cells to sample
  {
    // DataSetBuilderUniform would build a 2D cell set here, so build the 3D one directly.
    const viskores::Id3 dims(3, 3, 1);
    viskores::cont::CellSetStructured<3> cells;
    cells.SetPointDimensions(dims);
    viskores::cont::CoordinateSystem coords(
      "coords", viskores::cont::ArrayHandleUniformPointCoordinates(dims));
    viskores::cont::Field field(
      "values",
      viskores::cont::Field::Association::Points,
      viskores::cont::make_ArrayHandleMove(std::vector<viskores::Float32>(9, 1.f)));
    viskores::interop::anari::ANARIActor actor(cells, coords, field);
    viskores::interop::anari::ANARIMapperVolume mVol(d, actor);
    CheckRejected(mVol, "structured cells with one point along z");
  }

  // Empty cell set
  {
    viskores::cont::CellSetSingleType<> cells;
    viskores::cont::CoordinateSystem coords("coords",
                                            viskores::cont::ArrayHandle<viskores::Vec3f_32>{});
    viskores::cont::Field field("values",
                                viskores::cont::Field::Association::Points,
                                viskores::cont::ArrayHandle<viskores::Float32>{});
    viskores::interop::anari::ANARIActor actor(cells, coords, field);
    viskores::interop::anari::ANARIMapperVolume mVol(d, actor);
    CheckRejected(mVol, "an empty cell set");
  }

  // Cell set type the unstructured path does not handle
  {
    viskores::cont::CellSetStructured<2> cells;
    cells.SetPointDimensions(viskores::Id2(3, 3));
    std::vector<viskores::Vec3f_32> points;
    std::vector<viskores::Float32> values;
    for (viskores::IdComponent rowIndex = 0; rowIndex < 3; ++rowIndex)
    {
      for (viskores::IdComponent columnIndex = 0; columnIndex < 3; ++columnIndex)
      {
        points.emplace_back(static_cast<viskores::Float32>(columnIndex),
                            static_cast<viskores::Float32>(rowIndex),
                            0.f);
        values.push_back(static_cast<viskores::Float32>(columnIndex + rowIndex));
      }
    }
    viskores::cont::CoordinateSystem coords(
      "coords", viskores::cont::make_ArrayHandle(points, viskores::CopyFlag::On));
    viskores::cont::Field field("values",
                                viskores::cont::Field::Association::Points,
                                viskores::cont::make_ArrayHandle(values, viskores::CopyFlag::On));
    viskores::interop::anari::ANARIActor actor(cells, coords, field);
    viskores::interop::anari::ANARIMapperVolume mVol(d, actor);
    CheckRejected(mVol, "a 2D structured cell set");
  }

  // Connectivity that does not fit in the UInt32 indices ANARI requires
  {
    const viskores::Id tooLarge =
      static_cast<viskores::Id>(std::numeric_limits<viskores::UInt32>::max()) + 1;
    const std::vector<viskores::Id> connectivity = { 0, 1, 2, tooLarge };
    const std::vector<viskores::Vec3f_32> points = {
      { 0, 0, 0 }, { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 }
    };
    const std::vector<viskores::Float32> values = { 0, 1, 2, 3 };

    viskores::cont::CellSetSingleType<> cells;
    cells.Fill(static_cast<viskores::Id>(points.size()),
               viskores::CELL_SHAPE_TETRA,
               4,
               viskores::cont::make_ArrayHandle(connectivity, viskores::CopyFlag::On));
    viskores::cont::CoordinateSystem coords(
      "coords", viskores::cont::make_ArrayHandle(points, viskores::CopyFlag::On));
    viskores::cont::Field field("values",
                                viskores::cont::Field::Association::Points,
                                viskores::cont::make_ArrayHandle(values, viskores::CopyFlag::On));
    viskores::interop::anari::ANARIActor actor(cells, coords, field);
    viskores::interop::anari::ANARIMapperVolume mVol(d, actor);
    CheckRejected(mVol, "connectivity above UINT32_MAX");
  }

  // A valid actor replaced by an unsupported one leaves nothing behind
  {
    auto valid = MakeStructuredDataSet();
    viskores::interop::anari::ANARIActor validActor(
      valid.GetCellSet(), valid.GetCoordinateSystem(), valid.GetField("values"));
    viskores::interop::anari::ANARIMapperVolume mVol(d, validActor);
    mVol.GetANARIGroup();
    VISKORES_TEST_ASSERT(!mVol.GroupIsEmpty(), "Valid structured actor was rejected");

    viskores::cont::CellSetSingleType<> cells;
    viskores::cont::Field field("values",
                                viskores::cont::Field::Association::Points,
                                viskores::cont::ArrayHandle<viskores::Float32>{});
    mVol.SetActor(viskores::interop::anari::ANARIActor(cells, valid.GetCoordinateSystem(), field));
    CheckRejected(mVol, "an empty cell set replacing a valid actor");
  }

  anari_cpp::release(d, d);
}

void RenderTests()
{
  // Initialize ANARI /////////////////////////////////////////////////////////

  auto d = loadANARIDevice();

  // Create Viskores datasets /////////////////////////////////////////////////////

  viskores::source::Tangle source;
  source.SetPointDimensions({ 32 });
  auto tangle = source.Execute();

  // Map data to ANARI objects ////////////////////////////////////////////////

  auto world = anari_cpp::newObject<anari_cpp::World>(d);

  viskores::interop::anari::ANARIActor actor(
    tangle.GetCellSet(), tangle.GetCoordinateSystem(), tangle.GetField("tangle"));

  viskores::interop::anari::ANARIMapperVolume mVol(d, actor);
  setColorMap(d, mVol);

  auto volume = mVol.GetANARIVolume();
  anari_cpp::setParameterArray1D(d, world, "volume", &volume, 1);

  anari_cpp::commitParameters(d, world);

  // Render a frame ///////////////////////////////////////////////////////////

  renderTestANARIImage(d,
                       world,
                       viskores::Vec3f_32(-0.05, 1.43, 1.87),
                       viskores::Vec3f_32(0.32, -0.53, -0.79),
                       viskores::Vec3f_32(-0.20, -0.85, 0.49),
                       "interop/anari/volume.png");

  // Cleanup //////////////////////////////////////////////////////////////////

  anari_cpp::release(d, world);
  anari_cpp::release(d, d);
}

void VolumeTests()
{
  UnstructuredCellTypeTests();
  UnstructuredIndexTypeTests();
  StructuredGeometryTests();
  SwitchRepresentationTests();
  UnsupportedInputTests();
  RenderTests();
}

} // namespace

int UnitTestANARIMapperVolume(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(VolumeTests, argc, argv);
}
