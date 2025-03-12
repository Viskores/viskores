//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

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

#include <viskores/worklet/WorkletMapField.h>

#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/CellLocatorBoundingIntervalHierarchy.h>
#include <viskores/cont/CellLocatorGeneral.h>
#include <viskores/cont/DataSetBuilderRectilinear.h>
#include <viskores/cont/DataSetBuilderUniform.h>

#include <viskores/VecFromPortalPermute.h>

#include <viskores/exec/CellInterpolate.h>

#include <viskores/cont/testing/Testing.h>

namespace
{

constexpr viskores::Id DimensionSize = 50;
const viskores::Id3 DimensionSizes = viskores::Id3(DimensionSize);

////
//// BEGIN-EXAMPLE UseCellLocator
////
struct QueryCellsWorklet : public viskores::worklet::WorkletMapField
{
  using ControlSignature =
    void(FieldIn, ExecObject, WholeCellSetIn<Cell, Point>, WholeArrayIn, FieldOut);
  using ExecutionSignature = void(_1, _2, _3, _4, _5);

  template<typename Point,
           typename CellLocatorExecObject,
           typename CellSet,
           typename FieldPortal,
           typename OutType>
  VISKORES_EXEC void operator()(const Point& point,
                                const CellLocatorExecObject& cellLocator,
                                const CellSet& cellSet,
                                const FieldPortal& field,
                                OutType& out) const
  {
    // Use the cell locator to find the cell containing the point and the parametric
    // coordinates within that cell.
    viskores::Id cellId;
    viskores::Vec3f parametric;
    ////
    //// BEGIN-EXAMPLE HandleErrorCode
    ////
    viskores::ErrorCode status = cellLocator.FindCell(point, cellId, parametric);
    if (status != viskores::ErrorCode::Success)
    {
      this->RaiseError(viskores::ErrorString(status));
    }
    ////
    //// END-EXAMPLE HandleErrorCode
    ////

    // Use this information to interpolate the point field to the given location.
    if (cellId >= 0)
    {
      // Get shape information about the cell containing the point coordinate
      auto cellShape = cellSet.GetCellShape(cellId);
      auto indices = cellSet.GetIndices(cellId);

      // Make a Vec-like containing the field data at the cell's points
      auto fieldValues = viskores::make_VecFromPortalPermute(&indices, &field);

      // Do the interpolation
      viskores::exec::CellInterpolate(fieldValues, parametric, cellShape, out);
    }
    else
    {
      this->RaiseError("Given point outside of the cell set.");
    }
  }
};

//
// Later in the associated Filter class...
//

//// PAUSE-EXAMPLE
struct DemoQueryCells
{
  viskores::cont::Invoker Invoke;

  viskores::cont::ArrayHandle<viskores::Vec3f> QueryPoints;

  template<typename FieldType, typename Storage>
  VISKORES_CONT viskores::cont::ArrayHandle<FieldType> Run(
    const viskores::cont::DataSet& inDataSet,
    const viskores::cont::ArrayHandle<FieldType, Storage>& inputField)
  {
    //// RESUME-EXAMPLE
    ////
    //// BEGIN-EXAMPLE ConstructCellLocator
    ////
    viskores::cont::CellLocatorGeneral cellLocator;
    cellLocator.SetCellSet(inDataSet.GetCellSet());
    cellLocator.SetCoordinates(inDataSet.GetCoordinateSystem());
    cellLocator.Update();
    ////
    //// END-EXAMPLE ConstructCellLocator
    ////

    viskores::cont::ArrayHandle<FieldType> interpolatedField;

    this->Invoke(QueryCellsWorklet{},
                 this->QueryPoints,
                 &cellLocator,
                 inDataSet.GetCellSet(),
                 inputField,
                 interpolatedField);
    ////
    //// END-EXAMPLE UseCellLocator
    ////

    return interpolatedField;
  }
};

void TestCellLocator()
{
  using ValueType = viskores::Vec3f;
  using ArrayType = viskores::cont::ArrayHandle<ValueType>;

  viskores::cont::DataSet data =
    viskores::cont::DataSetBuilderUniform::Create(DimensionSizes);

  ArrayType inField;
  viskores::cont::ArrayCopy(viskores::cont::ArrayHandleUniformPointCoordinates(
                              DimensionSizes, ValueType(0.0f), ValueType(2.0f)),
                            inField);

  DemoQueryCells demo;

  viskores::cont::ArrayCopy(viskores::cont::ArrayHandleUniformPointCoordinates(
                              DimensionSizes - viskores::Id3(1), ValueType(0.5f)),
                            demo.QueryPoints);

  ArrayType interpolated = demo.Run(data, inField);

  viskores::cont::ArrayHandleUniformPointCoordinates expected(
    DimensionSizes - viskores::Id3(1), ValueType(1.0f), ValueType(2.0f));

  std::cout << "Expected: ";
  viskores::cont::printSummary_ArrayHandle(expected, std::cout);

  std::cout << "Interpolated: ";
  viskores::cont::printSummary_ArrayHandle(interpolated, std::cout);

  VISKORES_TEST_ASSERT(
    test_equal_portals(expected.ReadPortal(), interpolated.ReadPortal()));
}

void Run()
{
  TestCellLocator();
}

} // anonymous namespace

int GuideExampleCellLocator(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(Run, argc, argv);
}
