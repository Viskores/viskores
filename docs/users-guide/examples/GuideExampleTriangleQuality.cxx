//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//=============================================================================

#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/DataSet.h>
#include <viskores/cont/Invoker.h>

#include <viskores/worklet/WorkletMapTopology.h>

#include <viskores/CellShape.h>
#include <viskores/Math.h>
#include <viskores/VectorAnalysis.h>

////
//// BEGIN-EXAMPLE TriangleQualityWholeArray
////
namespace detail
{

static const viskores::Id TRIANGLE_QUALITY_TABLE_DIMENSION = 8;
static const viskores::Id TRIANGLE_QUALITY_TABLE_SIZE =
  TRIANGLE_QUALITY_TABLE_DIMENSION * TRIANGLE_QUALITY_TABLE_DIMENSION;

VISKORES_CONT
viskores::cont::ArrayHandle<viskores::Float32> GetTriangleQualityTable()
{
  // Use these precomputed values for the array. A real application would
  // probably use a larger array, but we are keeping it small for demonstration
  // purposes.
  static viskores::Float32 triangleQualityBuffer[TRIANGLE_QUALITY_TABLE_SIZE] = {
    0, 0,        0,        0,        0,        0,        0,        0,
    0, 0,        0,        0,        0,        0,        0,        0.24431f,
    0, 0,        0,        0,        0,        0,        0.43298f, 0.47059f,
    0, 0,        0,        0,        0,        0.54217f, 0.65923f, 0.66408f,
    0, 0,        0,        0,        0.57972f, 0.75425f, 0.82154f, 0.81536f,
    0, 0,        0,        0.54217f, 0.75425f, 0.87460f, 0.92567f, 0.92071f,
    0, 0,        0.43298f, 0.65923f, 0.82154f, 0.92567f, 0.97664f, 0.98100f,
    0, 0.24431f, 0.47059f, 0.66408f, 0.81536f, 0.92071f, 0.98100f, 1
  };

  return viskores::cont::make_ArrayHandle(
    triangleQualityBuffer, TRIANGLE_QUALITY_TABLE_SIZE, viskores::CopyFlag::Off);
}

template<typename T>
VISKORES_EXEC_CONT viskores::Vec<T, 3> TriangleEdgeLengths(
  const viskores::Vec<T, 3>& point1,
  const viskores::Vec<T, 3>& point2,
  const viskores::Vec<T, 3>& point3)
{
  return viskores::make_Vec(viskores::Magnitude(point1 - point2),
                            viskores::Magnitude(point2 - point3),
                            viskores::Magnitude(point3 - point1));
}

VISKORES_SUPPRESS_EXEC_WARNINGS
template<typename PortalType, typename T>
VISKORES_EXEC_CONT static viskores::Float32 LookupTriangleQuality(
  const PortalType& triangleQualityPortal,
  const viskores::Vec<T, 3>& point1,
  const viskores::Vec<T, 3>& point2,
  const viskores::Vec<T, 3>& point3)
{
  viskores::Vec<T, 3> edgeLengths = TriangleEdgeLengths(point1, point2, point3);

  // To reduce the size of the table, we just store the quality of triangles
  // with the longest edge of size 1. The table is 2D indexed by the length
  // of the other two edges. Thus, to use the table we have to identify the
  // longest edge and scale appropriately.
  T smallEdge1 = viskores::Min(edgeLengths[0], edgeLengths[1]);
  T tmpEdge = viskores::Max(edgeLengths[0], edgeLengths[1]);
  T smallEdge2 = viskores::Min(edgeLengths[2], tmpEdge);
  T largeEdge = viskores::Max(edgeLengths[2], tmpEdge);

  smallEdge1 /= largeEdge;
  smallEdge2 /= largeEdge;

  // Find index into array.
  viskores::Id index1 = static_cast<viskores::Id>(
    viskores::Floor(smallEdge1 * (TRIANGLE_QUALITY_TABLE_DIMENSION - 1) + 0.5));
  viskores::Id index2 = static_cast<viskores::Id>(
    viskores::Floor(smallEdge2 * (TRIANGLE_QUALITY_TABLE_DIMENSION - 1) + 0.5));
  viskores::Id totalIndex = index1 + index2 * TRIANGLE_QUALITY_TABLE_DIMENSION;

  return triangleQualityPortal.Get(totalIndex);
}

} // namespace detail

struct TriangleQualityWorklet : viskores::worklet::WorkletVisitCellsWithPoints
{
  using ControlSignature = void(CellSetIn cells,
                                FieldInPoint pointCoordinates,
                                WholeArrayIn triangleQualityTable,
                                FieldOutCell triangleQuality);
  using ExecutionSignature = _4(CellShape, _2, _3);
  using InputDomain = _1;

  template<typename CellShape,
           typename PointCoordinatesType,
           typename TriangleQualityTablePortalType>
  VISKORES_EXEC viskores::Float32 operator()(
    CellShape shape,
    const PointCoordinatesType& pointCoordinates,
    const TriangleQualityTablePortalType& triangleQualityTable) const
  {
    if (shape.Id != viskores::CELL_SHAPE_TRIANGLE)
    {
      this->RaiseError("Only triangles are supported for triangle quality.");
      return viskores::Nan32();
    }
    else
    {
      return detail::LookupTriangleQuality(triangleQualityTable,
                                           pointCoordinates[0],
                                           pointCoordinates[1],
                                           pointCoordinates[2]);
    }
  }
};

//
// Later in the associated Filter class...
//

//// PAUSE-EXAMPLE
struct DemoTriangleQuality
{
  viskores::cont::Invoker Invoke;

  template<typename CoordinatesType>
  VISKORES_CONT viskores::cont::ArrayHandle<viskores::Float32> Run(
    const viskores::cont::DataSet inputDataSet,
    const CoordinatesType& inputPointCoordinatesField)
  {
    //// RESUME-EXAMPLE
    viskores::cont::ArrayHandle<viskores::Float32> triangleQualityTable =
      detail::GetTriangleQualityTable();

    viskores::cont::ArrayHandle<viskores::Float32> triangleQualities;

    this->Invoke(TriangleQualityWorklet{},
                 inputDataSet.GetCellSet(),
                 inputPointCoordinatesField,
                 triangleQualityTable,
                 triangleQualities);
    ////
    //// END-EXAMPLE TriangleQualityWholeArray
    ////

    return triangleQualities;
  }
};

////
//// BEGIN-EXAMPLE TriangleQualityExecObject
////
class TriangleQualityTableExecutionObject
{
  using TableArrayType = viskores::cont::ArrayHandle<viskores::Float32>;
  using TablePortalType = typename TableArrayType::ReadPortalType;
  TablePortalType TablePortal;

public:
  VISKORES_CONT
  TriangleQualityTableExecutionObject(const TablePortalType& tablePortal)
    : TablePortal(tablePortal)
  {
  }

  template<typename T>
  VISKORES_EXEC viskores::Float32 GetQuality(const viskores::Vec<T, 3>& point1,
                                             const viskores::Vec<T, 3>& point2,
                                             const viskores::Vec<T, 3>& point3) const
  {
    return detail::LookupTriangleQuality(this->TablePortal, point1, point2, point3);
  }
};

class TriangleQualityTable : public viskores::cont::ExecutionObjectBase
{
public:
  VISKORES_CONT TriangleQualityTableExecutionObject
  PrepareForExecution(viskores::cont::DeviceAdapterId device,
                      viskores::cont::Token& token) const
  {
    return TriangleQualityTableExecutionObject(
      detail::GetTriangleQualityTable().PrepareForInput(device, token));
  }
};

struct TriangleQualityWorklet2 : viskores::worklet::WorkletVisitCellsWithPoints
{
  using ControlSignature = void(CellSetIn cells,
                                FieldInPoint pointCoordinates,
                                ExecObject triangleQualityTable,
                                FieldOutCell triangleQuality);
  using ExecutionSignature = _4(CellShape, _2, _3);
  using InputDomain = _1;

  template<typename CellShape,
           typename PointCoordinatesType,
           typename TriangleQualityTableType>
  VISKORES_EXEC viskores::Float32 operator()(
    CellShape shape,
    const PointCoordinatesType& pointCoordinates,
    const TriangleQualityTableType& triangleQualityTable) const
  {
    if (shape.Id != viskores::CELL_SHAPE_TRIANGLE)
    {
      this->RaiseError("Only triangles are supported for triangle quality.");
      return viskores::Nan32();
    }
    else
    {
      return triangleQualityTable.GetQuality(
        pointCoordinates[0], pointCoordinates[1], pointCoordinates[2]);
    }
  }
};

//
// Later in the associated Filter class...
//

//// PAUSE-EXAMPLE
struct DemoTriangleQuality2
{
  viskores::cont::Invoker Invoke;

  template<typename CoordinatesType>
  VISKORES_CONT viskores::cont::ArrayHandle<viskores::Float32> Run(
    const viskores::cont::DataSet inputDataSet,
    const CoordinatesType& inputPointCoordinatesField)
  {
    //// RESUME-EXAMPLE
    TriangleQualityTable triangleQualityTable;

    viskores::cont::ArrayHandle<viskores::Float32> triangleQualities;

    this->Invoke(TriangleQualityWorklet2{},
                 inputDataSet.GetCellSet(),
                 inputPointCoordinatesField,
                 triangleQualityTable,
                 triangleQualities);
    ////
    //// END-EXAMPLE TriangleQualityExecObject
    ////

    return triangleQualities;
  }
};

#include <viskores/cont/ArrayHandleUniformPointCoordinates.h>
#include <viskores/cont/DataSetBuilderExplicit.h>

#include <viskores/worklet/WorkletMapField.h>

#include <viskores/cont/testing/Testing.h>

namespace TriangleQualityNamespace
{

template<typename T>
VISKORES_EXEC T TriangleQuality(const viskores::Vec<T, 3>& edgeLengths)
{
  // Heron's formula for triangle area.
  T semiperimeter = (edgeLengths[0] + edgeLengths[1] + edgeLengths[2]) / 2;
  T areaSquared = (semiperimeter * (semiperimeter - edgeLengths[0]) *
                   (semiperimeter - edgeLengths[1]) * (semiperimeter - edgeLengths[2]));

  if (areaSquared < 0)
  {
    // If the edge lengths do not make a valid triangle (i.e. the sum of the
    // two smaller lengths is smaller than the larger length), then Heron's
    // formula gives an imaginary number. If that happens, just return a
    // quality of 0 for the degenerate triangle.
    return 0;
  }
  T area = viskores::Sqrt(areaSquared);

  // Formula for triangle quality.
  return 4 * area * viskores::Sqrt(T(3)) / viskores::MagnitudeSquared(edgeLengths);
}

struct ComputeTriangleQualityValues : viskores::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn, FieldOut);
  using ExecutionSignature = _2(_1);

  template<typename T>
  VISKORES_EXEC T operator()(const viskores::Vec<T, 3>& edgeLengths) const
  {
    return TriangleQuality(edgeLengths);
  }
};

VISKORES_CONT
viskores::cont::ArrayHandle<viskores::Float32> BuildTriangleQualityTable()
{
  // Repurpose uniform point coordinates to compute triange edge lengths.
  viskores::cont::ArrayHandleUniformPointCoordinates edgeLengths(
    viskores::Id3(detail::TRIANGLE_QUALITY_TABLE_DIMENSION,
                  detail::TRIANGLE_QUALITY_TABLE_DIMENSION,
                  1),
    viskores::Vec3f(0, 0, 1),
    viskores::Vec3f(1.0f / (detail::TRIANGLE_QUALITY_TABLE_DIMENSION - 1),
                    1.0f / (detail::TRIANGLE_QUALITY_TABLE_DIMENSION - 1),
                    1.0f));

  viskores::cont::ArrayHandle<viskores::Float32> triQualityArray;

  viskores::cont::Invoker invoke;
  invoke(ComputeTriangleQualityValues{}, edgeLengths, triQualityArray);

  return triQualityArray;
}

template<typename PortalType>
VISKORES_CONT void PrintTriangleQualityTable(const PortalType& portal)
{
  for (viskores::Id index = 0; index < portal.GetNumberOfValues(); index++)
  {
    if (index % detail::TRIANGLE_QUALITY_TABLE_DIMENSION == 0)
    {
      std::cout << std::endl;
    }
    std::cout << portal.Get(index) << ", ";
  }
  std::cout << std::endl << std::endl;
}

VISKORES_CONT
viskores::cont::DataSet BuildDataSet()
{
  static const viskores::Id NUM_ROWS = 5;

  viskores::cont::DataSetBuilderExplicitIterative dataSetBuilder;
  dataSetBuilder.Begin();

  for (viskores::Id row = 0; row < NUM_ROWS; row++)
  {
    dataSetBuilder.AddPoint(0, static_cast<viskores::Float32>(row * row), 0);
    dataSetBuilder.AddPoint(1, static_cast<viskores::Float32>(row * row), 0);
  }

  for (viskores::Id row = 0; row < NUM_ROWS - 1; row++)
  {
    viskores::Id firstPoint = 2 * row;

    dataSetBuilder.AddCell(viskores::CELL_SHAPE_TRIANGLE);
    dataSetBuilder.AddCellPoint(firstPoint + 0);
    dataSetBuilder.AddCellPoint(firstPoint + 1);
    dataSetBuilder.AddCellPoint(firstPoint + 2);

    dataSetBuilder.AddCell(viskores::CELL_SHAPE_TRIANGLE);
    dataSetBuilder.AddCellPoint(firstPoint + 1);
    dataSetBuilder.AddCellPoint(firstPoint + 3);
    dataSetBuilder.AddCellPoint(firstPoint + 2);
  }

  return dataSetBuilder.Create();
}

VISKORES_CONT
void CheckQualityArray(viskores::cont::ArrayHandle<viskores::Float32> qualities)
{
  viskores::cont::printSummary_ArrayHandle(qualities, std::cout);
  std::cout << std::endl;

  viskores::cont::ArrayHandle<viskores::Float32>::ReadPortalType qualityPortal =
    qualities.ReadPortal();

  // Pairwise triangles should have the same quality.
  for (viskores::Id pairIndex = 0; pairIndex < qualities.GetNumberOfValues() / 2;
       pairIndex++)
  {
    viskores::Float32 q1 = qualityPortal.Get(2 * pairIndex);
    viskores::Float32 q2 = qualityPortal.Get(2 * pairIndex + 1);
    VISKORES_TEST_ASSERT(test_equal(q1, q2),
                         "Isometric triangles have different quality.");
  }

  // Triangle qualities should be monotonically decreasing.
  viskores::Float32 lastQuality = 1;
  for (viskores::Id triIndex = 0; triIndex < qualities.GetNumberOfValues(); triIndex++)
  {
    viskores::Float32 quality = qualityPortal.Get(triIndex);
    VISKORES_TEST_ASSERT(test_equal(quality, lastQuality) || (quality <= lastQuality),
                         "Triangle quality not monotonically decreasing.");
    lastQuality = quality;
  }

  // The first quality should definitely be better than the last.
  viskores::Float32 firstQuality = qualityPortal.Get(0);
  VISKORES_TEST_ASSERT(firstQuality > lastQuality,
                       "First quality not better than last.");
}

VISKORES_CONT
void TestTriangleQuality()
{
  std::cout << "Building triangle quality array." << std::endl;
  viskores::cont::ArrayHandle<viskores::Float32> triQualityTable =
    BuildTriangleQualityTable();
  VISKORES_TEST_ASSERT(triQualityTable.GetNumberOfValues() ==
                         detail::TRIANGLE_QUALITY_TABLE_DIMENSION *
                           detail::TRIANGLE_QUALITY_TABLE_DIMENSION,
                       "Bad size for triangle quality array.");
  PrintTriangleQualityTable(triQualityTable.ReadPortal());

  std::cout << "Creating a data set." << std::endl;
  viskores::cont::DataSet dataSet = BuildDataSet();

  std::cout << "Getting triangle quality using whole array argument." << std::endl;
  viskores::cont::ArrayHandle<viskores::Float32> qualities =
    DemoTriangleQuality().Run(dataSet, dataSet.GetCoordinateSystem().GetData());
  CheckQualityArray(qualities);

  std::cout << "Getting triangle quality using execution object argument." << std::endl;
  qualities =
    DemoTriangleQuality2().Run(dataSet, dataSet.GetCoordinateSystem().GetData());
  CheckQualityArray(qualities);
}

} // namespace TriangleQualityNamespace

int GuideExampleTriangleQuality(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(
    TriangleQualityNamespace::TestTriangleQuality, argc, argv);
}
