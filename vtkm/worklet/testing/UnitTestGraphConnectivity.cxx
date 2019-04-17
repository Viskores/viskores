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
    std::vector<vtkm::Id> counts{ 1, 1, 2, 2, 2 };
    std::vector<vtkm::Id> offsets{ 0, 1, 2, 4, 6 };
    std::vector<vtkm::Id> conn{ 2, 4, 0, 3, 2, 4, 1, 3 };

    vtkm::cont::ArrayHandle<vtkm::Id> counts_h = vtkm::cont::make_ArrayHandle(counts);
    vtkm::cont::ArrayHandle<vtkm::Id> offsets_h = vtkm::cont::make_ArrayHandle(offsets);
    vtkm::cont::ArrayHandle<vtkm::Id> conn_h = vtkm::cont::make_ArrayHandle(conn);
    vtkm::cont::ArrayHandle<vtkm::Id> comps;

    vtkm::worklet::connectivity::GraphConnectivity().Run(counts_h, offsets_h, conn_h, comps);

    for (int i = 0; i < comps.GetNumberOfValues(); i++)
    {
      VTKM_TEST_ASSERT(comps.GetPortalConstControl().Get(i) == 0,
                       "Components has unexpected value.");
    }
  }
};

int UnitTestGraphConnectivity(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestGraphConnectivity(), argc, argv);
}
