//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/DispatcherMapTopology.h>
#include <vtkm/worklet/DispatcherPointNeighborhood.h>
#include <vtkm/worklet/MaskIndices.h>
#include <vtkm/worklet/ScatterUniform.h>

#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ArrayHandleConstant.h>
#include <vtkm/cont/ArrayHandleCounting.h>

#include <vtkm/Math.h>

#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>

namespace
{

using FieldType = vtkm::Float32;
#define FieldNull vtkm::Nan32()
constexpr vtkm::IdComponent IdNull = -2;

struct FieldWorklet : vtkm::worklet::WorkletMapField
{
  using ControlSignature = void(WholeCellSetIn<>, // Placeholder for interface consistency
                                FieldIn inputField,
                                FieldInOut fieldCopy,
                                FieldInOut visitCopy);
  using ExecutionSignature = void(_2, VisitIndex, _3, _4);
  using InputDomain = _2;

  using ScatterType = vtkm::worklet::ScatterUniform<2>;
  using MaskType = vtkm::worklet::MaskIndices;

  VTKM_EXEC void operator()(FieldType inField,
                            vtkm::IdComponent visitIndex,
                            FieldType& fieldCopy,
                            vtkm::IdComponent& visitCopy) const
  {
    fieldCopy = inField;
    visitCopy = visitIndex;
  }
};

struct TopologyWorklet : vtkm::worklet::WorkletVisitPointsWithCells
{
  using ControlSignature = void(CellSetIn,
                                FieldInPoint inputField,
                                FieldInOutPoint fieldCopy,
                                FieldInOutPoint visitCopy);
  using ExecutionSignature = void(_2, VisitIndex, _3, _4);
  using InputDomain = _1;

  using ScatterType = vtkm::worklet::ScatterUniform<2>;
  using MaskType = vtkm::worklet::MaskIndices;

  VTKM_EXEC void operator()(FieldType inField,
                            vtkm::IdComponent visitIndex,
                            FieldType& fieldCopy,
                            vtkm::IdComponent& visitCopy) const
  {
    fieldCopy = inField;
    visitCopy = visitIndex;
  }
};

struct NeighborhoodWorklet : vtkm::worklet::WorkletPointNeighborhood
{
  using ControlSignature = void(CellSetIn,
                                FieldIn inputField,
                                FieldInOut fieldCopy,
                                FieldInOut visitCopy);
  using ExecutionSignature = void(_2, VisitIndex, _3, _4);
  using InputDomain = _1;

  using ScatterType = vtkm::worklet::ScatterUniform<2>;
  using MaskType = vtkm::worklet::MaskIndices;

  VTKM_EXEC void operator()(FieldType inField,
                            vtkm::IdComponent visitIndex,
                            FieldType& fieldCopy,
                            vtkm::IdComponent& visitCopy) const
  {
    fieldCopy = inField;
    visitCopy = visitIndex;
  }
};

template <typename DispatcherType>
void TestMapWorklet()
{
  vtkm::cont::testing::MakeTestDataSet builder;
  vtkm::cont::DataSet data = builder.Make3DUniformDataSet1();

  vtkm::cont::CellSetStructured<3> cellSet =
    data.GetCellSet().Cast<vtkm::cont::CellSetStructured<3>>();
  vtkm::Id numPoints = cellSet.GetNumberOfPoints();

  vtkm::cont::ArrayHandle<FieldType> inField;
  inField.Allocate(numPoints);
  SetPortal(inField.GetPortalControl());

  vtkm::cont::ArrayHandle<FieldType> fieldCopy;
  vtkm::cont::ArrayCopy(vtkm::cont::make_ArrayHandleConstant(FieldNull, numPoints * 2), fieldCopy);

  vtkm::cont::ArrayHandle<vtkm::IdComponent> visitCopy;
  vtkm::cont::ArrayCopy(vtkm::cont::make_ArrayHandleConstant(IdNull, numPoints * 2), visitCopy);

  // The scatter is hardcoded to create 2 outputs for every input.
  // Set up the mask to select a range of values in the middle.
  vtkm::Id maskStart = numPoints / 2;
  vtkm::Id maskEnd = (numPoints * 2) / 3;
  vtkm::worklet::MaskIndices mask(
    vtkm::cont::make_ArrayHandleCounting(maskStart, vtkm::Id(1), maskEnd - maskStart));

  DispatcherType dispatcher(mask);
  dispatcher.Invoke(cellSet, inField, fieldCopy, visitCopy);

  // Check outputs
  auto fieldCopyPortal = fieldCopy.GetPortalConstControl();
  auto visitCopyPortal = visitCopy.GetPortalConstControl();
  for (vtkm::Id outputIndex = 0; outputIndex < numPoints * 2; ++outputIndex)
  {
    FieldType fieldValue = fieldCopyPortal.Get(outputIndex);
    vtkm::IdComponent visitValue = visitCopyPortal.Get(outputIndex);
    if ((outputIndex >= maskStart) && (outputIndex < maskEnd))
    {
      vtkm::Id inputIndex = outputIndex / 2;
      FieldType expectedField = TestValue(inputIndex, FieldType());
      VTKM_TEST_ASSERT(fieldValue == expectedField,
                       outputIndex,
                       ": expected ",
                       expectedField,
                       ", got ",
                       fieldValue);

      vtkm::IdComponent expectedVisit = static_cast<vtkm::IdComponent>(outputIndex % 2);
      VTKM_TEST_ASSERT(visitValue == expectedVisit,
                       outputIndex,
                       ": expected ",
                       expectedVisit,
                       ", got ",
                       visitValue);
    }
    else
    {
      VTKM_TEST_ASSERT(vtkm::IsNan(fieldValue), outputIndex, ": expected NaN, got ", fieldValue);
      VTKM_TEST_ASSERT(
        visitValue == IdNull, outputIndex, ": expected ", IdNull, ", got ", visitValue);
    }
  }
}

void Test()
{
  std::cout << "Try on WorkletMapField" << std::endl;
  TestMapWorklet<vtkm::worklet::DispatcherMapField<FieldWorklet>>();

  std::cout << "Try on WorkletMapCellToPoint" << std::endl;
  TestMapWorklet<vtkm::worklet::DispatcherMapTopology<TopologyWorklet>>();

  std::cout << "Try on WorkletPointNeighborhood" << std::endl;
  TestMapWorklet<vtkm::worklet::DispatcherPointNeighborhood<NeighborhoodWorklet>>();
}

} // anonymous namespace

int UnitTestScatterAndMask(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(Test, argc, argv);
}
