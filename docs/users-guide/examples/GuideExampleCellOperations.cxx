//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/exec/CellDerivative.h>
#include <vtkm/exec/CellInterpolate.h>
#include <vtkm/exec/ParametricCoordinates.h>

#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/worklet/WorkletMapTopology.h>

#include <vtkm/cont/Invoker.h>
#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>

namespace
{

////
//// BEGIN-EXAMPLE CellCenters
////
struct CellCenters : vtkm::worklet::WorkletVisitCellsWithPoints
{
  using ControlSignature = void(CellSetIn inputCells,
                                FieldInPoint inputField,
                                FieldOutCell outputField);
  using ExecutionSignature = void(CellShape, PointCount, _2, _3);
  using InputDomain = _1;

  template<typename CellShapeTag, typename FieldInVecType, typename FieldOutType>
  VTKM_EXEC void operator()(CellShapeTag shape,
                            vtkm::IdComponent pointCount,
                            const FieldInVecType& inputField,
                            FieldOutType& outputField) const
  {
    vtkm::Vec3f center;
    vtkm::ErrorCode status =
      vtkm::exec::ParametricCoordinatesCenter(pointCount, shape, center);
    if (status != vtkm::ErrorCode::Success)
    {
      this->RaiseError(vtkm::ErrorString(status));
      return;
    }
    vtkm::exec::CellInterpolate(inputField, center, shape, outputField);
  }
};
////
//// END-EXAMPLE CellCenters
////

////
//// BEGIN-EXAMPLE CellLookupInterp
////
struct CellLookupInterp : vtkm::worklet::WorkletMapField
{
  using ControlSignature = void(WholeCellSetIn<> inputCells,
                                WholeArrayIn inputField,
                                FieldOut outputField);
  using ExecutionSignature = void(InputIndex, _1, _2, _3);
  using InputDomain = _3;

  template<typename StructureType, typename FieldInPortalType, typename FieldOutType>
  VTKM_EXEC void operator()(vtkm::Id index,
                            const StructureType& structure,
                            const FieldInPortalType& inputField,
                            FieldOutType& outputField) const
  {
    // Normally you would use something like a locator to find the index to
    // a cell that matches some query criteria. For demonstration purposes,
    // we are just using a passed in index.
    auto shape = structure.GetCellShape(index);
    vtkm::IdComponent pointCount = structure.GetNumberOfIndices(index);

    vtkm::Vec3f center;
    vtkm::ErrorCode status =
      vtkm::exec::ParametricCoordinatesCenter(pointCount, shape, center);
    if (status != vtkm::ErrorCode::Success)
    {
      this->RaiseError(vtkm::ErrorString(status));
      return;
    }

    auto pointIndices = structure.GetIndices(index);
    vtkm::exec::CellInterpolate(pointIndices, inputField, center, shape, outputField);
  }
};
////
//// END-EXAMPLE CellLookupInterp
////

void TryCellCenters()
{
  std::cout << "Trying CellCenters worklet." << std::endl;

  vtkm::cont::DataSet dataSet =
    vtkm::cont::testing::MakeTestDataSet().Make3DUniformDataSet0();

  using ArrayType = vtkm::cont::ArrayHandle<vtkm::Float32>;
  ArrayType centers;

  vtkm::cont::Invoker invoke;
  invoke(CellCenters{},
         dataSet.GetCellSet(),
         dataSet.GetField("pointvar").GetData().AsArrayHandle<ArrayType>(),
         centers);
  vtkm::cont::printSummary_ArrayHandle(centers, std::cout);
  std::cout << std::endl;
  VTKM_TEST_ASSERT(centers.GetNumberOfValues() ==
                     dataSet.GetCellSet().GetNumberOfCells(),
                   "Bad number of cells.");
  VTKM_TEST_ASSERT(test_equal(60.1875, centers.ReadPortal().Get(0)), "Bad first value.");

  centers.Fill(0);
  invoke(CellLookupInterp{},
         dataSet.GetCellSet(),
         dataSet.GetField("pointvar").GetData().AsArrayHandle<ArrayType>(),
         centers);
  vtkm::cont::printSummary_ArrayHandle(centers, std::cout);
  std::cout << std::endl;
  VTKM_TEST_ASSERT(centers.GetNumberOfValues() ==
                     dataSet.GetCellSet().GetNumberOfCells(),
                   "Bad number of cells.");
  VTKM_TEST_ASSERT(test_equal(60.1875, centers.ReadPortal().Get(0)), "Bad first value.");
}
////
//// BEGIN-EXAMPLE CellDerivatives
////
struct CellDerivatives : vtkm::worklet::WorkletVisitCellsWithPoints
{
  using ControlSignature = void(CellSetIn,
                                FieldInPoint inputField,
                                FieldInPoint pointCoordinates,
                                FieldOutCell outputField);
  using ExecutionSignature = void(CellShape, PointCount, _2, _3, _4);
  using InputDomain = _1;

  template<typename CellShapeTag,
           typename FieldInVecType,
           typename PointCoordVecType,
           typename FieldOutType>
  VTKM_EXEC void operator()(CellShapeTag shape,
                            vtkm::IdComponent pointCount,
                            const FieldInVecType& inputField,
                            const PointCoordVecType& pointCoordinates,
                            FieldOutType& outputField) const
  {
    vtkm::Vec3f center;
    vtkm::ErrorCode status =
      vtkm::exec::ParametricCoordinatesCenter(pointCount, shape, center);
    if (status != vtkm::ErrorCode::Success)
    {
      this->RaiseError(vtkm::ErrorString(status));
      return;
    }
    vtkm::exec::CellDerivative(inputField, pointCoordinates, center, shape, outputField);
  }
};
////
//// END-EXAMPLE CellDerivatives
////

void TryCellDerivatives()
{
  std::cout << "Trying CellDerivatives worklet." << std::endl;

  vtkm::cont::DataSet dataSet =
    vtkm::cont::testing::MakeTestDataSet().Make3DUniformDataSet0();

  using ArrayType = vtkm::cont::ArrayHandle<vtkm::Float32>;
  vtkm::cont::ArrayHandle<vtkm::Vec3f_32> derivatives;

  vtkm::cont::Invoker invoke;
  vtkm::worklet::DispatcherMapTopology<CellDerivatives> dispatcher;
  invoke(CellDerivatives{},
         dataSet.GetCellSet(),
         dataSet.GetField("pointvar").GetData().AsArrayHandle<ArrayType>(),
         dataSet.GetCoordinateSystem().GetData(),
         derivatives);

  vtkm::cont::printSummary_ArrayHandle(derivatives, std::cout);
  std::cout << std::endl;

  VTKM_TEST_ASSERT(derivatives.GetNumberOfValues() ==
                     dataSet.GetCellSet().GetNumberOfCells(),
                   "Bad number of cells.");
  VTKM_TEST_ASSERT(
    test_equal(vtkm::make_Vec(10.025, 30.075, 60.125), derivatives.ReadPortal().Get(0)),
    "Bad first value.");
}

void Run()
{
  TryCellCenters();
  TryCellDerivatives();
}

} // anonymous namespace

int GuideExampleCellOperations(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(Run, argc, argv);
}
