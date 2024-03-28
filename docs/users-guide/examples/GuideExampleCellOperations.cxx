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

#include <vtkm/worklet/DispatcherMapTopology.h>
#include <vtkm/worklet/WorkletMapTopology.h>

#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>

namespace
{

////
//// BEGIN-EXAMPLE CellCenters
////
struct CellCenters : vtkm::worklet::WorkletVisitCellsWithPoints
{
  using ControlSignature = void(CellSetIn,
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

void TryCellCenters()
{
  std::cout << "Trying CellCenters worklet." << std::endl;

  vtkm::cont::DataSet dataSet =
    vtkm::cont::testing::MakeTestDataSet().Make3DUniformDataSet0();

  using ArrayType = vtkm::cont::ArrayHandle<vtkm::Float32>;
  ArrayType centers;

  vtkm::worklet::DispatcherMapTopology<CellCenters> dispatcher;
  dispatcher.Invoke(dataSet.GetCellSet(),
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

  vtkm::worklet::DispatcherMapTopology<CellDerivatives> dispatcher;
  dispatcher.Invoke(dataSet.GetCellSet(),
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
