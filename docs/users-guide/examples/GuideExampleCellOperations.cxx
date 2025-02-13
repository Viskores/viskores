//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <viskores/exec/CellDerivative.h>
#include <viskores/exec/CellInterpolate.h>
#include <viskores/exec/ParametricCoordinates.h>

#include <viskores/worklet/WorkletMapField.h>
#include <viskores/worklet/WorkletMapTopology.h>

#include <viskores/cont/Invoker.h>
#include <viskores/cont/testing/MakeTestDataSet.h>
#include <viskores/cont/testing/Testing.h>

namespace
{

////
//// BEGIN-EXAMPLE CellCenters
////
struct CellCenters : viskores::worklet::WorkletVisitCellsWithPoints
{
  using ControlSignature = void(CellSetIn inputCells,
                                FieldInPoint inputField,
                                FieldOutCell outputField);
  using ExecutionSignature = void(CellShape, PointCount, _2, _3);
  using InputDomain = _1;

  template<typename CellShapeTag, typename FieldInVecType, typename FieldOutType>
  VISKORES_EXEC void operator()(CellShapeTag shape,
                                viskores::IdComponent pointCount,
                                const FieldInVecType& inputField,
                                FieldOutType& outputField) const
  {
    viskores::Vec3f center;
    viskores::ErrorCode status =
      viskores::exec::ParametricCoordinatesCenter(pointCount, shape, center);
    if (status != viskores::ErrorCode::Success)
    {
      this->RaiseError(viskores::ErrorString(status));
      return;
    }
    viskores::exec::CellInterpolate(inputField, center, shape, outputField);
  }
};
////
//// END-EXAMPLE CellCenters
////

////
//// BEGIN-EXAMPLE CellLookupInterp
////
struct CellLookupInterp : viskores::worklet::WorkletMapField
{
  using ControlSignature = void(WholeCellSetIn<> inputCells,
                                WholeArrayIn inputField,
                                FieldOut outputField);
  using ExecutionSignature = void(InputIndex, _1, _2, _3);
  using InputDomain = _3;

  template<typename StructureType, typename FieldInPortalType, typename FieldOutType>
  VISKORES_EXEC void operator()(viskores::Id index,
                                const StructureType& structure,
                                const FieldInPortalType& inputField,
                                FieldOutType& outputField) const
  {
    // Normally you would use something like a locator to find the index to
    // a cell that matches some query criteria. For demonstration purposes,
    // we are just using a passed in index.
    auto shape = structure.GetCellShape(index);
    viskores::IdComponent pointCount = structure.GetNumberOfIndices(index);

    viskores::Vec3f center;
    viskores::ErrorCode status =
      viskores::exec::ParametricCoordinatesCenter(pointCount, shape, center);
    if (status != viskores::ErrorCode::Success)
    {
      this->RaiseError(viskores::ErrorString(status));
      return;
    }

    auto pointIndices = structure.GetIndices(index);
    viskores::exec::CellInterpolate(
      pointIndices, inputField, center, shape, outputField);
  }
};
////
//// END-EXAMPLE CellLookupInterp
////

void TryCellCenters()
{
  std::cout << "Trying CellCenters worklet." << std::endl;

  viskores::cont::DataSet dataSet =
    viskores::cont::testing::MakeTestDataSet().Make3DUniformDataSet0();

  using ArrayType = viskores::cont::ArrayHandle<viskores::Float32>;
  ArrayType centers;

  viskores::cont::Invoker invoke;
  invoke(CellCenters{},
         dataSet.GetCellSet(),
         dataSet.GetField("pointvar").GetData().AsArrayHandle<ArrayType>(),
         centers);
  viskores::cont::printSummary_ArrayHandle(centers, std::cout);
  std::cout << std::endl;
  VISKORES_TEST_ASSERT(centers.GetNumberOfValues() ==
                         dataSet.GetCellSet().GetNumberOfCells(),
                       "Bad number of cells.");
  VISKORES_TEST_ASSERT(test_equal(60.1875, centers.ReadPortal().Get(0)),
                       "Bad first value.");

  centers.Fill(0);
  invoke(CellLookupInterp{},
         dataSet.GetCellSet(),
         dataSet.GetField("pointvar").GetData().AsArrayHandle<ArrayType>(),
         centers);
  viskores::cont::printSummary_ArrayHandle(centers, std::cout);
  std::cout << std::endl;
  VISKORES_TEST_ASSERT(centers.GetNumberOfValues() ==
                         dataSet.GetCellSet().GetNumberOfCells(),
                       "Bad number of cells.");
  VISKORES_TEST_ASSERT(test_equal(60.1875, centers.ReadPortal().Get(0)),
                       "Bad first value.");
}
////
//// BEGIN-EXAMPLE CellDerivatives
////
struct CellDerivatives : viskores::worklet::WorkletVisitCellsWithPoints
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
  VISKORES_EXEC void operator()(CellShapeTag shape,
                                viskores::IdComponent pointCount,
                                const FieldInVecType& inputField,
                                const PointCoordVecType& pointCoordinates,
                                FieldOutType& outputField) const
  {
    viskores::Vec3f center;
    viskores::ErrorCode status =
      viskores::exec::ParametricCoordinatesCenter(pointCount, shape, center);
    if (status != viskores::ErrorCode::Success)
    {
      this->RaiseError(viskores::ErrorString(status));
      return;
    }
    viskores::exec::CellDerivative(
      inputField, pointCoordinates, center, shape, outputField);
  }
};
////
//// END-EXAMPLE CellDerivatives
////

void TryCellDerivatives()
{
  std::cout << "Trying CellDerivatives worklet." << std::endl;

  viskores::cont::DataSet dataSet =
    viskores::cont::testing::MakeTestDataSet().Make3DUniformDataSet0();

  using ArrayType = viskores::cont::ArrayHandle<viskores::Float32>;
  viskores::cont::ArrayHandle<viskores::Vec3f_32> derivatives;

  viskores::cont::Invoker invoke;
  viskores::worklet::DispatcherMapTopology<CellDerivatives> dispatcher;
  invoke(CellDerivatives{},
         dataSet.GetCellSet(),
         dataSet.GetField("pointvar").GetData().AsArrayHandle<ArrayType>(),
         dataSet.GetCoordinateSystem().GetData(),
         derivatives);

  viskores::cont::printSummary_ArrayHandle(derivatives, std::cout);
  std::cout << std::endl;

  VISKORES_TEST_ASSERT(derivatives.GetNumberOfValues() ==
                         dataSet.GetCellSet().GetNumberOfCells(),
                       "Bad number of cells.");
  VISKORES_TEST_ASSERT(test_equal(viskores::make_Vec(10.025, 30.075, 60.125),
                                  derivatives.ReadPortal().Get(0)),
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
  return viskores::cont::testing::Testing::Run(Run, argc, argv);
}
