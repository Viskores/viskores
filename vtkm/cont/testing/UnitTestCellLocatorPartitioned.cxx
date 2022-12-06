
//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <random>
#include <string>

#include <vtkm/cont/CellLocatorPartitioned.h>
#include <vtkm/cont/Invoker.h>

#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>

#include <vtkm/source/Amr.h>

#include <vtkm/ErrorCode.h>
#include <vtkm/worklet/WorkletMapField.h>

namespace
{
struct QueryCellsWorklet : public vtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn, ExecObject, FieldOut, FieldOut);
  using ExecutionSignature = void(_1, _2, _3, _4);

  template <typename PointType, typename CellLocatorExecObjectType>
  VTKM_EXEC void operator()(const PointType& point,
                            const CellLocatorExecObjectType& cellLocator,
                            vtkm::Id& cellId,
                            vtkm::Id& partitionId) const
  {
    vtkm::Vec3f parametric;
    vtkm::ErrorCode status = cellLocator.FindCell(point, partitionId, cellId, parametric);
    if (status != vtkm::ErrorCode::Success)
    {
      this->RaiseError(vtkm ::ErrorString(status));
      partitionId = -1;
      cellId = -1;
    }
  }
};

void Test()
{
  int dim = 3;
  int numberOfLevels = 3;
  int cellsPerDimension = 8;

  // Generate AMR
  vtkm::source::Amr source;
  source.SetDimension(dim);
  source.SetNumberOfLevels(numberOfLevels);
  source.SetCellsPerDimension(cellsPerDimension);
  vtkm::cont::PartitionedDataSet amrDataSet = source.Execute();

  // one point for each partition
  vtkm::cont::ArrayHandle<vtkm::Vec3f> queryPoints;
  queryPoints.Allocate(7);
  queryPoints.WritePortal().Set(0, vtkm::Vec3f(0.1f, 0.9f, 0.1f));
  queryPoints.WritePortal().Set(1, vtkm::Vec3f(0.1f, 0.4f, 0.4f));
  queryPoints.WritePortal().Set(2, vtkm::Vec3f(0.8f, 0.5f, 0.5f));
  queryPoints.WritePortal().Set(3, vtkm::Vec3f(0.0f));
  queryPoints.WritePortal().Set(4, vtkm::Vec3f(0.4999999f));
  queryPoints.WritePortal().Set(5, vtkm::Vec3f(0.5000001f));
  queryPoints.WritePortal().Set(6, vtkm::Vec3f(1.0f));

  // generate cellLocator on cont side
  vtkm::cont::CellLocatorPartitioned cellLocator;
  cellLocator.SetPartitions(amrDataSet);
  cellLocator.Update();
  vtkm::cont::ArrayHandle<vtkm::Id> cellIds;
  vtkm::cont::ArrayHandle<vtkm::Id> partitionIds;
  vtkm::cont::Invoker invoke;
  invoke(QueryCellsWorklet{}, queryPoints, &cellLocator, cellIds, partitionIds);

  for (vtkm::Id index = 0; index < queryPoints.GetNumberOfValues(); ++index)
  {
    VTKM_TEST_ASSERT(partitionIds.ReadPortal().Get(index) == index, "Incorrect partitionId");
  }
}

} // anonymous namespace

int UnitTestCellLocatorPartitioned(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(Test, argc, argv);
}
