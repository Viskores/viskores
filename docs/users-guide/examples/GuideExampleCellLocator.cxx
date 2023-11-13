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

#include <vtkm/worklet/WorkletMapField.h>

#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/CellLocatorBoundingIntervalHierarchy.h>
#include <vtkm/cont/CellLocatorGeneral.h>
#include <vtkm/cont/DataSetBuilderRectilinear.h>
#include <vtkm/cont/DataSetBuilderUniform.h>

#include <vtkm/VecFromPortalPermute.h>

#include <vtkm/exec/CellInterpolate.h>

#include <vtkm/cont/testing/Testing.h>

namespace
{

constexpr vtkm::Id DimensionSize = 50;
const vtkm::Id3 DimensionSizes = vtkm::Id3(DimensionSize);

////
//// BEGIN-EXAMPLE UseCellLocator
////
struct QueryCellsWorklet : public vtkm::worklet::WorkletMapField
{
  using ControlSignature =
    void(FieldIn, ExecObject, WholeCellSetIn<Cell, Point>, WholeArrayIn, FieldOut);
  using ExecutionSignature = void(_1, _2, _3, _4, _5);

  template<typename Point,
           typename CellLocatorExecObject,
           typename CellSet,
           typename FieldPortal,
           typename OutType>
  VTKM_EXEC void operator()(const Point& point,
                            const CellLocatorExecObject& cellLocator,
                            const CellSet& cellSet,
                            const FieldPortal& field,
                            OutType& out) const
  {
    // Use the cell locator to find the cell containing the point and the parametric
    // coordinates within that cell.
    vtkm::Id cellId;
    vtkm::Vec3f parametric;
    ////
    //// BEGIN-EXAMPLE HandleErrorCode
    ////
    vtkm::ErrorCode status = cellLocator.FindCell(point, cellId, parametric);
    if (status != vtkm::ErrorCode::Success)
    {
      this->RaiseError(vtkm::ErrorString(status));
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
      auto fieldValues = vtkm::make_VecFromPortalPermute(&indices, &field);

      // Do the interpolation
      vtkm::exec::CellInterpolate(fieldValues, parametric, cellShape, out);
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
  vtkm::cont::Invoker Invoke;

  vtkm::cont::ArrayHandle<vtkm::Vec3f> QueryPoints;

  template<typename FieldType, typename Storage>
  VTKM_CONT vtkm::cont::ArrayHandle<FieldType> Run(
    const vtkm::cont::DataSet& inDataSet,
    const vtkm::cont::ArrayHandle<FieldType, Storage>& inputField)
  {
    //// RESUME-EXAMPLE
    ////
    //// BEGIN-EXAMPLE ConstructCellLocator
    ////
    vtkm::cont::CellLocatorGeneral cellLocator;
    cellLocator.SetCellSet(inDataSet.GetCellSet());
    cellLocator.SetCoordinates(inDataSet.GetCoordinateSystem());
    cellLocator.Update();
    ////
    //// END-EXAMPLE ConstructCellLocator
    ////

    vtkm::cont::ArrayHandle<FieldType> interpolatedField;

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
  using ValueType = vtkm::Vec3f;
  using ArrayType = vtkm::cont::ArrayHandle<ValueType>;

  vtkm::cont::DataSet data = vtkm::cont::DataSetBuilderUniform::Create(DimensionSizes);

  ArrayType inField;
  vtkm::cont::ArrayCopy(vtkm::cont::ArrayHandleUniformPointCoordinates(
                          DimensionSizes, ValueType(0.0f), ValueType(2.0f)),
                        inField);

  DemoQueryCells demo;

  vtkm::cont::ArrayCopy(vtkm::cont::ArrayHandleUniformPointCoordinates(
                          DimensionSizes - vtkm::Id3(1), ValueType(0.5f)),
                        demo.QueryPoints);

  ArrayType interpolated = demo.Run(data, inField);

  vtkm::cont::ArrayHandleUniformPointCoordinates expected(
    DimensionSizes - vtkm::Id3(1), ValueType(1.0f), ValueType(2.0f));

  std::cout << "Expected: ";
  vtkm::cont::printSummary_ArrayHandle(expected, std::cout);

  std::cout << "Interpolated: ";
  vtkm::cont::printSummary_ArrayHandle(interpolated, std::cout);

  VTKM_TEST_ASSERT(test_equal_portals(expected.ReadPortal(), interpolated.ReadPortal()));
}

void Run()
{
  TestCellLocator();
}

} // anonymous namespace

int GuideExampleCellLocator(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(Run, argc, argv);
}
