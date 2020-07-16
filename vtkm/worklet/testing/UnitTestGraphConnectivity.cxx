//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/testing/Testing.h>

#include <vtkm/worklet/connectivities/GraphConnectivity.h>

class TestGraphConnectivity
{
public:
  void operator()() const
  {
    vtkm::cont::ArrayHandle<vtkm::Id> counts_h =
      vtkm::cont::make_ArrayHandle<vtkm::Id>({ 1, 1, 2, 2, 2 });
    vtkm::cont::ArrayHandle<vtkm::Id> offsets_h =
      vtkm::cont::make_ArrayHandle<vtkm::Id>({ 0, 1, 2, 4, 6 });
    vtkm::cont::ArrayHandle<vtkm::Id> conn_h =
      vtkm::cont::make_ArrayHandle<vtkm::Id>({ 2, 4, 0, 3, 2, 4, 1, 3 });
    vtkm::cont::ArrayHandle<vtkm::Id> comps;

    vtkm::worklet::connectivity::GraphConnectivity().Run(counts_h, offsets_h, conn_h, comps);

    for (int i = 0; i < comps.GetNumberOfValues(); i++)
    {
      VTKM_TEST_ASSERT(comps.ReadPortal().Get(i) == 0, "Components has unexpected value.");
    }
  }
};

int UnitTestGraphConnectivity(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestGraphConnectivity(), argc, argv);
}
