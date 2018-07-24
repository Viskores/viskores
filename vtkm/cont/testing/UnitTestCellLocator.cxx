//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2017 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2017 UT-Battelle, LLC.
//  Copyright 2017 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#include <vtkm/cont/CellLocatorHelper.h>
#include <vtkm/cont/DataSetBuilderUniform.h>
#include <vtkm/cont/testing/Testing.h>

namespace
{

void TestCellLocator()
{
  using PointType = vtkm::Vec<vtkm::FloatDefault, 3>;
  VTKM_DEFAULT_DEVICE_ADAPTER_TAG device;

  const vtkm::Id SIZE = 4;
  auto ds =
    vtkm::cont::DataSetBuilderUniform::Create(vtkm::Id3(SIZE), PointType(0.0f), PointType(1.0f));

  vtkm::cont::CellLocatorHelper locator;
  locator.SetCellSet(ds.GetCellSet());
  locator.SetCoordinates(ds.GetCoordinateSystem());
  locator.Build(device);

  PointType points[] = {
    { 0.25, 0.25, 0.25 }, { 1.25, 1.25, 1.25 }, { 2.25, 2.25, 2.25 }, { 3.25, 3.25, 3.25 }
  };

  vtkm::cont::ArrayHandle<vtkm::Id> cellIds;
  vtkm::cont::ArrayHandle<PointType> parametricCoords;
  locator.FindCells(vtkm::cont::make_ArrayHandle(points, 4), cellIds, parametricCoords, device);

  const vtkm::Id NCELLS_PER_AXIS = SIZE - 1;
  const vtkm::Id DIA_STRIDE = (NCELLS_PER_AXIS * NCELLS_PER_AXIS) + NCELLS_PER_AXIS + 1;
  for (int i = 0; i < 3; ++i)
  {
    VTKM_TEST_ASSERT(cellIds.GetPortalConstControl().Get(i) == (i * DIA_STRIDE),
                     "Incorrect cell-id value");
    VTKM_TEST_ASSERT(parametricCoords.GetPortalConstControl().Get(i) == PointType(0.25f),
                     "Incorrect parametric coordinate value");
  }
  VTKM_TEST_ASSERT(cellIds.GetPortalConstControl().Get(3) == -1, "Incorrect cell-id value");
}

} // anonymous namespace

int UnitTestCellLocator(int, char* [])
{
  return vtkm::cont::testing::Testing::Run(TestCellLocator);
}
