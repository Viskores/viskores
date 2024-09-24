//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/CellClassification.h>
#include <vtkm/RangeId3.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/CellSetStructured.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DataSetBuilderUniform.h>
#include <vtkm/cont/Field.h>
#include <vtkm/cont/Invoker.h>

#include <vtkm/worklet/WorkletCellNeighborhood.h>

#include <vtkm/cont/testing/Testing.h>

namespace
{

////
//// BEGIN-EXAMPLE SettingGhostCells
////
struct SetGhostCells : vtkm::worklet::WorkletCellNeighborhood
{
  using ControlSignature = void(CellSetIn cellSet,
                                WholeArrayIn blankedRegions,
                                FieldOut ghostCells);
  using ExecutionSignature = _3(_2, Boundary);

  template<typename BlankedRegionsPortal>
  VTKM_EXEC vtkm::UInt8 operator()(const BlankedRegionsPortal& blankedRegions,
                                   const vtkm::exec::BoundaryState& location) const
  {
    vtkm::UInt8 cellClassification = vtkm::CellClassification::Normal;

    // Mark cells at boundary as ghost cells.
    if (!location.IsRadiusInBoundary(1))
    {
      cellClassification |= vtkm::CellClassification::Ghost;
    }

    // Mark cells inside specified regions as blanked.
    for (vtkm::Id brIndex = 0; brIndex < blankedRegions.GetNumberOfValues(); ++brIndex)
    {
      vtkm::RangeId3 blankedRegion = blankedRegions.Get(brIndex);
      if (blankedRegion.Contains(location.GetCenterIndex()))
      {
        cellClassification |= vtkm::CellClassification::Blanked;
      }
    }

    return cellClassification;
  }
};

void MakeGhostCells(vtkm::cont::DataSet& dataset,
                    const std::vector<vtkm::RangeId3> blankedRegions)
{
  vtkm::cont::Invoker invoke;
  vtkm::cont::ArrayHandle<vtkm::UInt8> ghostCells;

  invoke(SetGhostCells{},
         dataset.GetCellSet(),
         vtkm::cont::make_ArrayHandle(blankedRegions, vtkm::CopyFlag::Off),
         ghostCells);

  dataset.SetGhostCellField(ghostCells);
}
////
//// END-EXAMPLE SettingGhostCells
////

void DoGhostCells()
{
  std::cout << "Do ghost cells\n";
  vtkm::cont::DataSetBuilderUniform dataSetBuilder;
  vtkm::cont::DataSet dataset = dataSetBuilder.Create({ 11, 11, 11 });
  MakeGhostCells(
    dataset, { { { 0, 5 }, { 0, 5 }, { 0, 5 } }, { { 5, 10 }, { 5, 10 }, { 5, 10 } } });

  vtkm::cont::ArrayHandle<vtkm::UInt8> ghostCells;
  dataset.GetGhostCellField().GetData().AsArrayHandle(ghostCells);
  auto ghosts = ghostCells.ReadPortal();
  vtkm::Id numGhosts = 0;
  vtkm::Id numBlanked = 0;
  for (vtkm::Id cellId = 0; cellId < ghostCells.GetNumberOfValues(); ++cellId)
  {
    vtkm::UInt8 flags = ghosts.Get(cellId);
    if ((flags & vtkm::CellClassification::Ghost) == vtkm::CellClassification::Ghost)
    {
      ++numGhosts;
    }
    if ((flags & vtkm::CellClassification::Blanked) == vtkm::CellClassification::Blanked)
    {
      ++numBlanked;
    }
  }
  std::cout << "Num ghosts: " << numGhosts << "\n";
  std::cout << "Num blanked: " << numBlanked << "\n";
  VTKM_TEST_ASSERT(numGhosts == 488);
  VTKM_TEST_ASSERT(numBlanked == 250);
}

void Run()
{
  DoGhostCells();
}

} // anonymous namespace

int GuideExampleFields(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(Run, argc, argv);
}
