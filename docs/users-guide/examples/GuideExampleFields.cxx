//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <viskores/CellClassification.h>
#include <viskores/RangeId3.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/CellSetStructured.h>
#include <viskores/cont/DataSet.h>
#include <viskores/cont/DataSetBuilderUniform.h>
#include <viskores/cont/Field.h>
#include <viskores/cont/Invoker.h>

#include <viskores/worklet/WorkletCellNeighborhood.h>

#include <viskores/cont/testing/Testing.h>

namespace
{

////
//// BEGIN-EXAMPLE SettingGhostCells
////
struct SetGhostCells : viskores::worklet::WorkletCellNeighborhood
{
  using ControlSignature = void(CellSetIn cellSet,
                                WholeArrayIn blankedRegions,
                                FieldOut ghostCells);
  using ExecutionSignature = _3(_2, Boundary);

  template<typename BlankedRegionsPortal>
  VISKORES_EXEC viskores::UInt8 operator()(
    const BlankedRegionsPortal& blankedRegions,
    const viskores::exec::BoundaryState& location) const
  {
    viskores::UInt8 cellClassification = viskores::CellClassification::Normal;

    // Mark cells at boundary as ghost cells.
    if (!location.IsRadiusInBoundary(1))
    {
      cellClassification |= viskores::CellClassification::Ghost;
    }

    // Mark cells inside specified regions as blanked.
    for (viskores::Id brIndex = 0; brIndex < blankedRegions.GetNumberOfValues();
         ++brIndex)
    {
      viskores::RangeId3 blankedRegion = blankedRegions.Get(brIndex);
      if (blankedRegion.Contains(location.GetCenterIndex()))
      {
        cellClassification |= viskores::CellClassification::Blanked;
      }
    }

    return cellClassification;
  }
};

void MakeGhostCells(viskores::cont::DataSet& dataset,
                    const std::vector<viskores::RangeId3> blankedRegions)
{
  viskores::cont::Invoker invoke;
  viskores::cont::ArrayHandle<viskores::UInt8> ghostCells;

  invoke(SetGhostCells{},
         dataset.GetCellSet(),
         viskores::cont::make_ArrayHandle(blankedRegions, viskores::CopyFlag::Off),
         ghostCells);

  dataset.SetGhostCellField(ghostCells);
}
////
//// END-EXAMPLE SettingGhostCells
////

void DoGhostCells()
{
  std::cout << "Do ghost cells\n";
  viskores::cont::DataSetBuilderUniform dataSetBuilder;
  viskores::cont::DataSet dataset = dataSetBuilder.Create({ 11, 11, 11 });
  MakeGhostCells(
    dataset, { { { 0, 5 }, { 0, 5 }, { 0, 5 } }, { { 5, 10 }, { 5, 10 }, { 5, 10 } } });

  viskores::cont::ArrayHandle<viskores::UInt8> ghostCells;
  dataset.GetGhostCellField().GetData().AsArrayHandle(ghostCells);
  auto ghosts = ghostCells.ReadPortal();
  viskores::Id numGhosts = 0;
  viskores::Id numBlanked = 0;
  for (viskores::Id cellId = 0; cellId < ghostCells.GetNumberOfValues(); ++cellId)
  {
    viskores::UInt8 flags = ghosts.Get(cellId);
    if ((flags & viskores::CellClassification::Ghost) ==
        viskores::CellClassification::Ghost)
    {
      ++numGhosts;
    }
    if ((flags & viskores::CellClassification::Blanked) ==
        viskores::CellClassification::Blanked)
    {
      ++numBlanked;
    }
  }
  std::cout << "Num ghosts: " << numGhosts << "\n";
  std::cout << "Num blanked: " << numBlanked << "\n";
  VISKORES_TEST_ASSERT(numGhosts == 488);
  VISKORES_TEST_ASSERT(numBlanked == 250);
}

void Run()
{
  DoGhostCells();
}

} // anonymous namespace

int GuideExampleFields(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(Run, argc, argv);
}
